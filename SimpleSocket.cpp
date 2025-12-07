//------------------------------------------------------------------------
//
// This class contains convenience "wrapper functions" for various lowish
// level socket operations -- most of this code was shamelessly stolen from some
// nameless programmer.
//
// Every routine here (with one exception) now exits from
//               errors by throwing (SimpleSocketExecption*) 
//
//-------------------------------------------------------------------------
#include "SimpleSocket.h"
#include <sstream>
#include <time.h>

const std::string SimpleSocket::STR_ERR_CREATING_SOCKET="unix: error creating socket";

const std::string SimpleSocket::STR_ERR_SETTING_SOCKET_TIMEOUT="unix: error setting socket timeout";
const std::string SimpleSocket::STR_ERR_GETTING_SOCKET_TIMEOUT="unix: error getting socket timeout";

const std::string SimpleSocket::STR_ERR_SETTING_SEND_BUFSIZE="unix: error setting send bufsize";
const std::string SimpleSocket::STR_ERR_GETTING_SEND_BUFSIZE="unix: error getting send bufsize";

const std::string SimpleSocket::STR_ERR_SETTING_RECV_BUFSIZE="unix: error setting recv bufsize";
const std::string SimpleSocket::STR_ERR_GETTING_RECV_BUFSIZE="unix: error getting recv bufsize";

const std::string SimpleSocket::STR_ERR_SETTING_BLOCKING_MODE="unix: error setting blocking mode";

const std::string SimpleSocket::STR_ERR_SETTING_DEBUG_MODE="unix: error setting debug mode";
const std::string SimpleSocket::STR_ERR_GETTING_DEBUG_MODE="unix: error getting debug mode";

const std::string SimpleSocket::STR_ERR_SETTING_REUSEADDR="unix: error setting reuseaddr";
const std::string SimpleSocket::STR_ERR_GETTING_REUSEADDR="unix: error getting reuseaddr";

const std::string SimpleSocket::STR_ERR_SETTING_KEEPALIVE="unix: error setting keepalive";
const std::string SimpleSocket::STR_ERR_GETTING_KEEPALIVE="unix: error getting keepalive";

const std::string SimpleSocket::STR_ERR_SETTING_LINGER="unix: error setting linger";
const std::string SimpleSocket::STR_ERR_GETTING_LINGER="unix: error getting linger";

const std::string SimpleSocket::STR_ERR_SETTING_LINGER_ONOFF="unix: error setting linger on/off";
const std::string SimpleSocket::STR_ERR_GETTING_LINGER_ONOFF="unix: error getting linger on/off";

const std::string SimpleSocket::STR_ERR_GETTING_HOST_BY_NAME="unix: error getting host by name";

const std::string SimpleSocket::STR_ERR_CALLING_ACCEPT="unix: error calling accept()";
const std::string SimpleSocket::STR_ERR_CALLING_CONNECT="unix: error calling connect()";
const std::string SimpleSocket::STR_ERR_CALLING_LISTEN="unix: error calling listen()";
const std::string SimpleSocket::STR_ERR_CALLING_BIND="unix: error calling bind()";
const std::string SimpleSocket::STR_ERR_CALLING_SEND="unix: error calling send()";
const std::string SimpleSocket::STR_ERR_CALLING_RECV="unix: error calling recv()";


SimpleSocket::SimpleSocket(void)
{
	m_nSocketId = -1;

	m_bPortNumberSet=false;
	m_nPortNumber=-1;

	// Don't want to set these here -- set them
	// only in the SimpleTcpSocket() subclass
	// m_bNonBlockingModeSet=false; 
	// m_nNonBlockingMode = 0;
	
	// m_bDebugModeSet=false;
	// m_bReuseModeSet=false;
	// m_bKeepAliveModeSet=false;
	// m_bLingerSecondsSet=false;
	// m_bLingerOnOffSet=false;

	// m_bAcceptTimeoutSecondsSet=false;
	// m_nAcceptTimeoutSeconds=0;
	
	// m_bSocketTimeoutSecondsSet=false;
	// m_nSocketTimeoutSeconds=0;
	
	// m_bSendBufSizeSet=false;
	// m_bReceiveBufSizeSet=false;
	
	// m_strClientHostName="";
	
	return;
}




void SimpleSocket::CreateSocket(void)
{
  if(m_nSocketId!=-1)
  {
#ifdef WINDOWS_XP
    closesocket(m_nSocketId);
#endif

#ifdef UNIX
    close(m_nSocketId);
#endif

    m_nSocketId=-1;
  }
  
  if ( (m_nSocketId=socket(AF_INET,SOCK_STREAM,0)) == -1)
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "";
    DetectErrorOpenWinSocket(&errorCode,errorMsg);
    SimpleSocketException* openWinSocketException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw openWinSocketException;
#endif

#ifdef UNIX
    SimpleSocketException* openUnixSocketException 
      = new SimpleSocketException(SOCKET_CREATE_ERROR,STR_ERR_CREATING_SOCKET.c_str());
    throw openUnixSocketException;
#endif
  }

}

void SimpleSocket::CloseSocket(void)
{
  if(m_nSocketId!=-1)
  {
	  std::cerr<<__FUNCTION__<<"(): Closing socket..."<<std::endl;
#ifdef WINDOWS_XP
    closesocket(m_nSocketId);
#endif
#ifdef UNIX
    close(m_nSocketId);
#endif
  }

  m_nSocketId=-1;

  return;
}

void SimpleSocket::SetPortNumber(int nPortNumber)
{
	m_bPortNumberSet=true;
	m_nPortNumber = nPortNumber;

	m_clientAddr.sin_family = AF_INET;
	m_clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_clientAddr.sin_port = htons(m_nPortNumber);
	
	std::stringstream sstr;
	sstr<<__FUNCTION__<<"(): Setting Port# to " << m_nPortNumber<<std::endl;
	
#ifdef WINDOWS_XP
	OutputDebugString(sstr.str().c_str());
#endif
	
#ifdef UNIX
	std::cout << sstr.str() << std::endl;
#endif
  return;
}
    
void SimpleSocket::SetDebug(int debugToggle)
{
  m_nDebugMode=debugToggle;
  
  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_DEBUG,
		    (char *)&debugToggle,sizeof(debugToggle)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "DEBUG option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException = 
      new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException = 
      new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_DEBUG_MODE.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bDebugModeSet=true;
  
}

void SimpleSocket::SetReuseAddr(int reuseToggle)
{
  m_nReuseMode=reuseToggle;

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_REUSEADDR,
		    (char *)&reuseToggle,sizeof(reuseToggle)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "REUSEADDR option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_REUSEADDR.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bReuseModeSet=true;
  
} 

void SimpleSocket::SetKeepAlive(int aliveToggle)
{
  m_nKeepAliveMode=aliveToggle;

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_KEEPALIVE,
		    (char *)&aliveToggle,sizeof(aliveToggle)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "ALIVE option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_KEEPALIVE.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bKeepAliveModeSet=true;
  
} 

void SimpleSocket::SetLingerSeconds(int seconds)
{
  struct linger lingerOption;
	
  m_nLingerSeconds=seconds;

  if ( seconds > 0 )
  {
    lingerOption.l_linger = seconds;
    lingerOption.l_onoff = 1;
  }
  else lingerOption.l_onoff = 0;
	 
  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_LINGER,
		    (char *)&lingerOption,sizeof(struct linger)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "LINGER option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_LINGER.c_str());
    throw unixSocketOptionException;
#endif
  }
  
  m_bLingerSecondsSet=true;
  
}

void SimpleSocket::SetLingerOnOff(bool lingerOn)
{
  struct linger lingerOption;

  m_bLingerOnOff=lingerOn;

  if ( lingerOn ) lingerOption.l_onoff = 1;
  else lingerOption.l_onoff = 0;

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_LINGER,
		    (char *)&lingerOption,sizeof(struct linger)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "LINGER option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_LINGER_ONOFF.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bLingerOnOffSet=true;
  
}

void SimpleSocket::SetAcceptTimeoutSeconds(int acceptTimeoutSeconds)
{
	m_bAcceptTimeoutSecondsSet=true;
	m_nAcceptTimeoutSeconds=acceptTimeoutSeconds;
	
	return;
}

void SimpleSocket::SetSocketTimeoutSeconds(int seconds)
{

  struct timeval tv_timeout;
  tv_timeout.tv_sec=seconds;
  tv_timeout.tv_usec=0;

  m_nSocketTimeoutSeconds=seconds;

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_RCVTIMEO,
		    (char *)&tv_timeout,sizeof(struct timeval)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "RECV TIMEOUT option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_SOCKET_TIMEOUT.c_str());
    throw unixSocketOptionException;
#endif
  }

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_SNDTIMEO,
		    (char *)&tv_timeout,sizeof(struct timeval)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "SEND TIMEOUT option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_SOCKET_TIMEOUT.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bSocketTimeoutSecondsSet=true;
  
}

void SimpleSocket::SetSendBufSize(int sendBufSize)
{
  m_nSendBufSize=sendBufSize;

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_SNDBUF,
		    (char *)&sendBufSize,sizeof(sendBufSize)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "SENDBUFSIZE option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_SEND_BUFSIZE.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bSendBufSizeSet=true;
  
} 

void SimpleSocket::SetReceiveBufSize(int receiveBufSize)
{
  m_nReceiveBufSize=receiveBufSize;

  if ( ::setsockopt(m_nSocketId,SOL_SOCKET,SO_RCVBUF,
		    (char *)&receiveBufSize,sizeof(receiveBufSize)) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "RCVBUF option:";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_RECV_BUFSIZE.c_str());
    throw unixSocketOptionException;
#endif
  }

  m_bReceiveBufSizeSet=true;
  
}

// Pass arg = 1 for non blocking, arg = 0 for blocking 
void SimpleSocket::SetSocketNonBlocking(int nNonBlockingMode)
{

// Nonblocking mode disabled for now...
#if(0)
  if (blockingToggle)
  {
    if (GetSocketBlocking()) return;
    else m_nBlockingMode = 1;
  }
  else
  {
    if (!GetSocketBlocking()) return;
    else m_nBlockingMode = 0;
  }
#endif

  m_nNonBlockingMode=nNonBlockingMode;

#ifdef WINDOWS_XP
  u_long ulNonBlockingMode;
  ulNonBlockingMode=nNonBlockingMode;
  if (::ioctlsocket(m_nSocketId,FIONBIO,&ulNonBlockingMode) == -1)
  {
    int errorCode;
    std::string errorMsg = "Blocking option: ";
    DetectErrorSetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
  }
#endif

#ifdef UNIX
  if (::ioctl(m_nSocketId,FIONBIO,(char *)&nNonBlockingMode) == -1)
  {
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_SET_ERROR,STR_ERR_SETTING_BLOCKING_MODE.c_str());
    throw unixSocketOptionException;
  }
#endif

  m_bNonBlockingModeSet=true;
  
}

int SimpleSocket::GetDebug()
{
  int SimpleSocketOption;
  socklen_t SimpleSocketOptionLen = sizeof(SimpleSocketOption);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_DEBUG,
		  (char *)&SimpleSocketOption,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get DEBUG option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_DEBUG_MODE.c_str());
    throw unixSocketOptionException;
#endif
  }
    
  return SimpleSocketOption;
}

int SimpleSocket::GetReuseAddr()
{
  int SimpleSocketOption;        
  socklen_t SimpleSocketOptionLen = sizeof(SimpleSocketOption);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_REUSEADDR,
		  (char *)&SimpleSocketOption,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get REUSEADDR option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_REUSEADDR.c_str());
    throw unixSocketOptionException;
#endif
  }

  return SimpleSocketOption;
}

int SimpleSocket::GetKeepAlive()
{
  int SimpleSocketOption;        
  socklen_t SimpleSocketOptionLen = sizeof(SimpleSocketOption);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_KEEPALIVE,
		  (char *)&SimpleSocketOption,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get KEEPALIVE option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_KEEPALIVE.c_str());
    throw unixSocketOptionException;
#endif
  }

  return SimpleSocketOption;    
}

int SimpleSocket::GetLingerSeconds()
{
  struct linger lingerOption;
  socklen_t SimpleSocketOptionLen = sizeof(struct linger);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_LINGER,
		  (char *)&lingerOption,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get LINGER option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_LINGER.c_str());
    throw unixSocketOptionException;
#endif
  }

  return lingerOption.l_linger;
}

bool SimpleSocket::GetLingerOnOff()
{
  struct linger lingerOption;
  socklen_t SimpleSocketOptionLen = sizeof(struct linger);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_LINGER,
		  (char *)&lingerOption,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get LINGER option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_LINGER_ONOFF.c_str());
    throw unixSocketOptionException;
#endif
  }

  if ( lingerOption.l_onoff == 1 ) 
    return true;
  else 
    return false;
}

int SimpleSocket::GetSendBufSize()
{
  int sendBuf;
  socklen_t SimpleSocketOptionLen = sizeof(sendBuf);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_SNDBUF,
		  (char *)&sendBuf,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get SNDBUF option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_SEND_BUFSIZE.c_str());
    throw unixSocketOptionException;
#endif
  }

  return sendBuf;
}    

int SimpleSocket::GetReceiveBufSize()
{
  int rcvBuf;
  socklen_t SimpleSocketOptionLen = sizeof(rcvBuf);

  if ( getsockopt(m_nSocketId,SOL_SOCKET,SO_RCVBUF,
		  (char *)&rcvBuf,&SimpleSocketOptionLen) == -1 )
  {
#ifdef WINDOWS_XP
    int errorCode;
    std::string errorMsg = "get RCVBUF option: ";
    DetectErrorGetSocketOption(&errorCode,errorMsg);
    SimpleSocketException* socketOptionException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketOptionException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketOptionException 
      = new SimpleSocketException(SOCKET_GET_ERROR,STR_ERR_GETTING_RECV_BUFSIZE.c_str());
    throw unixSocketOptionException;
#endif
  }

  return rcvBuf;
}

void SimpleSocket::TransferSocketSettings(SimpleSocket* pOCS)
{
	if(pOCS!=NULL)
	{
	  try
	  {
		if(pOCS->m_bDebugModeSet)
		{
			SetDebug(pOCS->m_nDebugMode);
		}
		if(pOCS->m_bReuseModeSet)
		{
			SetReuseAddr(pOCS->m_nReuseMode);
		}
		if(pOCS->m_bNonBlockingModeSet)
		{
			SetSocketNonBlocking(pOCS->m_nNonBlockingMode);
		}
		if(pOCS->m_bKeepAliveModeSet)
		{
			SetKeepAlive(pOCS->m_nKeepAliveMode);
		}
		if(pOCS->m_bLingerSecondsSet)
		{
			SetLingerSeconds(pOCS->m_nLingerSeconds);
		}
		if(pOCS->m_bLingerOnOffSet)
		{
			SetLingerOnOff(pOCS->m_bLingerOnOff);
		}
		if(pOCS->m_bSocketTimeoutSecondsSet)
		{
			SetSocketTimeoutSeconds(pOCS->m_nSocketTimeoutSeconds);
		}
		if(pOCS->m_bSendBufSizeSet)
		{
			SetSendBufSize(pOCS->m_nSendBufSize);
		}
		if(pOCS->m_bReceiveBufSizeSet)
		{
			SetReceiveBufSize(pOCS->m_nReceiveBufSize);
		}

	  }
	  catch(SimpleSocketException *exp)
	  {
	    exp->Response();
	    delete exp;
	  }
	  catch(...)
	  {
	    std::cerr<<__FUNCTION__<<"(): Mystery exception thrown and caught..."<<std::endl;
	    // Catch whatever unknown exception that might be thrown...
	  }
	  
	}
	return;
}

#ifdef WINDOWS_XP
void SimpleSocket::DetectErrorOpenWinSocket(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("Successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem or the associated service provider has failed.");
  else if ( *errCode == WSAEAFNOSUPPORT )
    errMsg.append("The specified address family is not supported.");
  else if ( *errCode == WSAEINPROGRESS )
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.");
  else if ( *errCode == WSAEMFILE )
    errMsg.append("No more socket descriptors are available.");
  else if ( *errCode == WSAENOBUFS )
    errMsg.append("No buffer space is available. The socket cannot be created.");
  else if ( *errCode == WSAEPROTONOSUPPORT )
    errMsg.append("The specified protocol is not supported.");
  else if ( *errCode == WSAEPROTOTYPE )
    errMsg.append("The specified protocol is the wrong type for this socket.");
  else if ( *errCode == WSAESOCKTNOSUPPORT )
    errMsg.append("The specified socket type is not supported in this address family.");
  else errMsg.append("unknown problems!");
}

void SimpleSocket::DetectErrorSetSocketOption(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEFAULT )
    errMsg.append("optval is not in a valid part of the process address space or optlen parameter is too small.");
  else if ( *errCode == WSAEINPROGRESS )
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.");
  else if ( *errCode == WSAEINVAL )
    errMsg.append("level is not valid, or the information in optval is not valid.");
  else if ( *errCode == WSAENETRESET )
    errMsg.append("Connection has timed out when SO_KEEPALIVE is set.");
  else if ( *errCode == WSAENOPROTOOPT )
    errMsg.append("The option is unknown or unsupported for the specified provider or socket (see SO_GROUP_PRIORITY limitations).");
  else if ( *errCode == WSAENOTCONN )
    errMsg.append("Connection has been reset when SO_KEEPALIVE is set.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else errMsg.append("unknown problem!");
}

void SimpleSocket::DetectErrorGetSocketOption(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEFAULT )
    errMsg.append("One of the optval or the optlen parameters is not a valid part of the user address space, or the optlen parameter is too small.");
  else if ( *errCode == WSAEINPROGRESS )
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.");
  else if ( *errCode == WSAEINVAL )
    errMsg.append("The level parameter is unknown or invalid.");
  else if ( *errCode == WSAENOPROTOOPT )
    errMsg.append("The option is unknown or unsupported by the indicated protocol family.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");

  else errMsg.append("unknown problems!");
}

#endif


// Set up a server socket 
SimpleTcpSocket::SimpleTcpSocket(int pNumber/*listening port*/)
{
	m_nSocketId = -1;
	
	m_bPortNumberSet=true;
	m_nPortNumber = pNumber;

	// Indicates that none of these have been set yet.
	// Flags will be set to true when appopriate Set() methods are called.
	m_bNonBlockingModeSet=false;
	m_bDebugModeSet=false;
	m_bReuseModeSet=false;
	m_bKeepAliveModeSet=false;
	m_bLingerSecondsSet=false;
	m_bLingerOnOffSet=false;

	m_bAcceptTimeoutSecondsSet=false;
	m_bSocketTimeoutSecondsSet=false;

	m_bSendBufSizeSet=false;
	m_bReceiveBufSizeSet=false;

	// Safe class member init values *not* necessarily default
	// socket operation settings. 
	m_nSendBufSize=DEFAULT_SEND_BUFSIZE;
	m_nReceiveBufSize=DEFAULT_RECV_BUFSIZE;
	m_nLingerSeconds=DEFAULT_LINGER_SECONDS;
	m_nAcceptTimeoutSeconds=DEFAULT_ACCEPT_TIMEOUT;
	m_nSocketTimeoutSeconds=DEFAULT_SOCKET_TIMEOUT;

	m_nLastSendTime_t=0;
	m_nLastRecvTime_t=0;

	m_nSendFailureCount=0;
	m_nRecvFailureCount=0;
	
	// Trigger reconnection in non-blocking mode
	m_nNonblockResetTimeout=NONBLOCK_RESET_TIMEOUT_SEC;
	m_nNonblockResetFailCount=NONBLOCK_RESET_FAIL_COUNT;

	
	/* 
	   set the initial address of client that shall be communicated with to 
	   any address as long as they are using the same port number. 
	   The m_clientAddr structure is used in the future for storing the actual
	   address of client applications with which communication is going
	   to start
	*/
	m_clientAddr.sin_family = AF_INET;
	m_clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_clientAddr.sin_port = htons(m_nPortNumber);
	
	m_strClientHostName="";

	try
	{
	  CreateSocket();
	  SetAcceptTimeoutSeconds(0);
	  SetPortNumber(pNumber);
	  SetSocketNonBlocking(0);
	  SetReuseAddr(true);
	  BindSocket();
	  SetSocketTimeoutSeconds(0);
	  ListenToClient();
	}
	catch(SimpleSocketException *exp)
	{
	  // If the constructor throws an exception,
	  // catch it to look at and then rethrow it.
	  exp->Response();
	  throw exp;
	}
	catch(...)
	{
	  std::cerr<<__FUNCTION__<<"(): Mystery exception..."<<std::endl;
	  SimpleSocketException *sexp = new SimpleSocketException(MISC_ERROR,
								  "Mystery execption thrown");
	  throw sexp;
	}
	
	// Now call AcceptConnection() and you are good to go.


}

//----------------------------------------------------------------------------------------
/**
Set up a client socket and make a first connection.


@param hostID, remote server host name or ip address
@param pNumber, remote server port port
@param bImpatient, true - no waiting during connection, false - waits until connected
*/
//----------------------------------------------------------------------------------------
SimpleTcpSocket::SimpleTcpSocket(const std::string& strHostNameOrIp,
				 const int pNumber, bool bImpatient)
{
	m_nSocketId = -1;
	
	m_bPortNumberSet=true;
	m_nPortNumber = pNumber;
	

	// Indicates that none of these have been set yet.
	// Flags will be set to true when appopriate Set() methods are called.
	m_bNonBlockingModeSet=false;
	m_bDebugModeSet=false;
	m_bReuseModeSet=false;
	m_bKeepAliveModeSet=false;
	m_bLingerSecondsSet=false;
	m_bLingerOnOffSet=false;

	m_bAcceptTimeoutSecondsSet=false;
	m_bSocketTimeoutSecondsSet=false;

	m_bSendBufSizeSet=false;
	m_bReceiveBufSizeSet=false;

	// Safe class member init values *not* necessarily default
	// socket operation settings. 
	m_nSendBufSize=DEFAULT_SEND_BUFSIZE;
	m_nReceiveBufSize=DEFAULT_RECV_BUFSIZE;
	m_nLingerSeconds=DEFAULT_LINGER_SECONDS;
	m_nAcceptTimeoutSeconds=DEFAULT_ACCEPT_TIMEOUT;
	m_nSocketTimeoutSeconds=DEFAULT_SOCKET_TIMEOUT;
	
	m_nLastSendTime_t=0;
	m_nLastRecvTime_t=0;

	m_nSendFailureCount=0;
	m_nRecvFailureCount=0;

	// Trigger reconnection in non-blocking mode
	m_nNonblockResetTimeout=NONBLOCK_RESET_TIMEOUT_SEC;
	m_nNonblockResetFailCount=NONBLOCK_RESET_FAIL_COUNT;


	/* 
	   set the initial address of client that shall be communicated with to 
	   any address as long as they are using the same port number. 
	   The m_clientAddr structure is used in the future for storing the actual
	   address of client applications with which communication is going
	   to start
	*/
	m_clientAddr.sin_family = AF_INET;
	m_clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_clientAddr.sin_port = htons(m_nPortNumber);
	
	m_strClientHostName=""; // server-side only -- name of client host


	// struct hostent *host;
	// struct in_addr h_addr;
	std::string strDottedQuadAddr;

	strDottedQuadAddr=GetDottedQuadIpAddr(strHostNameOrIp);
	
	try
	{
	  CreateSocket();
	  SetPortNumber(pNumber);
	  SetSocketNonBlocking(0); // default to blocking (i.e. non-blocking = 0)
	  SetReuseAddr(true);
	  SetSocketTimeoutSeconds(0); // never time out
	}
	catch(SimpleSocketException *exp)
	{
	  exp->Response();
	  throw exp;
	}
	

	WaitForServerConnection(strDottedQuadAddr,ADDRESS, bImpatient);
	
	return;
	
}

//----------------------------------------------------------------------------------------
/**
Set up a client socket and make a first connection.


@param hostID, remote server host
@param hType, host type (ip address or a name?)
@param pNumber, remote server port port
@param bImpatient, true - no waiting during connection, false - waits until connected
*/
//----------------------------------------------------------------------------------------
SimpleTcpSocket::SimpleTcpSocket(const std::string& hostID, 
				 hostType hType, int pNumber,
				 bool bImpatient)
{
	m_nSocketId = -1;
	
	m_bPortNumberSet=true;
	m_nPortNumber = pNumber;
	
	// Indicates that none of these have been set yet.
	// Flags will be set to true when appopriate Set() methods are called.
	m_bNonBlockingModeSet=false; 
	m_bDebugModeSet=false;
	m_bReuseModeSet=false;
	m_bKeepAliveModeSet=false;
	m_bLingerSecondsSet=false;
	m_bLingerOnOffSet=false;

	m_bAcceptTimeoutSecondsSet=false;
	m_bSocketTimeoutSecondsSet=false;

	m_bSendBufSizeSet=false;
	m_bReceiveBufSizeSet=false;

	// Safe class member init values *not* necessarily default
	// socket operation settings. 
	m_nSendBufSize=DEFAULT_SEND_BUFSIZE;
	m_nReceiveBufSize=DEFAULT_RECV_BUFSIZE;
	m_nLingerSeconds=DEFAULT_LINGER_SECONDS;
	m_nAcceptTimeoutSeconds=DEFAULT_ACCEPT_TIMEOUT;
	m_nSocketTimeoutSeconds=DEFAULT_SOCKET_TIMEOUT;
	
	m_nLastSendTime_t=0;
	m_nLastRecvTime_t=0;

	m_nSendFailureCount=0;
	m_nRecvFailureCount=0;

	// Trigger reconnection in non-blocking mode
	m_nNonblockResetTimeout=NONBLOCK_RESET_TIMEOUT_SEC;
	m_nNonblockResetFailCount=NONBLOCK_RESET_FAIL_COUNT;


	/* 
	   set the initial address of client that shall be communicated with to 
	   any address as long as they are using the same port number. 
	   The m_clientAddr structure is used in the future for storing the actual
	   address of client applications with which communication is going
	   to start
	*/
	m_clientAddr.sin_family = AF_INET;
	m_clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_clientAddr.sin_port = htons(m_nPortNumber);
	
	m_strClientHostName="";
	
	try
	{
	  CreateSocket();
	  SetPortNumber(pNumber);
	  SetSocketNonBlocking(0); // default to blocking
	  SetReuseAddr(true);
	  SetSocketTimeoutSeconds(0); // no socket timeout
	}
	catch(SimpleSocketException *exp)
	{
	  exp->Response();
	  throw exp;
	}
	
	WaitForServerConnection(hostID, hType, bImpatient);
	
	// Now call AcceptConnection() and you are good to go.
	// ...unless bImpatient is set to true.  If it is, then
	// there may be a possiblity it was not connected.  It
	// is up to the owner of this SimpleSocket object
	// to call a connect (initclient) again.


}

std::string SimpleTcpSocket::GetDottedQuadIpAddr(const std::string& strHostNameOrIp)
{
	
	struct hostent *host;
	std::string strDottedQuadAddr;

	
	host=gethostbyname(strHostNameOrIp.c_str());
	if(NULL==host)
	{
		SimpleSocketException *sexp 
			= new SimpleSocketException(MISC_ERROR,
						    "NULL hostname: Failed to resolve host name");
		throw sexp;
	}

	// Paranoia...
	if(NULL==((unsigned long*)host->h_addr_list[0]))
	{
		SimpleSocketException *sexp 
			= new SimpleSocketException(MISC_ERROR,
						    "NULL host-addr-list: Failed to resolve host name");
		throw sexp;
	}

#ifdef UNIX
	struct in_addr h_addr;
	h_addr.s_addr=*((unsigned long*)host->h_addr_list[0]);
	strDottedQuadAddr=std::string(inet_ntoa(h_addr));
#endif

#ifdef WINDOWS_XP
	strDottedQuadAddr=std::string(inet_ntoa(*(in_addr*)host->h_addr_list[0]));
#endif
	
	if(strDottedQuadAddr.length()<std::string("1.1.1.1").length())
	{
		// Test for min valid dotted-quad str length --
		// an easy sanity check
		SimpleSocketException *sexp 
			= new SimpleSocketException
			(MISC_ERROR,
			 "Failed to generate valid dotted-quad addr");
		throw sexp;
	}


	return strDottedQuadAddr;

}

void SimpleTcpSocket::BindSocket(void)
{
  if (bind(m_nSocketId,(struct sockaddr *)&m_clientAddr,sizeof(struct sockaddr_in))==-1)
  {

#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "error calling bind(): \n";
    DetectErrorBind(&errorCode,errorMsg);
    SimpleSocketException* socketBindException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketBindException;
#endif

#ifdef UNIX
    SimpleSocketException* unixSocketBindException 
      = new SimpleSocketException(BIND_ERROR,STR_ERR_CALLING_BIND.c_str());
    throw unixSocketBindException;
#endif

  }
  std::cout<<"Just bound socket to port..."<<std::endl;
  
}

void SimpleTcpSocket::ConnectToServer(const std::string& serverNameOrAddr)
{
	std::string strDottedQuadIpAddr;
	
	strDottedQuadIpAddr=GetDottedQuadIpAddr(serverNameOrAddr);
	
	ConnectToServer(strDottedQuadIpAddr,ADDRESS);
	
	return;
}


void SimpleTcpSocket::ConnectToServer(const std::string& serverNameOrAddr,hostType hType)
{ 
  /* 
     when this method is called, a client socket has been built already,
     so we have the socketId and portNumber ready.

     a SimpleSocketHostInfo instance is created, no matter how the server's name is 
     given (such as www.yuchen.net) or the server's address is given (such
     as 169.56.32.35), we can use this SimpleSocketHostInfo instance to get the 
     IP address of the server
  */

  SimpleSocketHostInfo serverInfo(serverNameOrAddr,hType);
	
  // Store the IP address and socket port number	
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr(serverInfo.GetHostIPAddress());
  serverAddress.sin_port = htons(m_nPortNumber);

  if (connect(m_nSocketId,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) == -1)
  {
#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "error calling connect():\n";
    DetectErrorConnect(&errorCode,errorMsg);
    SimpleSocketException* socketConnectException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketConnectException;
#endif
      
#ifdef UNIX
    std::cout<<"Error calling connect()\n";
    perror("connect()");
    SimpleSocketException* unixSocketConnectException 
      = new SimpleSocketException(CONNECT_ERROR,STR_ERR_CALLING_CONNECT.c_str());
    throw unixSocketConnectException;
#endif
  }
  return;
}


void SimpleTcpSocket::ReconnectToServer(const std::string& serverNameOrAddr)
{
	std::string strDottedQuadAddr;
	
	strDottedQuadAddr=GetDottedQuadIpAddr(serverNameOrAddr);
	
	ReconnectToServer(strDottedQuadAddr,ADDRESS);
	
	return;
}


void SimpleTcpSocket::ReconnectToServer(const std::string& serverNameOrAddr, hostType hType)
{
	int nNonBlockingMode;
	
	
	CloseSocket();
	
	CreateSocket();

	nNonBlockingMode=m_nNonBlockingMode;
	
	if(nNonBlockingMode!=0)
	{
		// Set socket to blocking until we connect
		SetSocketNonBlocking(0);
	}

	WaitForServerConnection(serverNameOrAddr,hType);

	if(nNonBlockingMode!=0)
	{
		// If originally non-blocking, set it back to non-blocking
		SetSocketNonBlocking(nNonBlockingMode);
	}

	// Transfer previous connection settings to new connection
	TransferSocketSettings(this);
	
	
	return;
	
}


void SimpleTcpSocket::WaitForServerConnection(const std::string& serverNameOrAddr)
{
	std::string strDottedQuadAddr;
	
	strDottedQuadAddr=GetDottedQuadIpAddr(serverNameOrAddr);
	
	WaitForServerConnection(strDottedQuadAddr,ADDRESS);
	
	return;
}


void SimpleTcpSocket::WaitForServerConnection(const std::string& serverNameOrAddr, hostType hType, bool bImPatient)
{
	
	bool bIsConnected=false;
	
	do
	{
		try
		{
			ConnectToServer(serverNameOrAddr,hType);
			bIsConnected=true;
		}
		catch(SimpleSocketException *exp)
		{
		  exp->Response();
		  delete exp;
		  bIsConnected=false;
		  SleepSec(1);
		}
		catch(...)
		{
		  std::cerr<<__FUNCTION__
			   <<"(): Mystery exception -- attempted reconnect failed -- try again..."
			   <<std::endl;
		  bIsConnected=false;
		  SleepSec(1);
		}
	} while(!bIsConnected && !bImPatient);
	
	return;
	
}


SimpleTcpSocket* SimpleTcpSocket::AcceptClient(std::string& clientHostOut)
{
	SimpleTcpSocket* pClientConnectSocket;
	
	pClientConnectSocket=AcceptClient(clientHostOut,
					  m_nAcceptTimeoutSeconds);
	
	pClientConnectSocket->TransferSocketSettings(this);
	
	return pClientConnectSocket;
}


SimpleTcpSocket* SimpleTcpSocket::AcceptClient(std::string& clientHostOut, 
					       int acceptTimeoutSec)
{
  int newSocket;   // the new socket file descriptor returned by the accept systme call

  std::stringstream sstr;
  sstr<<__FUNCTION__<<"(): m_nSocketId = " << m_nSocketId<<std::endl;
  
#ifdef WINDOWS_XP
  OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
  std::cout<<sstr.str()<<std::endl;
#endif

  if(acceptTimeoutSec>0)
  {
    struct timeval tv_timeout;
    tv_timeout.tv_sec=acceptTimeoutSec;
    tv_timeout.tv_usec=0;

    int selectRetVal;

    fd_set acceptSet;

    int maxSock=-1;
      
    FD_ZERO(&acceptSet);
    FD_SET(m_nSocketId,&acceptSet);

    maxSock=m_nSocketId;
      
    selectRetVal=select(maxSock+1,&acceptSet,NULL,NULL,&tv_timeout);
    if(selectRetVal<=0)
    {
      std::stringstream sstr;
      sstr<<"########### "<<__FUNCTION__<<"(): Server/Accept timeout " 
	  << "############"<<std::endl;

#ifdef WINDOWS_XP
      OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
      std::cout<<sstr.str()<<std::endl;
#endif

#ifdef WINDOWS_XP
      int errorCode = 0;
      std::string errorMsg = "Select timeout in acceptClient(): \n";
      DetectErrorAccept(&errorCode,errorMsg);
      SimpleSocketException* socketAcceptException 
	= new SimpleSocketException(errorCode,errorMsg);
      throw socketAcceptException;
#endif
      
#ifdef UNIX
      SimpleSocketException* unixSocketAcceptException 
	      = new SimpleSocketException(SELECT_ERROR,STR_ERR_CALLING_ACCEPT.c_str());
      throw unixSocketAcceptException;
#endif
    }
    else
    {
      std::stringstream sstr;
      sstr<<"########### "<<__FUNCTION__<<"(): Server/Accept select retval="
	  << selectRetVal << "############"<<std::endl;

#ifdef WINDOWS_XP
      OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
      std::cout<<sstr.str()<<std::endl;
#endif

    }
  }


  // the length of the client's address
  socklen_t clientAddressLen = sizeof(struct sockaddr_in);
  struct sockaddr_in clientAddress;    // Address of the client that sent data

  // Accepts a new client connection and stores its socket file descriptor
  if ((newSocket = accept(m_nSocketId, 
			  (struct sockaddr *)&clientAddress,&clientAddressLen)) == -1)
  {
#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "error calling accept(): \n";
    DetectErrorAccept(&errorCode,errorMsg);
    SimpleSocketException* socketAcceptException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketAcceptException;
#endif
      
#ifdef UNIX
    std::cout << "Error calling accept(): \n";
    SimpleSocketException* unixSocketAcceptException 
	    = new SimpleSocketException(ACCEPT_ERROR,STR_ERR_CALLING_ACCEPT.c_str());
    throw unixSocketAcceptException;
#endif
  }
    
  // Get the host name given the address
  char *sAddress = inet_ntoa((struct in_addr)clientAddress.sin_addr);
  SimpleSocketHostInfo clientInfo(std::string(sAddress),ADDRESS);
  const char* hostName = clientInfo.GetHostName();
  clientHostOut += std::string(hostName);
	
  // Create and return the new SimpleTcpSocket object
  SimpleTcpSocket* retSocket = new SimpleTcpSocket();
  retSocket->SetSocketId(newSocket);

  retSocket->SetAcceptTimeoutSeconds(acceptTimeoutSec);
  
  return retSocket;
}

// New 2012/02/17
SimpleTcpSocket* SimpleTcpSocket::AcceptClient(SimpleTcpSocket* pSimpleSocket,
					       std::string& clientHostOut)
{
	SimpleTcpSocket* pClientConnectedSocket;
	
	pClientConnectedSocket=AcceptClient(pSimpleSocket,clientHostOut,m_nAcceptTimeoutSeconds);
	
	pClientConnectedSocket->TransferSocketSettings(this);

	return pClientConnectedSocket;
}



SimpleTcpSocket* SimpleTcpSocket::AcceptClient(SimpleTcpSocket* pSimpleSocket,
					       std::string& clientHostOut, 
					       int acceptTimeoutSec)
{
  int newSocket;   // the new socket file descriptor returned by the accept systme call

  std::stringstream sstr;
  sstr<<__FUNCTION__<<"(): m_nSocketId = " << m_nSocketId<<std::endl;
  
#ifdef WINDOWS_XP
  OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
  std::cout<<sstr.str()<<std::endl;
#endif

  if(acceptTimeoutSec>0)
  {
    struct timeval tv_timeout;
    tv_timeout.tv_sec=acceptTimeoutSec;
    tv_timeout.tv_usec=0;

    int selectRetVal;

    fd_set acceptSet;

    int maxSock=-1;
      
    FD_ZERO(&acceptSet);
    FD_SET(m_nSocketId,&acceptSet);

    maxSock=m_nSocketId;
      
    selectRetVal=select(maxSock+1,&acceptSet,NULL,NULL,&tv_timeout);
    if(selectRetVal<=0)
    {
      std::stringstream sstr;
      sstr<<"########### "<<__FUNCTION__<<"(): Server/Accept timeout " 
	  << "############"<<std::endl;

#ifdef WINDOWS_XP
      OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
      std::cout<<sstr.str()<<std::endl;
#endif

#ifdef WINDOWS_XP
      int errorCode = 0;
      std::string errorMsg = "Select timeout in acceptClient(): \n";
      DetectErrorAccept(&errorCode,errorMsg);
      SimpleSocketException* socketAcceptException 
	= new SimpleSocketException(errorCode,errorMsg);
      throw socketAcceptException;
#endif
      
#ifdef UNIX
      SimpleSocketException* unixSocketAcceptException 
	      = new SimpleSocketException(SELECT_ERROR,STR_ERR_CALLING_ACCEPT.c_str());
      throw unixSocketAcceptException;
#endif
    }
    else
    {
      std::stringstream sstr;
      sstr<<"########### "<<__FUNCTION__<<"(): Server/Accept select retval="
	  << selectRetVal << "############"<<std::endl;

#ifdef WINDOWS_XP
      OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
      std::cout<<sstr.str()<<std::endl;
#endif

    }
  }


  // the length of the client's address
  socklen_t clientAddressLen = sizeof(struct sockaddr_in);
  struct sockaddr_in clientAddress;    // Address of the client that sent data

  // Accepts a new client connection and stores its socket file descriptor
  if ((newSocket = accept(m_nSocketId, 
			  (struct sockaddr *)&clientAddress,&clientAddressLen)) == -1)
  {
#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "error calling accept(): \n";
    DetectErrorAccept(&errorCode,errorMsg);
    SimpleSocketException* socketAcceptException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketAcceptException;
#endif
      
#ifdef UNIX
    std::cout << "Error calling accept(): \n";
    SimpleSocketException* unixSocketAcceptException 
	    = new SimpleSocketException(ACCEPT_ERROR,STR_ERR_CALLING_ACCEPT.c_str());
    throw unixSocketAcceptException;
#endif
  }
    
  // Get the host name given the address
  char *sAddress = inet_ntoa((struct in_addr)clientAddress.sin_addr);
  SimpleSocketHostInfo clientInfo(std::string(sAddress),ADDRESS);
  const char* hostName = clientInfo.GetHostName();
  clientHostOut = std::string(hostName);
	
  m_strClientHostName=clientHostOut;
  
  // Create and return the new SimpleTcpSocket object
  SimpleTcpSocket* retSocket = new SimpleTcpSocket(this);
  retSocket->SetSocketId(newSocket);

  retSocket->SetAcceptTimeoutSeconds(acceptTimeoutSec);

  return retSocket;
}



// Returns just the socket id, not the full tcpsocket object -- socket settings
// are not preserved -- must be restored "manually" after disconnect/reconnect.
int SimpleTcpSocket::AcceptClientSocket(std::string& clientHostOut, int acceptTimeoutSec)
{
  int newSocket;   // the new socket file descriptor returned by the accept systme call

  std::stringstream sstr;
  sstr<<__FUNCTION__<<"(): m_nSocketId = " << m_nSocketId<<std::endl;

#ifdef WINDOWS_XP
  OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
  std::cout<<sstr.str()<<std::endl;
#endif

  // the length of the client's address
  socklen_t clientAddressLen = sizeof(struct sockaddr_in);
  struct sockaddr_in clientAddress;    // Address of the client that sent data

  if(acceptTimeoutSec>0)
  {
    struct timeval tv_timeout;
    tv_timeout.tv_sec=acceptTimeoutSec;
    tv_timeout.tv_usec=0;

    int selectRetVal;

    fd_set acceptSet;

    int maxSock=-1;
      
    FD_ZERO(&acceptSet);
    FD_SET(m_nSocketId,&acceptSet);
    maxSock=m_nSocketId;
      
    selectRetVal=select(maxSock+1,&acceptSet,NULL,NULL,&tv_timeout);
    if(selectRetVal<=0)
    {
      std::stringstream sstr;
      sstr<<"########### "<<__FUNCTION__<<"(): Server/Accept timeout " 
	  << "############"<<std::endl;

#ifdef WINDOWS_XP
      OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
  std::cout<<sstr.str()<<std::endl;
#endif

#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "select error: \n";
    DetectErrorAccept(&errorCode,errorMsg);
    SimpleSocketException* socketAcceptException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketAcceptException;
#endif
      
#ifdef UNIX
    std::cout << "Select error: \n";
    SimpleSocketException* unixSocketAcceptException 
	    = new SimpleSocketException(SELECT_ERROR,STR_ERR_CALLING_ACCEPT.c_str());
    throw unixSocketAcceptException;
#endif


    }
    else
    {
      std::stringstream sstr;
      sstr<<"########### "<<__FUNCTION__<<"(): Server/Accept select retval="
	  << selectRetVal << "############"<<std::endl;
#ifdef WINDOWS_XP
      OutputDebugString(sstr.str().c_str());
#endif

#ifdef UNIX
  std::cout<<sstr.str()<<std::endl;
#endif

    }
  }

  // Accepts a new client connection and stores its socket file descriptor
  if ((newSocket = accept(m_nSocketId, 
			  (struct sockaddr *)&clientAddress,
			  &clientAddressLen)) == -1)
  {

#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "error calling accept(): \n";
    DetectErrorAccept(&errorCode,errorMsg);
    SimpleSocketException* socketAcceptException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketAcceptException;
#endif
      
#ifdef UNIX
    std::cout << "Error calling accept(): \n";
    SimpleSocketException* unixSocketAcceptException 
	    = new SimpleSocketException(ACCEPT_ERROR,STR_ERR_CALLING_ACCEPT.c_str());
    throw unixSocketAcceptException;
#endif
  }
    
  // Get the host name given the address
  char *sAddress = inet_ntoa((struct in_addr)clientAddress.sin_addr);
  SimpleSocketHostInfo clientInfo(std::string(sAddress),ADDRESS);
  const char* hostName = clientInfo.GetHostName();
  clientHostOut += std::string(hostName)+ std::string("\n");
	
  m_strClientHostName=clientHostOut;
  
#ifdef WINDOWS_XP
  OutputDebugString(clientHostOut.c_str());
#endif

#ifdef UNIX
  std::cout<<sstr.str()<<std::endl;
#endif


  return newSocket;
}


void SimpleTcpSocket::ListenToClient(int totalNumPorts) 
{
  if (listen(m_nSocketId,totalNumPorts) == -1)
  {
#ifdef WINDOWS_XP
    int errorCode = 0;
    std::string errorMsg = "error calling listen():\n";
    DetectErrorListen(&errorCode,errorMsg);
    SimpleSocketException* socketListenException 
      = new SimpleSocketException(errorCode,errorMsg);
    throw socketListenException;
#endif
      
#ifdef UNIX
    SimpleSocketException* unixSocketListenException 
      = new SimpleSocketException(LISTEN_ERROR,STR_ERR_CALLING_LISTEN.c_str());
    throw unixSocketListenException;
#endif
  }
}       

int SimpleTcpSocket::SendData(const char *dataBufIn, int dataBufLen)
{
	int nbytesSend=-1;
	
	nbytesSend=::send(m_nSocketId,dataBufIn,dataBufLen,
#ifdef UNIX
#ifdef LINUX
			  MSG_NOSIGNAL | /* Suppress SIGPIPE signals that can kill the app */
#endif
#endif
			  0);
	// a return val of -1 bytes throws an exception
	// For send(), a return val of 0 should not throw an exception.
	// "man send" for details.
	if(nbytesSend<0) 
	{
		// Blocking mode -- throw an exception immediately
		if(0==m_nNonBlockingMode)
		{
			std::cerr<<errno<<std::endl;
		
			SimpleSocketException *sendException =
				new SimpleSocketException(SEND_ERROR,
							  STR_ERR_CALLING_SEND.c_str());
			throw sendException;
		}
		else
		{
			m_nSendFailureCount++;
			if(m_nLastSendTime_t>0)
			{
				
				if(((time(NULL)-m_nLastSendTime_t)>m_nNonblockResetTimeout)
				   && (m_nSendFailureCount>m_nNonblockResetFailCount))
				{
					std::string strError = "NONBLOCK SEND ERROR CONNECTION RESET.\n";
					strError+=STR_ERR_CALLING_SEND;
					
					SimpleSocketException *sendException =
						new SimpleSocketException(SEND_ERROR, strError);

					throw sendException;
				}
			}
		}
	}
	else
	{
		m_nLastSendTime_t=time(NULL);
		m_nSendFailureCount=0;
	}
	
	return nbytesSend;
						  
}

int SimpleTcpSocket::RecvData(char *dataBufOut, int dataBufLen)
{
	int nbytesRecv=-1;
	
	nbytesRecv=::recv(m_nSocketId,dataBufOut,dataBufLen,
#ifdef UNIX
#ifdef LINUX
			  MSG_NOSIGNAL | /* Suppress SIGPIPE signals that can kill the app */
#endif
#endif
			  0);
	// For recv, a return val of 0 means the other end has 
	// shut down cleanly -- want to throw an exception for this case.
	if(nbytesRecv<=0) // A return val of 0 or -1 bytes throws an exception
	{
		if(0==m_nNonBlockingMode)
		{
			SimpleSocketException *recvException =
				new SimpleSocketException(RECV_ERROR,
							  STR_ERR_CALLING_RECV.c_str());
			throw recvException;
		}
		else
		{
			// Non-blocking mode -- will have to decide when to give up on the
			// connection and throw an exception...
			m_nRecvFailureCount++;
			if(m_nLastRecvTime_t>0)
			{
				if(((time(NULL)-m_nLastSendTime_t)>m_nNonblockResetTimeout)
				   && (m_nSendFailureCount>m_nNonblockResetFailCount))
				{
					std::string strError = "NONBLOCK RECV ERROR CONNECTION RESET.\n";
					strError+=STR_ERR_CALLING_RECV;
					
					SimpleSocketException *recvException =
						new SimpleSocketException(SEND_ERROR, strError);
					throw recvException;
				}
				
			}
			
		}
		
		
	}
	else
	{
		m_nLastRecvTime_t=time(NULL);
		m_nRecvFailureCount=0;
	}

	return nbytesRecv;
						  
}

SimpleTcpSocket* SimpleTcpSocket::WaitForClient(void)
{
	SimpleTcpSocket *pNewClientConnectSocket;
	
	std::string strClientHostName;
	
	bool bClientAccepted=false;
	while(false==bClientAccepted)
	{
		try
		{
			pNewClientConnectSocket
				=AcceptClient(strClientHostName);
			
			bClientAccepted=true;
		}
		catch(SimpleSocketException *exp)
		{
		  exp->Response();
		  delete exp;
		  bClientAccepted=false;
		}
		catch(...)
		{
		  std::cerr<<__FUNCTION__<<"(): Mystery exception caught..."<<std::endl;
		  bClientAccepted=false;
		}
		
	}

	return pNewClientConnectSocket;
	
}



SimpleTcpSocket* SimpleTcpSocket::ReconnectToClient(SimpleTcpSocket* pServerSocket,
						    SimpleTcpSocket *pOldClientConnectSocket)
{
	std::string strClientHostName;
	
	SimpleTcpSocket* pNewClientConnectSocket=NULL;
	
	if(pOldClientConnectSocket!=NULL)
	{

		pOldClientConnectSocket->CloseSocket();
	
		bool bClientAccepted=false;
	
		while(false==bClientAccepted)
		{
			try
			{
				pNewClientConnectSocket
					=pServerSocket->AcceptClient(pServerSocket,
								     strClientHostName);

				if(NULL!=pNewClientConnectSocket)
				{
					pNewClientConnectSocket->TransferSocketSettings(pOldClientConnectSocket);

					bClientAccepted=true;
				}
				else
				{
					sleep(1); // Don't want to hog the CPU if this happens
				}
					
			}
			catch(SimpleSocketException *exp)
			{
				std::cerr <<__FUNCTION__<< "(): Reconnection attempt failed..."<<std::endl;
				
				exp->Response();
				delete exp;
				bClientAccepted=false;
				sleep(1); // Avoid CPU hogging...
			}
			catch(...)
			{
			  std::cerr<<__FUNCTION__<<"(): Mystery exception caught..."<<std::endl;
			  bClientAccepted=false;
			  sleep(1); // Avoid cpu hogging...
			}
			
		}
		
		delete pOldClientConnectSocket;
	}

	if(NULL==pNewClientConnectSocket)
	{
		std::cerr<<__FUNCTION__<<"(): NULL pNewClientConnectSocket -- something went wrong..."<<std::endl;
	}
	else
	{
		std::cerr<<__FUNCTION__<<"(): Client reconnect succeeded..."<<std::endl;
	}
	
	
	return pNewClientConnectSocket;
}



#ifdef WINDOWS_XP

void SimpleTcpSocket::DetectErrorBind(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEADDRINUSE )
  {
    errMsg.append("A process on the machine is already bound to the same\n");
    errMsg.append("fully-qualified address and the socket has not been marked\n"); 
    errMsg.append("to allow address re-use with SO_REUSEADDR. For example,\n");
    errMsg.append("IP address and port are bound in the af_inet case");
  }
  else if ( *errCode == WSAEADDRNOTAVAIL )
    errMsg.append("The specified address is not a valid address for this machine.");
  else if ( *errCode == WSAEFAULT )
  {
    errMsg.append("The name or the namelen parameter is not a valid part of\n");
    errMsg.append("the user address space, the namelen parameter is too small,\n");
    errMsg.append("the name parameter contains incorrect address format for the\n");
    errMsg.append("associated address family, or the first two bytes of the memory\n");
    errMsg.append("block specified by name does not match the address family\n");
    errMsg.append("associated with the socket descriptor s.");
  }
  else if ( *errCode == WSAEINPROGRESS )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the\n");
    errMsg.append("service provider is still processing a callback function.");
  }
  else if ( *errCode == WSAEINVAL )
    errMsg.append("The socket is already bound to an address. ");
  else if ( *errCode == WSAENOBUFS )
    errMsg.append("Not enough buffers available, too many connections.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else errMsg.append("unknown problems!");
}

void SimpleTcpSocket::DetectErrorRecv(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEFAULT )
    errMsg.append("The buf parameter is not completely contained in a valid part of the user address space.");
  else if ( *errCode == WSAENOTCONN )
    errMsg.append("The socket is not connected.");
  else if ( *errCode == WSAEINTR )
    errMsg.append("The (blocking) call was canceled through WSACancelBlockingCall.");
  else if ( *errCode == WSAEINPROGRESS )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the\n");
    errMsg.append("service provider is still processing a callback function.");
  }
  else if ( *errCode == WSAENETRESET )
  {
    errMsg.append("The connection has been broken due to the keep-alive activity\n");
    errMsg.append("detecting a failure while the operation was in progress.");
  }
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else if ( *errCode == WSAEOPNOTSUPP )
  {
    errMsg.append("MSG_OOB was specified, but the socket is not stream-style\n");
    errMsg.append("such as type SOCK_STREAM, out-of-band data is not supported\n");
    errMsg.append("in the communication domain associated with this socket, or\n");
    errMsg.append("the socket is unidirectional and supports only send operations.");
  }
  else if ( *errCode == WSAESHUTDOWN )
  {
    errMsg.append("The socket has been shut down; it is not possible to recv on a\n");
    errMsg.append("socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.");
  }
  else if ( *errCode == WSAEWOULDBLOCK )
    errMsg.append("The socket is marked as nonblocking and the receive operation would block.");
  else if ( *errCode == WSAEMSGSIZE )
    errMsg.append("The message was too large to fit into the specified buffer and was truncated.");
  else if ( *errCode == WSAEINVAL )
  {
    errMsg.append("The socket has not been bound with bind, or an unknown flag\n");
    errMsg.append("was specified, or MSG_OOB was specified for a socket with\n");
    errMsg.append("SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.");
  }
  else if ( *errCode == WSAECONNABORTED )
  {
    errMsg.append("The virtual circuit was terminated due to a time-out or\n");
    errMsg.append("other failure. The application should close the socket as it is no longer usable.");
  }
  else if ( *errCode == WSAETIMEDOUT )
  {
    errMsg.append("The connection has been dropped because of a network\n");
    errMsg.append("failure or because the peer system failed to respond.");
  }
  else if ( *errCode == WSAECONNRESET )
  {
    errMsg.append("The virtual circuit was reset by the remote side executing a\n");
    errMsg.append("\"hard\" or \"abortive\" close. The application should close\n");
    errMsg.append("the socket as it is no longer usable. On a UDP datagram socket\n");
    errMsg.append("this error would indicate that a previous send operation\n");
    errMsg.append("resulted in an ICMP \"Port Unreachable\" message.");
  }
  else errMsg.append("unknown problems!");
}

void SimpleTcpSocket::DetectErrorConnect(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEADDRINUSE )
  {
    errMsg.append("The socket's local address is already in use and the socket\n");
    errMsg.append("was not marked to allow address reuse with SO_REUSEADDR. This\n");
    errMsg.append("error usually occurs when executing bind, but could be delayed\n");
    errMsg.append("until this function if the bind was to a partially wild-card\n");
    errMsg.append("address (involving ADDR_ANY) and if a specific address needs\n");
    errMsg.append("to be committed at the time of this function.");
  }
  else if ( *errCode == WSAEINTR )
    errMsg.append("The (blocking) Windows Socket 1.1 call was canceled through WSACancelBlockingCall.");
  else if ( *errCode == WSAEINPROGRESS )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or\n");
    errMsg.append("the service provider is still processing a callback function.");
  }
  else if ( *errCode == WSAEALREADY )
  {
    errMsg.append("A nonblocking connect call is in progress on the specified socket.\n");
    errMsg.append("Note In order to preserve backward compatibility, this error is\n");
    errMsg.append("reported as WSAEINVAL to Windows Sockets 1.1 applications that\n");
    errMsg.append("link to either WINSOCK.DLL or WSOCK32.DLL.");
  }
  else if ( *errCode == WSAEADDRNOTAVAIL )
    errMsg.append("The remote address is not a valid address (such as ADDR_ANY).");
  else if ( *errCode == WSAEAFNOSUPPORT )
    errMsg.append("Addresses in the specified family cannot be used with this socket.");
  else if ( *errCode == WSAECONNREFUSED )
    errMsg.append("The attempt to connect was forcefully rejected.");
  else if ( *errCode == WSAEFAULT )
  {
    errMsg.append("The name or the namelen parameter is not a valid part of\n");
    errMsg.append("the user address space, the namelen parameter is too small,\n");
    errMsg.append("or the name parameter contains incorrect address format for\n");
    errMsg.append("the associated address family.");
  }
  else if ( *errCode == WSAEINVAL )
  {
    errMsg.append("The parameter s is a listening socket, or the destination\n");
    errMsg.append("address specified is not consistent with that of the constrained\n");
    errMsg.append("group the socket belongs to.");
  }
  else if ( *errCode == WSAEISCONN )
    errMsg.append("The socket is already connected (connection-oriented sockets only).");
  else if ( *errCode == WSAENETUNREACH )
    errMsg.append("The network cannot be reached from this host at this time.");
  else if ( *errCode == WSAENOBUFS )
    errMsg.append("No buffer space is available. The socket cannot be connected.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else if ( *errCode == WSAETIMEDOUT )
    errMsg.append("Attempt to connect timed out without establishing a connection.");
  else if ( *errCode == WSAEWOULDBLOCK )
  {
    errMsg.append("The socket is marked as nonblocking and the connection\n");
    errMsg.append("cannot be completed immediately.");
  }
  else if ( *errCode == WSAEACCES )
  {
    errMsg.append("Attempt to connect datagram socket to broadcast address failed\n");
    errMsg.append("because setsockopt option SO_BROADCAST is not enabled.");
  }
  else errMsg.append("unknown problems!");
}

void SimpleTcpSocket::DetectErrorAccept(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this FUNCTION.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEFAULT )
    errMsg.append("The addrlen parameter is too small or addr is not a valid part of the user address space.");
  else if ( *errCode == WSAEINTR )
    errMsg.append("A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall.");
  else if ( *errCode == WSAEINPROGRESS )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the\n");
    errMsg.append("service provider is still processing a callback function.");
  }
  else if ( *errCode == WSAEINVAL )
    errMsg.append("The listen function was not invoked prior to accept.");
  else if ( *errCode == WSAEMFILE )
    errMsg.append("The queue is nonempty upon entry to accept and there are no descriptors available.");
  else if ( *errCode == WSAENOBUFS )
    errMsg.append("No buffer space is available.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else if ( *errCode == WSAEOPNOTSUPP )
    errMsg.append("The referenced socket is not a type that supports connection-oriented service.");
  else if ( *errCode == WSAEWOULDBLOCK )
    errMsg.append("The socket is marked as nonblocking and no connections are present to be accepted.");
  else errMsg.append("unknown problems!");
}

void SimpleTcpSocket::DetectErrorListen(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEADDRINUSE )
  {
    errMsg.append("The socket's local address is already in use and the socket was\n");
    errMsg.append("not marked to allow address reuse with SO_REUSEADDR. This error\n");
    errMsg.append("usually occurs during execution of the bind function, but could\n");
    errMsg.append("be delayed until this function if the bind was to a partially\n");
    errMsg.append("wild-card address (involving ADDR_ANY) and if a specific address\n");
    errMsg.append("needs to be \"committed\" at the time of this function.");
  }
  else if ( *errCode == WSAEINPROGRESS )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service\n");
    errMsg.append("provider is still processing a callback function.");
  }
  else if ( *errCode == WSAEINVAL )
    errMsg.append("The socket has not been bound with bind.");
  else if ( *errCode == WSAEISCONN )
    errMsg.append("The socket is already connected.");
  else if ( *errCode == WSAEMFILE )
    errMsg.append("No more socket descriptors are available.");
  else if ( *errCode == WSAENOBUFS )
    errMsg.append("No buffer space is available.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else if ( *errCode == WSAEOPNOTSUPP )
    errMsg.append("The referenced socket is not of a type that supports the listen operation.");
  else errMsg.append("unknown problems!");
}

void SimpleTcpSocket::DetectErrorSend(int* errCode,std::string& errMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errMsg.append("A successful WSAStartup must occur before using this function.");
  else if ( *errCode == WSAENETDOWN )
    errMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAEACCES )
  {
    errMsg.append("The requested address is a broadcast address,\n");
    errMsg.append("but the appropriate flag was not set. Call setsockopt\n");
    errMsg.append("with the SO_BROADCAST parameter to allow the use of the broadcast address.");
  }
  else if ( *errCode == WSAEINTR )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call was canceled\n");
    errMsg.append("through WSACancelBlockingCall.");
  }
  else if ( *errCode == WSAEINPROGRESS )
  {
    errMsg.append("A blocking Windows Sockets 1.1 call is in progress,\n");
    errMsg.append("or the service provider is still processing a callback function.");
  }
  else if ( *errCode == WSAEFAULT )
  {
    errMsg.append("The buf parameter is not completely contained in a\n");
    errMsg.append("valid part of the user address space.");
  }
  else if ( *errCode == WSAENETRESET )
  {
    errMsg.append("The connection has been broken due to the keep-alive\n");
    errMsg.append("activity detecting a failure while the operation was in progress.");
  }
  else if ( *errCode == WSAENOBUFS )
    errMsg.append("No buffer space is available.");
  else if ( *errCode == WSAENOTCONN )
    errMsg.append("The socket is not connected.");
  else if ( *errCode == WSAENOTSOCK )
    errMsg.append("The descriptor is not a socket.");
  else if ( *errCode == WSAEOPNOTSUPP )
  {
    errMsg.append("MSG_OOB was specified, but the socket is not stream-style\n");
    errMsg.append("such as type SOCK_STREAM, out-of-band data is not supported\n");
    errMsg.append("in the communication domain associated with this socket,\n");
    errMsg.append("or the socket is unidirectional and supports only receive operations.");
  }
  else if ( *errCode == WSAESHUTDOWN )
  {
    errMsg.append("The socket has been shut down; it is not possible to send\n");
    errMsg.append("on a socket after shutdown has been invoked with how set\n");
    errMsg.append("to SD_SEND or SD_BOTH.");
  }
  else if ( *errCode == WSAEWOULDBLOCK )
    errMsg.append("The socket is marked as nonblocking and the requested operation would block.\n");
  else if ( *errCode == WSAEMSGSIZE )
  {
    errMsg.append("The socket is message oriented, and the message is larger\n");
    errMsg.append("than the maximum supported by the underlying transport.");
  }
  else if ( *errCode == WSAEHOSTUNREACH )
    errMsg.append("The remote host cannot be reached from this host at this time.");
  else if ( *errCode == WSAEINVAL )
  {
    errMsg.append("The socket has not been bound with bind, or an unknown flag\n");
    errMsg.append("was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled.");
  }
  else if ( *errCode == WSAECONNABORTED )
  {
    errMsg.append("The virtual circuit was terminated due to a time-out or \n");
    errMsg.append("other failure. The application should close the socket as it is no longer usable.");
  }
  else if ( *errCode == WSAECONNRESET )
  {
    errMsg.append("The virtual circuit was reset by the remote side executing a \"hard\" \n");
    errMsg.append("or \"abortive\" close. For UPD sockets, the remote host was unable to\n");
    errMsg.append("deliver a previously sent UDP datagram and responded with a\n");
    errMsg.append("\"Port Unreachable\" ICMP packet. The application should close\n");
    errMsg.append("the socket as it is no longer usable.");
  }
  else if ( *errCode == WSAETIMEDOUT )
  {
    errMsg.append("The connection has been dropped, because of a network failure\n");
    errMsg.append("or because the system on the other end went down without notice.");
  }
  else errMsg.append("unknown problems!");
}

#endif









	
std::ostream& operator<<(std::ostream& io,  SimpleSocket& s)
{
  std::string flagStr = "";

  io << "--------------- Summary of socket settings -------------------" << std::endl;
  io << "   Socket Id:     " << s.GetSocketId() << std::endl;
  io << "   port #:        " << s.GetPortNumber() << std::endl;
  io << "   debug:         " << (flagStr = s.GetDebug()? "true":"false" ) << std::endl;
  io << "   reuse addr:    " << (flagStr = s.GetReuseAddr()? "true":"false" ) << std::endl;
  io << "   keep alive:    " << (flagStr = s.GetKeepAlive()? "true":"false" ) << std::endl;
  io << "   send buf size: " << s.GetSendBufSize() << std::endl;
  io << "   recv bug size: " << s.GetReceiveBufSize() << std::endl;
  io << "   nonblocking:   " << (flagStr = s.GetSocketNonBlocking()? "true":"false" ) << std::endl;
  io << "   linger on:     " << (flagStr = s.GetLingerOnOff()? "true":"false" ) << std::endl;
  io << "   linger seconds: " << s.GetLingerSeconds() << std::endl;
  io << "----------- End of Summary of socket settings ----------------" << std::endl;
  return io;
}

