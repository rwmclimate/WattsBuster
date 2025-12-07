/***************************************************************

  Stolen shamelessly from Liyang Yu (although he mentions that mentioning him, the original
  article and his work is enough for licensing, see:
  http://beta.codeproject.com/Messages/3793113/Re-Once-again-License.aspx and the licenses
  are at:
  http://beta.codeproject.com/info/Licenses.aspx so we could probably assume the CPOL).

  I added some additional "goodies" to make this class more convenient to use.  I.e.
  "auto-reconnect" capabilities for both the client/server modes should the
  connected server/client shutdown or disconnect.

  Wrapper class for general TCP/IP socket operations.

  bind(), connect(), listen(), setsockopt(), etc....

// ########################### BEGIN SERVER SOCKET EXAMPLE1 ##################################

int nPort=8888;

SimpleTcpSocket *pServerSocket;

// Sets up a server socket, binds to port nPort.
// All ready for an accept() call
pServerSocket = new SimpleTcpSocket(nPort);

// The client connection socket returned by accept()
SimpleTcpSocket *pClientConnectSocket;

// This method waits for a client to show up -- when one does, 
// it calls accept() and returns pClientConnectSocket.
pClientConnectSocket = pServerSocket->WaitForClient();


// Now call pClientConnectSocket->SendData(...)/->RecvData(..) to send/receive
// data.  These methods throw an exception for any type of connection error.

// To enable auto-reconnect, catch all exceptions thrown by SendData()/RecvData() 
// and call pClientConnectSocket->ReconnectToClient(pServerSocket,pClientConnectSocket).
// Socket settings (timeout, reuseaddr, etc.) will be transferred to the new
// client connect socket.

// i.e.
//
while(true)
{ 
  try
  {
    pClientConnectSocket->SendData(dataBuf,dataLen);
    pClientConnectSocket->RecvData(dataBuf,dataLen);
  }
  catch(SimpleSocketException* pExcept)
  {
    pExcept->Response();
    delete pExcept;  // Must delete/deallocate exception ptr to avoid memory leak
    pClientConnectSocket=pClientConnectSocket->ReconnectToClient
                                   (pServerSocket,pClientConnectSocket);
  }
  catch(...) // Just in case some other exception is thrown
  {
      pClientConnectSocket=pClientConnectSocket->ReconnectToClient
                                   (pServerSocket,pClientConnectSocket);
  }
}
// #################### END SERVER SOCKET EXAMPLE 1 ##################################





// #################### BEGIN SERVER SOCKET EXAMPLE 2 ################################

// Slightly less simple way to set up a server socket  
// Allows more customization (i.e. socket settings) 

  SimpleTcpSocket *pServerSocket;
  pServerSocket = new SimpleTcpSocket(); 
  
  pServerSocket->CreateSocket();

  pServerSocket->SetPortNumber(nPort);
  pServerSocket->SetSocketBlocking(true);
  pServerSocket->bindSocket();

  pServerSocket->ListenToClient(5);

  SimpleTcpSocket *pClientConnectSocket;
  pClientConnectSocket = pServerSocket->AcceptClient(strHostNameOut,5); // timeout 5 sec
  
  // To enable auto-reconnect, catch all exceptions thrown by SendData()/RecvData() 
  // Call ReconnectToClient() to re-establish a connection to the client.
  while(true)
  { 
    try
    {
      pClientConnectSocket->SendData(dataBuf,dataLen);
      pClientConnectSocket->RecvData(dataBuf,dataLen);
    }
    catch(SimpleSocketException* pExcept)
    {
      pExcept->Response();
      delete pExcept;  // Must delete/deallocate exception ptr to avoid memory leak
      pClientConnectSocket->SleepSec(3); // A good idea to throttle things a bit
      pClientConnectSocket=pClientConnectSocket->ReconnectToClient
                                     (pServerSocket,pClientConnectSocket);
    }
    catch(...) // Just in case some other exception is thrown
    {
        pClientConnectSocket->SleepSec(3); // A good idea to throttle things a bit
        pClientConnectSocket=pClientConnectSocket->ReconnectToClient
                                     (pServerSocket,pClientConnectSocket);
    }
  }
// ##################### END SERVER SOCKET EXAMPLE 2 ##################################





// ##################### BEGIN CLIENT SOCKET EXAMPLE ###################################
// To set up a client socket, do the following 

SimpleTcpSocket pSocket;

std::string strServerHost="127.0.0.1"; // (also "localhost")
int nPort=8888;
pSocket = new SimpleTcpSocket(strServerHost,nPort);

// Now ready to read/write data
// Catch all exceptions and auto-reconnect per below.
while(true)
{

   int nbytesSend,nbytesRecv;
	
   try
   {
      nbytesRecv=pSocket->SendData(...)
      nbytesSend=pSocket->RecvData(...)
   }
   catch(SimpleSocketException* pExcept)
   {
      pExcept->Response();
      delete pExcept;  // Must delete/deallocate exception ptr to avoid memory leak
      
      pClientConnectSocket->SleepSec(3); // A good idea to throttle things a bit

      pSocket->ReconnectToServer(strServerHost);
   }
   catch(...)
   {
     pClientConnectSocket->SleepSec(3); // A good idea to throttle things a bit

     pSocket->ReconnectToServer(strServerHost);
   }
}
//###################### END CLIENT SOCKET EXAMPLE ####################################


For non-blocking mode, getting the auto-reconnect feature to work may
require some "tweaking".  send()/recv() return values cannot
tell you whether or not the other end of the tcp connection has dropped.

The best way to handle this is to set thresholds to determine when
to force a reconnect.  The thresholds are: (1) time(seconds) since the
last successful send() or recv(), and (2) The number of send()/recv()
failures.  A combination of the two are used to decide when to force
a reconnect.  

Use the following public functions: 

    SetNonBlockResetTimeout(int timeoutSec)
    SetNonBlockResetFailCount(int nFailCount)

A connection reset (client side and server side) will be initiated
when the elapsed time since the last successful send()/recv() is
greater than timeoutSec *and* the number of send()/recv() failures
exceeds nFailCount.


******************************************************************************/

//------------------------------------------------------------------

#ifndef SimpleSocket_H
#define SimpleSocket_H

#include "SimpleSocketPlatformDef.h"
#include "SimpleSocketHostInfo.h"
#include "SimpleSocketException.h"
#include "SimpleSocketLog.h"

#ifdef UNIX
    #include <stdlib.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <iostream>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <stdio.h>
    #include <string.h>
#endif

#ifdef WINDOWS_XP
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

    #include <sstream>
    #include <string>

enum  DISCONNECT_CODE 
{
  MISC_ERROR,
  SOCKET_CREATE_ERROR,
  SOCKET_SET_ERROR,
  SOCKET_GET_ERROR,
  BIND_ERROR,
  LISTEN_ERROR,
  ACCEPT_ERROR,
  CONNECT_ERROR,
  SELECT_ERROR,
  SEND_ERROR,
  RECV_ERROR
};



// So far we only implement the TCP socket -- no UDP
// const int MAX_RECV_LEN = 1048576;
const int MAX_RECV_LEN = 8096;
const int MAX_MSG_LEN = 1024;
const int PORTNUM = 1200;

class SimpleSocket
{


public:
    SimpleSocket(void); // Basic constructor -- don't set port number, etc.
    
    virtual ~SimpleSocket()
    {
	    std::cerr<<__FUNCTION__<<"(): Closing socket..."<<std::endl;
#ifdef WINDOWS_XP
      closesocket(m_nSocketId);
#else
      close(m_nSocketId);
#endif
    };

    static const std::string STR_ERR_CREATING_SOCKET;

    static const std::string STR_ERR_SETTING_SOCKET_TIMEOUT;
    static const std::string STR_ERR_GETTING_SOCKET_TIMEOUT;

    static const std::string STR_ERR_SETTING_SEND_BUFSIZE;
    static const std::string STR_ERR_GETTING_SEND_BUFSIZE;

    static const std::string STR_ERR_SETTING_RECV_BUFSIZE;
    static const std::string STR_ERR_GETTING_RECV_BUFSIZE;

    static const std::string STR_ERR_SETTING_BLOCKING_MODE;

    static const std::string STR_ERR_SETTING_DEBUG_MODE;
    static const std::string STR_ERR_GETTING_DEBUG_MODE;

    static const std::string STR_ERR_SETTING_REUSEADDR;
    static const std::string STR_ERR_GETTING_REUSEADDR;

    static const std::string STR_ERR_SETTING_KEEPALIVE;
    static const std::string STR_ERR_GETTING_KEEPALIVE;

    static const std::string STR_ERR_SETTING_LINGER;
    static const std::string STR_ERR_GETTING_LINGER;

    static const std::string STR_ERR_SETTING_LINGER_ONOFF;
    static const std::string STR_ERR_GETTING_LINGER_ONOFF;

    static const std::string STR_ERR_GETTING_HOST_BY_NAME;

    static const std::string STR_ERR_CALLING_ACCEPT;
    static const std::string STR_ERR_CALLING_CONNECT;
    static const std::string STR_ERR_CALLING_LISTEN;
    static const std::string STR_ERR_CALLING_BIND;
    static const std::string STR_ERR_CALLING_SEND;
    static const std::string STR_ERR_CALLING_RECV;


    // Some safe defaults...
    // These are *not* the default socket() operational
    // parameters -- just safe initialization vals
    // for this class
    static const int DEFAULT_LINGER_SECONDS=30;
    static const int DEFAULT_ACCEPT_TIMEOUT=0;
    static const int DEFAULT_SOCKET_TIMEOUT=0;
    static const int DEFAULT_SEND_BUFSIZE=1024;
    static const int DEFAULT_RECV_BUFSIZE=1024;
    
    // For non-blocking socket mode -- use these to determine
    // when we should assume a disconnect and reset accordingly.
    static const int NONBLOCK_RESET_TIMEOUT_SEC=10;
    static const int NONBLOCK_RESET_FAIL_COUNT=3;
    
public:

	// socket options : ON/OFF
    void CreateSocket(void);
    void CloseSocket(void);

    void SetDebug(int);

    void SetReuseAddr(int);

    void SetKeepAlive(int);

    void SetLingerOnOff(bool);
    void SetLingerSeconds(int);

    void SetSocketTimeoutSeconds(int acceptTimeoutSeconds);
    void SetAcceptTimeoutSeconds(int acceptTimeoutSeconds);

    // 1 for nonblocking, 0 for blocking (non-nonblocking)
    void SetSocketNonBlocking(int nNonBlockingMode);

    // size of the send and receive buffer
    void SetSendBufSize(int);
    void SetReceiveBufSize(int);


    void  SetPortNumber(int portNum);

    void  SetNonBlockingResetTimeout(int nSec)
    {
	    m_nNonblockResetTimeout=nSec;
    };
    
    void SetNonBlockResetFailCount(int nFailCount)
    {
	    m_nNonblockResetFailCount=nFailCount;
    };


    // retrieve socket option settings
    int  GetDebug();
    int  GetReuseAddr();
    int  GetKeepAlive();
    int  GetSendBufSize();
    int  GetReceiveBufSize();
    int GetSocketNonBlocking() { return m_nNonBlockingMode; }
    int  GetLingerSeconds();
    bool GetLingerOnOff();
	
    // returns the socket file descriptor
    int GetSocketId() { return m_nSocketId; }

    int& GetSocketIdReference() { return m_nSocketId; }

    // returns the port number
    int GetPortNumber() { return m_nPortNumber; }
    
    void SleepSec(const int nSec)
    {
#ifdef UNIX
      sleep(nSec);
#endif
#ifdef WINDOWS_XP
      int nMsec=nSec*1000;
      Sleep(nMsec);
#endif
    };

    // show the socket 
    friend std::ostream& operator<<(std::ostream& logFileStream, SimpleSocket& robustSocket);

    bool m_bPortNumberSet;
    bool m_bNonBlockingModeSet;
    bool m_bDebugModeSet;
    bool m_bReuseModeSet;
    bool m_bKeepAliveModeSet;
    bool m_bLingerSecondsSet;
    bool m_bLingerOnOffSet;
    bool m_bAcceptTimeoutSecondsSet;
    bool m_bSocketTimeoutSecondsSet;
    bool m_bSendBufSizeSet;
    bool m_bReceiveBufSizeSet;

protected:

    // When a client socket is disconnect/reconnected,
    // transfer settings from old socket connection to new one.
    // (Client-side only)
    void TransferSocketSettings(SimpleSocket *pOCS);
    
	/*
	   only used when the socket is used for client communication
	   once this is done, the next necessary call is SetSocketId(int)
	*/
    void SetSocketId(int socketFd) { m_nSocketId = socketFd; }

    int m_nPortNumber;        // Socket port number    
    int m_nSocketId;          // Socket file descriptor

    int m_nNonBlockingMode;          // Blocking flag

    int m_nDebugMode;
    int m_nReuseMode;
    int m_nKeepAliveMode;
    int m_nLingerSeconds;

    bool m_bLingerOnOff;

    int m_nAcceptTimeoutSeconds;
    int m_nSocketTimeoutSeconds;
    int m_nSendBufSize;
    int m_nReceiveBufSize;
    
    std::string m_strClientHostName;
    
    struct sockaddr_in m_clientAddr;    // Address of the client that sent data

    time_t m_nLastSendTime_t;
    time_t m_nLastRecvTime_t;
    
    int    m_nSendFailureCount;
    int    m_nRecvFailureCount;

    int m_nNonblockResetTimeout;
    int m_nNonblockResetFailCount;
    

private:

	// Gets the system error
#ifdef WINDOWS_XP
    void DetectErrorOpenWinSocket(int*,std::string&);
    void DetectErrorSetSocketOption(int*,std::string&);
    void DetectErrorGetSocketOption(int*,std::string&);
#endif

#ifdef UNIX
    char *GetStrError(void)
    {
	    return strerror(errno);
    }
#endif

};

class SimpleTcpSocket : public SimpleSocket
{

public:

	/* 
	   Constructors used for creating instances dedicated to client/server 
	   communication:

	   when accept() is successful, a socketId is generated and returned
	   this socket id is then used to build a new socket using the following
	   constructor, therefore, the next necessary call should be SetSocketId()
	   using this newly generated socket fd
	*/

	// Simple constructor -- will have to set up manually to be a client or server
	// socket later.
	SimpleTcpSocket() {};

	// Create a server socket that listens on nListeningPort
	// Constructor.  
	SimpleTcpSocket(int nListeningPortId); // : SimpleSocket(portId) { };


	// Create a client socket that connects to strHostNameOrIP, port nPort
	// strHostNameOrIP can be the name or the dotted-quad ip addr.
	SimpleTcpSocket(const std::string& strHostNameOrIP,
			const int nPort, bool bImpatient = false);

	// Create a client socket that connects to hostID, port nPort
	// Must tell this constructor whether you are passing it a name
	// or a dotted-quad ip addr (hType=NAME or ADDRESS)
	// The bImpatient flag exists to prevent permenent blocking when
	// issuing the first connection.
	SimpleTcpSocket(const std::string& hostID, hostType hType, 
			int nPort, bool bImpatient = false);
	

	// Socket spawned off by accept should inherit server-socket's settings
	// Probably overkill, but doesn't hurt to make sure...
	SimpleTcpSocket(SimpleTcpSocket* pSimpleTcpSocket){*this=*pSimpleTcpSocket;};
    
	virtual ~SimpleTcpSocket()
	{
		std::cerr<<__FUNCTION__
			 <<"(): Doing nothing (base class destructor should do the work......)"
			 <<std::endl;
	};

	/*
	   Binds the socket to an address and port number
	   a server call
	*/
	virtual void BindSocket(void);


	// Server-side socket
	// Listens to connecting clients, a server call
	virtual void ListenToClient(int nQueue = 5);


	
	//  Server-side socket
	//  Accepts a connecting client.  The address of the connected client 
	//  is stored in the parameter
	//  Accept() timeout determined by m_nAcceptTimeoutSec
	virtual SimpleTcpSocket* AcceptClient(std::string&); 

	// Returns client host name in second arg.
	//  Accept() timeout determined by m_nAcceptTimeoutSec
	virtual SimpleTcpSocket* AcceptClient(SimpleTcpSocket* pSimpleSocket, 
					      std::string& clientHostNameOut);

	// Server-side socket
	// Hang around waiting for a client -- when one shows up,
	// accept it and establish the connection. -- server call
	virtual SimpleTcpSocket* WaitForClient(void);

	// Server-side socket
	// Reconnect to client, transfer old settings to
	// new connection (may be redundant/overkill, but want to make sure...)
	virtual SimpleTcpSocket* ReconnectToClient(SimpleTcpSocket *pServerSocket,
						   SimpleTcpSocket *pOldClientConnectSocket);


	// Client-side socket
	// Connect to the server 
	// Figures out automatically whether you are passing it
	// a host name or IP address. 
	virtual void ConnectToServer(const std::string& strServerNameOrIp);


	// Client-side call -- waits for server 
	virtual void WaitForServerConnection(const std::string& serverNameOrAddr);

	
	// Client-side call 
	// reconnect to server (closes out pre-existing connection)
	virtual void ReconnectToServer(const std::string& serverNameOrAddr);


	virtual int SendData(const char *dataBufIn, int dataBufLen);
	
	virtual int RecvData(char *recvDataOut, int dataBufLen);
	


  protected:

	// Name or dotted-quad addr goes in; dotted-quad addr comes out */
	virtual std::string GetDottedQuadIpAddr(const std::string& strName);


	// Connect to the server, a client call -- you have to tell
	// this method whether you are passing it a name (hostType=NAME)
	// or a dotted-quad IP address (hostType=ADDRESS)
	virtual void ConnectToServer(const std::string& serverHost,
				     hostType enumAddressOrName);

	// Have to tell this method whether you are passing it a name (hostType=NAME)
	// or dotted-quad IP addr (hostType=ADDRESS)
	virtual void WaitForServerConnection(const std::string& serverNameOrAddr, 
					     hostType enumAddressOrName, bool bImPatient = false);

	// Have to tell this method whether you are passing it a name (hostType=NAME)
	// or dotted-quad IP addr (hostType=ADDRESS)
	virtual void ReconnectToServer(const std::string& serverNameOrAddr,
			       hostType enumAddressOrName);


	// Want socket spawned off by accept() to inherit all of the server socket's
	// settings... -- called inside ReconnectToClient(...)
	virtual SimpleTcpSocket* AcceptClient(SimpleTcpSocket* pSimpleSocket, 
					      std::string& clientHostNameOut,
					      int acceptTimeoutSec);

	// Time out if a client doesn't show up within acceptTimeoutSec
	virtual SimpleTcpSocket* AcceptClient(std::string&, 
					      int acceptTimeoutSec); 


	// Returns just the bare client-connection socket -- probably won't ever be used.
	virtual int AcceptClientSocket(std::string&, int acceptTimeoutSec=0);



private:
	#ifdef WINDOWS_XP
		// Windows NT version of the MSG_WAITALL option
		int XPreceiveMessage(std::string&);

	void DetectErrorBind(int*,std::string&);
	void DetectErrorSend(int*,std::string&);
	void DetectErrorRecv(int*,std::string&);
	void DetectErrorConnect(int*,std::string&);
	void DetectErrorAccept(int*,std::string&);
	void DetectErrorListen(int*,std::string&);

	#endif

};

#endif
        
