/***************************************************************

  Stolen shamelessly from Liyang Yu (although he mentions that mentioning him, the original
  article and his work is enough for licensing, see:
  http://beta.codeproject.com/Messages/3793113/Re-Once-again-License.aspx and the licenses
  are at:
  http://beta.codeproject.com/info/Licenses.aspx so we could probably assume the CPOL).

***************************************************************/


//----------------------------------------------------------------------------
/* 
   This class defines the Exception objects thrown by 
   SimpleSocket Network IO functions in the event of network-related errors.

   Response() dumps the specified error message to the console output.

   SetRemoteDisconnectCode() is set to indicate which network connection
                  failed (if possible).  Used to determine whether to reset 
                  the RAID connection,  SPAC connection, or both.

   GetRemoteDisconnectCode() -- retrieve the disconnect code.

   Used primarily to tell the Simple when to reset connections to RAID
   and/or SPAC.

*/
//-----------------------------------------------------------------------------
#ifndef ROBUSTSOCKETEXCEPTION_H
#define ROBUSTSOCKETEXCEPTION_H

#include "SimpleSocketPlatformDef.h"
#include "SimpleSocket.h"
#include "SimpleSocketLog.h"

// using namespace ROBUST_SOCKET;

class SimpleSocketException
{

public:

    // int: error code, string is the concrete error message
	SimpleSocketException(int,const std::string&);   
	virtual ~SimpleSocketException() {};

	/*
	   how to handle the exception is done here, so 
	   far, just write the message to screen and log file
	*/
	virtual void Response();  
	int GetErrCode()    { return m_iErrorCode; }

	void SetRemoteDisconnectCode(int disconnectCode)
	{
	  m_disconnectCode=disconnectCode;
	}

	int GetRemoteDisconnectCode() {return m_disconnectCode; }

	std::string& GetErrMsg() { return m_strErrorMsg; }


private:
	void InitVars();

private:
	int   m_iErrorCode;

	std::string m_strErrorMsg;

	int m_disconnectCode;
};

#endif
