/***************************************************************

  Stolen shamelessly from Liyang Yu (although he mentions that mentioning him, the original
  article and his work is enough for licensing, see:
  http://beta.codeproject.com/Messages/3793113/Re-Once-again-License.aspx and the licenses
  are at:
  http://beta.codeproject.com/info/Licenses.aspx so we could probably assume the CPOL).

***************************************************************/

//----------------------------------------------------------------------------
//
//   Class: SimpleSocketLog
//   
//   Dumps connection logging information to the console or to a log file
//
//-----------------------------------------------------------------------------

#ifndef SimpleSocketLog_H
#define SimpleSocketLog_H 

#include "SimpleSocketPlatformDef.h"

#include <string>
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>

// using namespace std;

/* 
   the log file has to be accessed from any code
   which includes this header (similiar to cout, cerr, clog, etc..)
*/

class SimpleSocketLog;
extern SimpleSocketLog winLog;

// const string SD_DEFAULT_LOGFILE = "simDelta.log";

const std::ios_base::openmode LOG_WRITE  = std::ios_base::out;
const std::ios_base::openmode LOG_APPEND = std::ios_base::app;

const int EXIT_MSG_SIZE = 512;
const int MAX_EXIT_CODES = 3;

class SimpleSocketLog : public std::ofstream
{

    public:

	enum logLevels 
	{
		LEVEL_0,       // buffer all log messages
		LEVEL_1,       // buffer Level one, two and three log messages
		LEVEL_2,       // buffer Level two and three log messages
		LEVEL_3,       // buffer Level three log messages
		QUIET_MODE     // do not print out any messages
	};
 
	SimpleSocketLog();
	SimpleSocketLog(const std::string&);
	SimpleSocketLog(const std::string&,enum logLevels);
	virtual ~SimpleSocketLog();

    private:

	void initVars();
	void init(const std::string&);
	void init(const std::string&, std::ios_base::openmode mode);
	void init();

	char* getExecTime();
	void  getExecTime(int*,int*);
	void  openLog(const std::string&, std::ios_base::openmode mode);
	void  printHeader(int);

    private:

	static const std::string STR_ROBUST_SOCKET_LOG_FILE;


	std::string logName;
	enum logLevels logLevel;
	time_t startTime;

};

const enum SimpleSocketLog::logLevels L0 = SimpleSocketLog::LEVEL_0;
const enum SimpleSocketLog::logLevels L1 = SimpleSocketLog::LEVEL_1;
const enum SimpleSocketLog::logLevels L2 = SimpleSocketLog::LEVEL_2;
const enum SimpleSocketLog::logLevels L3 = SimpleSocketLog::LEVEL_3;
const enum SimpleSocketLog::logLevels LQUIET = SimpleSocketLog::QUIET_MODE;

#endif


