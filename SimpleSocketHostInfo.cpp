#include "SimpleSocketHostInfo.h"
#include "SimpleSocketLog.h"
#include "SimpleSocketException.h"

SimpleSocketHostInfo::SimpleSocketHostInfo()
{

#ifdef UNIX
	// endhostent();
	// m_cSearchHostDB = 1;
	// sethostent(1);
  OpenHostDb();
  // winLog<<"UNIX version SimpleSocketHostInfo() is called...\n";
#endif

#ifdef WINDOWS_XP
	
  char sName[HOST_NAME_LENGTH+1];
  memset(sName,0,sizeof(sName));
  gethostname(sName,HOST_NAME_LENGTH);

  try 
    {
      mp_cHostPtr = gethostbyname(sName);
      if (mp_cHostPtr == NULL)
	{
	  std::cerr<<__FILE__<<":"<<__LINE__<<"   gethostbyname exception."<<std::endl;
	  
	  int errorCode;
	  std::string errorMsg = "";
	  DetectErrorGethostbyname(&errorCode,errorMsg);
	  SimpleSocketException* gethostbynameException = 
	    new SimpleSocketException(errorCode,errorMsg);
	  throw gethostbynameException;
	}
    }
  catch(SimpleSocketException* excp)
    {
      if(NULL!=excp)
      {
          excp->Response();
	  delete excp;
      }
      std::cerr<<"This error/exception can safely be ignored.  Carry on..."<<std::endl;
// dont want to throw this      throw excp; // RRR 2011/07/21 the exit() kills the program
      //      exit(1);
    }
	
#endif

}

SimpleSocketHostInfo::SimpleSocketHostInfo(const std::string& hostName,hostType type)
{
#ifdef UNIX
  m_cSearchHostDB = 0;
#endif

  try 
    {
      if (type == NAME)
	{
	  // Retrieve host by name
	  mp_cHostPtr = gethostbyname(hostName.c_str());

	  if (mp_cHostPtr == NULL)
	    {

#ifdef WINDOWS_XP
	      int errorCode;
	      std::string errorMsg = "";
	      DetectErrorGethostbyname(&errorCode,errorMsg);
	      SimpleSocketException* gethostbynameException \
		= new SimpleSocketException(errorCode,errorMsg);
	      throw gethostbynameException;
#endif
	
#ifdef UNIX
	      std::cerr<<__FILE__<<":"<<__LINE__<<"   gethostbyname exception."<<std::endl;
	      std::cerr<<__FUNCTION__<<"(): gethostbyname exeption."<<std::endl;
	      std::cerr<<"       "<<hostName<<", "<<type<<std::endl;
	      
	      SimpleSocketException* gethostbynameException 
		= new SimpleSocketException(0,"unix: error getting host by name");
	      throw gethostbynameException;
#endif
	    }
        }
      else if (type == ADDRESS)
	{
	  // Retrieve host by address
	  unsigned long netAddr = inet_addr(hostName.c_str());
	  if (netAddr == INADDR_NONE)
	    {
	      SimpleSocketException* inet_addrException 
		= new SimpleSocketException(0,"Error calling inet_addr()");
	      throw inet_addrException;
	    }
	  mp_cHostPtr = gethostbyaddr((char *)&netAddr, 
	  			      sizeof(netAddr), AF_INET);
	  if (mp_cHostPtr == NULL)
	    {

#ifdef WINDOWS_XP
	      int errorCode;
	      std::string errorMsg = "";
	      DetectErrorGethostbyaddr(&errorCode,errorMsg);
	      SimpleSocketException* gethostbyaddrException 
		= new SimpleSocketException(errorCode,errorMsg);
	      throw gethostbyaddrException;
#endif
	
#ifdef UNIX
	      std::cerr<<__FILE__<<":"<<__LINE__<<"   gethostbyname exception."<<std::endl;
	      std::cerr<<__FUNCTION__<<"(): gethostbyname exception."<<std::endl;
	      std::cerr<<"       "<<hostName<<", "<<type<<std::endl;

	      SimpleSocketException* gethostbynameException 
		= new SimpleSocketException(0,"unix: error getting host by addr");
	      throw gethostbynameException;
#endif
	    }
        }
      else
	{
	  SimpleSocketException* unknownTypeException = 
	    new SimpleSocketException
	    (0,"unknown host type: host name/address has to be given ");
	  throw unknownTypeException;
	}
    }
  catch(SimpleSocketException* excp)
    {
      if(NULL!=excp)
      {
         excp->Response();
	 delete excp;
      }
      std::cerr<<"This error/exception can safely be ignored.  Carry on..."<<std::endl;
      
      // dont want to throw this: throw excp; // RRR 2011/07/21 the exit() kills the program
      //      exit(1);
    }
}

#ifdef WINDOWS_XP
void SimpleSocketHostInfo::DetectErrorGethostbyname(int* errCode,std::string& errorMsg)
{
  *errCode = WSAGetLastError();
	
  if ( *errCode == WSANOTINITIALISED )
    errorMsg.append
      ("need to call WSAStartup to initialize socket system on Window system.");
  else if ( *errCode == WSAENETDOWN )
    errorMsg.append("The network subsystem has failed.");
  else if ( *errCode == WSAHOST_NOT_FOUND )
    errorMsg.append("Authoritative Answer Host not found.");
  else if ( *errCode == WSATRY_AGAIN )
    errorMsg.append("Non-Authoritative Host not found, or server failure.");
  else if ( *errCode == WSANO_RECOVERY )
    errorMsg.append("Nonrecoverable error occurred.");
  else if ( *errCode == WSANO_DATA )
    errorMsg.append("Valid name, no data record of requested type.");
  else if ( *errCode == WSAEINPROGRESS )
    errorMsg.append
      ("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.");
  else if ( *errCode == WSAEFAULT )
    errorMsg.append
      ("The name parameter is not a valid part of the user address space.");
  else if ( *errCode == WSAEINTR )
    errorMsg.append
      ("A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.");
}
#endif

#ifdef WINDOWS_XP
void SimpleSocketHostInfo::DetectErrorGethostbyaddr(int* errCode,std::string& errorMsg)
{
  *errCode = WSAGetLastError();

  if ( *errCode == WSANOTINITIALISED )
    errorMsg.append("A successful WSAStartup must occur before using this function.");
  if ( *errCode == WSAENETDOWN )
    errorMsg.append("The network subsystem has failed.");
  if ( *errCode == WSAHOST_NOT_FOUND )
    errorMsg.append("Authoritative Answer Host not found.");
  if ( *errCode == WSATRY_AGAIN )
    errorMsg.append("Non-Authoritative Host not found, or server failed."); 
  if ( *errCode == WSANO_RECOVERY )
    errorMsg.append("Nonrecoverable error occurred.");
  if ( *errCode == WSANO_DATA )
    errorMsg.append("Valid name, no data record of requested type.");
  if ( *errCode == WSAEINPROGRESS )
    errorMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.");
  if ( *errCode == WSAEAFNOSUPPORT )
    errorMsg.append("The type specified is not supported by the Windows Sockets implementation.");
  if ( *errCode == WSAEFAULT )
    errorMsg.append("The addr parameter is not a valid part of the user address space, or the len parameter is too small.");
  if ( *errCode == WSAEINTR )
    errorMsg.append("A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.");
}
#endif

#ifdef UNIX
char SimpleSocketHostInfo::GetNextHost(void)
{
  // winLog<<"UNIX getNextHost() is called...\n";
  // Get the next host from the database
  // if (m_cSearchHostDB == 1)
  //   {
  //     if ((mp_cHostPtr = gethostent()) == NULL)
  // 	return 0;
  //     else
  // 	return 1;
  //   }
  return 0;
}
#endif

