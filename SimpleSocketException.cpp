#include "SimpleSocketException.h"

SimpleSocketException::SimpleSocketException(int errCode,const std::string& errMsg)
{
	InitVars();
	m_iErrorCode = errCode;
	if ( errMsg[0] ) m_strErrorMsg.append(errMsg);
}

void SimpleSocketException::InitVars()
{
	m_iErrorCode = 0;
	m_strErrorMsg = "";
}

void SimpleSocketException::Response()
{
	/*m_proxyLog << "Error detect: " << endl;
	m_proxyLog << "		==> error code: " << errorCode << endl;
	m_proxyLog << "		==> error message: " << errorMsg << endl;*/

	std::cout << "Error detect: " << std::endl;
	std::cout << "		==> error code: " << m_iErrorCode << std::endl;
	std::cout << "		==> error message: " << m_strErrorMsg << std::endl;
	std::cout.flush();

}
