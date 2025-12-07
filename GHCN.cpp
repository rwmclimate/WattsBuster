#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>

#include "GHCN.hpp"
#include "GnuPlotter.hpp"
#include "SimpleSocket.h"

#define cerr_dbg cerr


// by -9999
const float GHCN::GHCN_NOTEMP9999 = (-9999.0);
const float GHCN::GHCN_NOTEMP999 = (-999.0);


// BEST "bad/no sample" designator
const float GHCN::BEST_NOTEMP99=(-99.0);

const float GHCN::CRU4_NOTEMP99=(-99.0);


// When comparing floating pt. vals against GHCN_NOTEMP,GHCN_NOTEMP999,
// make sure that we don't get bitten by floating-pt precision limitations.
const float GHCN::ERR_EPS = (0.1);
  
const float GHCN::DEFAULT_GRID_LAT_DEG=20.;
const float GHCN::DEFAULT_GRID_LONG_DEG=20.;

const float GHCN::MIN_GRID_LAT_DEG=0.5;
const float GHCN::MIN_GRID_LONG_DEG=0.5;

const float GHCN::MAX_GRID_LAT_DEG=180.;
const float GHCN::MAX_GRID_LONG_DEG=360.;

// Globals, yuck.  
int nAnnualAvg_g=1; // 1 for annual avg output, 0 for monthly avg output, default=annual.

int avgNyear_g;
int minBaselineSampleCount_g=0;

char *metadataFile_g=NULL;
std::string strStationType_g="X";  // X means "all station types: rural/suburban/urban"
int fAirportDistKm_g=GHCN::IMPOSSIBLE_VALUE;
// For airport processing -- max distance to nearest airport.
// If > 0, then process stations within fAirportDistKm of the nearest airport.
// If < 0, then process stations farther away then fAirportDistKm
// of the nearest airport.

char *gridSizeStr_g=NULL;

float gridSizeLatDeg_g=GHCN::DEFAULT_GRID_LAT_DEG;
float gridSizeLongDeg_g=GHCN::DEFAULT_GRID_LONG_DEG;

// The ResultsToDisplay struct is for use specifically with the browser/javascript front-end
// in ../BROWSER-FRONTEND.d -- Highly customized -- will work only with the runit.sh script.
// Default is to display raw and adjusted data results.
UResultsToDisplay uResultsToDisplay_g;
  

STATION_SELECT_MODE eStationSelectMode_g;

// Set to true to enable "sparse station" processing
bool bSparseStationSelect_g=false;

// These specify the min record length
// or max record start and min record end
// times of stations to be processed
bool bSetStationRecordSpecs_g=false;

bool bMinStationRecordLength_g=false;
std::size_t  nMinStationRecordLength_g=0;

bool bMaxStationRecordStartYear_g=false;
std::size_t  nMaxStationRecordStartYear_g=0;

bool bMinStationRecordEndYear_g=false;
std::size_t  nMinStationRecordEndYear_g=0;

// Set to true to process JJA months.
bool bNHSummerMonthsOnly_g=false;

bool bPartialYear_g=false;
int nStartMonth_g=1;
int nEndMonth_g=12;

// For "sparse stations" processing. Choose the single longest-record
// station per grid cell.
std::size_t nLongestStations_g=1;

int nPlotStartYear_g=1880;

// For statistical bootstrapping runs.
int nSeedBootstrap_g=1234; 
int nStationsBootstrap_g=0;
int nEnsembleRunsBootstrap_g=0;

// Set to true to adjust for the small variations in grid-cell areas
bool bAreaWeighting_g=false;

// Default GHCN version = 4 (newest)
int nGhcnVersion_g=4;

// Default data version GHCN (vs. BEST or CRU)
int nDataVersion_g=DATA_GHCN;

// Default CRU version = 3 or 4(newest) -- default 3
int nCruVersion_g=3;

char *cGhcnDailyData_g=NULL;
int nGhcnDailyData_g=0; // Data mode: 0 for (TMIN+TMAX)/2, 1 for TMIN, 2 for TMAX, 3 for TAVG from hourly readings.
int nGhcnDailyMode_g=0; // Processing mode: 0 for Avg, 1 for Min, 2 for Max
int nGhcnDailyMinDaysPerMonth_g=15; // Min #days for a valid monthly result, 15 a reasonable default.

// USHCN "estimated data" flag.
// If enabled, process estimated monthly temp entries
// in the homogenized USHCN data files.
int nUshcnEstimatedFlag_g=1;

std::vector <std::string> ensembleElementHeadingStr_g;

char *csvFileName_g=NULL;
char *csvFileNameStationCount_g=NULL;

bool bGenKmlFile_g=false;
bool bGenJavaScriptFile_g=false;

GHCN* pGhcn_g=NULL;

int nServerPort_g=0;

// NOTE: This is a holdover from a quick workaround
//       to handle multi-byte string issues with QString.
//       No longer needed -- fixed the QString problem
//       by sending str(QString obj) instead of the QString obj
//       directly.
// Default to 1 
int nPythonByteSize_g=1;  

bool bComputeAvgStationLatAlt_g=false;


SimpleTcpSocket* pServerSocket_g=NULL;

// Callback fcn passed via fcn ptr -- cannot be non-static class member.
int ExternCru4FtwFcn(const char* fPath, const struct stat* fb, int typeFlag);
int ExternUshcnFtwFcn(const char* fPath, const struct stat* fb, int typeFlag);
int ExternItrdbFtwFcn(const char* fPath, const struct stat* fb, int typeFlag);

// Just extracting integers -- so wrap up in a simple fcn
// cTok mods must be preserved in the calling fcn after each call,
// so pass in a reference to the cTok ptr.
int ParseNextTok(char*& cTok, const char* delim=":")
{
    int intVal=GHCN::IMPOSSIBLE_VALUE;
    if(NULL==cTok)
        return GHCN::IMPOSSIBLE_VALUE;
  
    cTok=::strtok(NULL, delim);
    if(NULL!=cTok)
        intVal=::atoi(cTok);
  
    return intVal;
}

std::size_t ParseNextTokSizeT(char*& cTok, const char* delim=":")
{
    std::size_t size_t_Val=GHCN::IMPOSSIBLE_VALUE;
    if(NULL==cTok)
        return GHCN::IMPOSSIBLE_VALUE;
  
    cTok=::strtok(NULL, delim);
    char* pEnd;
    if(NULL!=cTok)
    {
        size_t_Val=::strtol(cTok,&pEnd,10);

        std::vector<std::string> vStrSizeT;
        boost::split(vStrSizeT,std::string(cTok),boost::is_any_of(":"));

        std::string strStdSizeT;
        for(auto & iv: vStrSizeT)
        {
            if(iv.size()>0)
                strStdSizeT=iv;
            break;
        }
      
        ::sscanf(strStdSizeT.c_str(),"%luz",&size_t_Val);
    }
  
    return size_t_Val;
}

std::size_t ParseWmoIdStdSizeT(const std::string & strMsg)
{
    std::size_t size_t_Val=GHCN::IMPOSSIBLE_VALUE;

    std::vector<std::string> vStrSizeT;
    boost::split(vStrSizeT,strMsg,boost::is_any_of(":"));

    std::string strStdSizeT;
    for(auto & iv: vStrSizeT)
    {
        if(iv.size()>1)
        {
            strStdSizeT=iv;
            break;
        }
    }

    if(strStdSizeT.size()>1)
        ::sscanf(strStdSizeT.c_str(),"%luz",&size_t_Val);
  
    return size_t_Val;
}



GHCN::GHCN(const char  *inFile, const int& avgNyear)
{
    int ichar=0;

    mInputFstream=NULL;
  
    mTotalGridCellArea=0;
  
    mCbuf = new char[BUFLEN];

    ichar=0;
    mCcountry=&mCbuf[ichar];
    ichar+=3;
    mCwmoId=&mCbuf[ichar];
    ichar+=5;
    mCmodifier=&mCbuf[ichar];
    ichar+=3;
    mCduplicate=&mCbuf[ichar];
    ichar+=1;
    mCyear = &mCbuf[ichar];
    ichar+=4;
    mCtemps=&mCbuf[ichar];
  
    mInputFileName=std::string(inFile);
  
    mIavgNyear=avgNyear;
  
    mMinBaselineSampleCount=minBaselineSampleCount_g;
  
    struct tm* tmNowp;
    time_t timeNow;
    timeNow=time(NULL);
    tmNowp=gmtime(&timeNow);
  
    m_nCurrentYear=tmNowp->tm_year;


    mGridCellAverageAnnualAnomalies.clear();
    mGridCellAverageMonthlyAnomalies.clear();
    mSmoothedGlobalAverageAnnualAnomaliesMap.clear();
    mGlobalAverageAnnualAnomaliesMap.clear();
  
    mGridCellAreasVec.clear();
    mGridCellYearsWithDataVec.clear();
  
    m_pGnuPopen=NULL; // Moved to class GnuPlotter

    // Reasonable default values
    m_nDisplayResults=1;
    m_nDisplayIndex=1;  // Default plot color index
  
    m_MannItrdbListFileName=std::string("");
  
    m_bComputeAvgStationLatAlt=false;
}

GHCN::GHCN(const char  *inFile, const char* strMannItrdbListFileName, 
           const int& avgNyear)
{
    int ichar=0;

    mInputFstream=NULL;
  
    mTotalGridCellArea=0;
  
    mCbuf = new char[BUFLEN];

    ichar=0;
    mCcountry=&mCbuf[ichar];
    ichar+=3;
    mCwmoId=&mCbuf[ichar];
    ichar+=5;
    mCmodifier=&mCbuf[ichar];
    ichar+=3;
    mCduplicate=&mCbuf[ichar];
    ichar+=1;
    mCyear = &mCbuf[ichar];
    ichar+=4;
    mCtemps=&mCbuf[ichar];
  
    mInputFileName=std::string(inFile);
  
    mIavgNyear=avgNyear;
  
    mMinBaselineSampleCount=minBaselineSampleCount_g;
  
    struct tm* tmNowp;
    time_t timeNow;
    timeNow=time(NULL);
    tmNowp=gmtime(&timeNow);
  
    m_nCurrentYear=tmNowp->tm_year;


    mGridCellAverageAnnualAnomalies.clear();
    mGridCellAverageMonthlyAnomalies.clear();
    mSmoothedGlobalAverageAnnualAnomaliesMap.clear();
    mGlobalAverageAnnualAnomaliesMap.clear();

    mGridCellAreasVec.clear();
    mGridCellYearsWithDataVec.clear();

    m_pGnuPopen=NULL; // Moved to class GnuPlotter

    // Reasonable default values
    m_nDisplayResults=1;
    m_nDisplayIndex=1;  // Default plot color index
  
    m_MannItrdbListFileName=strMannItrdbListFileName;
  
    m_bComputeAvgStationLatAlt=false;

    return;
}

GHCN::~GHCN()
{
    delete mInputFstream;
    delete[] mCbuf;
}

std::string GHCN::GetInputFileName()
{
    return mInputFileName;
}


void GHCN::OpenGhcnFile(void)
{
  
    try
    {
        mInputFstream = new std::fstream(mInputFileName.c_str(),std::fstream::in);
        mbFileIsOpen=true;
    }
    catch(...) // some sort of file open failure
    {
        std::cerr << std::endl << std::endl;
        std::cerr << "Failed to create a stream for " << mInputFileName << std::endl;
        std::cerr << "Exiting.... " << std::endl;
        std::cerr << std::endl << std::endl;
        mInputFstream->close(); // make sure it's closed
        delete mInputFstream;
        exit(1);
    }

    if(!mInputFstream->is_open())
    {
        std::cerr << std::endl << std::endl;
        std::cerr << "Failed to open " << mInputFileName << std::endl;
        std::cerr << "Exiting.... " << std::endl;
        std::cerr << std::endl << std::endl;
        exit(1);
    }

    mInputFstream->exceptions(std::fstream::badbit 
                              | std::fstream::failbit 
                              | std::fstream::eofbit);

    return;
  
}

void GHCN::CloseGhcnFile(void)
{
    if(mInputFstream!=NULL)
    {
        mInputFstream->close();
        delete mInputFstream;
        mInputFstream=NULL;
    }
    return;
}

void GHCN::OpenItrdbFile(void)
{
    OpenGhcnFile();
    return;
}

void GHCN::CloseItrdbFile(void)
{
    CloseGhcnFile();
    return;
}

void GHCN::OpenCruFile(void)
{
    OpenGhcnFile();
    return;
}


void GHCN::CloseCruFile(void)
{
    CloseGhcnFile();
    return;
}

void GHCN::OpenUshcnFile(void)
{
    OpenGhcnFile();
    return;
}

void GHCN::CloseUshcnFile(void)
{
    CloseGhcnFile();
    return;
}

bool GHCN::IsFileOpen()
{
    return mbFileIsOpen;
}





// This routine reads in temperature data and in the case of multiple
// temperature time-series sharing a single WMO id number, computes a single
// time-series of average temperature data. i.e, if
// 10 time-series share a single WMO id, the data from those 10 time-seriess are
// averaged to generate a single average temperature time-series.
// Invalid data (delineated by -9999) are excluded from the computations.

// StationSet is a list of complete 8-digit station id's
// (5-digit wmo id + 3 digit modifier)
void GHCN::ReadGhcnV2TempsAndMergeStations(const std::set<std::size_t>& stationSet)
{
    // std::cerr<<"##########################################################"<<std::endl;
    // std::cerr<<__FUNCTION__<<"(): is broken. Fix or use GHCNV4"<<std::endl;
    // std::cerr<<"##########################################################"<<std::endl;
    // exit(0);
    
    
    std::string year;
    std::string month;
    int cc; // country code (3 digits)
    int ww; // wmo id (5 digits)
    // not used any more... int mm; // modifier (3 digits)
  
    // int ssl; // complete stationid (wmoid+modifier+duplicate)

    std::hash<std::string> strHash;

    int tt[12];
    int yy;

    mTempsMap.clear();
    mTempsMapCount.clear();

    OpenGhcnFile();
  
    try
    {
        while(!mInputFstream->eof())
        {
        
            mInputFstream->getline(mCbuf,BUFLEN);
            std::string strWmoId=std::string(mCbuf).substr(0,11);
            size_t nWmoStationId=(std::size_t)strHash(strWmoId);

            if(mStationHashvsStationWmoId.find(nWmoStationId)==mStationHashvsStationWmoId.end())
            {
                mStationHashvsStationWmoId[nWmoStationId]=strWmoId;
            }

            sscanf(mCyear,"%4d",&yy);
            mIyear=(size_t)yy;

            if(stationSet.find(nWmoStationId) == stationSet.end())
            {
                // Station id is not on the list -- skip it
                continue;
            }

            // Initialize to "invalid" temperature values
            // so we can keep track of data-gaps.
            int ii;
            for(ii=0; ii<12; ii++)
            {
                tt[ii]=GHCN_NOTEMP9999;
            }

            if(mIyear >= MIN_GISS_YEAR)
            {
                sscanf(mCtemps,"%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d",
                       &tt[0],&tt[1],&tt[2],&tt[3],&tt[4],&tt[5],
                       &tt[6],&tt[7],&tt[8],&tt[9],&tt[10],&tt[11]);
        
                if(mTempsMap[nWmoStationId][mIyear].size()==0)
                {
                    mTempsMap[nWmoStationId][mIyear].resize(12);
                    mTempsMapCount[nWmoStationId][mIyear].resize(12);
                    for(ii=0; ii<12; ii++)
                    {
                        mTempsMap[nWmoStationId][mIyear][ii]=0;
                        mTempsMapCount[nWmoStationId][mIyear][ii]=0;
                    }
                }
        
                for(ii=0; ii<12; ii++)
                {
                    if(tt[ii]>GHCN_NOTEMP999+ERR_EPS)
                    {
                        // Got a valid value? Divide the GHCN temperature*10
                        // number down to get the proper temperature value
                        // and add it to the temperature sum for this WMO station-id,
                        // year and month.  This will be divided by the number of
                        // of stations sharing the WMO id for this year/month 
                        // to compute the average (merged) temperature below.
                        mTempsMap[nWmoStationId][mIyear][ii]+=tt[ii]/10.0;
                        mTempsMapCount[nWmoStationId][mIyear][ii]+=1;
                    }
                    else
                    {
                        // Invalid temperature sample -- don't include
                        // it in the station-id average.
                    }
                }
            }
        }
    }
    catch(std::fstream::failure& ff)
    {
        std::cerr << std::endl 
                  << "Finished reading temperature data "
                  << std::endl;
    }

    CloseGhcnFile();
  
    // Now iterate through each WMO id number key in mTempsMap and divide all temperature sums
    // for each year/month by the number of time-series sharing that WMO id.  
    // For year/months with no time-series data
    // for any particular WMO id, set the temperature value to GHCN_NOTEMP.
    // This will produce average temperatures for all time-series sharing a WMO 
    // id number.
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss;  // station id iterator
    std::map<std::size_t, std::vector<float> >::iterator isy;             // year iterator
    std::map<std::size_t, std::map<std::size_t, std::vector<int> > >::iterator icc;// count iterator (over station ids)
    std::map<std::size_t, std::vector<int> >::iterator icy;               // count iterator (over years)
    int imm;
    for(iss=mTempsMap.begin(),icc=mTempsMapCount.begin(); 
        iss!=mTempsMap.end(); ++iss,++icc)
    {
        for(isy=iss->second.begin(), icy=icc->second.begin(); 
            isy!=iss->second.end(); ++isy,++icy)
        {
            for(imm=0; imm<12; imm++)
            {
                if((isy->second[imm]>GHCN_NOTEMP999+ERR_EPS)&&(icy->second[imm]>0))
                {
                    // Got at least one station with this WMO reporting for this month and year
                    isy->second[imm]=isy->second[imm] /= (1.*icy->second[imm]);
                }
                else
                {
                    // No stations with this WMO reporting for this month/year.
                    isy->second[imm]=GHCN_NOTEMP9999;
                }
            }
        }
    }
  
}



void GHCN::ReadGhcnV3Temps(const std::set<std::size_t>& stationSet)
{
    std::string year;
    std::string month;

    int tt[12];
    char qc[12];
    char cDummy;
  
    mTempsMap.clear();
    mTempsMapCount.clear();

    OpenGhcnFile();
  
    // Cols             Data field
    //
    // 0-10          Complete station id
    // 0-2           Country Code
    // 3-7           WMO ID (WMO stations only)
    // 8-10          000 for WMO stations (XXX for non-WMO stations)
    // 11-14         Year
    // 15-18         Type (TAVG, TMAX...)
    // 19-23         Temp VAL1 (degree*10)
    // 24-24         DMFLAG1
    // 25-25         QCFLAG1
    // 26-26         DSFLAG1
    // ...
    // ...
    // 107-111       Temp VAL12      
    // 112-112       DMFLAG12
    // 113-113       QCFLAG12
    // 114-114       DSFLAG12

    // (107-19)=88
    // (26-19+1)=8

    // Do something like this to read in the GHCN V3 temp data.
    // sscanf(&ghcnBuf[0],"%11d%4d",&stationId,&year) // complete station ID and year
    // toffset=19
    // for(ii=0; ii<11; ii++)
    // {
    //    sscanf(&ghcnBuf[toffset],"%5d",&temp[ii]);
    //    toffset+=8;
    // }
  
    try
    {
        while(!mInputFstream->eof())
        {
        
            mInputFstream->getline(mCbuf,BUFLEN);

            // First 3 digits are country code -- skip them.
            // Including country code makes int val too big to store
            // in a 32-bit int. So skip 'em
            // Orig sscanf(&mCbuf[3],"%8ld%4ld",&mICompleteStationId,&mIyear); // complete station ID and year
            std::hash<std::string> str_hash;

            std::string strWmoId=std::string(mCbuf).substr(0,11);
            size_t nWmoStationId=(std::size_t)str_hash(strWmoId);

            if(mStationHashvsStationWmoId.find(nWmoStationId)==mStationHashvsStationWmoId.end())
            {
                mStationHashvsStationWmoId[nWmoStationId]=strWmoId;
            }

      
            if(stationSet.find(nWmoStationId) == stationSet.end())
            {
                // Station id is not on the list -- skip it
                continue;
            }

            // Initialize to "invalid" temperature values
            // so we can keep track of data-gaps.
            int ii;
            for(ii=0; ii<12; ii++)
            {
                tt[ii]=GHCN_NOTEMP9999;
            }

            if(mIyear >= MIN_GISS_YEAR)
            {
                // sscanf(mCtemps,"%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d",
                // &tt[0],&tt[1],&tt[2],&tt[3],&tt[4],&tt[5],
                // &tt[6],&tt[7],&tt[8],&tt[9],&tt[10],&tt[11]);

                int toffset=19;
                for(ii=0; ii<12; ii++)
                {
                    sscanf(&mCbuf[toffset],"%5d%c%c",&tt[ii],&cDummy,&qc[ii]);
                    toffset+=8;
                }
        
                if(mTempsMap[nWmoStationId][mIyear].size()==0)
                {
                    mTempsMap[nWmoStationId][mIyear].resize(12);
                    mTempsMapCount[nWmoStationId][mIyear].resize(12);
                    for(ii=0; ii<12; ii++)
                    {
                        mTempsMap[nWmoStationId][mIyear][ii]=0;
                        mTempsMapCount[nWmoStationId][mIyear][ii]=0;
                    }
                }
        
                for(ii=0; ii<12; ii++)
                {
                    if((tt[ii]>GHCN_NOTEMP999+ERR_EPS)&&(qc[ii]==' '))
                    {
                        // Got a valid value? Divide the GHCN temperature*10
                        // number down to get the proper temperature value.
                        // With V3, only 1 time-series per station ID, so no
                        // need to merge temp data 
                        // Note -- must divide by 100 to get degrees C (not 10
                        // as with V2).
                        mTempsMap[nWmoStationId][mIyear][ii]=tt[ii]/100.0;
                    }
                    else
                    {
                        mTempsMap[nWmoStationId][mIyear][ii]=GHCN_NOTEMP9999;
                    }
                }
            }
        }
    }
    catch(std::fstream::failure & ff)
    {
        std::cerr << std::endl 
                  << "Finished reading temperature data "
                  << std::endl;
    }

    CloseGhcnFile();
  
}


void GHCN::ReadGhcnV4Temps(const std::set<std::size_t>& stationSet)
{
    std::string year;
    std::string month;

    int tt[12];
    char qc[12];
    char cDummy;
  
    mTempsMap.clear();
    mTempsMapCount.clear();

    OpenGhcnFile();
  
    // Cols             Data field
    //
    // 0-10          Complete station id
    // 0-2           Country Code
    // 3-7           WMO ID (WMO stations only)
    // 8-10          000 for WMO stations (XXX for non-WMO stations)
    // 11-14         Year
    // 15-18         Type (TAVG, TMAX...)
    // 19-23         Temp VAL1 (degree*10)
    // 24-24         DMFLAG1
    // 25-25         QCFLAG1
    // 26-26         DSFLAG1
    // ...
    // ...
    // 107-111       Temp VAL12      
    // 112-112       DMFLAG12
    // 113-113       QCFLAG12
    // 114-114       DSFLAG12

    // (107-19)=88
    // (26-19+1)=8

    // Do something like this to read in the GHCN V3 temp data.
    // sscanf(&ghcnBuf[0],"%11d%4d",&stationId,&year) // complete station ID and year
    // toffset=19
    // for(ii=0; ii<11; ii++)
    // {
    //    sscanf(&ghcnBuf[toffset],"%5d",&temp[ii]);
    //    toffset+=8;
    // }

    std::hash<std::string> strHash;
  
    try
    {
        while(!mInputFstream->eof())
        {
        
            mInputFstream->getline(mCbuf,BUFLEN);

            // First 3 digits are country code -- skip them.
            // Including country code makes int val too big to store
            // in a 32-bit int. So skip 'em
      
            char cCompleteStationId[16];
            ::memset(cCompleteStationId,'\0',16);
            sscanf(&mCbuf[0],"%11s%4ld",&cCompleteStationId[0],&mIyear); // complete station ID and year

            mICompleteStationId=strHash(std::string(cCompleteStationId));

            if(mStationHashvsStationWmoId.find(mICompleteStationId)==mStationHashvsStationWmoId.end())
            {
                std::string strWmoId=std::string(cCompleteStationId);
                mStationHashvsStationWmoId[mICompleteStationId]=strWmoId;
            }

            if(stationSet.find(mICompleteStationId) == stationSet.end())
            {
                // Station id is not on the list -- skip it
                continue;
            }

            // Initialize to "invalid" temperature values
            // so we can keep track of data-gaps.
            int ii;
            for(ii=0; ii<12; ii++)
            {
                tt[ii]=GHCN_NOTEMP9999;
            }

            if((mIyear >= MIN_GISS_YEAR)&&(mIyear <= MAX_GISS_YEAR))
            {
                // sscanf(mCtemps,"%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d",
                // &tt[0],&tt[1],&tt[2],&tt[3],&tt[4],&tt[5],
                // &tt[6],&tt[7],&tt[8],&tt[9],&tt[10],&tt[11]);

                int toffset=19;
                for(ii=0; ii<12; ii++)
                {
                    sscanf(&mCbuf[toffset],"%5d%c%c",&tt[ii],&cDummy,&qc[ii]);
                    toffset+=8;
                }
        
                if(mTempsMap[mICompleteStationId][mIyear].size()==0)
                {
                    mTempsMap[mICompleteStationId][mIyear].resize(12);
                    mTempsMapCount[mICompleteStationId][mIyear].resize(12);
                    for(ii=0; ii<12; ii++)
                    {
                        mTempsMap[mICompleteStationId][mIyear][ii]=0;
                        mTempsMapCount[mICompleteStationId][mIyear][ii]=0;
                    }
                }

                for(ii=0; ii<12; ii++)
                {
                    if((tt[ii]>GHCN_NOTEMP999+ERR_EPS)&&(qc[ii]==' '))
                    {
                        // Got a valid value? Divide the GHCN temperature*10
                        // number down to get the proper temperature value.
                        // With V3+, only 1 time-series per station ID, so no
                        // need to merge temp data 
                        // Note -- must divide by 100 to get degrees C (not 10
                        // as with V2).

                        if(bNHSummerMonthsOnly_g)
                        {
                            if((ii>=5)&(ii<=7))
                            {    // 0==jan... 11==dec
                                // JJA months only
                                mTempsMap[mICompleteStationId][mIyear][ii]=tt[ii]/100.0;
                            }
                            else
                                mTempsMap[mICompleteStationId][mIyear][ii]=GHCN_NOTEMP9999;
                  
                        }
                        else
                            mTempsMap[mICompleteStationId][mIyear][ii]=tt[ii]/100.0;
                    }
                    else
                    {
                        mTempsMap[mICompleteStationId][mIyear][ii]=GHCN_NOTEMP9999;
                    }
                }
            }
        }
    }
    catch(std::fstream::failure & ff)
    {
        std::cerr << std::endl 
                  << "Finished reading temperature data "
                  << std::endl;
    }

    CloseGhcnFile();
  
}
// Compute results from tmin only, tmax only, and tavg=(tmin+tmax)/2.
// Works only with .dly files where stations have both TMAX and TMIN data.
bool GHCN::DailyToMonthlyAllTmaxTminStations(const std::string& strTmaxBuf,
                                             const std::string& strTminBuf,
                                             const int tMode, // 0 for (TMIN+TMAX)/2, 1 for TMIN, 2 for TMAX.
                                             const int calcMode, // 0 for avg, 1 for MIN, 2 for MAX
                                             const int nMinDays, // 1-28 for min days needed for a valid month result.
                                             float& fMonthlyTempOut)
{
    // Indexed re:0
    // Station identifier: 0-10 (11 chars)      (ASN00041359)
    // Year and Month YYYYMM:  11-16 (6 chars)  (197309)
    // Element (TMIN/TMAX): 17-20:  (4 chars)   (i.e TMIN)
    // Value 1: 21-25  (First daily temp)
    // Value 2: 29-33  (Second daily temp)
    // Delta between subsequent values: 8

    bool bStatus=true;
    
    float fMonthlyTemp10=GHCN_NOTEMP9999;
    
    std::string strTmaxStationId=strTmaxBuf.substr(0,11);  // Country code + WMO ID
    std::string strTminStationId=strTminBuf.substr(0,11);  // Country code + WMO ID
    
    std::string strTmaxDate=strTmaxBuf.substr(11,6); // YYYYMM
    std::string strTminDate=strTminBuf.substr(11,6); // YYYYMM
    
    std::string strTmax=strTmaxBuf.substr(17,4); // better be TMAX
    std::string strTmin=strTminBuf.substr(17,4); // better be TMIN
    

    // Don't have a valid TMIN/TMAX pair for this station ID and date (year/month).
    if((strTminStationId!=strTmaxStationId)||(strTminDate!=strTmaxDate)||("TMIN"!=strTmin)||("TMAX"!=strTmax))
    {
        bStatus=false;
        return bStatus;
    }

    // Initialize fMonthlyTemp according to whether it will be used
    // to calculate monthly average, minimum, or maximum temps.
    switch(calcMode)
    {
        default:
        case 0: // AvgCalcMode
            fMonthlyTemp10=0;
            break;

        case 1: // MinCalcMode
            fMonthlyTemp10=-1*GHCN_NOTEMP9999; // GHCN_NOTEMP9999 is negative, so this is ok.
            break;

        case 2: // MaxCalcMode
            fMonthlyTemp10=GHCN_NOTEMP9999; // GHCN_NOTEMP9999 is negative, so this is ok.
            break;
            
    }

    int nDays=0;
    int nExceptions=0;

    // Up to 31 days.
    for(int ii=0; ii<=30; ++ii)
    {

        std::string strQcFlagMin=strTminBuf.substr(27+8*ii,1);
        std::string strQcFlagMax=strTminBuf.substr(27+8*ii,1);
        
        // At least one of the tmin/tmax temps failed QC. Skip to the next day.
        if((" "!=strQcFlagMin)||(" "!=strQcFlagMax))
            continue;
        
        std::string strMinTemp=strTminBuf.substr(21+8*ii,5);
        boost::trim(strMinTemp);
        
        std::string strMaxTemp=strTmaxBuf.substr(21+8*ii,5);
        boost::trim(strMaxTemp);

        int nTminTemp10=GHCN_NOTEMP9999;
        try
        {
            nTminTemp10=boost::lexical_cast<int>(strMinTemp);
        }
        catch(...)
        {
            std::cerr<<"Exception thrown parsing Tmin for station "<< strTminStationId<<std::endl;
            nExceptions++;
            nTminTemp10=GHCN_NOTEMP9999;
        }
        // Must have both valid tmin and tmax (see below) values
        // for this day to process this day's data.
        if(nTminTemp10<=GHCN_NOTEMP9999+ERR_EPS)
            continue;
        
        int nTmaxTemp10=GHCN_NOTEMP9999;
        try
        {
            nTmaxTemp10=boost::lexical_cast<int>(strMaxTemp);
        }
        catch(...)
        {
            std::cerr<<"Exception thrown parsing Tmax for station "<< strTmaxStationId<<std::endl;
            nExceptions++;
            nTmaxTemp10=GHCN_NOTEMP9999;
        }

        // Must have both valid tmin (see above) and tmax values
        // for this day to process this day's data. GHCN_NOTEMP9999
        // is the standard GHCN placeholder for a missing/invalid daily tmin or tmax temp.
        if(nTmaxTemp10<=GHCN_NOTEMP9999+ERR_EPS)
            continue;


        switch(tMode)
        {
            case 0:  // process TAVG (TMIN+TMAX)/2 data
                switch(calcMode)
                {
                    default:
                    case 0:   // AvgMode: calculate the average TAVG value for the month
                        fMonthlyTemp10+=1.*(nTminTemp10+nTmaxTemp10)/2.;
                        break;
                    case 1:  // MinMode: find the minimum TAVG value for the month.
                        fMonthlyTemp10=std::min(fMonthlyTemp10,(float)(1.*(nTminTemp10+nTmaxTemp10)/2.));
                        break;
                    case 2:  // MaxMode: find the maximum TAVG value for the month
                        fMonthlyTemp10=std::max(fMonthlyTemp10,(float)(1.*(nTminTemp10+nTmaxTemp10)/2.));
                        break;
                }
                break;

            case 1: // process TMIN data
                switch(calcMode)
                {
                    default: // Calculate average TMIN value for the month.
                    case 0:
                        fMonthlyTemp10+=1.*(nTminTemp10);
                        break;
                        
                    case 1:  // Find the minimum TMIN value for the month.
                        fMonthlyTemp10=std::min(fMonthlyTemp10,(float)(1.*nTminTemp10));
                        break;
                        
                    case 2:  // Find the maximum TMIN value for the month.
                        fMonthlyTemp10=std::max(fMonthlyTemp10,(float)(1.*nTminTemp10));
                        break;
                }
                break;

            case 2: // process TMAX data
                switch(calcMode)
                {
                    default:
                    case 0: // AvgMode: calculate the average TMAX for the month.
                        fMonthlyTemp10+=1.*(nTmaxTemp10);
                        break;
                    case 1: // MinMode: find the minimum TMAX value for the month.
                        fMonthlyTemp10=std::min(fMonthlyTemp10,(float)1.*nTmaxTemp10);
                        break;
                    case 2: // MaxMode: find the maximum TMAX value for the momth.
                        fMonthlyTemp10=std::max(fMonthlyTemp10,(float)1.*nTmaxTemp10);
                        break;
                }
                break;

            default:
                std::cerr<<"\n";
                std::cerr<<"tMode must be 0(TAVG), 1(TMIN), or 2(TMAX) \n";
                std::cerr<<"\n";
                exit(1);
                break;
        }
        
        nDays++;
    }

    // At least nMinDays of data for a valid monthly average, min, or max.
    if(nDays>=nMinDays)
    {
        if(0==calcMode) // Average Mode
            fMonthlyTemp10/=(1.*nDays);    
        fMonthlyTempOut=fMonthlyTemp10/10.;
    }
    else
    {
        fMonthlyTempOut=GHCN_NOTEMP9999;
    }
    // If we don't have at least 15 days of temps,
    // set fMonthlyTemp to GHCN_NOTEMP9999;
    
    return bStatus;
}


// Compute Tmin&Tmax only from stations that also have Tavg data -- experimental,
// not fully plumbed in to the app.
bool GHCN::DailyToMonthlyTmaxTminFromStationsWithTavg(const std::string& strTmaxBuf,
                                                      const std::string& strTminBuf,
                                                      const std::string& strTavgBuf,
                                                      const int tMode, // 0 for (TMIN+TMAX)/2, 1 for TMIN, 2 for TMAX.
                                                      const int calcMode, // 0 for avg, 1 for MIN, 2 for MAX
                                                      const int nMinDays, // 1-28 for min days needed for a valid month result.
                                                      float& fMonthlyTempOut)
{
    // Indexed re:0
    // Station identifier: 0-10 (11 chars)      (ASN00041359)
    // Year and Month YYYYMM:  11-16 (6 chars)  (197309)
    // Element (TMIN/TMAX): 17-20:  (4 chars)   (i.e TMIN)
    // Value 1: 21-25  (First daily temp)
    // Value 2: 29-33  (Second daily temp)
    // Delta between subsequent values: 8

    bool bStatus=true;
    
    float fMonthlyTemp10=GHCN_NOTEMP9999;
    
    std::string strTmaxStationId=strTmaxBuf.substr(0,11);  // Country code + WMO ID
    std::string strTminStationId=strTminBuf.substr(0,11);  // Country code + WMO ID 
    std::string strTavgStationId=strTavgBuf.substr(0,11);  // Country code + WMO ID
   
    std::string strTmaxDate=strTmaxBuf.substr(11,6); // YYYYMM
    std::string strTminDate=strTminBuf.substr(11,6); // YYYYMM
    std::string strTavgDate=strTminBuf.substr(11,6); // YYYYMM
    
    std::string strTmax=strTmaxBuf.substr(17,4); // better be TMAX
    std::string strTmin=strTminBuf.substr(17,4); // better be TMIN
    std::string strTavg=strTavgBuf.substr(17,4); // better be TMIN

    if(("TMIN"!=strTmin)||("TMAX"!=strTmax)||("TAVG"!=strTavg))
    {
        bStatus=false;
        return bStatus;
    }

    // Don't have a valid TMIN/TMAX pair for this station ID and date (year/month).
    if((strTminStationId!=strTmaxStationId)||(strTminDate!=strTmaxDate) ||
       (strTminStationId!=strTavgStationId)||(strTminDate!=strTavgDate))
    {
        bStatus=false;
        return bStatus;
    }


    // Initialize fMonthlyTemp according to whether it will be used
    // to calculate monthly average, minimum, or maximum temps.
    fMonthlyTemp10=0;
    switch(calcMode)
    {
        default:
        case 0: // AvgCalcMode
            fMonthlyTemp10=0;
            break;

        case 1: // MinCalcMode
            fMonthlyTemp10=0;
            break;

    }

    int nDays=0;
    int nExceptions=0;
    for(int ii=0; ii<=30; ++ii)
    {

        std::string strQcFlagMin=strTminBuf.substr(27+8*ii,1);
        std::string strQcFlagMax=strTminBuf.substr(27+8*ii,1);
        
        // At least one of the tmin/tmax temps failed QC. Skip to the next day.
        if((" "!=strQcFlagMin)||(" "!=strQcFlagMax))
            continue;
        
        std::string strMinTemp=strTminBuf.substr(21+8*ii,5);
        boost::trim(strMinTemp);
        
        std::string strMaxTemp=strTmaxBuf.substr(21+8*ii,5);
        boost::trim(strMaxTemp);

        std::string strAvgTemp=strTavgBuf.substr(21+8*ii,5);
        boost::trim(strAvgTemp);

        int nTavgTemp10=GHCN_NOTEMP9999;
        int nTminTemp10=GHCN_NOTEMP9999;
        int nTmaxTemp10=GHCN_NOTEMP9999;
        try
        {
            nTavgTemp10=boost::lexical_cast<int>(strAvgTemp);
            nTminTemp10=boost::lexical_cast<int>(strMinTemp);
            nTmaxTemp10=boost::lexical_cast<int>(strMaxTemp);
        }
        catch(...)
        {
            std::cerr<<"Exception thrown parsing Tavg for station "<< strTavgStationId<<std::endl;
            nExceptions++;
            nTavgTemp10=GHCN_NOTEMP9999;
            nTminTemp10=GHCN_NOTEMP9999;
            nTmaxTemp10=GHCN_NOTEMP9999;
        }

        // Must have valid tmin, tmax and tavg (see below) values
        // for this day to process this day's data.
        if((nTavgTemp10<=GHCN_NOTEMP9999+ERR_EPS)||
           (nTminTemp10<=GHCN_NOTEMP9999+ERR_EPS)||
           (nTmaxTemp10<=GHCN_NOTEMP9999+ERR_EPS))
            continue;

        switch(tMode)
        {
            case 4:  // process TAVG (TMIN+TMAX)/2 data for stations that also have TAVG calculated from hourly.
                switch(calcMode)
                {
                    default:
                    case 0:
                        // AvgMode: calculate the average TAVG value for the month
                        fMonthlyTemp10+=1.*(nTminTemp10+nTmaxTemp10)/2.;
                        break;
                        
                    case 1:
                        fMonthlyTemp10+=nTavgTemp10;
                        break;
                }
                break;

            default:
                std::cerr<<"\n";
                std::cerr<<"Mode must be to Calculate Tavg from (Tmin+Tmax/2) or with TAVG from hourly) \n";
                std::cerr<<"This is intended to process data only from GHCN daily stations that have TMIN,TMAX,\n";
                std::cerr<<"and TAVG from hourly readings. \n";
                std::cerr<<"\n";
                exit(1);
                break;
        }
        
        nDays++;
    }

    // At least nMinDays of data for a valid monthly average, min, or max.
    if(nDays>=nMinDays)
    {
        if((0==calcMode)||(1==calcMode)) // Average Mode
            fMonthlyTemp10/=(1.*nDays);    
        fMonthlyTempOut=fMonthlyTemp10/10.;
    }
    else
    {
        fMonthlyTempOut=GHCN_NOTEMP9999;
    }
    // If we don't have at least 15 days of temps,
    // set fMonthlyTemp to GHCN_NOTEMP9999;
    
    return bStatus;
}

// For GHCN daily with hourly temperature measurements.
bool GHCN::DailyToMonthlyAllTavgStations(const std::string& strTavgBuf,
                                         const int calcMode, // 0 for avg, 1 for MIN, 2 for MAX
                                         const int nMinDays, // 1-28 for min days needed for a valid month result.
                                         float& fMonthlyTempOut)
{
    // Indexed re:0
    // Station identifier: 0-10 (11 chars)      (ASN00041359)
    // Year and Month YYYYMM:  11-16 (6 chars)  (197309)
    // Element (TMIN/TMAX): 17-20:  (4 chars)   (i.e TMIN)
    // Value 1: 21-25  (First daily temp)
    // Value 2: 29-33  (Second daily temp)
    // Delta between subsequent values: 8

    bool bStatus=true;
    
    float fMonthlyTemp10=GHCN_NOTEMP9999;
    
    std::string strTavgStationId=strTavgBuf.substr(0,11);  // Country code + WMO ID
    
    std::string strTavgDate=strTavgBuf.substr(11,6); // YYYYMM
    
    std::string strTavg=strTavgBuf.substr(17,4); // better be TAvg

    // Don't have a valid TMIN/TMAX pair for this station ID and date (year/month).
    if(("TAVG"!=strTavg))
    {
        bStatus=false;
        return bStatus;
    }

    // Initialize fMonthlyTemp according to whether it will be used
    // to calculate monthly average, minimum, or maximum temps.
    switch(calcMode)
    {
        default:
        case 0: // AvgCalcMode
            fMonthlyTemp10=0;
            break;

        case 1: // MinCalcMode
            fMonthlyTemp10=-1*GHCN_NOTEMP9999; // GHCN_NOTEMP9999 is negative, so this is ok.
            break;

        case 2: // MaxCalcMode
            fMonthlyTemp10=GHCN_NOTEMP9999; // GHCN_NOTEMP9999 is negative, so this is ok.
    }

    int nDays=0;
    int nExceptions=0;
    for(int ii=0; ii<=30; ++ii)
    {
        std::string strTavgTemp=strTavgBuf.substr(21+8*ii,5);
        boost::trim(strTavgTemp);
        
        int nTavgTemp10=GHCN_NOTEMP9999;
        try
        {
            nTavgTemp10=boost::lexical_cast<int>(strTavgTemp);
        }
        catch(...)
        {
            std::cerr<<"Exception thrown parsing Tavg for station "<< strTavgStationId<<std::endl;
            nExceptions++;
            nTavgTemp10=GHCN_NOTEMP9999;
        }
        // Must have both valid tmin and tmax (see below) values
        // for this day to process this day's data.
        if(nTavgTemp10<=GHCN_NOTEMP9999+ERR_EPS)
            continue;
        
        switch(calcMode)
        {
            default:
            case 0:   // AvgMode: calculate the average TAVG value for the month
                fMonthlyTemp10+=1.*nTavgTemp10;
                break;
            case 1:  // MinMode: find the minimum TAVG value for the month.
                fMonthlyTemp10=std::min(fMonthlyTemp10,(float)(1.*nTavgTemp10));
                break;
            case 2:  // MaxMode: find the maximum TAVG value for the month
                fMonthlyTemp10=std::max(fMonthlyTemp10,(float)(1.*nTavgTemp10));
                break;
        }
        nDays++;
    }

    // At least nMinDays of data for a valid monthly average, min, or max.
    if(nDays>=nMinDays)
    {
        if(0==calcMode) // Average Mode
            fMonthlyTemp10/=(1.*nDays);    
        fMonthlyTempOut=fMonthlyTemp10/10.;
    }
    else
    {
        fMonthlyTempOut=GHCN_NOTEMP9999;
    }
    // If we don't have at least 15 days of temps,
    // set fMonthlyTemp to GHCN_NOTEMP9999;
    
    return bStatus;
}


void GHCN::ReadGhcnDailyTemps(const std::set<std::size_t>& stationSet)
{
    std::string year;
    std::string month;

    int tt[12];
    char qc[12];
    char cDummy;
  
    mTempsMap.clear();
    mTempsMapCount.clear();

    OpenGhcnFile();

    // Indexed re:0
    // Station identifier: 0-10 (11 chars)      (ASN00041359)
    // Year and Month YYYYMM:  11-16 (6 chars)  (197309)
    // Element (TMIN/TMAX): 17-20:  (4 chars)   (i.e TMIN)
    // Value 1: 21-25  (First daily temp)
    // Value 2: 29-33  (Second daily temp)
    // Delta between subsequent values: 8
    char cBuf[BUFLEN+1]; // guaranteed to be long enough
    std::hash<std::string> strHash;

    // FOR Tmax/Tmin processing, Tmax/Tmin data lines for a station/year-month always start with Tmax
    // FOR Tmax/Tmin/Tavg processing, Tmax/Tmin/Tavg data lines for a station/year-month always start with Tmax
    
    try
    {
        std::string strPrevYear="";
        std::string strThisYear="";
        std::string strPrevMonth="";
        std::string strThisMonth="";

        int nLine=0;
        int nStationTot=0;
        int nStationInclude=0;
        int nStationExclude=0;

        while(!mInputFstream->eof())
        {
            std::string strTmaxBuf;
            std::string strTminBuf;
            std::string strTavgBuf;
            // Data type: 0 for Tavg from (Tmin+Tmax)/2
            //            1 for Tmin only
            //            2 for Tmax only
            //            >2 -- Tavg from ghcnd hourly extractions.
            if(nGhcnDailyData_g<=2)
            {
                mInputFstream->getline(cBuf,BUFLEN);
                if(mInputFstream->eof())
                    break;
                nLine++;
                
                std::string strBuf=std::string(cBuf);
                // Tmax/Tmin pairs always start with Tmax.
                // If the first line read on this iteration isnt a Tmax line,
                // skip it (don't want to get out of sync.
                if(strBuf.substr(17,4)!="TMAX")
                    continue;

                if(strBuf.substr(17,4)=="TMAX")
                    strTmaxBuf=strBuf;
                else if(strBuf.substr(17,4)=="TMIN")
                    strTminBuf=strBuf;
                
                mInputFstream->getline(cBuf,BUFLEN);
                if(mInputFstream->eof())
                    break;
                nLine++;

                strBuf=std::string(cBuf);
                if(strBuf.substr(17,4)=="TMAX")
                    strTmaxBuf=strBuf;
                else if(strBuf.substr(17,4)=="TMIN")
                    strTminBuf=strBuf;

                if((strTminBuf.size()==0)||(strTmaxBuf.size()==0))
                {
                    continue;
                }
            }
            else if(3==nGhcnDailyData_g) // Extract Tavg from ghcnd hourly
            {
                std::string strBuf;
                
                mInputFstream->getline(cBuf,BUFLEN);
                if(mInputFstream->eof())
                    break;
                nLine++;
                strBuf=std::string(cBuf);
                if(strBuf.substr(17,4)=="TAVG")
                    strTavgBuf=strBuf;
                else
                    continue;

                if(strTavgBuf.size()==0)
                    continue;
            }
            else if(4==nGhcnDailyData_g) // Process data only from stations with TMIN,TMAX,& TAVG.
            {
                std::string strBuf;

                mInputFstream->getline(cBuf,BUFLEN);
                if(mInputFstream->eof())
                    break;
                strBuf=std::string(cBuf);

                // Tmax/Tmin/Tavg triads always start with Tmax.
                // If the first line read in on this iteration isnt a Tmax line,
                // skip it (don't want to get out of sync).
                if(strBuf.substr(17,4)!="TMAX")
                    continue;

                if(strBuf.substr(17,4)=="TMAX")
                    strTmaxBuf=strBuf;
                else if(strBuf.substr(17,4)=="TMIN")
                    strTminBuf=strBuf;
                else if(strBuf.substr(17,4)=="TAVG")
                    strTavgBuf=strBuf;
                
                mInputFstream->getline(cBuf,BUFLEN);
                if(mInputFstream->eof())
                    break;
                nLine++;
                
                strBuf=std::string(cBuf);
                if(strBuf.substr(17,4)=="TMAX")
                    strTmaxBuf=strBuf;
                else if(strBuf.substr(17,4)=="TMIN")
                    strTminBuf=strBuf;
                else if(strBuf.substr(17,4)=="TAVG")
                    strTavgBuf=strBuf;

                mInputFstream->getline(cBuf,BUFLEN);
                if(mInputFstream->eof())
                    break;
                nLine++;
                
                strBuf=std::string(cBuf);
                if(strBuf.substr(17,4)=="TMAX")
                    strTmaxBuf=strBuf;
                else if(strBuf.substr(17,4)=="TMIN")
                    strTminBuf=strBuf;
                else if(strBuf.substr(17,4)=="TAVG")
                    strTavgBuf=strBuf;
                
                if((strTminBuf.size()==0)||(strTmaxBuf.size()==0)||(strTavgBuf.size()==0))
                {
                    continue;
                }
            }
            
            if(!mInputFstream->good())
            {
                std::cerr<<"All finished! "<<std::endl<<std::endl;
                break;
            }

            // strTminBuf,strTmaxBuf, and strTavgBuf (if present)
            // should have identical station ids and year/month date-time stamps for each iteration.
            std::string strTprocBuf; // Use this only for station id & date-time processing.
            if(nGhcnDailyData_g<=2)
                strTprocBuf=strTminBuf;
            else
                // for nGhcnDailyData_g==3 and nGhcnDailyData_g, use strAvgBuf to get station id & year/month data info.
                strTprocBuf=strTavgBuf;

            std::string strCompleteStationId=strTprocBuf.substr(0,11);
            strPrevYear=strThisYear;
            strThisYear=strTprocBuf.substr(11,4);
            strPrevMonth=strThisMonth;
            strThisMonth=strTprocBuf.substr(15,2);

            try
            {
                mIyear=boost::lexical_cast<int>(strThisYear);
            }
            catch(...)
            {
                std::cerr<<"lexical_cast exception thrown: nLine="<<nLine<<" (year="<<strThisYear<<")"<<std::endl;
                exit(1);
            }
            
            if((mIyear < MIN_GISS_YEAR)||(mIyear > MAX_GISS_YEAR))
            {
                // Out of bounds year -- skip it.
                continue;
            }

            int nMonth=0;
            try
            {
                nMonth=boost::lexical_cast<int>(strThisMonth);
            }
            catch(...)
            {
                std::cerr<<"lexical_cast exception thrown: nLine="<<nLine<<" (month="<<strThisMonth<<")"<<std::endl;
                exit(1);
            }

            if(strThisYear!=strPrevYear)
            {
                // Got a new year -- set up to add in all the months for this year
                mICompleteStationId=strHash(strCompleteStationId);
                if(mTempsMap[mICompleteStationId][mIyear].size()==0)
                {
                    mTempsMap[mICompleteStationId][mIyear].resize(12);
                    mTempsMapCount[mICompleteStationId][mIyear].resize(12);
                    for(int ii=0; ii<12; ii++)
                    {
                        mTempsMap[mICompleteStationId][mIyear][ii]=GHCN_NOTEMP9999;
                        mTempsMapCount[mICompleteStationId][mIyear][ii]=0;
                    }
                }
            }

            float fMonthlyTemp=GHCN_NOTEMP9999;

            bool bMonthlyAvgStatus=false;
            if((nGhcnDailyData_g>=0)&&(nGhcnDailyData_g<=2))
                bMonthlyAvgStatus=DailyToMonthlyAllTmaxTminStations(strTmaxBuf,strTminBuf,
                                                                    nGhcnDailyData_g,nGhcnDailyMode_g,nGhcnDailyMinDaysPerMonth_g,
                                                                    fMonthlyTemp);
            else if(3==nGhcnDailyData_g)
                   bMonthlyAvgStatus=DailyToMonthlyAllTavgStations(strTavgBuf,nGhcnDailyMode_g,nGhcnDailyMinDaysPerMonth_g,
                                                                   fMonthlyTemp);
            else if(4==nGhcnDailyData_g)
                bMonthlyAvgStatus=DailyToMonthlyTmaxTminFromStationsWithTavg(strTmaxBuf,strTminBuf,
                                                                             strTavgBuf,nGhcnDailyData_g,
                                                                             nGhcnDailyMode_g,nGhcnDailyMinDaysPerMonth_g,
                                                                             fMonthlyTemp);
            else
            {
                std::cerr<<std::endl;
                std::cerr<<"Must specify Daily DataType=0-4."<<std::endl;
                std::cerr<<"( i.e. -D DataType:DataMode:MinMonthDays,where DataType=0-4)"<<std::endl;
                std::cerr<<std::endl;
                exit(1);
            }
            
                                                        
            // Didn't get a valid TMIN/TMAX pair to process.
            if(false==bMonthlyAvgStatus)
                continue;
          
            if(mStationHashvsStationWmoId.find(mICompleteStationId)==mStationHashvsStationWmoId.end())
            {
                std::string strWmoId=strCompleteStationId;
                mStationHashvsStationWmoId[mICompleteStationId]=strWmoId;
                std::cerr<<__FUNCTION__<<"(): nLine="<<nLine
                         <<": Reading data for station "<<strWmoId<<"        \r";
            }

            if(stationSet.find(mICompleteStationId) == stationSet.end())
            {
                // Station id is not on the list -- skip it
                std::string strWmoId=strCompleteStationId;
                // std::cerr<<"Skipping station "<<strWmoId<<"              \r";
                continue;
            }

        
            if(fMonthlyTemp>GHCN_NOTEMP999+ERR_EPS)
            {
                // Got a valid value? Divide the GHCN temperature*10
                // number down to get the proper temperature value.
                // With V3, only 1 time-series per station ID, so no
                // need to merge temp data 
                // Note -- must divide by 100 to get degrees C (not 10
                // as with V2).

                if(bNHSummerMonthsOnly_g)
                {
                    if((nMonth>=5)&(nMonth<=7))
                    {    // 0==jan... 11==dec
                        // JJA months only
                        mTempsMap[mICompleteStationId][mIyear][nMonth-1]=fMonthlyTemp;
                    }
                    else
                        mTempsMap[mICompleteStationId][mIyear][nMonth-1]=GHCN_NOTEMP9999;
                  
                }
                else
                    mTempsMap[mICompleteStationId][mIyear][nMonth-1]=fMonthlyTemp;
            }
            else
            {
                mTempsMap[mICompleteStationId][mIyear][nMonth-1]=GHCN_NOTEMP9999;
            }
        }
    }
    catch(std::fstream::failure & ff)
    {
        std::cerr << std::endl 
                  << "Finished reading temperature data "
                  << std::endl;
    }


    // Extend finding the daily max for each month to allow us to
    // compute global results based on the maximum (not average) monthly temperature
    // for every station and year.
    if(2==nGhcnDailyMode_g)
    {
        std::map<size_t,std::map<size_t,std::vector<float> > >::iterator imStation;
        std::map<size_t,std::vector<float> >:: iterator imYear;
        std::vector<float>::iterator imMonth;
        std::map<size_t,std::map<size_t,std::vector<float> > >::iterator imStationBegin;

        // This is a kluge that will let us compute global temps based on
        // maximum (not average) station monthly temperature for each year.
        // For each station & year, determine the maximum monthly temperature. Then
        // replace every temperature for that station&year with the maximum monthly temperature.
        // Then computing annual averages downstream from here will give us the desired maximum
        // monthly temperature value.
        for(imStation=mTempsMap.begin(); imStation!=mTempsMap.end(); ++imStation)
        {
            for(imYear=imStation->second.begin(); imYear!=imStation->second.end(); ++imYear)
            {
                float fTmax=GHCN_NOTEMP9999;
                for(imMonth=imYear->second.begin(); imMonth!=imYear->second.end(); ++imMonth)
                {
                    if(*imMonth>(GHCN_NOTEMP9999+ERR_EPS))
                    {
                        fTmax=std::max(fTmax,*imMonth);
                    }
                }
                for(imMonth=imYear->second.begin(); imMonth!=imYear->second.end(); ++imMonth)
                {
                    if(*imMonth>(GHCN_NOTEMP9999+ERR_EPS))
                    {
                        *imMonth=fTmax;
                    }
                }
            }
        }
    }
    
#if(0)    
    if(mTempsMap[mICompleteStationId][mIyear].size()==0)
                {
                    mTempsMap[mICompleteStationId][mIyear].resize(12);
                    mTempsMapCount[mICompleteStationId][mIyear].resize(12);
                    for(int ii=0; ii<12; ii++)
                    {
                        mTempsMap[mICompleteStationId][mIyear][ii]=GHCN_NOTEMP9999;
                        mTempsMapCount[mICompleteStationId][mIyear][ii]=0;
                    }
                }
#endif
    
    CloseGhcnFile();
  
}

void GHCN::ReadCru3Temps(const std::set<std::size_t>& stationSet)
{
  
    char *cWmoId;
    cWmoId=&mCbuf[0];
  
    int year;
    std::size_t wmoId; // wmo id (up to6 digits)

    int tt[12];

    mTempsMap.clear();
    mTempsMapCount.clear();

    std::cerr << "Station list length = " << stationSet.size() << std::endl;
  

    OpenCruFile();
  
    int readNbytes;
  
    try
    {
        while(!mInputFstream->eof())
        {
        
            mInputFstream->getline(mCbuf,BUFLEN);
            readNbytes=strlen(mCbuf);
            if(readNbytes>67) // We have a guaranteed header line
            {
                wmoId=0; // impossible value
                sscanf(cWmoId, "%6lu", &wmoId); // 5-digit wmo id

                char cWmoId0[8];
                sscanf(cWmoId, "%6s", &cWmoId0[0]); // 5-digit wmo id

                std::hash<std::string> strHash;
                wmoId=strHash(std::string(cWmoId0));

                std::string strWmoId=std::string(cWmoId0);

                if(mStationHashvsStationWmoId.find(mICompleteStationId)==mStationHashvsStationWmoId.end())
                {
                    mStationHashvsStationWmoId[wmoId]=strWmoId;
                }

            }
            else // we have a temperature data line
            {
                if(stationSet.find(wmoId) == stationSet.end())
                {
                    // Station id is not on the list -- skip it
                    continue;
                }
                // Initialize to "invalid" temperature values
                // so we can keep track of data-gaps.
                int ii;
                for(ii=0; ii<12; ii++)
                {
                    tt[ii]=GHCN_NOTEMP999;
                }

                // A place to put a debug breakpoint
                // if(wmoId==944800)
                // {
                //   int aaa;
                //   aaa=ii+3;
                // }
        
                sscanf(mCbuf,"%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d%5d",
                       &year,
                       &tt[0],&tt[1],&tt[2],&tt[3],&tt[4],&tt[5],
                       &tt[6],&tt[7],&tt[8],&tt[9],&tt[10],&tt[11]);
                mIyear=(size_t)year;
        
                if((year >= MIN_GISS_YEAR) && (year <= MAX_GISS_YEAR))
                {
                    if(mTempsMap[wmoId][year].size()==0)
                    {
                        mTempsMap[wmoId][year].resize(12);
                        mTempsMapCount[wmoId][year].resize(12);
                        for(ii=0; ii<12; ii++)
                        {
                            mTempsMap[wmoId][year][ii]=0;
                            mTempsMapCount[wmoId][year][ii]=0;
                        }
                    }
        
                    for(ii=0; ii<12; ii++)
                    {
                        if(tt[ii]>GHCN_NOTEMP999+ERR_EPS)
                        {
                            // Got a valid value? Divide the GHCN temperature*10
                            // number down to get the proper temperature value
                            // and add it to the temperature sum for this WMO station-id,
                            // year and month.  This will be divided by the number of
                            // of stations sharing the WMO id for this year/month 
                            // to compute the average (merged) temperature below.
                            mTempsMap[wmoId][year][ii]=tt[ii]/10.0;
                            mTempsMapCount[wmoId][year][ii]=1;

                            if(mTempsMap[wmoId][year][ii]<-80)
                            {
                                std::cerr << "Invalid temperature in mTempsMap" << std::endl;
                            }
                        }
                        else
                        {
                            mTempsMap[wmoId][year][ii]=GHCN_NOTEMP9999;
                            // Invalid temperature sample -- don't include
                            // it in the station-id average.
                        }
                    }
                }
            }  // if header line / else temperature line
        } // while(!mInputFstream->eof()
    }   // try/catch
    catch(std::fstream::failure & ff)
    {
        std::cerr << std::endl 
                  << "fstream:: Failure error thrown. " << std::endl
                  << "(No problemo -- just reached end of file)." 
                  << std::endl;
    }

    CloseCruFile();
  
    std::map<std::size_t, std::map <std::size_t, std::vector<float> > >::iterator iwmo;
    std::map<std::size_t, std::vector<float> >::iterator iyy;
    float avgtmp;
    float maxtmp=-9999,mintmp=9999;
  
    int icountt=0;
  
    for(iwmo=mTempsMap.begin(); iwmo!=mTempsMap.end(); ++iwmo)
    {
        avgtmp=0;
        icountt=0;
    
        for(iyy=iwmo->second.begin(); iyy!=iwmo->second.end(); ++iyy)
        {
            int imm;
            for(imm=0; imm<12; imm++)
            {
                if(maxtmp<iyy->second[imm])
                    maxtmp=iyy->second[imm];
                if(mintmp>iyy->second[imm])
                    mintmp=iyy->second[imm];
        
                avgtmp += iyy->second[imm];
                icountt++;
        
            }
      
        }
    
    }


    return;
  
}

// Metadata embedded in CRU4 data files; will
// collect metadata as we read in temp data.
int GHCN::ReadCru4Temps()
{
    // No file open here -- need to walk a directory tree
    // man ftw for details

    // Want to parse, save metadata in
    // std::map<std::size_t, GHCNLatLong> m_CompleteWmoLatLongMap;
    // do not need std::set<std::size_t> stationIdSet


    int ftwStatus;
  
    mTempsMap.clear();
    mTempsMapCount.clear();

    ftwStatus=ftw(mInputFileName.c_str(), ExternCru4FtwFcn, 20);
  
    return ftwStatus;
  
}

int GHCN::ReadCru4TempFile(const char* cru4File)
{
    // Open cru4file
    // Parse out temps and metadata
    // Close cru4file
    const char* cCru4MetaDataDelim="=";
    const char* cCru4DataDelim=" ";
  
    mInputFileName=std::string(cru4File);
    
    OpenCruFile();
  
//  bool bReachedTempData=false;
    int parseStationStatus=0;
  
    m_iCru4Id=IMPOSSIBLE_VALUE;
    m_fCru4Lat=IMPOSSIBLE_VALUE;
    m_fCru4Long=IMPOSSIBLE_VALUE;
    m_iCru4StartYear=IMPOSSIBLE_VALUE;
    m_iCru4EndYear=IMPOSSIBLE_VALUE;
  
    char *strMetaDataPtr=NULL;
    char *strDataPtr=NULL;
    std::string strBuf;
  
    GHCNLatLong latLong;
 
    bool bSavedMetaData=false;
  
    try
    {
        while(!mInputFstream->eof())
        {
            mInputFstream->getline(mCbuf,BUFLEN);
  
            if(std::string(mCbuf).find("=")!=std::string::npos)
            {
                // Metadata defs portion of file
                strMetaDataPtr=strtok(mCbuf,cCru4MetaDataDelim);
                parseStationStatus+=ParseCru4StationInfo(strMetaDataPtr,
                                                         cCru4MetaDataDelim);
            }
            else
            {
                strDataPtr=strtok(mCbuf,cCru4DataDelim);
            }
      
            if(false==bSavedMetaData)
            {
                if((m_iCru4Id>IMPOSSIBLE_VALUE)&&
                   (m_fCru4Lat>IMPOSSIBLE_VALUE)&&
                   (m_fCru4Long>IMPOSSIBLE_VALUE)&&
                   (m_iCru4StartYear>IMPOSSIBLE_VALUE)&&
                   (m_iCru4EndYear>IMPOSSIBLE_VALUE))
                {
                    // Translate lats from -90->90 to 0->180,
                    //           longs from -180->180 to 0->360
                    latLong.strStationName=m_strCru4Name+std::string(", ")+m_strCru4Country;
                    latLong.lat_deg=m_fCru4Lat+90.;     
                    latLong.long_deg=-1.*m_fCru4Long+180.;  // #$@! Brits reversed the longitude signs! 
                    latLong.first_year=m_iCru4StartYear;
                    latLong.last_year=m_iCru4EndYear;
                    latLong.num_years=latLong.last_year-latLong.first_year+1;
                    m_CompleteWmoLatLongMap[m_iCru4Id]=latLong;
                    bSavedMetaData=true;
                }
            }
      
            if(strDataPtr!=NULL)
            {
                if(std::string(strDataPtr)==std::string("Obs:"))
                {
                    //  bReachedTempData=true;
                }
                else
                {
                    int iMonth;
                    float fTemp;
          
                    int iYear=-999; // impossible value
                    if(strDataPtr!=NULL)
                    {
                        iYear=static_cast<int>(strtol(strDataPtr,NULL,10));
                    }
                    if(iYear>=MIN_GISS_YEAR)
                    {
                        if(mTempsMap[m_iCru4Id][iYear].size()==0)
                        {
                            mTempsMap[m_iCru4Id][iYear].resize(12);
                            mTempsMapCount[m_iCru4Id][iYear].resize(12);
                            for(iMonth=0; iMonth<12; iMonth++)
                            {
                                mTempsMap[m_iCru4Id][iYear][iMonth]=0;
                                mTempsMapCount[m_iCru4Id][iYear][iMonth]=0;
                            }
                        }

                        iMonth=0;
                        while((strDataPtr=strtok(NULL,cCru4DataDelim))!=NULL)
                        {
                            fTemp=strtod(strDataPtr,NULL);
                            if(fTemp>CRU4_NOTEMP99+ERR_EPS)
                            {
                                mTempsMap[m_iCru4Id][iYear][iMonth]=fTemp;
                                mTempsMapCount[m_iCru4Id][iYear][iMonth]=1;
                            }
                            else
                            {
                                // Invalid temp sample -- don't include it
                                // Set it to GHCN_NOTEMP9999 to guarantee that it won't
                                // be included in the calculations.
                                mTempsMap[m_iCru4Id][iYear][iMonth]=GHCN_NOTEMP9999;
                            }
                            iMonth++;
                            if(iMonth>=12) // Paranoid sanity check
                                break;
                        }
                    }
                }
            }
      
        }


    }
    catch(...)
    {
        // End of file dumps us here
    
    
    }
  
    CloseCruFile();
  
    return 0;
}

int GHCN::ParseCru4StationInfo(char *strPtr, const char *strDelim)
{
//  char *endPtr;
  
    if(strPtr!=NULL)
    {
        if(std::string(strPtr)==std::string("Name"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                m_strCru4Name=std::string(strPtr);
            }
        }
        if(std::string(strPtr)==std::string("Country"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                m_strCru4Country=std::string(strPtr);
            }
        }
        else if(std::string(strPtr)==std::string("Number"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                // Extract station number
                m_iCru4Id=static_cast<int>(strtol(strPtr,NULL,10));
                return 1;
            }
        }
        else if(std::string(strPtr)==std::string("Lat"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                m_fCru4Lat=strtof(strPtr,NULL);
                return 2;
            }
        }
        else if(std::string(strPtr)==std::string("Long"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                m_fCru4Long=strtof(strPtr,NULL);
                return 3;
            }
        }
        else if(std::string(strPtr)==std::string("Start year"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                m_iCru4StartYear=static_cast<int>(strtol(strPtr,NULL,10));
                return 4;
            }
        }
        else if(std::string(strPtr)==std::string("End year"))
        {
            strPtr=strtok(NULL,strDelim);
            if(strPtr!=NULL)
            {
                m_iCru4EndYear=static_cast<int>(strtol(strPtr,NULL,10));
                return 5;
            }
        }
    
    }
  
    return 0;
}

int GHCN::ReadUshcnTemps()
{
    // No file open here -- need to walk a directory tree
    // man ftw for details

    // Want to parse, save metadata in
    // std::map<std::size_t, GHCNLatLong> m_CompleteWmoLatLongMap;
    // do not need std::set<std::size_t> stationIdSet


    int ftwStatus;
  
    mTempsMap.clear();
    mTempsMapCount.clear();

    ftwStatus=ftw(mInputFileName.c_str(), ExternUshcnFtwFcn, 20);
  
    return ftwStatus;
  
}


//USHCN data file line:
//USH00244558 1897 -9999     -267     -379a     827     1496     1513b    1588     1961     1248      643     -222b    -520b

//In the data file:
// mCbuf[] containes 1 line.
// Chars  0-10:      station ID 
// Chars 12-15:      year
// Chars 16-21:      Temp1 (Jan)
// Chars 22-22:      DMFlag1  a-i: #missing measurements (days in a month). E=estimated value
// Chars 23-23:      QCFlag1: D=duplicate,I=inconsistent,L=isolated,M=Manually flagged as erroneous
//                            O=5 stddev from mean, S=failed spatial consistency check
//                            W=duplicate from prev month,
//                            A=alternative adjustment(qca data only),M=qcu problem
// Chars 24-24:      DSFlag1: Data source (not important here)
//
// Chars 25-31       Temp2 (Feb)
// Chars 32-32       DMFlag2
// Chars 33-33       QCFlag2
// Chars 34-34       DSFlag2
//
// Chars 34-40       Temp3 (Mar)
//
// Chars 43-49       Temp4 (Apr)
//
// Chars 52-58       Temp5 (May)
//
// Chars 61-67       Temp6 (Jun)
//
// Chars 70-76       Temp7 (Jul)
//
// Chars 79-85       Temp8 (Aug)
//
// Chars 88-94      Temp9 (Sep)
//
// Chars 97-103     Temp10 (Oct)
//
// Chars 106-112    Temp11 (Nov)
//
// Chars 115-120    Temp12 (Dec)

int GHCN::ReadUshcnTempFile(const char* ushcnFile)
{
    // Open USHCNfile
    // Parse out temps
    // Close USHCN file

    std::hash<std::string> strHash;

    mInputFileName=std::string(ushcnFile);
    
    OpenUshcnFile();
  
    GHCNLatLong latLong;
 
    bool bSavedMetaData=false;
  
    int iLineCount=0;

    while(!mInputFstream->eof())
    {
        try
        {
            iLineCount+=1;
          
            mInputFstream->getline(mCbuf,BUFLEN);

            if(mInputFstream->eof())
            {
                std::cout<<"Reached end of "<<mInputFileName<<std::endl;
                break;
            }
          
            std::string strBuf=std::string(mCbuf);
          
            // USHCN -- use entire 11 char ID string, including country code and station id (including leading zeros)
            // per void ReadUshcnMetadataFile(const char* inFile,...), GHCNMAIN.cpp:2691
            std::string strStationId=strBuf.substr(0,11);
            std::size_t nStationId=strHash(strStationId);
          
            std::string strYear=strBuf.substr(12,4);
            int iYear=-9999;
            try
            {
                iYear=boost::lexical_cast<int>(strYear);
            }
            catch(...)
            {
                continue;
            }
          
            if((iYear>=MIN_GISS_YEAR)&&(iYear<=MAX_GISS_YEAR))
            {
                if(mTempsMap[nStationId][iYear].size()==0)
                {
                    mTempsMap[nStationId][iYear].resize(12);
                    mTempsMapCount[nStationId][iYear].resize(12);
                }
                for(int iMonth=0,iMonthOffset=16; iMonthOffset<=115; iMonthOffset+=9,++iMonth)
                {
                    if(iMonth>=12) // Paranoid sanity check
                        break;

                    float fTemp;
                  
                    std::string strTemp=strBuf.substr(iMonthOffset,6);
                    std::string strEstimatedFlag=strBuf.substr(iMonthOffset+6,1);

                    // If estimated data processing is disabled and we come across
                    // an estimated temp reading, set fTemp to USHCN_NOTEMP9999
                    if((0==nUshcnEstimatedFlag_g)&&("E"==strEstimatedFlag))
                    {
                        fTemp=USHCN_NOTEMP9999;
                    }
                    else
                    {
                        boost::trim(strTemp);
                        try
                        {
                            fTemp=boost::lexical_cast<float>(strTemp);
                            if(fTemp>USHCN_NOTEMP9999+ERR_EPS)
                                fTemp/=100.;
                        }
                        catch(...)
                        {
                            fTemp=USHCN_NOTEMP9999;
                        }
                    }
                  
                    //fTemp=strtod(strDataPtr,NULL);
                    if(fTemp>USHCN_NOTEMP9999+ERR_EPS)
                    {
                        mTempsMap[nStationId][iYear][iMonth]=fTemp;
                        mTempsMapCount[nStationId][iYear][iMonth]=1;
                    }
                    else
                    {
                        // Invalid temp sample -- don't include it
                        // Set it to GHCN_NOTEMP9999 to guarantee that it won't
                        // be included in the calculations.
                        mTempsMap[nStationId][iYear][iMonth]=USHCN_NOTEMP9999;
                    }
                } // iMonth
            } // if(iYear>=MIN_GISS_YEAR)
        } 
        catch(...) // try{}
        {
            std::cerr<<__FILE__<<":"<<__LINE__<<" Finished reading data from "<<mInputFileName
                     <<". Line count="<<iLineCount<<std::endl;
            // End of file dumps us here
        }
    } //while(!mInputFstream->eof())
  
    CloseUshcnFile();
  
    return 0;
}


void GHCN::ReadBestTempsAndMergeStations(void)
{
    std::string year;
    std::string month;

    OpenGhcnFile();
  
    mInputFstream->exceptions(std::fstream::badbit 
                              | std::fstream::failbit 
                              | std::fstream::eofbit);

    int icount=0;

    mTempsMap.clear();
//  mTempsMapWmoIdSelect.clear();
    mTempsMapCount.clear();
  
    std::size_t wmoBaseId;
    std::size_t wmoModId;
    float fDate;
    float fDummy;
    float fTemp;
  
    int nDays;
    int nYear;
    int nMonth;
  
    try
    {
        while(!mInputFstream->eof())
        {
        
            mInputFstream->getline(mCbuf,BUFLEN);

            if(mCbuf[0]=='%') // Comment line -- skip it
                continue;
      
            sscanf(mCbuf,"%lu %lu %f %f %f %d",
                   &wmoBaseId,&wmoModId,&fDate,&fTemp,&fDummy,&nDays);
  
            if(nDays<27)
                continue;

            FDateToYearAndMonth(fDate,nYear,nMonth);
      
            if(nYear >= MIN_GISS_YEAR)
            {
        
                if(mTempsMap[wmoBaseId][nYear].size()==0)
                {
                    mTempsMap[wmoBaseId][nYear].resize(12);
                    mTempsMapCount[wmoBaseId][nYear].resize(12);
                    int ii;
                    for(ii=0; ii<12; ii++)
                    {
                        mTempsMap[wmoBaseId][nYear][ii]=0;
                        mTempsMapCount[wmoBaseId][nYear][ii]=0;
                    }
                }
        
                if(fTemp>BEST_NOTEMP99+ERR_EPS)
                {
                    // Got a valid value? Divide the GHCN temperature*10
                    // number down to get the proper temperature value
                    // and add it to the temperature sum for this WMO station-id,
                    // year and month.  This will be divided by the number of
                    // of stations sharing the WMO id for this year/month 
                    // to compute the average (merged) temperature below.
                    mTempsMap[wmoBaseId][nYear][nMonth]+=fTemp;
                    mTempsMapCount[wmoBaseId][nYear][nMonth]+=1;
                }
                else
                {
                    // Invalid temperature sample -- don't include
                    // it in the station-id average.
                }
            }
            icount++;
      
        }
    }
    catch(std::fstream::failure & ff)
    {
        std::cerr << std::endl 
                  << "fstream:: Failure error thrown. " << std::endl
                  << "(No problemo -- just reached end of file)." 
                  << std::endl;
    }

    mInputFstream->close();
    delete mInputFstream;
  

    // Now iterate through each WMO id number mTempsMap and divide all temperature sums
    // for each year/month by the number of stations sharing that WMO id.  
    // For year/months with no reporting stations
    // for any particular WMO id, set the temperature value to GHCN_NOTEMP9999.
    // This will produce average temperatures for all stations sharing a WMO 
    // id number.
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss;  // station id iterator
    std::map<std::size_t, std::vector<float> >::iterator isy; // year iterator
    std::map<std::size_t, std::map<std::size_t, std::vector<int> > >::iterator icc; // count iterator (over station ids)
    std::map<std::size_t, std::vector<int> >::iterator icy; // count iterator (over years)
    int imm;
    for(iss=mTempsMap.begin(),icc=mTempsMapCount.begin(); 
        iss!=mTempsMap.end(); ++iss,++icc)
    {
        for(isy=iss->second.begin(), icy=icc->second.begin(); 
            isy!=iss->second.end(); ++isy,++icy)
        {
            for(imm=0; imm<12; imm++)
            {
                if((isy->second[imm]>BEST_NOTEMP99+ERR_EPS)&&(icy->second[imm]>0))
                {
                    // Got at least one station with this WMO reporting for this month and year
                    isy->second[imm]=isy->second[imm] /= (1.*icy->second[imm]);
                }
                else
                {
                    // No stations with this WMO reporting for this month/year.
                    isy->second[imm]=GHCN_NOTEMP9999;
                }
            }
        }
    }
  
}

// RWM 2012/09/02
// Add data-specific info to the station map elements
// (i.e. info not available in the metadata file, like
// temperature record start/end years)
void GHCN::AddDataInfoToWmoLatLongMap(void)
{
  
    std::map<std::size_t, std::vector<float> >::iterator mi;
    std::map<std::size_t, std::vector<float> >::reverse_iterator rmi;
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator si;
  
//  m_CompleteWmoLatLongMap.clear(); // No, don't want to do this!
  
    for(si=mTempsMap.begin(); si!= mTempsMap.end(); ++si)
    {
        mIWmoStationId=si->first;
        mi=si->second.begin();
        rmi=si->second.rbegin();

        // All elements in the map should already exist. Do not create new elements;
        // simply add the info below to existing elements
        if(m_CompleteWmoLatLongMap.find(mIWmoStationId) != m_CompleteWmoLatLongMap.end())
        {
            m_CompleteWmoLatLongMap[mIWmoStationId].first_year=mi->first;
            m_CompleteWmoLatLongMap[mIWmoStationId].last_year=rmi->first;
            m_CompleteWmoLatLongMap[mIWmoStationId].num_years=si->second.size();

            // Now iterate through years (for each individual station)
            // to identify the contiguous-time-segments of coverage.
            std::map<std::size_t, std::vector<float> >::iterator iyBegin;
            std::map<std::size_t, std::vector<float> >:: reverse_iterator iryBegin;
      
            std::map<std::size_t, std::vector<float> >::iterator iyEnd;

            std::pair<int,int>yearSeg;

            int deltaYear=1; // appropriate default value

            iyBegin=si->second.begin();
            iryBegin=si->second.rbegin();
      
            iyEnd=iyBegin;
            iyEnd++;
      
            yearSeg.first=iyBegin->first;
      
            while(iyEnd!=si->second.end())
            {
                deltaYear=iyEnd->first-iyBegin->first;
                if(deltaYear>1)
                {
                    // Break in contiguous time coverage
                    // -- record begin/end of the segment.

                    // Save off iyBegin->first as the last year of
                    // the previous segment and push the segment on to the list.
                    yearSeg.second=iyBegin->first;

                    m_CompleteWmoLatLongMap[mIWmoStationId].year_seg.push_back(yearSeg);
          
                    // iyEnd->first is the first year of the next segment
                    yearSeg.first=iyEnd->first;
                }

                iyBegin++;
                iyEnd++;

            }
            yearSeg.second=iryBegin->first;

            // Wrap up with the last segment (end of data)
            m_CompleteWmoLatLongMap[mIWmoStationId].year_seg.push_back(yearSeg);
      
#if(0)
            // If the year_seg list has zero length at this point, that means that we
            // didn't find any gaps.  So put the whole data duration in as one segment.
            if(m_CompleteWmoLatLongMap[mIWmoStationId].year_seg.size()<1)
            {
                yearSeg.first=mi->first;
                yearSeg.second=rmi->first;
                m_CompleteWmoLatLongMap[mIWmoStationId].year_seg.push_back(yearSeg);
            }
#endif
        }
    }
  
    return;
}


void GHCN::FDateToYearAndMonth(const float& fDate,
                               int& nYearOut,
                               int& nMonthOut)
{
  
    float fMonthOfYear;
  
//  bool bLeap;
  
    nYearOut=(int)fDate;
  
//  bLeap=IsLeapYear(nYearOut);
  
    fMonthOfYear=fDate-nYearOut;
  
    nMonthOut=(int)(12.0*fMonthOfYear);
  
    if(nMonthOut>11)
        nMonthOut=11;
  
    return;
  
}

bool GHCN::IsLeapYear(const int& nYear)
{
    if(((0==nYear%4)&&(0!=nYear%100))||(0==nYear%400))
    {
        return true;
    }
    else
    {
        return false;
    }
  
}



void GHCN::CountWmoIds(void)
{
    std::map <std::size_t,std::map<std::size_t, std::vector < float > > >::iterator iss;
  
    int icount=0;
  
    for(iss=mTempsMap.begin(); iss!= mTempsMap.end(); ++iss)
    {
        icount++;
    }
  
    std::cerr << std::endl;
    std::cerr << "Just read in " << icount << " Station ids" << std::endl;
    std::cerr << std::endl;

    return; 
}

void GHCN::ComputeBaselines(const int nMinBaselineSampleCount)
{

    int yykey;
  
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss;
    int imm;

    int nMinBaselineYear;
    int nMaxBaselineYear;
    
    if(nMinBaselineSampleCount>0)
    {
        nMinBaselineYear=FIRST_BASELINE_YEAR;
        nMaxBaselineYear=LAST_BASELINE_YEAR;
    }
    else
    {
        nMinBaselineYear=MIN_GISS_YEAR;
        nMaxBaselineYear=MAX_GISS_YEAR;
    }
    
  
    mBaselineSampleCountMap.clear();
    mBaselineTemperatureMap.clear();
  
    // Iterate through all the stations in the temperature map.
    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        // Loop through the years in the temperature baseline period.
        for(yykey=nMinBaselineYear; yykey<=nMaxBaselineYear; ++yykey)
        {
            // Do we have an entry for this particular year?
            if(iss->second.find(yykey)!=iss->second.end())
            {
                for(imm=0; imm<12; imm++)
                {
                    // Check for sample validity.  Invalid/missing samples
                    // have been set equal to GHCN_NOTEMP9999 (-9999). Skip over those
                    // missing temperature values.
                    if(iss->second[yykey][imm] > GHCN_NOTEMP999+ERR_EPS)
                    {
                        bool first_count=false;

                        // Probably overkill here -- but I don't want to assume that
                        // all new map entries are initialized to 0.
                        // Check to see if there's a station entery in the 
                        // baseline sample count map.
                        if(mBaselineSampleCountMap.find(iss->first) 
                           == mBaselineSampleCountMap.end())
                        {
                            first_count=true;
                        }
                        // If there is a station entry, check to see if there's a month
                        // entry in this map.
                        else if(mBaselineSampleCountMap[iss->first].find(imm) 
                                == mBaselineSampleCountMap[iss->first].end())
                        {
                            first_count=true;
                        }

                        // No station/month entry in this map yet -- so create one
                        // and make sure that the baseline sample-count value is initialized to 0.
                        if(first_count==true)
                        {
                            mBaselineSampleCountMap[iss->first][imm]=0;
                        }

                        // First valid temperature for this station and month?
                        // Then initialize the baseline temperature map to the
                        // temperature value.
                        if(mBaselineSampleCountMap[iss->first][imm]<1)
                        {
                            mBaselineTemperatureMap[iss->first][imm]=iss->second[yykey][imm];
                        }
                        else
                        {
                            // Already have a temperature sum going -- update it.
                            mBaselineTemperatureMap[iss->first][imm]+=iss->second[yykey][imm];
                        }
                        // Increment the baseline sample count for this station and month.
                        mBaselineSampleCountMap[iss->first][imm]+=1;
                    }
                }
            }
        }

    }

    // Divide each baseline temperature sum by the number of valid samples 
    // found in the baseline time-period for each station and month to
    // get the baseline average temperature for the corresponding station and month.
    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        for(imm=0; imm<12; imm++)
        {
            if(mBaselineSampleCountMap[iss->first][imm]>1)
            {
                mBaselineTemperatureMap[iss->first][imm] /= mBaselineSampleCountMap[iss->first][imm];
            }
        }
    }

    std::map<std::size_t, std::map<std::size_t,int> >::iterator imb;
    std::map<std::size_t,int>::iterator imc;
    int ibCount=0;
    for(imb=mBaselineSampleCountMap.begin(); imb!=mBaselineSampleCountMap.end(); ++imb)
    {
        for(imc=imb->second.begin(); imc!=imb->second.end(); ++imc)
        {
            if(imc->second>=nMinBaselineSampleCount)
            {
                ibCount++;
                break;
            }
        }
    }

    std::cerr<<std::endl;
    std::cerr<<"Total #Stations in this temperature database:   " << mTempsMap.size()<<std::endl;
    std::cerr<<"Total #Stations with valid baseline data:  " << ibCount<<std::endl;
    std::cerr<<std::endl;
  
    return;
  
}


std::set<std::size_t> GHCN::ComputeGridElementStationSet(const std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap, 
                                                         const LatLongGridElement& latLongEl)
{
    std::map<std::size_t,GHCNLatLong>::const_iterator iss;
    std::set<std::size_t> ghcnLatLongSet;
    ghcnLatLongSet.clear();

    for(iss=ghcnLatLongMap.begin(); iss!=ghcnLatLongMap.end(); ++iss)
    {
        if(InLatLongGridElement(iss->second,latLongEl)==true)
        {
            ghcnLatLongSet.insert(iss->first);
        }
    }
    return ghcnLatLongSet;
}


bool GHCN::InLatLongGridElement(const GHCNLatLong& ghcnLatLong, 
                                const LatLongGridElement& latLongEl)
{
    // Pair <= ops with > ops so as to avoid counting
    // stations on grid element boundaries twice (rare, but
    // theoretically possible condition)
    if(((ghcnLatLong.lat_deg<=latLongEl.maxLatDeg)
        &&(ghcnLatLong.lat_deg>latLongEl.minLatDeg))
       &&
       ((ghcnLatLong.long_deg<=latLongEl.maxLongDeg)
        &&(ghcnLatLong.long_deg>latLongEl.minLongDeg)))
    {
        return true;
    }
    else
    {
        return false;
    }
  
}

void GHCN::SelectStationsGlobal(const std::map<std::size_t,GHCNLatLong>& latLongMapIn,
                                std::map<std::size_t,GHCNLatLong>& latLongMapOut)
{
    std::set<std::size_t> completeStationSet;
    std::map<std::size_t,GHCNLatLong>::const_iterator ism;
  
    for(ism=latLongMapIn.begin(); ism!=latLongMapIn.end(); ++ism)
    {
        completeStationSet.insert(ism->first);
    }
  

    std::set<std::size_t> sSelectedStationSet;
  
    switch(eStationSelectMode_g)
    {
        case SELECT_MAX_START_DATE:
        case SELECT_MIN_END_DATE:
        case SELECT_MAX_START_MIN_END_DATE:
        case SELECT_MAX_START_MIN_END_DATE_MIN_DURATION:      // Select all stations with data on or after a maximum year
            // Select all stations with data on or before a minimum year
            // Select all stations with a combo of the above
            std::cerr<<"Selecting stations based on record start and/or end years.."<<std::endl;
            sSelectedStationSet=PickStationsByRecordSpecs(&completeStationSet);
            break;
      
        case SELECT_MIN_DURATION:
            // Select all stations with a record length exceeding a
            // minimum duration
            std::cerr<<"Selecting stations based on record length....."<<std::endl;
            sSelectedStationSet=MinStationRecordLength(&completeStationSet);
            break;
      
        case SELECT_GREATEST_DURATION:
            // Search the list for the N stations with the longest temperature records.
            std::cerr<<"Selecting N stations ranked by record length....."<<std::endl;
            sSelectedStationSet=PickLongestNStations(&completeStationSet,nLongestStations_g);
            break;
      
        default:
            // Safe (but inefficient) fallback -- just use the complete set of stations
            std::cerr<<"Default/safe fallback -- just use all stations..."<<std::endl;
            sSelectedStationSet=completeStationSet;
            // singleWmoId = PickLongestStation(&completeStationSet);
            // pSelectedStationSet->insert(singleWmoId);
            break;
      
    }

    std::set<std::size_t>::const_iterator iss;
    std::size_t nWmoId;
    for(iss=sSelectedStationSet.begin(); iss!=sSelectedStationSet.end(); ++iss)
    {
        GHCNLatLong latLong;
        nWmoId=*iss;
        std::map<std::size_t,GHCNLatLong>::const_iterator iss;
        if(latLongMapIn.find(nWmoId)!=latLongMapIn.end())
        {
            iss=latLongMapIn.find(nWmoId);
          
            latLongMapOut[nWmoId]=iss->second;
        }
    
    }
  
    return;
}


void GHCN::SelectStationsPerGridCell(const float initDelLatDeg,
                                     const float initDelLongDeg, 
                                     const std::map<std::size_t,GHCNLatLong>& latLongMapIn,
                                     std::map<std::size_t,GHCNLatLong>& latLongMapOut)
{
  
    std::set<std::size_t> gridElementStationSet;
  
    LatLongGridElement gridEl;
  
    float delLongDeg, delLatDeg;
    
    int nLongDeg, nLatDeg;
  
    float minLatDeg, maxLatDeg;

    float minLongDeg, maxLongDeg;

    float baseArea;

    latLongMapOut.clear();
  
    nLatDeg=std::round(180./initDelLatDeg);
    nLatDeg=MAX(1,nLatDeg);
    delLatDeg=180./nLatDeg;

    delLongDeg=initDelLongDeg;

    if(nLatDeg%2==0)
    {
        baseArea = ABS(sin(PI/180.*0)-sin(PI/180.*delLatDeg))
            * (ABS(delLongDeg));
    }
    else
    {
        // Odd# latitude elements means the center one straddles the equator
        baseArea = ABS(sin(1.*PI/180.*delLatDeg/2)-sin(-1.*PI/180.*delLatDeg/2.))
            * (ABS(delLongDeg));
    }
  
    if(baseArea<=0)
    {
        std::cerr << "baseArea="<<baseArea<<std::endl;
        std::cerr << "        SHowstopper problem here -- exiting... " << std::endl;
        std::cerr << std::endl;
        exit(1);
    }  

    // Remember: 180 added to all longtidues, and 90 added to all latitudes
    //           to give all positive values for lats, longs (easier bookkeeping)
    // Start off at latitude -90 (translated to 0) and go north
    minLatDeg=0;
    maxLatDeg=delLatDeg;

    mTotalGridCellArea=0;

    int iLatDeg=0;
    while(iLatDeg<nLatDeg)
    {

        delLongDeg=baseArea/(ABS(sin(PI/180.*(minLatDeg-90))
                                 -sin(PI/180.*(maxLatDeg-90))));
    
        nLongDeg=std::round(360./delLongDeg);

        if(nLongDeg<1)
            nLongDeg=1;

        delLongDeg=360./nLongDeg; // Want integer number of delta-longitudes 
        //                           per 360 degrees...

        minLongDeg=0;
        maxLongDeg=delLongDeg;

        int iLongDeg=0;
        while(iLongDeg<nLongDeg)
        {
            gridEl.minLatDeg=minLatDeg;
            gridEl.maxLatDeg=maxLatDeg;
            gridEl.minLongDeg=minLongDeg;
            gridEl.maxLongDeg=maxLongDeg;

            // For current minLongDeg,maxLongDeg,
            //             minLatDeg,maxLatDeg, do the following:
            // Find all the WMO-ids in this lat/long "rectangle"

            gridElementStationSet.clear();

            std::size_t singleWmoId;
      
            gridElementStationSet=ComputeGridElementStationSet(latLongMapIn,gridEl);

            std::set<std::size_t> sGridElementSelectedStationSet;
      
            switch(eStationSelectMode_g)
            {
                case SELECT_MAX_START_DATE:
                case SELECT_MIN_END_DATE:
                case SELECT_MAX_START_MIN_END_DATE_MIN_DURATION:      // Select all stations with data on or after a maximum year
                    // Select all stations with data on or after a maximum year
                    // Select all stations with data on or before a minimum year
                    // Select all stations with a combo of the above
                    sGridElementSelectedStationSet=PickStationsByRecordSpecs(&gridElementStationSet);
                    break;
          
                case SELECT_MIN_DURATION:
                    // Select all stations with a record length exceeding a
                    // minimum duration
                    sGridElementSelectedStationSet=MinStationRecordLength(&gridElementStationSet);
                    break;

                case SELECT_GREATEST_DURATION:
                default:
                    // Select the single station with the greatest record length
                    singleWmoId = PickLongestStation(&gridElementStationSet);
                    if(singleWmoId>0)
                        sGridElementSelectedStationSet.insert(singleWmoId);
                    break;
          
            }

            std::set<std::size_t>::const_iterator iss;
            std::size_t nWmoId;
            for(iss=sGridElementSelectedStationSet.begin(); 
                iss!=sGridElementSelectedStationSet.end(); ++iss)
            {
                GHCNLatLong latLong;
                nWmoId=*iss;
                std::map<std::size_t,GHCNLatLong>::const_iterator iss2;
                if(latLongMapIn.find(nWmoId)!=latLongMapIn.end())
                {
                    iss2=latLongMapIn.find(nWmoId);
          
                    latLongMapOut[nWmoId]=iss2->second;
                }

            }

            minLongDeg+=delLongDeg;
            maxLongDeg+=delLongDeg;
            iLongDeg+=1;
        }

        maxLatDeg+=delLatDeg;
        minLatDeg+=delLatDeg;
        iLatDeg+=1;
    }


  
    return;
  
}

// RWM 2025/11/09 new -- need vector instead of set as input here
void GHCN::SelectStationsGlobalRandom(const std::vector<std::size_t>& vGhcnStationIdHashesIn,
                                      const int nSelectStationsIn,
                                      const std::map<std::size_t,GHCNLatLong>& ghcnLatLongMapIn,
                                      std::map<std::size_t,GHCNLatLong>& ghcnLatLongMapOut)
{
    unsigned int unStationCount=(int)(vGhcnStationIdHashesIn.size());
    std::set<std::size_t> setStationIndexHashes;

    int iss=0;
    int iCountSafety=0;

    // iCountSafety counter added to guarantee no infinite loopage.
    while((iss<nSelectStationsIn)&&(iCountSafety<50*nSelectStationsIn))
    {
        unsigned int unRandomStationIndexInt=::rand();
        // Want to round down to get an index int 0->unStationCount-1
        unRandomStationIndexInt=(unsigned int)(1.*unRandomStationIndexInt/RAND_MAX*unStationCount);

        // Paranoia check
        if(unRandomStationIndexInt>=unStationCount)
            unRandomStationIndexInt=unStationCount-1;

        if(setStationIndexHashes.find(unRandomStationIndexInt)==setStationIndexHashes.end())
        {
            // Select the station only if it hasn't already been selected.
            setStationIndexHashes.insert(unRandomStationIndexInt);
            std::map<std::size_t,GHCNLatLong>::const_iterator gll;
            if(ghcnLatLongMapIn.find(vGhcnStationIdHashesIn.at(unRandomStationIndexInt))!=ghcnLatLongMapIn.end())
            {
                gll=ghcnLatLongMapIn.find(vGhcnStationIdHashesIn.at(unRandomStationIndexInt));
                ghcnLatLongMapOut[vGhcnStationIdHashesIn.at(unRandomStationIndexInt)]
                    =gll->second;
                iss++;
            }
        }
        iCountSafety++;
    }

    std::cerr<<std::endl;
    std::cerr<<__FUNCTION__<<"(): "<<iss<<" stations selected after "<<iCountSafety<<" iterations."<<std::endl;
    std::cerr<<"ghcnLatLongMapOut.size() = "<<ghcnLatLongMapOut.size()<<std::endl;
    std::cerr<<std::endl;
    
    return;
}

// RRR 2015/04/30 new
void GHCN::ComputeGridCellAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                  const float& initDelLatDeg,
                                                  const float& initDelLongDeg,
                                                  const std::map<std::size_t, GHCNLatLong>& latLongMap)
{

/*
  area = 2*pi*ABS(sin(max_lat_rad-sin(min_lat_rad)*ABS(longitude_extent)

  longitude_extent = area/(2*pi*ABS(sin(max_lat_rad)-sin(min_lat_rad))

  nlongitude=360./longitude_extent;

  nlongitude=(int)nlongitude (round down)
  latitude_extent=360./nlongitude;

  area = 2*pi*ABS(sin(max_lat_rad)-sin(min_lat_rad))
  *ABS(max_long_rad-min_long_rad);



  A = 2*pi*R^2 |sin(lat1)-sin(lat2)| |lon1-lon2|/360
  = (pi/180)R^2 |sin(lat1)-sin(lat2)| |lon1-lon2|
*/ 

    std::set<std::size_t> gridElementStationSet;
    LatLongGridElement gridEl;
  
    float delLongDeg, delLatDeg;
    
    int nLongDeg, nLatDeg;
  
    float minLatDeg, maxLatDeg;
    float minLongDeg, maxLongDeg;

    float baseArea;

    // Will process from 90S to 90N (lat=5 corresponds to 85S (-85)
    // Areal approximations sort of break down at the poles...
    // Still, close enough for message-board science!!
  
  
    // Latitudes are mapped to 0=South Pole, 180=North Pole
    // Will process all latitudes.
  
    /* first, adjust initialDelLatDeg and initialDelLongDeg to give 
       an integer number of latitude segments 0-90 and longitude segments
       0-360 (equatorial).
    */

    mGridCellAverageMonthlyAnomalies.clear();
    mGlobalAverageMonthlyAnomaliesMap.clear(); // RWMM 2012/08/29 1230
    mAverageStationCountMap.clear(); // RWMM 2012/08/29 1230
    mGridCellAreasVec.clear();
    mTotalGridCellArea=0;
  
    nLatDeg=std::round(180./initDelLatDeg);
    nLatDeg=MAX(1,nLatDeg);
    delLatDeg=180./nLatDeg;
  
    delLongDeg=initDelLongDeg;

    if(nLatDeg%2==0)
    {
        baseArea = ABS(sin(PI/180.*0)-sin(PI/180.*delLatDeg))
            * (ABS(delLongDeg));
    }
    else
    {
        // Odd# latitude elements means the center one straddles the equator
        baseArea = ABS(sin(1.*PI/180.*delLatDeg/2)-sin(-1.*PI/180.*delLatDeg/2.))
            * (ABS(delLongDeg));
    }

    if(baseArea<=0)
    {
        std::cerr << "baseArea="<<baseArea<<std::endl;
        std::cerr << "        SHowstopper problem here -- exiting... " << std::endl;
        std::cerr << std::endl;
        exit(1);
    }  


    // Remember: 180 added to all longtidues, and 90 added to all latitudes
    //           to give all positive values for lats, longs (easier bookkeeping)
    // Start off at latitude -90 (translated to 0) and go north
    minLatDeg=0;
    maxLatDeg=delLatDeg;

    mTotalGridCellArea=0;

    std::string strBlanks="                                                              ";
    int iLatDeg=0;
    while(iLatDeg<nLatDeg) //maxLatDeg<=180.)
    {
        
        delLongDeg=baseArea/(ABS(sin(PI/180.*(minLatDeg-90))
                                 -sin(PI/180.*(maxLatDeg-90))));

        nLongDeg=std::round(360./delLongDeg);

        if(nLongDeg<1)
            nLongDeg=1;
        std::cerr<<strBlanks<< "\r";
        std::cerr<<__FUNCTION__<<std::fixed<<std::setprecision(3)<<std::setfill(' ')<<
            "(): Lat band "<<minLatDeg-90.<<"-->"<<maxLatDeg-90.
                 <<": Cell lon width "<<360./nLongDeg<<" \r";
        
        delLongDeg=360./nLongDeg; // Want integer number of delta-longitudes 
        //                        per 360 degrees...

        minLongDeg=0;
        maxLongDeg=delLongDeg;

        // std::cerr<<__FUNCTION__<<"(): Avg lat="<<(maxLatDeg+minLatDeg)/2.-90.
        //       <<"  delLongDeg="<<delLongDeg<<std::endl;

        int iLongDeg=0;
        while(iLongDeg<nLongDeg)
        {
            gridEl.minLatDeg=minLatDeg;
            gridEl.maxLatDeg=maxLatDeg;
            gridEl.minLongDeg=minLongDeg;
            gridEl.maxLongDeg=maxLongDeg;

            // 2024/03/10
            int nSubcellLat=1;
            int nSubcellLon=1;
      
            // For current minLongDeg,maxLongDeg,
            //             minLatDeg,maxLatDeg, do the following:
            // Find all the WMO-ids in this lat/long "rectangle"
            // Calculate the average anomalies for all WMO base ids
            // in this rectangle.

            // If at least one qualifying WMO base id is found
            // in this grid element, add it to the global-sum.
            // Increment the grid-element counter so as to
            // be able to convert the global-sum to a global-average.

            // ("borrowing" mGlobalAverageAnnualAnomaliesMap for the intermediate
            // grid-cell computations here).

            gridElementStationSet.clear();

            gridElementStationSet=ComputeGridElementStationSet(latLongMap,gridEl);
      
            if(gridElementStationSet.size()>0) 
                // At least one station in this lat/long grid cell
            {

                // Dump info about current lat/long grid-cell
                // Translate back to original lat/long coord vals
                std::set<std::size_t> yearsWithDataSet; // RWMM 2012/09/29
                yearsWithDataSet=ComputeAverageMonthlyAnomalies(minBaselineSampleCount,
                                                                gridEl,
                                                                nSubcellLat,nSubcellLon,
                                                                &gridElementStationSet);
                // RWMM 2012/08/29 -- have at least one year 
                // of data for this grid-cell
                if(yearsWithDataSet.size()>0)
                {
                    // Add the anomaly results for this particular grid-cell to the
                    // grid-cell list. -- this is for all years for which data is present
                    // in this grid-cell
                    mGridCellAverageMonthlyAnomalies.push_back(mGlobalAverageMonthlyAnomaliesMap);
                }
            }

            // RRR 2024/02/03 move this outside the if(gridElementStationSet.size()>0) block.
            if(true==bAreaWeighting_g)
            {
                double gridCellArea;
                gridCellArea=ComputeGridCellArea(minLatDeg-90.,maxLatDeg-90.,
                                                 minLongDeg-180.,maxLongDeg-180.);
                mGridCellAreasVec.push_back(gridCellArea);
                // Include elements with 0 years data (RRR 2024/02/03)
                std::set<std::size_t> setZero;
                mGridCellYearsWithDataVec.push_back(setZero);
            }

            minLongDeg+=delLongDeg;
            maxLongDeg+=delLongDeg;
            iLongDeg+=1;

        }

        maxLatDeg+=delLatDeg;
        minLatDeg+=delLatDeg;
        iLatDeg+=1;

    }

    return;
  
}



double GHCN::ComputeGridCellArea(const float minLatDeg, const float maxLatDeg,
                                 const float minLonDeg, const float maxLonDeg)
{
    // pi/180)R^2 |sin(lat1)-sin(lat2)| |lon1-lon2|
    double gridCellArea;

    gridCellArea = PI/180.*ABS(maxLonDeg-minLonDeg)
        * ABS(sin(PI/180.*maxLatDeg)-sin(PI/180.*minLatDeg));

    return gridCellArea;
  
}

void GHCN::Clear(void)
{

    mGridCellAverageAnnualAnomalies.clear();
    mGridCellAverageMonthlyAnomalies.clear();
    mGlobalAverageAnnualAnomaliesMap.clear();
    mGlobalAverageMonthlyAnomaliesMap.clear();
    mGlobalAverageAnnualAnomalyGridCountMap.clear();
    mAverageStationCountMap.clear();
    mAverageAnnualStationCountMap.clear();

    mGridCellAreasVec.clear();
    mGridCellYearsWithDataVec.clear();

    return;
}



SERVER_PLOT_OP GHCN::ParseWmoIdMessage(std::string& strMsg,
                                       std::set<std::size_t>& selectedWmoIdSet)
{
  
    std::size_t nWmoId=0;

    // Initialize to vals that allow us to 
    // detect errors.
    // WMO ID# delimited by '$' and '*'
    std::size_t iStart=0;
    std::size_t  iEnd=0; 

    char modeChar=' ';

    // Incoming msg will look something like one
    // of these: 
    //
    // "$=123456*"
    // "$+123456*"
    // "$-123456*"

    iStart=strMsg.find('$');
    if(iStart==std::string::npos)
    {
        // parse fail
        return SERVER_PLOT_NONE;
    }

    // skip past '$' and modeChar ('=','+', or '-')
    // to get to beginning of WMO ID# or command
    modeChar=strMsg[iStart+1];
    iStart+=2; 
  
    // Check for the minimum valid command length
    if(iStart>strMsg.size()-2)
    {
        // parse fail
        return SERVER_PLOT_NONE;
    }
  
    iEnd=strMsg.find('*');
    if(iEnd==std::string::npos)
    {
        // parse fail
        return SERVER_PLOT_NONE;
    }

    // Bump back to end of WMO ID# or command
    iEnd-=1;
  
    // Another check for the minimum valid command length
    if(iEnd-iStart<2)
    {
        // delims too close together
        // parse fail
        return SERVER_PLOT_NONE;
    }
  
    strMsg=strMsg.substr(iStart,(iEnd-iStart+1));
    std::cerr<<"### Just received command: "<<strMsg<<std::endl;
  
    // first letter of reset/clear/exit/quit command
    // may have been snipped -- so just search for
    // the remainder of the command.
    if((strcasestr(strMsg.c_str(),"eset")!=NULL) ||
       (strcasestr(strMsg.c_str(),"lear")!=NULL))
    {
        // Clear out the plot.
        selectedWmoIdSet.clear();
        return SERVER_PLOT_NEW;
    }
    else if((strcasestr(strMsg.c_str(),"xit")!=NULL) ||
            (strcasestr(strMsg.c_str(),"uit")!=NULL))
    {
        selectedWmoIdSet.clear();
        return SERVER_PLOT_QUIT; // Exit/shutdown code
    }
    else if(strcasestr(strMsg.c_str(), "efresh")!=NULL)
    {
        // The only thing that may change here is what results (raw vs adjusted)
        // get displayed.  The station list will remain unchanged.
        // Very simple command to parse -- no need for a new fcn.
        // Just need the desired plot start year.
        // Sample command to parse: "$+1:1:1840:Refresh*"
        std::vector<std::string> vStrToks;
        boost::split(vStrToks,strMsg,boost::is_any_of(":"));
        // tok 0: "$+1"  1/0 for plot/don't-plot raw
        // tok 1: "1"    1/0 for plot/don't-plot adj
        // tok 2: "1840" plot start year.
        nPlotStartYear_g=boost::lexical_cast<int>(vStrToks.at(2));
        return SERVER_PLOT_NEW;
    
    }
    else if(strcasestr(strMsg.c_str(), "atchPlot")!=NULL)
    {
        // A "simple" batch plot command was sent from the user front-end.
        // Plot "all rural", "all urban", "all suburban/mixed" or "all" stations.
        // One of the following commands is expected: "BatchPlotRural", "BatchPlotUrban"
        // "BatchPlotMixed" or "BatchPlotAll".

        // As of 2013/01/31, the following will be appended ":X1:X2".
        // if X1==1, then compute/plot raw data results to display with NASA/GISS results.
        // if X2==1, tnen compute/plot adjusted data results to display with NASA/GISS results.
        // If either value is 0, then don't plot the respected results.
        // ":0:0" means plot neither (just plot the NASA/GISS results).

        selectedWmoIdSet=SelectStationsByRuralUrbanStatus(strMsg);

        return SERVER_PLOT_NEW;
    }
    else if(strcasestr(strMsg.c_str(), "customPlot") != NULL)
    {
        // "Custom" batch plot command.
        // begin RRR 2012/12/03 -- plot data from javascript selected stations.
        // Stations selected by rural/urban/surban, start/stop/duration, etc. status
        // or any combination thereof (as opposed to the simplistic "all rural", "all urban"
        // etc. designations per the previous else if{ } block.
        selectedWmoIdSet.clear();

        ClientSelectFields csf;

        csf=ParseCustomBatchPlotCommand(strMsg);

        selectedWmoIdSet=SelectStationsByClientRequest(csf);
    
        return SERVER_PLOT_NEW;
        // end RRR 2012/12/03
    }
    else
    {
    
        // Got this far? Then the message is one
        // of the following:
        // "=A:B:XXXXXX"
        // "+A:B:XXXXXX"
        // "-A:B:XXXXXX"
        // A=1 for display raw rslts, 0 not to display raw rslts.
        // B=1 to display adj rslts, 0 not to display adj rslts.
        // XXXXXX is a WMO ID#
        // '=' means clear the WMO list and then
        //     add XXXXXX to the list.
        // '+' means add XXXXXX to the WMO id list
        //     (without clearing the list).
        // '-' means remove XXXXXX from the WMO id list.
        // ptrStart now points at the =/+/- char
  
        int nDisplayRaw;
        int nDisplayAdj;

        char *cTok=NULL;
        char *cTokBuf = new char[strMsg.length()+1];
        ::strncpy(cTokBuf,strMsg.c_str(), strMsg.length());

        // RRR 2013/01/31 -- add in raw/adj data display mode fields here.
        // The 3 stmts below will replace the csf.bnSelectAll=::atoi(cTok) stmt below.
        // selectDataFlag_g[0]=::atoi(cTok);
        // selectDataFlag_g[1]=::ParseNextTok(cTok);
        // csf.bnSelectAll=::ParseNextTok(cTok);
        cTok=strtok(cTokBuf,":");
        // No valid tokens?  bail out w/o plotting
        if(NULL==cTok)
        {
            delete [] cTokBuf;
            return SERVER_PLOT_NONE;
        }

//    int ParseNextTok(char*& cTok, char* delim=":");
    
        // New raw/adj display flags added to the beginning of every message
        // 2013/01/30
        nDisplayRaw=::atoi(cTok);
        nDisplayAdj=::ParseNextTok(cTok);
        // For GHCNv4 & USHCN, need room for std::size_t station hash ids.
        //nWmoId=::ParseNextTokSizeT(cTok);
        nWmoId=ParseWmoIdStdSizeT(strMsg);

        delete [] cTokBuf;
    
        switch(modeChar)
        {
            case '=':
//      nWmoId=atoi(strMsg.c_str());
                selectedWmoIdSet.clear();
                selectedWmoIdSet.insert(nWmoId);
                std::cerr<<"Set now contains only WMO ID "<<nWmoId<<std::endl;
                return SERVER_PLOT_NEW;
                break;
        
            case '+':
//      nWmoId=atoi(strMsg.c_str());
                selectedWmoIdSet.insert(nWmoId);
                mGlobalAverageAnnualAnomaliesMap.clear();
                std::cerr<<"Added WMO ID "<<nWmoId<<" to the list..."<<std::endl;
                return SERVER_PLOT_INCREMENT;
                break;
        
            case '-':
//      nWmoId=atoi(strMsg.c_str());
                selectedWmoIdSet.erase(nWmoId);
                mGlobalAverageAnnualAnomaliesMap.clear();
                std::cerr<<"Removed WMO ID "<<nWmoId<<" from the list..."<<std::endl;
                return SERVER_PLOT_INCREMENT;
                break;

                // Invalid/unkown plot mode -- do nothing
            default:
                mGlobalAverageAnnualAnomaliesMap.clear();
                break;
      
        }
  
    
    }
  
    return SERVER_PLOT_NONE; // Got this far? Nothing to plot
    
}

UResultsToDisplay GHCN::ParseResultsToDisplay(const std::string& strCommandIn)
{
    // Typical strCommandIn: "strCommandIn=$=1:1:4944954229482091473:1930*"
    // Cmd format: "$=X:Y:......:*"
    //     or      "$+X:Y:......:*"
    //     or      "$-X:Y:......:*"
    //    X is 1/0 raw data display flag
    //    Y is 1/0 adj data display flag
    UResultsToDisplay urtd;

    char *cTok=NULL;
    char *cTokBuf = new char[strCommandIn.length()+1];
    ::strncpy(cTokBuf,strCommandIn.c_str(), strCommandIn.length());

    // Safe defaults in case something goes wrong...
    urtd.resultsToDisplay.nPlotRawResults=1;
    urtd.resultsToDisplay.nPlotAdjResults=1;
  
    cTok=strtok(&cTokBuf[2],":");  // Skip past the "$=","$+",or "$-" preamble
  
    urtd.resultsToDisplay.nPlotRawResults=::atoi(cTok);
    urtd.resultsToDisplay.nPlotAdjResults=::ParseNextTok(cTok);

    // if(cTok!=NULL)
    {
        // If the plot command string does not contain "batchPlot", then
        // token 3 is the WMO id which we must skip over to get the nPlotStartYear token.
        if(strCommandIn.find("batchPlot")==std::string::npos)
            std::size_t nWmoId=::ParseNextTok(cTok); // Read and skip the WMO id.
        urtd.resultsToDisplay.nPlotStartYear=::ParseNextTok(cTok);
        nPlotStartYear_g=urtd.resultsToDisplay.nPlotStartYear;
    }

    std::cout<<"RWM: "<<__FILE__<<":"<<__LINE__<<" nPlotStartYear_g="<<nPlotStartYear_g<<std::endl;
    std::cout<<"RWM: "<<__FILE__<<":"<<__LINE__<<" strCommandIn="<<strCommandIn<<std::endl;
  
    delete [] cTokBuf;
  
    return urtd;
}



  
ClientSelectFields GHCN::ParseCustomBatchPlotCommand(const std::string& strBatchCommandIn)
{
  
    /* Batch plot command format
       <GHCN_COMMAND>$bTypeFlag:nType:bAirportFlag:nAirport:bAltitudeFlag:nAltitudeMode:nAltitude \
       bStartYearFlag:nStartYearMode:nStartYear:bEndYearFlat:nEndYearMode:nEndYear \
       bNumYearFlag:nNumYearMode:nNumYear:nYearOrAnd:nPlotStartYear*</GHCN_COMMAND>

    */
    int ParseNextTok(char*& cTok, char* delim=":");
    int ParseNextTokSizeT(char*& cTok, char* delim=":");
  
    std::string strCmd;
  
    strCmd=strBatchCommandIn;

    char *cTok=NULL;
    char *cTokBuf = new char[strBatchCommandIn.length()+1];
    ::strncpy(cTokBuf,strBatchCommandIn.c_str(), strBatchCommandIn.length());

    cTok=strtok(cTokBuf,":");
  
    ClientSelectFields csf;

  
    if(NULL!=cTok)
    {
    
        // RRR 2013/01/31 -- add in raw/adj data display mode fields here.
        // The 3 stmts below will replace the csf.bnSelectAll=::atoi(cTok) stmt below.
        // selectDataFlag_g[0]=::atoi(cTok);
        // selectDataFlag_g[1]=::ParseNextTok(cTok);
        // csf.bnSelectAll=::ParseNextTok(cTok);

        csf.bnDisplayRaw=::atoi(cTok);
        csf.bnDisplayAdj=::ParseNextTok(cTok);
        csf.bnSelectAll=::ParseNextTok(cTok);

        // csf.bnSelectAll == 1 means Plot all station data -- no need to bother with the rest of the fields
        if(0==csf.bnSelectAll)
        {
            // Out of bounds bnSelect vals mapped to 0 (false)
            csf.bnSelectType=::ParseNextTok(cTok); // station type: rural/urban/suburban
            csf.nType=::ParseNextTok(cTok);
      
            csf.bnSelectAirport=::ParseNextTok(cTok); // is it near an airport?
            csf.nAirport=::ParseNextTok(cTok);
      
            csf.bnSelectAltitude=::ParseNextTok(cTok); // screen by altitude 
            csf.nAltitudeMode=::ParseNextTok(cTok);    // =< or >= some altitude
            csf.nAltitude=::ParseNextTok(cTok);        // the altitude
      
            csf.bnSelectMinLatitude=::ParseNextTok(cTok);  // Min latitude select
            csf.nMinLatitude100=::ParseNextTok(cTok);
      
            csf.bnSelectMaxLatitude=::ParseNextTok(cTok);  // Max latitude select
            csf.nMaxLatitude100=::ParseNextTok(cTok);
      
            csf.bnSelectStartYear=::ParseNextTok(cTok);    // Start year select
            csf.nStartYearMode=::ParseNextTok(cTok);
            csf.nStartYear=::ParseNextTok(cTok);
      
            csf.bnSelectEndYear=::ParseNextTok(cTok);      // End year select
            csf.nEndYearMode=::ParseNextTok(cTok);
            csf.nEndYear=::ParseNextTok(cTok);
      
            csf.bnSelectNumYears=::ParseNextTok(cTok);     // #years select
            csf.nNumYearsMode=::ParseNextTok(cTok);
            csf.nNumYears=::ParseNextTok(cTok);
      
            csf.nYearOrAnd=::ParseNextTok(cTok);          // And/or year-select mode.

            std::cerr<<std::endl;
            std::cerr<<"csf.nType="<<csf.nType<<std::endl;
            std::cerr<<"csf.nAirport="<<csf.nAirport<<std::endl;
            std::cerr<<"csf.nAltitude="<<csf.nAltitude<<std::endl;
            std::cerr<<"csf.nStartYear="<<csf.nStartYear<<std::endl;
            std::cerr<<"csf.nEndYear="<<csf.nEndYear<<std::endl;
            std::cerr<<"csf.bnSelectNumYears="<<csf.bnSelectNumYears<<std::endl;
            std::cerr<<"csf.nNumYearsMode="<<csf.nNumYearsMode<<std::endl;
            std::cerr<<"csf.nNumYears="<<csf.nNumYears<<std::endl;
            std::cerr<<std::endl;
        }
        // if(cTok!=NULL)
        {
            csf.nPlotStartYear=::ParseNextTok(cTok);
            nPlotStartYear_g=csf.nPlotStartYear;
        }
        std::cout<<"RWM: "<<__FILE__<<":"<<__LINE__<<"  nPlotStartYear_g="<<nPlotStartYear_g<<std::endl;
    
    }
  
    delete [] cTokBuf;
  
    return csf;
}

// Use the following methods to sift out stations per requests
// from the browser front-end

// Simple "block" selection -- Rural, Urban, or Suburban/Mixed
std::set<std::size_t> GHCN::SelectStationsByRuralUrbanStatus(const std::string& strMsg)
{
    std::map<std::size_t, GHCNLatLong> stationMapAll;
    std::map<std::size_t, GHCNLatLong>::iterator iMap;
    stationMapAll=GetCompleteWmoLatLongMap();
    std::string strStationType;
    
    std::set<std::size_t> selectedWmoIdSet;
    
    // RRR -- add in these lines to select raw/adj data to display.
    // char* cTok=NULL;
    // char cTokBuf=new char[strMsg.length()+1];
    // selectDataFlag_g[0]=::atoi(cTok);
    // selectDataFlag_g[1]=::ParseNextTok(cTok);
    // csf.bnSelectAll=::ParseNextTok(cTok);

    strStationType=std::string("ALL");  // Default is all stations
    
    if(strcasestr(strMsg.c_str(),"lotRural")!=NULL)
    {
        strStationType=std::string("RURAL");
    }
    if(strcasestr(strMsg.c_str(),"lotMixed")!=NULL)
    {
        strStationType=std::string("SUBURBAN");
    }
    else if(strcasestr(strMsg.c_str(),"lotUrban")!=NULL)
    {
        strStationType=std::string("URBAN");
    }
    
    // Make sure the station list is cleared out
    selectedWmoIdSet.clear();
    
    if(strStationType==std::string("ALL"))
    {
        // Process all stations
        for(iMap=stationMapAll.begin(); iMap!=stationMapAll.end(); ++iMap)
        {
            selectedWmoIdSet.insert(iMap->first);
        }
    }
    else // Process selected stations (rural/suburban/urban)
    {
        for(iMap=stationMapAll.begin(); iMap!=stationMapAll.end(); ++iMap)
        {
            if(iMap->second.strStationType==strStationType)
            {
                selectedWmoIdSet.insert(iMap->first);
            }
        }
    }

    return selectedWmoIdSet;
    
}

// Finer-grained selection: any combination of start/end/duration/altitude/airport 
// /rural/urban/suburban designations)
std::set<std::size_t> GHCN::SelectStationsByClientRequest(const ClientSelectFields& csf)
{
    std::map<std::size_t,GHCNLatLong>::iterator iMap;
    std::set<std::size_t> stationSet;

    // Initialize to complete set of stations    
    for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
    {
        stationSet.insert(iMap->first);
    }
  
    std::cerr<<__LINE__<<": #stations = "<<stationSet.size()<<std::endl;
  
    if(1==csf.bnSelectAll)
    {
        // Use all station data -- don't bother parsing the rest of the msg
        return stationSet;
    }
  
    // Select by type (rural/suburban/urban)
    if(1==csf.bnSelectType)
    {
        std::cerr<<"Selecting stations by type: "<<csf.nType<<std::endl;
    
        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(csf.nType!=iMap->second.iStationType) // Remove stations that don't match requested type
            {
                stationSet.erase(iMap->first);
            }
        }
    }

    std::cerr<<__LINE__<<": #stations = "<<stationSet.size()<<std::endl;
  
  
    // Now select by airport proximity
    if(1==csf.bnSelectAirport)
    {
        std::cerr<<"Selecting stations by airport proximity: "<<csf.nAirport<<std::endl;

        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(csf.nAirport != iMap->second.isNearAirport)
            {
                stationSet.erase(iMap->first);  // Remove stations that don't match requested airport status
            }
        }
    }
  
    std::cerr<<__LINE__<<": #stations = "<<stationSet.size()<<std::endl;
  
    // Select by altitude
    if(1==csf.bnSelectAltitude)
    {
        std::cerr << "Selecting stations by altitude "<<csf.nAltitude<<"  mode="<<csf.nAltitudeMode<<std::endl;
    
        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(iMap->second.altitude<-900)
            {
                // Station altitude unknown -- exclude it
                // Unknown altitudes designated by -999 in GHCN database
                // Use -900 to allow for any possible "slop" in the data.
                stationSet.erase(iMap->first);
            }
            else
            {
                switch(csf.nAltitudeMode)
                {
                    case 0: // want to keep stations <= csf.nAltitude
            
                        if(csf.nAltitude < iMap->second.altitude)
                        {
                            stationSet.erase(iMap->first);  // Remove stations that don't match requested altitude range
                        }
                        break;
            
                    case 1: // want to keep stations >= csf.nAltitude
                        if(csf.nAltitude > iMap->second.altitude)
                        {
                            std::cerr<<"Excluding Station "<<iMap->first<<" station altitude "<<iMap->second.altitude<<" < "<<csf.nAltitude<<std::endl;
              
                            stationSet.erase(iMap->first);  // Remove stations that don't match requested altitude range
                        }
                        break;
          
                    default: // out of range -- do nothing
                        break;

                }
        
            }
      
        }
    
    }

    std::cerr<<__LINE__<<": #stations = "<<stationSet.size()<<std::endl;
    // Remember that this program saves latitudes 0-180 (not -90 to 90)
    // So subtract 90 deg from station latitudes below
    // Select by min latitude
    if(1==csf.bnSelectMinLatitude)
    {
        std::cerr << "Selecting stations by min latitude "<<csf.nMinLatitude100/100.<<std::endl;
          
        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(iMap->second.lat_deg-90.<csf.nMinLatitude100/100.)
            {
                stationSet.erase(iMap->first);  // Remove stations outside min latitude limit
            }
        }
    
    }

    // Select by max latitude
    if(1==csf.bnSelectMaxLatitude)
    {
        std::cerr << "Selecting stations by max latitude "<<csf.nMaxLatitude100/100.<<std::endl;
          
        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(iMap->second.lat_deg-90.>csf.nMaxLatitude100/100.)
            {
                stationSet.erase(iMap->first);  // Remove stations outside min latitude limit
            }
        }
    
    }

    std::cerr<<__LINE__<<": #stations = "<<stationSet.size()<<std::endl;

    if(1==csf.nYearOrAnd) // Year "and" operation requested
    {

        std::cerr << "Selecting stations -- year \"and\" mode..."<<std::endl;
    
        // Create a new station map with just the stations that have
        // made it through the selection process so far.
        std::map<std::size_t,GHCNLatLong> sMap;
        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(stationSet.find(iMap->first) != stationSet.end())
            {
                sMap[iMap->first]=iMap->second;
            }
        }

        stationSet = SelectStationsByYearsAnd(csf,sMap,stationSet);
      
        std::cerr<<"#stations selected in \"And\" mode = "<<stationSet.size()<<std::endl;
    
    }
    
    else // Begin "or" year select mode
    {
        std::cerr << "Selecting stations -- year \"or\" mode..."<<std::endl;

        // Create a new station map with just the stations that have
        // made it through the selection process so far.
        std::map<std::size_t,GHCNLatLong> sMap;
    
        for(iMap=m_CompleteWmoLatLongMap.begin(); iMap!=m_CompleteWmoLatLongMap.end(); ++iMap)
        {
            if(stationSet.find(iMap->first) != stationSet.end())
            {
                sMap[iMap->first]=iMap->second;
            }
        }
    
        stationSet = SelectStationsByYearsOr(csf,sMap,stationSet);
    
     
    }
  
    return stationSet;
  
}

std::set<std::size_t> GHCN::SelectStationsByYearsAnd(const ClientSelectFields& csf, const std::map<std::size_t, GHCNLatLong>& sMap,
                                                     const std::set<std::size_t>& stationSetIn)
{
  
    std::set<std::size_t> stationSet;

    std::map<std::size_t,GHCNLatLong>::const_iterator iMap;
    std::map<std::size_t,GHCNLatLong> mapSelect, mapSave;

    mapSave=sMap;
    mapSelect=sMap;
  
    stationSet=stationSetIn;
  
    // Select by start year
    if(1==csf.bnSelectStartYear)
    {
        std::cerr << "Selecting stations -- start year "<<csf.nStartYear<<std::endl;

        for(iMap=mapSave.begin(); iMap!=mapSave.end(); ++iMap)
        {
            switch(csf.nStartYearMode)
            {
                case 0: // want to keep stations with <= specified start year
                    if(iMap->second.first_year > csf.nStartYear)
                    {
                        stationSet.erase(iMap->first);  // Remove stations that don't match requested first year
                        mapSelect.erase(iMap->first);
                    }
                    break;
          
                case 1: // want stations >= specified start year
                    if(iMap->second.first_year < csf.nStartYear)
                    {
                        stationSet.erase(iMap->first);  // Remove stations that don't match requested first year
                        mapSelect.erase(iMap->first);
                    }
                    break;
          
                default: // out of range -- do nothing
                    break;
          
            }
        }
    }

    std::cerr<<stationSet.size()<<" stations remaining (end year next)..."<<std::endl;
  
    // mapSave=mapSelect;

    // Select by end year
    if(1==csf.bnSelectEndYear)
    {
        std::cerr << "Selecting stations -- end year "<<csf.nEndYear<<std::endl;
        for(iMap=mapSave.begin(); iMap!=mapSave.end(); ++iMap)
        {
            switch(csf.nEndYearMode)
            {
                case 0: // want to keep stations with <= specified end year
                    if(iMap->second.last_year > csf.nEndYear)
                    {
                        stationSet.erase(iMap->first);  // Remove stations that don't match requested last year
                        mapSelect.erase(iMap->first);
                    }
                    break;
          
                case 1: // want stations >= specified end year
                    if(iMap->second.last_year < csf.nEndYear)
                    {
                        stationSet.erase(iMap->first);  // Remove stations that don't match requested last year
                        mapSelect.erase(iMap->first);
                    }
                    break;
          
                default: // out of range -- do nothing
                    break;
          
            }
        }
    }
  
    std::cerr<<stationSet.size()<<" stations remaining (num years next)..."<<std::endl;

    //mapSave=mapSelect;
  
    // Select by num years (data record length)
    if(1==csf.bnSelectNumYears)
    {
        std::cerr << "Selecting stations -- num years "<<csf.nNumYears<<std::endl;
        for(iMap=mapSave.begin(); iMap!=mapSave.end(); ++iMap)
        {
            switch(csf.nNumYearsMode)
            {
                case 0: // want to keep stations with <= specified num years
//          if(csf.nNumYears > iMap->second.num_years)
                    if(iMap->second.num_years > csf.nNumYears)
                    {
                        stationSet.erase(iMap->first);  // Remove stations that don't match requested #years
                    }
                    break;
            
                case 1: // want stations >= specified num years
                    if(iMap->second.num_years < csf.nNumYears)
                    {
                        stationSet.erase(iMap->first);  // Remove stations that don't match requested #years
                    }
                    break;
            
                default: // out of range -- do nothing
                    break;
            
            }
        }
    }
  
  
    std::cerr <<__FUNCTION__<<":  Selected a total of "<<stationSet.size()<<" stations.."<<std::endl;
  
    return stationSet;
}

std::set<std::size_t> GHCN::SelectStationsByYearsOr(const ClientSelectFields& csf, 
                                                    const std::map<std::size_t,GHCNLatLong>& sMap, 
                                                    const std::set<std::size_t>& stationSetIn)
{
    std::map<std::size_t,GHCNLatLong>::const_iterator iMap;
    std::set<std::size_t> startSet, endSet, numSet;
    std::set<std::size_t> stationSet=stationSetIn;
  
    bool bExecutedASelectOp=false;
  
    // Select by start year
    if(1==csf.bnSelectStartYear)
    {
        std::cerr << "Selecting stations -- start year <= "<<csf.nStartYear<<std::endl;
        bExecutedASelectOp=true;
      
        for(iMap=sMap.begin(); iMap!=sMap.end(); ++iMap)
        {
            switch(csf.nStartYearMode)
            {
                case 0: // want to keep stations with <= specified start year
                    if(iMap->second.first_year <= csf.nStartYear)
                    {
                        if(stationSet.find(iMap->first)!=stationSet.end())
                        {
                            startSet.insert(iMap->first);
                        }
                    }
                    break;
            
                case 1: // want stations >= specified start year
                    if(iMap->second.first_year >= csf.nStartYear)
                    {
                        if(stationSet.find(iMap->first)!=stationSet.end())
                        {
                            startSet.insert(iMap->first);
                        }
                    }
                    break;
            
                default: // out of range -- do nothing
                    startSet=stationSet;
                    break;
            
            }
        }
    }

    // select by "end" year
    if(1==csf.bnSelectEndYear)
    {
        bExecutedASelectOp=true;
        std::cerr << "Selecting stations -- end year <= "<<csf.nEndYear<<std::endl;
        for(iMap=sMap.begin(); iMap!=sMap.end(); ++iMap)
        {
            switch(csf.nEndYearMode)
            {
                case 0: // want to keep stations with <= specified end year
                    if(iMap->second.last_year <= csf.nEndYear)
                    {
                        if(stationSet.find(iMap->first)!=stationSet.end())
                        {
                            endSet.insert(iMap->first);
                        }
                    }
                    break;
            
                case 1: // want stations >= specified end year
                    if(iMap->second.last_year >= csf.nEndYear)
                    {
                        if(stationSet.find(iMap->first)!=stationSet.end())
                        {
                            endSet.insert(iMap->first);
                        }
                    }
                    break;
            
                default: // out of range -- do nothing
                    endSet=stationSet;
                    break;
            
            }
        }
    }

    // select by num years
    if(1==csf.bnSelectNumYears)
    {
        bExecutedASelectOp=true;
        std::cerr << "Selecting stations -- num years <= "<<csf.nNumYears<<std::endl;

        for(iMap=sMap.begin(); iMap!=sMap.end(); ++iMap)
        {
            switch(csf.nNumYearsMode)
            {
                case 0: // want to keep stations with <= specified num years
                    if(iMap->second.num_years <= csf.nNumYears)
                    {
                        if(stationSet.find(iMap->first)!=stationSet.end())
                        {
                            numSet.insert(iMap->first);
                        }
                    }
                    break;
            
                case 1: // want stations >= specified num years
                    if(iMap->second.num_years >= csf.nNumYears)
                    {
                        if(stationSet.find(iMap->first)!=stationSet.end())
                        {
                            numSet.insert(iMap->first);
                        }
                    }
                    break;
            
                default: // out of range -- do nothing
                    numSet=stationSet;
                    break;
            
            }
        }
    }
 
    // Final station set computed by "or-ing" startSet, endSet and numSet.
    std::set<std::size_t>::reverse_iterator irbegin;
    std::set<std::size_t>::iterator iend;

    if(true==bExecutedASelectOp)
    {
        std::set<std::size_t> stationSetSelect;
        stationSetSelect=stationSetIn;
      
        std::set<std::size_t> stationSetYears;
        std::set<std::size_t>::iterator iSet;
      
        // If any year selection op was executed, then
        // *and* the year selection with the current selection
      
        // First, *or* the year selections together
        stationSetYears=startSet;
        stationSetYears.insert(endSet.begin(),endSet.end());
        stationSetYears.insert(numSet.begin(),numSet.end());

        // Now *and* the current selection with the year-selection
        for(iSet=stationSetIn.begin(); iSet!=stationSetIn.end(); ++iSet)
        {
            if(stationSetYears.find(*iSet) == stationSetYears.end())
            {
                stationSetSelect.erase(*iSet);
            }
        }

        stationSet=stationSetSelect;

    }
    
    return stationSet;
}





// RRR 2015/04/30 new fdn
// USE THIS FOR SERVER MODE -- selectedWmoIdSet will contain the
// station Id's sent over by the client
void GHCN::ComputeGriddedGlobalAverageAnnualAnomalies(const int& minBaselineSampleCount,
                                                      const float& initDelLatDeg,
                                                      const float& initDelLongDeg,
                                                      std::map<std::size_t, GHCNLatLong>& latLongMap,// can't be const
                                                      const std::set<std::size_t>& selectedWmoIdSet)
{
    std::map<std::size_t,GHCNLatLong> selectedLatLongMap;
    std::map<std::size_t,GHCNLatLong>::iterator iml;
    std::set<std::size_t>::const_iterator isl;

    GHCNLatLong lTest;

    bool bFoundAStation=false;

    m_ProcessedWmoLatLongMap.clear();
    selectedLatLongMap.clear();
  
    for(isl=selectedWmoIdSet.begin(); isl!=selectedWmoIdSet.end(); ++isl)
    {
        if(latLongMap.find(*isl)!=latLongMap.end())
        {
            // station *isl has been located -- save off its GHCNLatLong element
            //selectedLatLongMap[static_cast<int>(*isl)]=latLongMap[0];
            selectedLatLongMap[*isl]=static_cast<GHCNLatLong>(latLongMap[*isl]);
            bFoundAStation=true;
        }
    }
  
    if(true==bFoundAStation)
    {
        ComputeGriddedGlobalAverageAnnualAnomalies(minBaselineSampleCount,
                                                   initDelLatDeg, initDelLongDeg,
                                                   selectedLatLongMap);
    }
    else
    {
        // Do nothing and carry on...
    }
  

    return;
  

}

// RRR 2015/04/30 new function
void GHCN::ComputeGriddedGlobalAverageAnnualAnomalies(const int& minBaselineSampleCount,
                                                      const float& initDelLatDeg,
                                                      const float& initDelLongDeg,
                                                      const std::map<std::size_t, GHCNLatLong>& latLongMap)
{

    std::map<std::size_t, GHCNLatLong>::const_iterator imll;

    std::cerr<<std::endl;
    std::cerr<<"Total number of station entries in latLongMap: "<<latLongMap.size()<<std::endl;
    std::cerr<<std::endl;
  
    // RWMM 2012/08/29 begin
    mAverageAnnualStationCountMap.clear();

    // RWM 2015/01/15 -- iYear now <=MAX_GISS_YEAR
    int iYear;
    for(iYear=MIN_GISS_YEAR; iYear<=MAX_GISS_YEAR; iYear++)
    {
        mAverageAnnualStationCountMap[iYear]=0.;
    }
    // RWMM 2012/08/29 end

    ComputeGridCellAverageMonthlyAnomalies(minBaselineSampleCount,
                                           initDelLatDeg,initDelLongDeg,
                                           latLongMap);

    ComputeGlobalAverageAnnualAnomaliesFromGridCellMonthlyAnomalies();

    ComputeAnnualMovingAvg();
  
    std::map<std::size_t,float>::iterator iasc;

#if(1)  // DEBUG/VERIFICATION CODE
    double dNstations=0;
    int nYears=0;
    for(iasc=mAverageAnnualStationCountMap.begin(); iasc!=mAverageAnnualStationCountMap.end(); ++iasc)
    {
        // std::cerr<<"Year="<<iasc->first
        //          <<"   #Stations="<<iasc->second<<std::endl;
        dNstations+=iasc->second;
        nYears+=1;
    }
    double dAvgNstations;
    dAvgNstations=dNstations/nYears;

    std::cerr<<std::endl;
    std::cerr<<"Avg #Stations per year = "<<dAvgNstations<<std::endl;
#endif

    return;

}


void GHCN::ComputeGriddedGlobalAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                      const float& initDelLatDeg,
                                                      const float& initDelLongDeg,
                                                      std::map<std::size_t, GHCNLatLong>& latLongMap,// can't be const
                                                      const std::set<std::size_t>& selectedWmoIdSet)
{
    std::map<std::size_t,GHCNLatLong> selectedLatLongMap;
    std::map<std::size_t,GHCNLatLong>::iterator iml;
    std::set<std::size_t>::const_iterator isl;

    GHCNLatLong lTest;

    bool bFoundAStation=false;

    m_ProcessedWmoLatLongMap.clear();
    selectedLatLongMap.clear();
  
    for(isl=selectedWmoIdSet.begin(); isl!=selectedWmoIdSet.end(); ++isl)
    {
        if(latLongMap.find(*isl)!=latLongMap.end())
        {
            // station *isl has been located -- save off its GHCNLatLong element
            //selectedLatLongMap[static_cast<int>(*isl)]=latLongMap[0];
            selectedLatLongMap[*isl]=static_cast<GHCNLatLong>(latLongMap[*isl]);
            bFoundAStation=true;
        }
    }
  
    if(true==bFoundAStation)
    {
        ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount,
                                                   initDelLatDeg, initDelLongDeg,
                                                   selectedLatLongMap);
    }
    else
    {
        // Do nothing and carry on...
    }
  

    return;
  

}

void GHCN::ComputeGriddedGlobalAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                      const float& initDelLatDeg,
                                                      const float& initDelLongDeg,
                                                      const std::map<std::size_t, GHCNLatLong>& latLongMap)
{

    std::map<std::size_t, GHCNLatLong>::const_iterator imll;

    std::cerr<<std::endl;
    std::cerr<<__FUNCTION__<<"(): Total number of station entries in latLongMap: "
             <<latLongMap.size()<<std::endl;
    std::cerr<<std::endl;
  
    // RWMM 2012/08/29 begin
    mAverageAnnualStationCountMap.clear();

    // RWM 2015/01/15 -- iYear now <=MAX_GISS_YEAR
    int iYear;
    for(iYear=MIN_GISS_YEAR; iYear<=MAX_GISS_YEAR; iYear++)
    {
        mAverageAnnualStationCountMap[iYear]=0.;
    }
    // RWMM 2012/08/29 end
    std::cerr<<__FUNCTION__<<"(): Now will compute grid cell monthly anomalies."<<std::endl;

    ComputeGridCellAverageMonthlyAnomalies(minBaselineSampleCount,
                                           initDelLatDeg,initDelLongDeg,
                                           latLongMap);

    std::cerr<<"Computed grid cell monthly anomalies."<<std::endl;
    
    ComputeGlobalAverageMonthlyAnomaliesFromGridCellMonthlyAnomalies();

    std::cerr<<"Computed global average monthly anomalies from grid cells."<<std::endl;
    
    ComputeMonthlyMovingAvg();
  
    std::map<std::size_t,float>::iterator iasc;

#if(1)  // DEBUG/VERIFICATION CODE
    double dNstations=0;
    int nYears=0;
    for(iasc=mAverageAnnualStationCountMap.begin(); iasc!=mAverageAnnualStationCountMap.end(); ++iasc)
    {
        // std::cerr<<"Year="<<iasc->first
        //          <<"   #Stations="<<iasc->second<<std::endl;
        dNstations+=iasc->second;
        nYears+=1;
    }
    double dAvgNstations;
    dAvgNstations=dNstations/nYears;

    std::cerr<<std::endl;
    std::cerr<<"Avg #Stations per year = "<<dAvgNstations<<std::endl;
#endif

    return;

}



std::set<std::size_t> GHCN::PickLongestNStations(std::set<std::size_t>* ghcnStationSetIn, const int nStations)
{
        
    std::set<std::size_t> stationSet;

    std::set<std::size_t> sStationsFoundSet;
        
    if(ghcnStationSetIn==NULL)
        return sStationsFoundSet;
        
    std::set<std::size_t>::iterator iss;
        
    stationSet=*ghcnStationSetIn;

    std::size_t wmoId=0; // Initialize to a value that no station wmoId can have.
        
    int iStation;
    for(iStation=0; iStation<nStations; iStation++)
    {
        // Station not picked yet
        wmoId=PickLongestStation(&stationSet);

        if(sStationsFoundSet.find(wmoId)==sStationsFoundSet.end())
        {
            sStationsFoundSet.insert(wmoId); // Add to the list of stations found

            stationSet.erase(stationSet.find(wmoId)); // Remove from the list we are searching
        }
                
        if(stationSet.size()<1) 
        {
            // No more stations to look for (i.e. we
            // picked all of the stations in the
            // list we were given)
            break;
        }
    }
        
    // The list of N stations with the longest records of all the
    // stations in ghcnStationSetIn.

    return sStationsFoundSet;
}

  
// Searches the set of stations for the one with the longest
// temperature record.
std::size_t GHCN::PickLongestStation(std::set<std::size_t>* ghcnStationSetIn)
{
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss; //station-id iterator
  
    std::map<std::size_t, std::vector<float> >::reverse_iterator riyy; // reverse year

    std::map<std::size_t, std::vector<float> >:: iterator iyy; // year iterator

    int nRecordLength=0;
    int nMaxRecordLength=0;
    std::size_t nMaxLengthWmoId=0;

    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        if(ghcnStationSetIn!=NULL)
        {
            if(ghcnStationSetIn->find(iss->first) == ghcnStationSetIn->end())
            {
                // Station list exists and this station isn't on it.
                // Skip it and go to the next one
                continue;
            }
            riyy=iss->second.rbegin();
            iyy=iss->second.begin();
            nRecordLength=riyy->first-iyy->first+1;
            if(nRecordLength>nMaxRecordLength)
            {
                nMaxRecordLength=nRecordLength;
                nMaxLengthWmoId=iss->first;
            }
        }
    }

#if(1)
    // Found a station with the longest minyear to maxyear length.
    // Other stations may have the same minyear, maxyear values.
    // Now search again to find the station with the fewest "missing years"
    // between minyear and maxyear
    int nYears;
    int nMaxYears=0;
  
    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        if(ghcnStationSetIn!=NULL)
        {
            if(ghcnStationSetIn->find(iss->first) != ghcnStationSetIn->end())
            {
                riyy=iss->second.rbegin();
                iyy=iss->second.begin();
                nRecordLength=riyy->first-iyy->first+1;
                nYears=iss->second.size();
                if(nRecordLength>=nMaxRecordLength)
                {
                    if(nYears>nMaxYears)
                    {
                        nMaxYears=nYears;
                        nMaxLengthWmoId=iss->first;
                    }
                }
            }
        }
    }
#endif

    return nMaxLengthWmoId;
  
}

// Choose only stations with a certain record length (or greater)
std::set<std::size_t> GHCN::MinStationRecordLength(std::set<std::size_t>* ghcnStationSetIn)
{

    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss; //station-id iterator
  
    std::map<std::size_t, std::vector<float> >:: iterator iyy; // year iterator

    std::map<std::size_t, std::vector<float> >:: reverse_iterator riyy; // year iterator

    std::size_t nRecordLength=0;

    std::set<std::size_t> sSelectedStationsSet;

    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        if(ghcnStationSetIn!=NULL)
        {
            if(ghcnStationSetIn->find(iss->first) == ghcnStationSetIn->end())
            {
                // Station list exists and this station isn't on it.
                // Skip it and go to the next one
                continue;
            }
            iyy=iss->second.begin();
            riyy=iss->second.rbegin();

            nRecordLength=iss->second.size();
      
            // Choose the station if it has a record length of 
            // at least 100 years
            if(nRecordLength>=nMinStationRecordLength_g)
            {
                sSelectedStationsSet.insert(iss->first);
            }
        }
    }

    return sSelectedStationsSet;

}

// Choose only stations with max record start years and/or 
// min record end years
std::set<std::size_t> GHCN::PickStationsByRecordSpecs(std::set<std::size_t>* ghcnStationSetIn)
{

    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss; //station-id iterator
  
    std::map<std::size_t, std::vector<float> >:: iterator iyy; // year iterator

    std::map<std::size_t, std::vector<float> >:: reverse_iterator riyy; // year iterator

    std::set<std::size_t> sSelectedStationsSet;
  
    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        if(ghcnStationSetIn!=NULL)
        {
            if(ghcnStationSetIn->find(iss->first) == ghcnStationSetIn->end())
            {
                // Station list exists and this station isn't on it.
                // Skip it and go to the next one
                continue;
            }
            iyy=iss->second.begin();
            riyy=iss->second.rbegin();

            if((true==bMaxStationRecordStartYear_g) &&
               (true==bMinStationRecordEndYear_g) &&
               (true==bMinStationRecordLength_g))
            {
                std::size_t nRecordLength;
                nRecordLength=riyy->first-iyy->first+1;
                if((iyy->first<=nMaxStationRecordStartYear_g)&&
                   (riyy->first>=nMinStationRecordEndYear_g)&&
                   (nRecordLength>=nMinStationRecordLength_g))
                {
                    sSelectedStationsSet.insert(iss->first);
                }
                continue;
            }
            else if((true==bMaxStationRecordStartYear_g) &&
                    (true==bMinStationRecordEndYear_g))
            {
                // Max start and Min end years specified
                if((iyy->first<=nMaxStationRecordStartYear_g)&&
                   (riyy->first>=nMinStationRecordEndYear_g))
                {
                    sSelectedStationsSet.insert(iss->first);
                }
                continue;
            }
            else if(true==bMaxStationRecordStartYear_g)
            {
                if(iyy->first<=nMaxStationRecordStartYear_g)
                {
                    sSelectedStationsSet.insert(iss->first);
                }
                continue;
            }
            else if(true==bMinStationRecordEndYear_g)
            {
                if(riyy->first>=nMinStationRecordEndYear_g)
                {
                    sSelectedStationsSet.insert(iss->first);
                }
                continue;
            }
            else if(true==bMinStationRecordLength_g)
            {
                iyy=iss->second.begin();
                riyy=iss->second.rbegin();
                std::size_t nRecordLength;
                nRecordLength=riyy->first-iyy->first+1;

                // Choose the station if it has a record length of 
                // at least 100 years
                if(nRecordLength>=nMinStationRecordLength_g)
                {
                    sSelectedStationsSet.insert(iss->first);
                }
            }
        }
    }

    return sSelectedStationsSet;

}

void GHCN::PrecomputeStationAnomalies(const int nMinBaselineSampleCount)
{
  
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss; //station-id iterator
    std::map<std::size_t, std::vector<float> >:: iterator iyy; // year iterator
    int imm; // month index

    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        for(iyy=iss->second.begin(); iyy!=iss->second.end(); ++iyy)
        {
            if((iyy->first>=MIN_GISS_YEAR)&&(iyy->first<=MAX_GISS_YEAR))
            {
                for(imm=0; imm<12; imm++)
                {
                    // Valid temperature sample?
                    if(iyy->second[imm]>GHCN_NOTEMP999+ERR_EPS)
                    {
                        //Enough baseline-period samples for valid baseline?
                        if(nMinBaselineSampleCount>0)
                        {
                            if(mBaselineSampleCountMap[iss->first][imm]>=nMinBaselineSampleCount)
                            {
                                iyy->second[imm]=iyy->second[imm]-mBaselineTemperatureMap[iss->first][imm];
                            }
                        }
                    }
                }
            }
        }
    }
  
    return;
}

// 2024/03/10
// 1) 
// 3)   gridElIn -- contains current grid element lat/longs.
// 4-5) nSubcellLat,nSubcellLon -- number of lat/long subdivisions
//      to create subcells from.
// 6) latLongMap: map of stations (station id hashes) with their latlong locations.
// 7) ghcnStationSet -- stations located inside the main gridEl.
// Return std::pair<fAnomaly,nSubGridCells>
float GHCN::ComputeGridCellAnomalyFromSubCells
(std::map<std::size_t, std::vector<float> >:: iterator iYy, // year iterator
 const int iMm,
 LatLongGridElement& gridElIn,
 const int& nSubCellLatIn, const int& nSubCellLongIn,
 const std::map<std::size_t, GHCNLatLong>& latLongMapIn)
{

    int nSubCellLat=nSubCellLatIn;
    int nSubCellLong=nSubCellLongIn;
    
    if(nSubCellLat<1)
        nSubCellLat=1;
    if(nSubCellLong<1)
        nSubCellLong=1;
    
    float fAnomaly=0;

    float fMainCellMinLat=gridElIn.minLatDeg;
    float fMainCellMaxLat=gridElIn.maxLatDeg;
    float fMainCellMinLong=gridElIn.minLongDeg;
    float fMainCellMaxLong=gridElIn.maxLongDeg;
    
    float fMainCellLatSize=fMainCellMaxLat-fMainCellMinLat;
    float fMainCellLongSize=fMainCellMaxLong-fMainCellMinLong;
    
    float fSubCellLatSize=fMainCellLatSize/nSubCellLat;
    float fSubCellLongSize=fMainCellLongSize/nSubCellLong;

    int nSubCellCount=0;

    std::vector<float> vfSubCellAnomaly;
    
    LatLongGridElement subCellGridEl;
    for(float fLat=fMainCellMinLat; fLat<fMainCellMaxLat; fLat+=fSubCellLatSize)
    {
        subCellGridEl.minLatDeg=fLat;
        subCellGridEl.maxLatDeg=fLat+fSubCellLatSize;
        
        for(float fLong=fMainCellMinLong; fLat<fMainCellMaxLong; fLat+=fSubCellLongSize)
        {
            subCellGridEl.minLongDeg=fLong;
            subCellGridEl.maxLongDeg=fLong+fSubCellLongSize;
            // For each subcell, need to determine which (if any) stations are located
            // in said subcell.
            std::set<std::size_t> subCellStationSet=ComputeGridElementStationSet(latLongMapIn,subCellGridEl);
            if(subCellStationSet.size()>0)
            {
                // Accumulate temperature reading sums for all stations
                // within this grid subcell for year iYy->first, month iMm(0-11)
                // mGlobalAverageMonthlyAnomaliesMap[iYy->first][iMm] 
                // Station data in mTempsMap[stationid][iYy->first][imm]
                // Need to summ up
                // mTempsMap[for all stations in this subcell][for year iYy->first][and for month iMm]
                for(auto & iss:subCellStationSet)
                    fAnomaly+=mTempsMap[iss][iYy->first][iMm];
                fAnomaly/=(1.*subCellStationSet.size());
                vfSubCellAnomaly.push_back(fAnomaly);
                nSubCellCount+=1;
            }
            else
            {
                // Empty grid cell -- do nothing here.
            }
        }
    }

    // re-using fAnomaly here
    fAnomaly=0;
    
    if(vfSubCellAnomaly.size()<1)
        fAnomaly=GHCN_NOTEMP9999;
    else
    {
        for(auto& iv: vfSubCellAnomaly)
        {
            fAnomaly+=iv;
        }
    }

    fAnomaly/=vfSubCellAnomaly.size();
    
    return fAnomaly;
}

// Returns a set of years corresponding to reported results.
// Entries indicate years where this grid cell has station data.
std::set<std::size_t> GHCN::ComputeAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                           const LatLongGridElement& gridEl,
                                                           const int& nSubcellLat, const int& nSubcellLon,
                                                           std::set<std::size_t>* ghcnStationSetIn/*=NULL*/)
{

    std::set<std::size_t> yearsToReportSet;
  
    std::map<std::size_t, std::map<std::size_t, std::vector<float> > >::iterator iss; //station-id iterator
  
    std::map<std::size_t, std::vector<float> >:: iterator iyy; // year iterator
  
    int imm; // month index

    mAverageStationCountMap.clear();
    // These need to be reset each time this fcn is called.
    // Results for the the set of ghcnStationSetIn stations are computed.
    // Those results will be combined later to produce global results.
    mAverageStationCountMap.clear();
    int iYear,iMonth;
    // RWM 2015/01/15 now: iYear<=MAX_GISS_YEAR
    for(iYear=MIN_GISS_YEAR; iYear<=MAX_GISS_YEAR; iYear++)
    {
        for(iMonth=0; iMonth<12; iMonth++)
        {
            mAverageStationCountMap[iYear][iMonth]=0;
        }
    }
    mGlobalAverageMonthlyAnomaliesMap.clear();


    int nStationCount=0;

    std::set<std::size_t>* ghcnStationSet=NULL;

    ghcnStationSet=ghcnStationSetIn; // RWMM 2012/08/30 new

    // iss: station iterator.
    for(iss=mTempsMap.begin(); iss!=mTempsMap.end(); ++iss)
    {
        if(ghcnStationSet!=NULL)
        {
            if(ghcnStationSet->find(iss->first) == ghcnStationSet->end())
            {
                // Station list exists and this station isn't on it.
                // Skip it and go to the next one
                continue;
            }
        }
        // iyy: year iterator.
        for(iyy=iss->second.begin(); iyy!=iss->second.end(); ++iyy)
        {
            if(mGlobalAverageMonthlyAnomaliesMap[iyy->first].size() == 0)
            {
                mGlobalAverageMonthlyAnomaliesMap[iyy->first].resize(12);
                for(imm=0; imm<12; imm++)
                {
                    mGlobalAverageMonthlyAnomaliesMap[iyy->first][imm]=0; // This is OK 
                }
            }
            // imm: month iterator.
            for(imm=0; imm<12; imm++)
            {
                // Do we have a valid temperature sample?
                // Raw GHCN 1/10 deg samps have already been divided by 10 here,
                // so compare against GHCN_NOTEMP999 (-999) instead
                // of GHCN_NOTEMP9999 (-9999)
                if(iyy->second[imm]>GHCN_NOTEMP999+ERR_EPS)
                {
                    // Do we have enough baseline temperature samples to include
                    // this station/month in the anomaly average?
                    if(mBaselineSampleCountMap[iss->first][imm]>=minBaselineSampleCount)
                    {
                        // Got at least one valid temperature data sample for this
                        // set of stations for this year -- add this year to the bResultsToReportSet
                        // set.
                        if(yearsToReportSet.find(iyy->first)==yearsToReportSet.end())
                            yearsToReportSet.insert(iyy->first);
            
                        if(mGlobalAverageMonthlyAnomaliesMap.find(iyy->first)
                           ==mGlobalAverageMonthlyAnomaliesMap.end())  
                        {
                            // 1st element for this particular year and month?  Need
                            // to create a new map entry.
                            mGlobalAverageMonthlyAnomaliesMap[iyy->first][imm] = 
                                iyy->second[imm]; // -mBaselineTemperatureMap[iss->first][imm];
                            mAverageStationCountMap[iyy->first][imm]=1;
                        }
                        else
                        {
                            // Accumulate the anomaly sum for this grid cell for this year/month.
                            // 2024/03/10 -- want to put the subcell operation here when it's ready.
                            mGlobalAverageMonthlyAnomaliesMap[iyy->first][imm] 
                                += (iyy->second[imm]); //-mBaselineTemperatureMap[iss->first][imm]);

                            // Station counter element already exists for year/month -- increment it.
                            mAverageStationCountMap[iyy->first][imm]+=1;
                        }
                        // RWMM 2012/08/29 begin -- build list of stations actually processed
                        if(m_CompleteWmoLatLongMap.find(iss->first) != m_CompleteWmoLatLongMap.end())
                            m_ProcessedWmoLatLongMap[iss->first]=m_CompleteWmoLatLongMap[iss->first];
                        // RWMM 2012/08/29 end


                    }
                }
            }
        }

        nStationCount++;

    }

    // Now have anomaly sums (summed over all qualifying stations) 
    // for each year and month. Divide by the number of stations included 
    // for each year and month to get the average anomaly values.
    std::map<std::size_t,std::vector<double> >::iterator avg_iyy;
    for(avg_iyy=mGlobalAverageMonthlyAnomaliesMap.begin(); 
        avg_iyy!=mGlobalAverageMonthlyAnomaliesMap.end(); ++avg_iyy)
    {
        // Loop over months in a given year.
        for(imm=0; imm<12; imm++)
        {
            mAverageAnnualStationCountMap[avg_iyy->first]
                +=mAverageStationCountMap[avg_iyy->first][imm]/12.;
      
            if(mAverageStationCountMap[avg_iyy->first][imm]>=1)
            {
                avg_iyy->second[imm] /= mAverageStationCountMap[avg_iyy->first][imm];
            }
            else
            {
                // No station data found for this year/month?
                // Then set to GHCN_NOTEMP9999 so that this entry won't
                // used to compute the annual anomaly temperatures.
                avg_iyy->second[imm]=GHCN_NOTEMP9999;
            }
        }
    }

    return yearsToReportSet;
  
}

#if(1) // 2025/10/15 compute global average monthly anomalies indexed by year+month/12
void GHCN::ComputeGlobalAverageMonthlyAnomaliesFromGridCellMonthlyAnomalies(void)
{
  
    std::vector<std::map<std::size_t,std::vector<double> > >::iterator iCell;
    std::map<std::size_t, std::vector<double> >::iterator iYear;

    // For each year+month/12, compute total number of grid cell temperature values.
    std::map<int, float> gridXYearFract1000Map;
    
    // Iterate over grid-cells
    int icc=1;
    for(iCell=mGridCellAverageMonthlyAnomalies.begin();
        iCell!=mGridCellAverageMonthlyAnomalies.end(); ++iCell,++icc)
    {
        std::cerr<<__FUNCTION__<<"(): Processing grid cell "
                 <<icc<<" of "<<mGridCellAverageMonthlyAnomalies.size()<<"     \r";
        // Iterate over years
        for(iYear=iCell->begin(); iYear!=iCell->end(); ++iYear)
        {
            int imm=0;
            // iterate over months
            for(imm=0; imm<12; imm++)
            {
                // Limit floating point precision to .XXX
                ComputeGridCellYearFractSums(iCell,gridXYearFract1000Map);
            }
        }
    }

    // Have the grid cell sums for every year+month/12 -- now divide by the count to
    // get the averages for each separate year+month/12 entry.
    std::map<int, double >::iterator iYearFract1000;
    for(iYearFract1000=mGlobalAverageYearFractAnomaliesMap.begin();
        iYearFract1000!=mGlobalAverageYearFractAnomaliesMap.end(); ++iYearFract1000)
    {
        if(gridXYearFract1000Map[iYearFract1000->first]>1)
        {
            mGlobalAverageYearFractAnomaliesMap[iYearFract1000->first] /=
                gridXYearFract1000Map[iYearFract1000->first];
        }
    }


    return;
}
#endif

// 2025/08/23 new version
void GHCN::ComputeGlobalAverageAnnualAnomaliesFromGridCellMonthlyAnomalies(void)
{
    std::vector<std::map<std::size_t,std::vector<double> > >::iterator iCell;
    std::map<std::size_t, std::vector<double> >::iterator iYear;

    // For each year, total number of grid cell x month temperature values.
    std::map<std::size_t, float> annualGridXMonthMap;
  
    // Iterate over grid-cells
    for(iCell=mGridCellAverageMonthlyAnomalies.begin();
        iCell!=mGridCellAverageMonthlyAnomalies.end(); ++iCell)
    {
        if(bPartialYear_g)
        {
            // Command-line args specify months 1->12,
            // but internal processing uses month indices 0-11, so subtract
            // 1 from the start/end month args here.
            ComputeGridCellPartialAnnualSums(iCell,nStartMonth_g-1,nEndMonth_g-1,
                                             annualGridXMonthMap);
        }
        else
        {
            ComputeGridCellFullAnnualSums(iCell,annualGridXMonthMap);
            
        }
    }

    // Have the sums -- now divide by the gridxmonth count to
    // get the averages for each separate year.
    std::map<std::size_t, double >::iterator iYear2;
    for(iYear2=mGlobalAverageAnnualAnomaliesMap.begin();
        iYear2!=mGlobalAverageAnnualAnomaliesMap.end(); ++iYear2)
    {
        if(annualGridXMonthMap[iYear2->first]>1)
        {
            mGlobalAverageAnnualAnomaliesMap[iYear2->first] /=
                annualGridXMonthMap[iYear2->first];
        }
    }
          
    return;

}

// 2025/10/15 1600MDT.
void GHCN::ComputeGridCellYearFractSums(std::vector<std::map<std::size_t,
                                        std::vector<double> > >::iterator & iCell,
                                        std::map<int, float>& gridXYearFract1000Map)

{

#if(1) // 2025/10/15
    std::map<std::size_t, std::vector<double> >::iterator iYear;

    // Iterate over years for grid-cell iCell
    for(iYear=iCell->begin(); iYear!=iCell->end(); ++iYear)
    {
        int imm=0;
        // iterate over months
        for(imm=0; imm<12; imm++)
        {
            // Limit floating pt fractional year to 3 digits precision
            // to eliminate problems with floating pt precision rounding.
            int nFract1000=std::round(1000*imm/12);
            int nYearFract1000=iYear->first*1000+nFract1000;
            if(iYear->second[imm]>GHCN_NOTEMP999+ERR_EPS)
            {
                if(mGlobalAverageYearFractAnomaliesMap.find(nYearFract1000) ==
                   mGlobalAverageYearFractAnomaliesMap.end())
                {
                    mGlobalAverageYearFractAnomaliesMap[nYearFract1000]=iYear->second[imm];
                    gridXYearFract1000Map[nYearFract1000]=1;
                }
                else
                {
                    gridXYearFract1000Map[nYearFract1000]+=1;
                    mGlobalAverageYearFractAnomaliesMap[nYearFract1000]+=iYear->second[imm];
                }
            }
        }
    }
#endif
    return;
}

void GHCN::ComputeGridCellFullAnnualSums(std::vector<std::map<std::size_t,
                                         std::vector<double> > >::iterator & iCell,
                                         std::map<std::size_t, float>& annualGridXMonthMap)

{
    std::map<std::size_t, std::vector<double> >::iterator iYear;

    // Iterate over years for grid-cell iCell
    for(iYear=iCell->begin(); iYear!=iCell->end(); ++iYear)
    {
        int imm=0;
        // iterate over months
        for(imm=0; imm<12; imm++)
        {
            if(iYear->second[imm]>GHCN_NOTEMP999+ERR_EPS)
            {
                if(mGlobalAverageAnnualAnomaliesMap.find(iYear->first) ==
                   mGlobalAverageAnnualAnomaliesMap.end())
                {
                    mGlobalAverageAnnualAnomaliesMap[iYear->first]=iYear->second[imm];
                    annualGridXMonthMap[iYear->first]=1;
                }
                else
                {
                    mGlobalAverageAnnualAnomaliesMap[iYear->first]+=iYear->second[imm];
                    annualGridXMonthMap[iYear->first]+=1;
                }
            }
        }
    }

    return;
}

void GHCN::ComputeGridCellPartialAnnualSums
(std::vector<std::map<std::size_t,std::vector<double> > >::iterator & iCell,
 const int& nMinMonthIn0, const int& nMaxMonthIn0,
 std::map<std::size_t, float>& annualGridXMonthMap)
{
    int nMinMonth0=nMinMonthIn0;
    int nMaxMonth0=nMaxMonthIn0;
    
    if(nMinMonth0<0)
        nMinMonth0=0;
    else if(nMinMonth0>11)
        nMinMonth0=11;

    if(nMaxMonth0<0)
        nMaxMonth0=0;
    else if(nMaxMonth0>11)
        nMaxMonth0=11;

    std::map<std::size_t, std::vector<double> >::iterator iYear;
    std::map<std::size_t, std::vector<double> >::iterator iYearPrev;

    // Month range spans a single year
    if(nMaxMonth0>=nMinMonth0) 
    {
        // Iterate over years
        for(iYear=iCell->begin(); iYear!=iCell->end(); ++iYear)
        {
            int imm=0;
            // iterate over months
            for(imm=nMinMonth0; imm<=nMaxMonth0; ++imm)
            {
                if(iYear->second[imm]>GHCN_NOTEMP999+ERR_EPS)
                {
                    if(mGlobalAverageAnnualAnomaliesMap.find(iYear->first) ==
                       mGlobalAverageAnnualAnomaliesMap.end())
                    {
                        mGlobalAverageAnnualAnomaliesMap[iYear->first]=iYear->second[imm];
                        annualGridXMonthMap[iYear->first]=1;
                    }
                    else
                    {
                        mGlobalAverageAnnualAnomaliesMap[iYear->first]+=iYear->second[imm];
                        annualGridXMonthMap[iYear->first]+=1;
                    }
                }
            }
        }
    }
    else
    {
        // nMinMonth0>nMaxMonth0
        // Month range spans 2 years (crosses December->January divide)
        // Iterate over years
        int nLoopCount=0;
        for(iYear=iCell->begin(); iYear!=iCell->end(); ++iYear)
        {
            // std::prev returns questionable results for the first iYear iterator,
            // so skip over the first iteration here.
            if(0==nLoopCount)
            {
                nLoopCount++;
                continue;
            }
            
            // Need the previous iYear iterator element here
            iYearPrev=std::prev(iYear);
            int nPrevYear=iYearPrev->first;
            int nCurrentYear=iYear->first;

            // Must have 2 consecutive years of data for this to work.
            // If there's a break (i.e. missing year), then do nothing
            // & skip to the next iYear iterator element.
            if((nCurrentYear-nPrevYear)==1)
            {
                // iterate over "previous year" months
                for(int imm=nMinMonth0; imm<12; ++imm)
                {
                    if(iYearPrev->second[imm]>GHCN_NOTEMP999+ERR_EPS)
                    {
                        if(mGlobalAverageAnnualAnomaliesMap.find(iYear->first) ==
                           mGlobalAverageAnnualAnomaliesMap.end())
                        {
                            // Have data for the current year and the previous year.
                            // So can use nMaxMonth0-->11 monthly entries from the
                            // previous year.  Credit "previous year" months to the
                            // current year.
                            mGlobalAverageAnnualAnomaliesMap[iYear->first]=iYearPrev->second[imm];
                            annualGridXMonthMap[iYear->first]=1;
                        }
                        else
                        {
                            // Credit "previous year" months to the current year.
                            mGlobalAverageAnnualAnomaliesMap[iYear->first]+=iYearPrev->second[imm];
                            annualGridXMonthMap[iYear->first]+=1;
                        }
                    }
                    
                }
                // Remainder of months in this years average are from the current year,
                // so the accounting is simpler.
                for(int imm=0; imm<nMaxMonth0+1; ++imm)
                {
                    if(iYear->second[imm]>GHCN_NOTEMP999+ERR_EPS)
                    {
                        if(mGlobalAverageAnnualAnomaliesMap.find(iYear->first) ==
                           mGlobalAverageAnnualAnomaliesMap.end())
                        {
                                // Have data for the current year and the previous year.
                                // So can use nMaxMonth0-->11 monthly entries from the
                                // previous year.
                            mGlobalAverageAnnualAnomaliesMap[iYear->first]=iYear->second[imm];
                            annualGridXMonthMap[iYear->first]=1;
                        }
                        else
                        {
                            mGlobalAverageAnnualAnomaliesMap[iYear->first]+=iYear->second[imm];
                            annualGridXMonthMap[iYear->first]+=1;
                        }
                    }
                }
            }
        }
    }
    
    return;
}

void GHCN::ComputeAnnualMovingAvg(void)
{
    std::map<std::size_t, double >::iterator iyy_init;
    std::map<std::size_t, double >::iterator iyy_center;
    std::map<std::size_t, double >::iterator iyy_leading;
    std::map<std::size_t, double >::iterator iyy_trailing;

    int filt_halfwidth;
  
    double year_avg;
    int icount;
    int nel;
  
    nel=mIavgNyear;
  
    if(nel%2==0)
    {
        // make sure nel is odd
        std::cerr << std::endl;
        std::cerr << "Moving average filter length incremented to " << nel+1 << std::endl;
        std::cerr << "Moving average filter length must be odd. " << std::endl;
        std::cerr << std::endl;
    
        nel+=1;
    }
    filt_halfwidth = nel/2;
  
    mSmoothedGlobalAverageAnnualAnomaliesMap.clear();
  
    if(nel==1)
    {
        // No smoothing -- Just do a straight copy and return.
        for(iyy_init=mGlobalAverageAnnualAnomaliesMap.begin(); 
            iyy_init!=mGlobalAverageAnnualAnomaliesMap.end(); ++iyy_init)
        {
            mSmoothedGlobalAverageAnnualAnomaliesMap[iyy_init->first]=iyy_init->second;
        }
        return;
    }

    icount=0;

    iyy_trailing=mGlobalAverageAnnualAnomaliesMap.begin();
    iyy_init=iyy_trailing;

    // This is ugly -- but it's the only way that I could
    // get the iterator initialized to the moving-average
    // window center.
    iyy_center=mGlobalAverageAnnualAnomaliesMap.begin();
    for(icount=0; icount<filt_halfwidth; icount++)
    {
        iyy_center++;
    }

    // Initialize the moving-average window sum...
    year_avg=0;
    iyy_leading=mGlobalAverageAnnualAnomaliesMap.begin();
    for(icount=0; icount<nel; icount++)
    {
        year_avg+=iyy_leading->second/nel;
        iyy_leading++;
    }
    mSmoothedGlobalAverageAnnualAnomaliesMap[iyy_center->first] = year_avg;

    iyy_center++;

    // Got the moving average started up -- walk through the remaining
    // data, adding the leading value and subtracting the trailing
    // value for each new moving-average output.
    while(iyy_leading != mGlobalAverageAnnualAnomaliesMap.end())
    {
        year_avg += 1.*(iyy_leading->second-iyy_trailing->second)/nel;
        mSmoothedGlobalAverageAnnualAnomaliesMap[iyy_center->first] = year_avg;

        iyy_center++;
        iyy_leading++;
        iyy_trailing++;
    }

    return;
  
}

// Note: mSmoothedGlobalAverageYearFractAnomaliesMap and mGlobalAverageYearFractAnomaliesMap
// are indexed by year*1000+std::round(imonth/12.*1000) to give predictable integer key values.
// To represent the proper year/month date in output plots, must divide the map key values by 1000.
void GHCN::ComputeMonthlyMovingAvg(void)
{
    std::map<int, double >::iterator iyy_init;
    std::map<int, double >::iterator iyy_center;
    std::map<int, double >::iterator iyy_leading;
    std::map<int, double >::iterator iyy_trailing;

    int filt_halfwidth;
  
    double month_avg;
    int icount;
    int nel;
  
    nel=mIavgNyear;
  
    if(nel%2==0)
    {
        // make sure nel is odd
        std::cerr << std::endl;
        std::cerr << "Moving average filter length incremented to " << nel+1 << std::endl;
        std::cerr << "Moving average filter length must be odd. " << std::endl;
        std::cerr << std::endl;
    
        nel+=1;
    }
    filt_halfwidth = nel/2;
  
    mSmoothedGlobalAverageYearFractAnomaliesMap.clear();
  
    if(nel==1)
    {
        // No smoothing -- Just do a straight copy and return.
        for(iyy_init=mGlobalAverageYearFractAnomaliesMap.begin(); 
            iyy_init!=mGlobalAverageYearFractAnomaliesMap.end(); ++iyy_init)
        {
            mSmoothedGlobalAverageYearFractAnomaliesMap[iyy_init->first]=iyy_init->second;
        }
        return;
    }

    icount=0;

    iyy_trailing=mGlobalAverageYearFractAnomaliesMap.begin();
    iyy_init=iyy_trailing;

    // This is ugly -- but it's the only way that I could
    // get the iterator initialized to the moving-average
    // window center.
    iyy_center=mGlobalAverageYearFractAnomaliesMap.begin();
    for(icount=0; icount<filt_halfwidth; icount++)
    {
        iyy_center++;
    }

    // Initialize the moving-average window sum...
    month_avg=0;
    iyy_leading=mGlobalAverageYearFractAnomaliesMap.begin();
    for(icount=0; icount<nel; icount++)
    {
        month_avg+=iyy_leading->second/nel;
        iyy_leading++;
    }
    mSmoothedGlobalAverageYearFractAnomaliesMap[iyy_center->first] = month_avg;

    iyy_center++;

    // Got the moving average started up -- walk through the remaining
    // data, adding the leading value and subtracting the trailing
    // value for each new moving-average output.
    while(iyy_leading != mGlobalAverageYearFractAnomaliesMap.end())
    {
        month_avg += 1.*(iyy_leading->second-iyy_trailing->second)/nel;
        mSmoothedGlobalAverageYearFractAnomaliesMap[iyy_center->first] = month_avg;

        iyy_center++;
        iyy_leading++;
        iyy_trailing++;
    }

    return;
  
}

void GHCN::AccumulateGlobalAverageAnnualBootstrapRuns(void)
{
    mSmoothedGlobalAnnualAnomalyEnsembleRunMapVector.push_back(mSmoothedGlobalAverageAnnualAnomaliesMap);
}

void GHCN::AccumulateGlobalAverageAnnualBootstrapStationCounts(void)
{
    mAverageAnnualBootstrapStationCountMapVector.push_back(mAverageAnnualStationCountMap);
}

#if(0)

void GHCN::AccumulateGlobalAverageAnnualBootstrapStatistics(void)
{

    //     std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesMap
    //     std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap;
    //     std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap;

    for(auto& imm:mSmoothedGlobalAverageAnnualAnomaliesMap)
    {
        if(mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap.find(imm.first) ==
           mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap.end())
        {
            mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[imm.first]=imm.second;
            mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap[imm.first]=imm.second*imm.second;
            mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[imm.first]=1;
        }
        else
        {
            mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[imm.first]+=imm.second;
            mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap[imm.first]+=(imm.second*imm.second);
            mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[imm.first]+=1;
        }
        
    }
}
#endif

void GHCN::ComputeGlobalAverageAnnualBootstrapStatistics(void)
{

    // First, compute ensemble means for each year.
    for(auto& iv:mSmoothedGlobalAnnualAnomalyEnsembleRunMapVector)
    {
        // for(std::size_t iy=MIN_GISS_YEAR; iy<=MAX_GISS_YEAR; ++iy)
        for(auto& iy:iv)
        {
            if(iv.find(iy.first)!=iv.end())
            {
                if(mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap.find(iy.first)
                   == mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap.end())
                {
                    mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[iy.first]=iv[iy.first];
                    mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[iy.first]=1;
                }
                else
                {
                    mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[iy.first]+=iv[iy.first];
                    mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[iy.first]+=1;
                }
            }
        }
    }

    // Now normalize the ensemble mean sums for each year by the number of ensemble runs
    // with data for that year.
    int igiss_yr=MIN_GISS_YEAR;
    for(auto & imy: mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap)
    {
        imy.second/=(1.*mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[imy.first]);
        ++igiss_yr;
    }
    
    // Have estimated means -- now compute variance / StdDev estimates.
    for(auto& iv:mSmoothedGlobalAnnualAnomalyEnsembleRunMapVector)
    {
        // for(std::size_t iy=MIN_GISS_YEAR; iy<=MAX_GISS_YEAR; ++iy)
        for(auto & iy:iv)
        {
            if(iv.find(iy.first)!=iv.end())
            {
                if(mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap.find(iy.first)
                   == mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap.end())
                {
                    float fMean=mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[iy.first];
                    mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap[iy.first]
                        =(iv[iy.first]-fMean)*(iv[iy.first]-fMean);
                }
                else
                {
                    float fMean=mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[iy.first];
                    mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap[iy.first]
                        +=(iv[iy.first]-fMean)*(iv[iy.first]-fMean);
                }
            }
        }
    }

    // Now normalize variance sums for each year by the number of ensemble runs with data for that year.
    igiss_yr=MIN_GISS_YEAR;
    for(auto & imy: mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap)
    {
        int nRunCount=mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[imy.first];
        // Subtract 1 from the run count number to give us unbiased variance estimates,
        // but only if nRunCount>1
        if(nRunCount>1)
            nRunCount-=1;
        
        imy.second/=(1.*nRunCount);
        ++igiss_yr;
    }

    // sqrt() everything to give std devs
    for(auto & imy: mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap)
    {
        imy.second=::sqrt(imy.second);
    }

    std::cerr<<"All done with bootstrap statistics computations"<<std::endl;

}

void GHCN::ComputeGlobalAverageAnnualBootstrapStationCountMeans(void)
{

    // First, compute ensemble means for each year.
    for(auto& iv:mAverageAnnualBootstrapStationCountMapVector)
    {
        // for(std::size_t iy=MIN_GISS_YEAR; iy<=MAX_GISS_YEAR; ++iy)
        for(auto& iy:iv)
        {
            if(iv.find(iy.first)!=iv.end())
            {
                if(mAverageAnnualBootstrapStationCountMeanMap.find(iy.first)
                   == mAverageAnnualBootstrapStationCountMeanMap.end())
                {
                    mAverageAnnualBootstrapStationCountMeanMap[iy.first]=iv[iy.first];
                }
                else
                {
                    mAverageAnnualBootstrapStationCountMeanMap[iy.first]+=iv[iy.first];
                }
            }
        }
    }

    // Now normalize the ensemble mean sums for each year by the number of ensemble runs
    // with data for that year.
    int igiss_yr=MIN_GISS_YEAR;
    for(auto & imy: mAverageAnnualBootstrapStationCountMeanMap)
    {
        imy.second/=(1.*mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap[imy.first]);
        ++igiss_yr;
    }

    std::cerr<<"All done with bootstrap station count average computations"<<std::endl;

}

void GHCN::AccumulateGlobalAverageMonthlyBootstrapStatistics(void)
{
    // TODO (or not)
}

int GHCN::ComputeAvgStationLatAlt(void)
{
    //   std::map<std::size_t, std::map<std::size_t, std::vector<float> > > mTempsMap;
    //   mTempsMap[station_id][year][month]

    std::map<int,int > mapYearNStations;
    std::map<int,float > mapYearFStations;

    std::map<int,std::pair<float,float> >  mapYearAvgLatAlt;

    std::vector<std::size_t> vYears0;
    for(int iy=MIN_GISS_YEAR; iy<=MAX_GISS_YEAR; ++iy)
    {
        // Iterate through stations
        float fAvgLat=0;
        float fAvgAlt=0;
        int nStationCount=0;
        int nStationMonthCount=0;
        for(auto& im:mTempsMap)
        {
            if(im.second.find(iy)!=im.second.end())
            {
                if(m_ProcessedWmoLatLongMap.find(im.first)!=m_ProcessedWmoLatLongMap.end())
                {
                    // If we find a year entry (iy) for station im, and that station is listed
                    // in the w_CompleteWmoLatLongMap metadata map, add station ID im.first to the list
                    // Compute average abs latitude so this works for both hemispheres.
                    // Want avg lat dist from the equator no matter which hemisphere we process.
                    if(m_ProcessedWmoLatLongMap[im.first].lat_deg-90.>0)
                    {
                        for(int ivm=0; ivm<12; ivm++)
                        {
                            if(im.second[iy].at(ivm)>GHCN_NOTEMP9999+ERR_EPS)
                            {
                                fAvgLat+=std::abs(m_ProcessedWmoLatLongMap[im.first].lat_deg-90.);
                                fAvgAlt+=m_ProcessedWmoLatLongMap[im.first].altitude;
                                nStationMonthCount+=1;
                            }
                            nStationCount+=1;
                        }
                    }
                }
            }
        }
        
        if(nStationCount>0)
        {
            fAvgLat/=(1.*nStationMonthCount);
            fAvgAlt/=(1.*nStationMonthCount);
            m_mapYearStationAvgLatAlt[iy] = std::pair<float,float>(fAvgLat,fAvgAlt);
            // Save off all the station ids that have data for year iy
            mapYearFStations[iy]=nStationMonthCount/12.;
        }
    }

    m_mapYearStationCountAvgLatAlt=mapYearNStations;

    return 0;
    }

int GHCN::ReadMannItrdbList(void)
{
    char cBuf[BUFLEN+1];
    int iCount=0;
  
    m_pMannItrdbListFstream = new std::fstream(m_MannItrdbListFileName.c_str(),std::fstream::in);
  
    m_pMannItrdbListFstream->exceptions(/* std::fstream::badbit 
                                           | */std::fstream::failbit 
                                           | std::fstream::eofbit);

    m_MannItrdbListSet.clear();
    int iLine=0;
    std::string strDelim=std::string(",");
    while(!m_pMannItrdbListFstream->eof())
    {
        iLine++;
        ::memset(cBuf,'\0', BUFLEN+1);
        try
        {
            m_pMannItrdbListFstream->getline(cBuf,BUFLEN);
        }
        catch(...)
        {
            if(m_pMannItrdbListFstream->eof()==true)
            {
                std::cerr<<"Reached end of file -- all done..."<<std::endl;
//          break;
            }
            else if(m_pMannItrdbListFstream->fail()==true)
            {
                std::cerr<<"Line "<<iLine<<" is longer than the cBuf buffer -- carry on..."<<std::endl;
                // Didn't find the delimiter (line longer than BUFLEN)
                // That's ok -- carry on
                m_pMannItrdbListFstream->clear(); // Have to clear the error flag..
            }
        }

        int iTok;
        char* pTok;
        
        pTok=::strtok(cBuf,strDelim.c_str());
        if(NULL==pTok)
        {
            continue;
        }
        if(std::string(pTok)==std::string("ITRDB extraction"))
        {
            for(iTok=1; iTok<4; iTok++)
            {
                //ITRDB extraction,,tree-width,9000,ar052,
                pTok=::strtok(NULL,strDelim.c_str());
            }
            // 
            // pTok should now point at the chronology name/id (file name minus suffix)
            // Append the suffix so that we can do direct file-name comparisons
            // for tree-ring chronology filtering/selection.
            m_MannItrdbListSet.insert(std::string(pTok)+std::string(".crn"));
            iCount++;
        }
    }

    m_pMannItrdbListFstream->close();
    delete m_pMannItrdbListFstream;
    m_pMannItrdbListFstream=NULL;
      
    return iCount;
}

// Metadata embedded in Itrdb data files; will
// collect metadata as we read in chronology data.
int GHCN::ReadItrdbData(void)
{
    // No file open here -- need to walk a directory tree
    // man ftw for details

    // Want to parse, save metadata in
    // std::map<std::size_t, GHCNLatLong> m_CompleteWmoLatLongMap;
    // do not need std::set<std::size_t> stationIdSet


    int ftwStatus;
  
    mItrdbMap.clear();
    mItrdbCountMap.clear();

    // Here, mInputFileName is the name of the top level of
    // the directory tree to walk through...
    ftwStatus=ftw(mInputFileName.c_str(), ExternItrdbFtwFcn, 20);
  
    return ftwStatus;
  
}

int GHCN::ReadItrdbDataFile(const char* itrdbFile,
                            const std::set<std::string>& itrdbFileSet)
{
    int nStatus=0;
  
    if(itrdbFileSet.find(itrdbFile)!=itrdbFileSet.end())
    {
        // itrdbFile is in the list of files to read -- read it.
        nStatus=ReadItrdbDataFile(itrdbFile);
    }
  
    return nStatus;
}

int GHCN::ReadItrdbDataFile(const char* itrdbFile)
{
    // Open Itrdbfile
    // Parse out data and metadata
    // Close Itrdb file
    //const char* cItrdbMetaDataDelim="=";
    //const char* cItrdbDataDelim=" ";
  
    mInputFileName=std::string(itrdbFile);
    
    OpenItrdbFile();
    
    GHCNLatLong latLong;
 
    std::string strItrdbId;
    char cItrdbId[8];

    int iLine=0;
    // std::map<std::string, std::map<std::size_t, float> > mItrdbMap;
  
    int iYearOffset=0;
  
    while(!mInputFstream->eof())
    {
        try
        {
            ::memset(mCbuf,'\0',BUFLEN);
            mInputFstream->clear(); // Clear out error flags
            mInputFstream->getline(mCbuf,BUFLEN-1); // specify ^M (0xd, decimal 13) delimiter here
        }
        catch(...)
        {
            if(mInputFstream->fail()==true)
            {
                // Line longer than buffer -- no problem
                std::cerr<<"Long line found -- carry on..."<<std::endl;
            }
        }
      
        if(0==iLine)
        {
            latLong.strStationName=std::string(&mCbuf[9]);
        }
        else if(1==iLine)
        {
            // Lat/long information here.
            int latDeg,latMin;
            int lonDeg,lonMin;
            int firstYear,lastYear;
            if(mCbuf[46]=='-')
            {
                ::sscanf(&mCbuf[46], "%3d%2d",
                         &latDeg,&latMin);
            }
            else
            {
                ::sscanf(&mCbuf[47], "%2d%2d",
                         &latDeg,&latMin);
            }
          
            if(mCbuf[66]=='-')
            {
                ::sscanf(&mCbuf[51], "%4d%2d",
                         &lonDeg,&lonMin);
            }
            else
            {
                ::sscanf(&mCbuf[52], "%3d%2d",
                         &lonDeg,&lonMin);
            }
          
            ::sscanf(&mCbuf[66],"%d %d",&firstYear,&lastYear);

            latLong.lat_deg=latDeg+1.*latMin/60.+90;
            latLong.long_deg=lonDeg+1*lonMin/60.+180;
            latLong.first_year=firstYear;
            latLong.last_year=lastYear;
          
            iYearOffset=latLong.first_year%10;
        }
        else if(iLine>=3) // Data begins on line 3 (re:0)
        {
            //  Station ID starts at col 0 0-5
            // (Decade) Year starts at col 6 (re:0)
            //  Samples begin at cols:
            //         10  17  24  31  38  45  52  59  66  73  
            int iYearDec;

            ::sscanf(&mCbuf[0],"%6s%4d",&cItrdbId[0],&iYearDec);
            strItrdbId=std::string(cItrdbId);
            int iiY;
          
            for(iiY=iYearOffset; (iiY<10) && (iYearDec+iiY<latLong.last_year); ++iiY)
            {
                int offset;
                int iVal=9990;
                offset=10+7*iiY;
                ::sscanf(&mCbuf[offset],"%4d",&iVal);
                if(9990!=iVal)
                {
                    mItrdbMap[strItrdbId][iYearDec+iiY]=iVal;
                }
                iYearOffset=0;
            }
          
            if(iiY+iYearDec>=latLong.last_year)
            {
                break;
                // Finished reading in all the chronology data
            }
        }
        iLine++;
    }
    // Save off this chronology's metadata
    mItrdbLatLongMap[strItrdbId]=latLong;
    
    // Want to standardize each chronology to zero mean and unit variance
    // over the period FIRST_ITRDB_BASELINE_YEAR, LAST_ITRDB_BASELINE_YEAR

    CloseItrdbFile();
  
    return 0;
}

void GHCN::NormalizeItrdbData(void)
{
    std::map<std::string, double> meanMap;
    std::map<std::string, double> varMap;
  
    std::map<std::string,std::map<std::size_t,float> >::iterator iItrdb;
    std::map<std::size_t,float>::iterator iYear;
  
    int nYear=0;
    double mean=0;
    double var=0;
    double sdev=0;

    // Iterate over ITRDB chronologies
    for(iItrdb=mItrdbMap.begin(); iItrdb!=mItrdbMap.end(); iItrdb++)
    {
        mean=0;
        var=0;
        nYear=0;
        // Iterate over year
        for(iYear=iItrdb->second.begin(); iYear!=iItrdb->second.end(); iYear++)
        {
            if((iYear->first>=FIRST_ITRDB_BASELINE_YEAR)&&(iYear->first<=LAST_ITRDB_BASELINE_YEAR))
            {
                mean += iYear->second;
                var += iYear->second*iYear->second;
                nYear+=1;
            }
        }

        var /= (1.*nYear);
        mean /= (1.*nYear);

        var = var-mean*mean;

        if(var<=0)
        {
            // Bad data -- eliminate this chronology from the map
            //             C++11 standard: .erase() returns the next iterator element 
            // iItrdb=mItrdbMap.erase(iItrdb);
            continue;
        }
    
        sdev=sqrt(var);
    
        // Now normalize this chronology re: mean, var computed for the baseline region
        for(iYear=iItrdb->second.begin();  iYear!=iItrdb->second.end(); iYear++)
        {
            iYear->second -= mean;
            if(sdev>0)
            {
                iYear->second /= sdev;
            }
        }
    
    }
  
    return;
}

std::map<std::size_t,float> GHCN::ComputeItrdbAvg(void) 
{
  
    std::map<std::string,std::map<std::size_t,float> >::iterator iItrdb;
    std::map<std::size_t,float>::iterator iYear;
  
    // Contains average itrdb values for all stations in setItrdb,
    // one average item per year.
    std::map<std::size_t,float> mapfItrdbAvg;
  
    // #stations in each average, for each year
    std::map<std::size_t,int> mapnItrdbAvg;
  
    for(iItrdb=mItrdbMap.begin(); iItrdb!=mItrdbMap.end(); iItrdb++)
    {
        // Station in the set -- include it in the average
        for(iYear=iItrdb->second.begin(); iYear!=iItrdb->second.end(); iItrdb++)
        {
            if(mapfItrdbAvg.find(iYear->first)==mapfItrdbAvg.end())
            {
                mapfItrdbAvg[iYear->first]=iYear->second;
                mapnItrdbAvg[iYear->first]=1;
            }
            else
            {
                mapfItrdbAvg[iYear->first]+=iYear->second;
                mapnItrdbAvg[iYear->first]+=1;
            }
        }
    }

    // Divide by #stations per avg to get the avg values for each year
    std::map<std::size_t,float>::iterator imap;
    for(imap=mapfItrdbAvg.begin(); imap!=mapfItrdbAvg.end(); imap++)
    {
        imap->second /= mapnItrdbAvg[imap->first];
    }
  
    return mapfItrdbAvg;
  
}

std::map<std::size_t,float> GHCN::ComputeItrdbMannListAvg(void)
{
    std::map<std::size_t,float> m_mapAvg;
  
    m_mapAvg=ComputeItrdbAvg(m_MannItrdbListSet);
  
    return m_mapAvg;
}

std::map<std::size_t,float> GHCN::ComputeItrdbAvg(const std::set<std::string>& setItrdb)
{
  
    std::map<std::string,std::map<std::size_t,float> >::iterator iItrdb;
    std::map<std::size_t,float>::iterator iYear;
  
    // Contains average itrdb values for all stations in setItrdb,
    // one average item per year.
    std::map<std::size_t,float> mapfItrdbAvg;
  
    // #stations in each average, for each year
    std::map<std::size_t,int> mapnItrdbAvg;
  
    for(iItrdb=mItrdbMap.begin(); iItrdb!=mItrdbMap.end(); iItrdb++)
    {
        if(setItrdb.find(iItrdb->first) != setItrdb.end())
        {
            // Station in the set -- include it in the average
            for(iYear=iItrdb->second.begin(); iYear!=iItrdb->second.end(); iItrdb++)
            {
                if(mapfItrdbAvg.find(iYear->first)==mapfItrdbAvg.end())
                {
                    mapfItrdbAvg[iYear->first]=iYear->second;
                    mapnItrdbAvg[iYear->first]=1;
                }
                else
                {
                    mapfItrdbAvg[iYear->first]+=iYear->second;
                    mapnItrdbAvg[iYear->first]+=1;
                }
            }
        }
    }

    // Divide by #stations per avg to get the avg values for each year
    std::map<std::size_t,float>::iterator imap;
    for(imap=mapfItrdbAvg.begin(); imap!=mapfItrdbAvg.end(); imap++)
    {
        imap->second /= mapnItrdbAvg[imap->first];
    }
  
    return mapfItrdbAvg;
  
}

void GHCN::AverageItrdbGridCells(void)
{
  
    std::vector<std::map<std::size_t,float> >::iterator ivm;
    std::map<std::size_t,float>::iterator imm;
    std::map<std::size_t,int> mapNCells;
  
    // Iterate over grid-cells
    for(ivm=m_vmapItrdbGridCellAvg.begin(); ivm!=m_vmapItrdbGridCellAvg.end(); ivm++)
    {
        // Iterate over all years of results in a grid-cell
        for(imm=ivm->begin(); imm!=ivm->end(); imm++)
        {
            if(m_mapItrdbAvg.find(imm->first)==m_mapItrdbAvg.end())
            {
                m_mapItrdbAvg[imm->first]=imm->second;
                mapNCells[imm->first]=1;
            }
            else
            {
                m_mapItrdbAvg[imm->first]+=imm->second;
                mapNCells[imm->first]+=1;
            }
        }
    }
  
    // Divide the sums in m_mapItrdbAvg by the #items in each avg
    // to get the avg vals

    for(imm=m_mapItrdbAvg.begin(); imm!=m_mapItrdbAvg.end(); imm++)
    {
        imm->second /= mapNCells[imm->first];
    }
  

    return;
}



// May not need this
void GHCN::ComputeItrdbBaselines(void)
{

    int yykey;
  
    std::map<std::string, std::map<std::size_t, float> >::iterator iss;

  
    mBaselineItrdbSampleCountMap.clear();
    mBaselineItrdbMap.clear();
  
    // Iterate through all the stations in the temperature map.
    for(iss=mItrdbMap.begin(); iss!=mItrdbMap.end(); iss++)
    {
        // Loop through the years in the temperature baseline period.
        for(yykey=FIRST_ITRDB_BASELINE_YEAR; yykey<=LAST_ITRDB_BASELINE_YEAR; yykey++)
        {
            // Do we have an entry for this particular year?
            if(iss->second.find(yykey)!=iss->second.end())
            {
                if(iss->second[yykey] < 9990.)  // 9990 marks an invalid chronology value
                {
                    bool first_count=false;

                    // Probably overkill here -- but I don't want to assume that
                    // all new map entries are initialized to 0.
                    // Check to see if there's a station entery in the 
                    // baseline sample count map.
                    if(mBaselineItrdbSampleCountMap.find(iss->first) 
                       == mBaselineItrdbSampleCountMap.end())
                    {
                        first_count=true;
                    }

                    // No station/month entry in this map yet -- so create one
                    // and make sure that the baseline sample-count value is initialized to 0.
                    if(first_count==true)
                    {
                        mBaselineItrdbSampleCountMap[iss->first]=0;
                    }

                    // First valid temperature for this station and month?
                    // Then initialize the baseline itrdb map to the
                    // current itrdb value.
                    if(mBaselineItrdbSampleCountMap[iss->first]<1)
                    {
                        mBaselineItrdbMap[iss->first]=iss->second[yykey];
                    }
                    else
                    {
                        // Already have an itrdb data sum sum going -- update it.
                        mBaselineItrdbMap[iss->first]+=iss->second[yykey];
                    }
                    // Increment the baseline sample count for this station and month.
                    mBaselineItrdbSampleCountMap[iss->first]+=1;
                }
            }
        }
    }

    // Divide each baseline itrdb sum by the number of valid samples 
    // found in the baseline time-period for each itrdb location id to
    // get the baseline average itrdb value for the corresponding itrdb 
    // id
  
    for(iss=mItrdbMap.begin(); iss!=mItrdbMap.end(); iss++)
    {
        if(mBaselineItrdbSampleCountMap[iss->first]>1)
        {
            mBaselineItrdbMap[iss->first] /= mBaselineItrdbSampleCountMap[iss->first];
        }
    }

    return;
  
}

// Callback fcn passed into GHCN method via ptr -- cannot be
// non-static class member.
int ExternCru4FtwFcn(const char* fPath, const struct stat* fb, int typeFlag)
{
    // A ptr to this fcn will be passed into ftw():
    //     
    // int ftw(const char *dirpath,
    //         int (*fn) (const char *fpath, const struct stat *sb, int typeflag), 
    //         int  nopenfd);
    //
    // 1) dirpath is the top-level of the CRUTEM4 directory tree.
    //
    // 2) fn is a ptr to the fcn that reads/parses each temperature file
    //    in the directory tree.
    //         A) fPath is the name of the file/dir currently being processed
    //         B) fb contains inode info/etc -- won't need any of this.
    //         C) typeFlag will be FTD_F if it is a file.
    //            if FTD_F, process the temperature file and return 0; 
    //            otherwise do nothing and return 0.
    //         A return of 0 means keep walking the directory tree.
    //
    // 3) nopen is the max #dirs/files to keep open.  Set to something like
    //    20 or so (optimum val not known at this pt).
    //

    if(NULL==pGhcn_g)
        return -1;
  
    if(FTW_F==typeFlag)
    {
        pGhcn_g->ReadCru4TempFile(fPath);
    }
    else
    {
//    std::cerr<<"Taversing directory"<<fPath<<std::endl;
    }
    
    return 0;

};


// Callback fcn passed into GHCN method via ptr -- cannot be
// non-static class member.
int ExternUshcnFtwFcn(const char* fPath, const struct stat* fb, int typeFlag)
{
    // A ptr to this fcn will be passed into ftw():
    //     
    // int ftw(const char *dirpath,
    //         int (*fn) (const char *fpath, const struct stat *sb, int typeflag), 
    //         int  nopenfd);
    //
    // 1) dirpath is the top-level of the CRUTEM4 directory tree.
    //
    // 2) fn is a ptr to the fcn that reads/parses each temperature file
    //    in the directory tree.
    //         A) fPath is the name of the file/dir currently being processed
    //         B) fb contains inode info/etc -- won't need any of this.
    //         C) typeFlag will be FTD_F if it is a file.
    //            if FTD_F, process the temperature file and return 0; 
    //            otherwise do nothing and return 0.
    //         A return of 0 means keep walking the directory tree.
    //
    // 3) nopen is the max #dirs/files to keep open.  Set to something like
    //    20 or so (optimum val not known at this pt).
    //

    if(NULL==pGhcn_g)
        return -1;
  
    if(FTW_F==typeFlag)
    {
        pGhcn_g->ReadUshcnTempFile(fPath);
    }
    else
    {
//    std::cerr<<"Taversing directory"<<fPath<<std::endl;
    }
    
    return 0;

};

int ExternItrdbFtwFcn(const char* fPath, const struct stat* fb, int typeFlag)
{
    int nStatus=0;
  
    int ExternCru4FtwFcn(const char* fPath, const struct stat* fb, int typeFlag);
    if(FTW_F==typeFlag)
    {
        pGhcn_g->ReadItrdbDataFile(fPath);
    }
    else
    {
        std::cerr<<"Traversing directory"<<fPath<<std::endl;
    }
  
    return nStatus;
}




#if(0)
int GHCN::UshcnFtwFcn(const char* fPath, const struct stat* fb, int typeFlag)
{
    // A ptr to this fcn will be passed into ftw():
    //     
    // int ftw(const char *dirpath,
    //         int (*fn) (const char *fpath, const struct stat *sb, int typeflag), 
    //         int  nopenfd);
    //
    // 1) dirpath is the top-level of the CRUTEM4 directory tree.
    //
    // 2) fn is a ptr to the fcn that reads/parses each temperature file
    //    in the directory tree.
    //         A) fPath is the name of the file/dir currently being processed
    //         B) fb contains inode info/etc -- won't need any of this.
    //         C) typeFlag will be FTD_F if it is a file.
    //            if FTD_F, process the temperature file and return 0; 
    //            otherwise do nothing and return 0.
    //         A return of 0 means keep walking the directory tree.
    //
    // 3) nopen is the max #dirs/files to keep open.  Set to something like
    //    20 or so (optimum val not known at this pt).
    //

    if(NULL==pGhcn_g)
        return -1;
  
    if(FTW_F==typeFlag)
    {
        pGhcn_g->ReadUshcnTempFile(fPath);
    }
    else
    {
//    std::cerr<<"Taversing directory"<<fPath<<std::endl;
    }
    
    return 0;

};
#endif



