#include "SimpleSocketLog.h"

const std::string SimpleSocketLog::STR_ROBUST_SOCKET_LOG_FILE="syslog.log";

const enum SimpleSocketLog::logLevels DEFAULT_LOG_LEVEL = SimpleSocketLog::LEVEL_1;

SimpleSocketLog::SimpleSocketLog()
{
        initVars();
        init();
} 

SimpleSocketLog::SimpleSocketLog(const std::string& fileName)
{
        initVars();
        init(fileName);
}

SimpleSocketLog::SimpleSocketLog(const std::string& fileName,enum logLevels levelIn)
{
        initVars();
        logLevel = levelIn;
        init(fileName);
}

SimpleSocketLog::~SimpleSocketLog()
{
        if ( logLevel<QUIET_MODE )
        {
                clear(std::ios::goodbit);
                *this << std::endl;
                printHeader(1);    // add ending time to log file
        }
        close();
}

void SimpleSocketLog::init(const std::string& fileName)
{
        if ( (fileName.c_str())[0] )
                openLog(fileName,LOG_WRITE);
        else
                openLog(STR_ROBUST_SOCKET_LOG_FILE.c_str(),LOG_WRITE);
}

void SimpleSocketLog::init(const std::string& fileName, std::ios_base::openmode mode)
{
        if ( (fileName.c_str())[0] )
                openLog(fileName,mode);
        else
                openLog(STR_ROBUST_SOCKET_LOG_FILE.c_str(), mode);
}

void SimpleSocketLog::init()
{
        openLog(STR_ROBUST_SOCKET_LOG_FILE.c_str(),LOG_WRITE);
}

void SimpleSocketLog::openLog(const std::string& fileName, std::ios_base::openmode mode)
{
        if (logLevel < QUIET_MODE)
        {
                open(fileName.c_str(),mode);
      
                // fail() returns a null zero when operation fails   
                // rc = (*this).fail();
    
                if ( fail() == 0 )
                {
                        logName = fileName;
                        printHeader(0);         // insert start time into top of log file
                }
                else
                {
                        std::cout << "ERROR: Log file " << fileName.c_str() 
                             << " could not be opened for write access." << std::endl;
                        logLevel = QUIET_MODE;
                }
        }
        else
        {
                std::cout << "Logging disabled (QUIET_MODE set)" << std::endl;
        }
}

void SimpleSocketLog::initVars()
{
        time(&startTime);
        logLevel = DEFAULT_LOG_LEVEL;
}

void SimpleSocketLog::printHeader(int theEnd)
{
        if ( logLevel < QUIET_MODE )
        {
                clear(std::ios::goodbit);

                // setup time
                time_t sttime;
                time(&sttime);

                // convert to gm time
                // struct tm*  tim; // RRR
                //      struct tm * tim = gmtime(&sttime); -- RRR change to gmtime_s
                struct tm  tim; // RRR

#ifdef WINDOWS_XP
                gmtime_s(&tim,&sttime);
#endif

#ifdef UNIX
                gmtime_r(&sttime,&tim);
#endif

                // set data items
                int sec  = tim.tm_sec;           // second (0-61, allows for leap seconds)
                int min  = tim.tm_min;           // minute (0-59)
                int hour = tim.tm_hour;          // hour (0-23)

                int mon  = tim.tm_mon + 1;       // month (0-11)
                int mday = tim.tm_mday;          // day of the month (1-31)
                int year = tim.tm_year % 100;    // years since 1900

                char cur_time[12];
                char cur_date[12];

                char line_1[64];
                char line_2[128];

#ifdef WINDOWS_XP
                sprintf_s(cur_time,9,"%02d:%02d:%02d",hour,min,sec);
                sprintf_s(cur_date,9,"%02d/%02d/%02d",mon,mday,year);

                sprintf_s(line_1,63,"A yuChen Technology Development Work Log File");

                std::string logNameTrunc=logName.substr(0,29);
                sprintf_s(line_2,63,"DATE: %s - %s%30s",cur_date,cur_time,logNameTrunc.c_str());
#endif

#ifdef UNIX
                snprintf(cur_time,9,"%02d:%02d:%02d",hour,min,sec);
                snprintf(cur_date,9,"%02d/%02d/%02d",mon,mday,year);

                snprintf(line_1,61,"A yuChen Technology Development Work Log File");

                snprintf(line_2,61,"DATE: %s - %s%30s",cur_date,cur_time,logName.c_str());
#endif
                *this << line_1 << std::endl;
                *this << line_2 << std::endl << std::endl;

                if (theEnd) *this << getExecTime() << std::endl;
        }
        
} 

void SimpleSocketLog::getExecTime(int* min, int* sec)
{
        time_t endTime;
        time(&endTime);
        *min = (int)((endTime - startTime)/60);
        *sec = (int)((endTime - startTime)%60);
} 

char* SimpleSocketLog::getExecTime()
{
        int min = 0;
        int sec = 0;
        getExecTime(&min, &sec);
        static char execTime[128];
#ifdef WINDOWS_XP
        sprintf_s(execTime,128, "Execution time: %d minutes %d seconds", min, sec);
#endif

#ifdef UNIX
        snprintf(execTime,128, "Execution time: %d minutes %d seconds", min, sec);
#endif
        return execTime;
}
