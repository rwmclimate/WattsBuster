/*
  DISCLAIMER: These comments are not up-to-date

  Compiles with g++ and runs on Linux/iMac/Windows-Cygwin platforms.

  To install Cygwin on a Windows machine, go to cygwin.org and follow
  the installation instructions. Be sure to install g++, make, etc.


  How to compile (Unix/Cygwin command-line):

    Just type 'make' (in the directory/folder where all the files are located)


  How to get the raw temperature data:

    Download and uncompress the GHCN v2/v3 temperature files 
              (V2 files are v2.mean.Z, v2.mean_adj.Z, etc.,
              V3 files have longer, more meaningful (and uglier)
              file names)

    You will also need to get the GHCN V2/V3 metadata files v2.inv.Z, etc. 
    (Metatdata files contain station latitude/longitude, etc.).

    The V2 compressed data files used to be available at: 
                     ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/v2/
        
    Google will help if you can't find them.  V3 data files
    can be found at: ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/v3/


    The program can also process HadCRUT3, HadCRUT4, and BEST 
    temperature data -- Google can help you find these data sets.

    To uncompress the GHCN files on the Unix/Cygwin command-line:
         uncompress v2.whatever.Z or whatever...


  How to run:

    To crunch the GHCN V3 monthly-mean temperature data (with all default 
    processing values), do this:

      ./ghcn.exe -G -M v3-metadata-whatever.dat v3.data-whatever.dat -o results.csv


    Multiple temperature files (additional command-line args) can be processed in one run.
    Anomaly outputs will be stored in results.csv in spreadsheet-readable form.
   
    For more information about processing options, run the
    program as follows:

    ./ghcn.exe 2>&1 | more
 



  Some parameters to twiddle with (will require a recompile)...:

  MIN_GISS_YEAR defines the first year of data to extract.
                The standard value is 1880 (NASA/GISS convention)
  MAX_GISS_YEAR -- last year to process -- update each year             

  FIRST_BASELINE_YEAR defines the first year of the baseline period.
  LAST_BASELINE_YEAR defines the last year of the baseline period.
                     The NASA/GISS standard baseline period is 1951-1980

  DEFAULT_MIN_BASELINE_SAMPLE_COUNT defines the default minimum number 
                                   of valid temperature samples in the baseline 
                                   period for a temperature station-id in a particular 
                                   month to be included in the anomaly calculations.  
                                   Valid range is 1-#baseline-years. #baseline years 
                                   is determined by FIRST_BASELINE_YEAR and 
                                   LAST_BASELINE_YEAR above.

                                   Can be overridden by the command-line arg -B

  DEFAULT_AVG_NYEAR defines the default length of the moving-average smoothing 
                   filter.  This is set to 1 year (below).  Can be overridden
                   with the command-line arg -A .



Overall approach.


1) The program reads in temperature data from a GHCN/HadCRUT/BEST
   temperature data file and places the temperature samples in
   the mTempsMap class member.

   mTempsMap is a nested STL map template indexed by WMO base
   id number and year.  Each WMO base-id/year element is a
   12 element vector containing the temperature for each month
   for the given WMO base id and year.

   Temperatures are indexed by: mTempsMap[WMO base-id][year][month]

   The STL map template and iterator function make easy to deal
   with data gaps (not all WMO base-ids have temperature data for
   all years/months).

   Note: In the old GHCN V2 data, multiple temperature time-series can share the same WMO base 
   id number.  This routine averages the data from those multiple time
   series into a single temperature time-series for each WMO base id number.  
   The mTempsMapCount member keeps track of the number of WMO base ids sharing each
   WMO base id for each year/month that data are available.  Data gaps can 
   cause the number of reporting WMO base ids to vary over time.  mTempsMapCount
   is used to compute correct averages for each WMO base id.

   Invalid samples (i.e. missing data) designated by -9999 in the
   GHCN data files are excluded from the averages.  If no valid
   samples for a particular year/month for a WMO base-id are found,
   then the average value for that year/month/base-id is set to
   an invalid value.  It will be excluded from further processing.


2) The baseline temperatures are computed for each WMO base
   id and month, and are then placed in the class member 
   mBaselineTemperatureMap

   The default min/max baseline years are the NASA baseline years
   of 1951 and 1980, respectively.

   mBaselineTemperatureMap is a nested STL map indexed by WMO base-id and month.

   Since there are gaps in the data, there can be variations in the
   number of valid temperature samples in the baseline period for the
   different base-ids/months.

   The class member mBaselineSampleCountMap keeps track of the number
   of valid samples for each WMO base-id/month in the baseline period.
   (mBaselineSampleCountMap is a nested map indexed by WMO-base-id and month.)

   For a temperature WMO base-id to be included in the final average anomaly
   calculations for any given month, the WMO base-id must have 
   a minumum number of valid temperature samples.
   (see DEFAULT_MIN_BASELINE_SAMPLE_COUNT above). 

   
3) After all the baseline temperatures are calculated for each WMO
   base-id/month, temperature anomalies for each base-id/year/month
   are calculated by subtracting the mBaselineTemperatureMap[base-id][month]
   element from the mTempMap elements corresponding to the same base-id/month;
   and averaged over all base-id's to produce global average anomalies
   for each year/month (stored in mGlobalAverageMonthlyAnomaliesMap).

   A simple gridding/averaging procedure is used to calculate the 
   global-average anomalies for each year/month.  The default grid size
   is 20 deg-lat x 20 deg-long (at the Equator).  Longitude dimensions
   are adjusted to maintain *approximately* constant grid-size areas
   as you move away from the Equator in latitude.  The longitude
   dimensions are adjusted so that you have an integer number of grid-cells
   for each 20-deg latitude "row". (This means that the grid-cell areas
   aren't exactly constant, but this is still good enough for "message-board
   science").

   The default 20-deg x 20-deg grid size was chosen to ensure that nearly
   all grid-cells covering land area have at least a few temperature stations.
   (This ensures that the Northern Hemisphere -- where most of the temperature
   stations are -- doesn't get overweighted too much relative to the Southern
   Hemisphere.)  Make the grid size too small, and and the warming estimates
   will be too high (due to overweighting of the NH).

   Grid-cell sizes can be specified on the command-line.

4) For each year, the monthly global-average anomalies are averaged together 
   to generate single annual global average anomaly for that year.  

5) Annual anomalies may be smoothed with a moving-average filter.
   The moving-average filter length can be changed by changing AVG_NYEAR
   and recompiling or by setting the appropriate command-line option.

6) Final results are written out to an output plain-text file in CSV format.

7) Each csv column in the output file is labeled with the corresponding data 
   file name.  The file can be read into Excel/OpenOffice and plotted.
   
   Disclaimer: For experimental/demo uses only -- YMMV

 */
#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <ftw.h>
#include <time.h>
#include <errno.h>
#include <cstddef>
#include <stdio.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <functional>

#include <iostream>
#include<fstream>
#include <filesystem>
#include <algorithm>    // std::min

#include <boost/function.hpp>
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


enum SERVER_PLOT_OP
{
  SERVER_PLOT_INCREMENT,
  SERVER_PLOT_NEW,
  SERVER_PLOT_NONE,
  SERVER_PLOT_RESET,
  SERVER_PLOT_QUIT
};

enum DATA_VERSION 
{
  DATA_GHCN,
  DATA_CRU,
  DATA_BEST,
  DATA_USHCN,
  DATA_GHCN_DAILY
};

// These specify the min record length
// or max record start and min record end
// times of stations to be processed
enum STATION_SELECT_MODE 
{
  SELECT_ALL,
  SELECT_MAX_START_DATE,
  SELECT_MIN_END_DATE,
  SELECT_MAX_START_MIN_END_DATE,
  SELECT_MIN_DURATION,
  SELECT_GREATEST_DURATION,
  SELECT_MAX_START_MIN_END_DATE_MIN_DURATION
};

struct GHCNLatLong
{
  std::size_t wmo_id;
    std::string strWmoId;
  std::string strStationName;
  std::string strStationType; // Urban,Suburban,Rural
  int iStationType; // Added to match browser/client request format 1=rural,2=suburban,3=urban,0=all
  
  int first_year;
  int last_year;
  int num_years;

  // List of contiguous year segments -- each
  // break in continuous coverage will generate
  // a new segment. pair.first=begin_year, pair.second=end_year
  std::vector< std::pair< int, int> > year_seg;
  
  float altitude;
  
  float lat_deg;
  float long_deg;

  int isNearAirport;

  GHCNLatLong()
    {
      wmo_id=0;
      strWmoId="UNK";
      first_year=0;
      last_year=0;
      num_years=0;
      lat_deg=0;
      long_deg=0;
      altitude=0;
      isNearAirport=0;
      iStationType=0;
    };
};

typedef struct GHCNLatLong GHCNLatLong;

typedef struct
{
  float minLatDeg;
  float maxLatDeg;
  float minLongDeg;
  float maxLongDeg;
} LatLongGridElement;

struct ResultsToDisplay
{
  int nPlotRawResults; // 1=plot results, 0=don't plot results 
  int nPlotAdjResults;
  int nPlotStartYear;
    
  // ResultsToDisplay(void)  -- constructor not allowed in union
  //   {
  //     bPlotRawResults=true;
  //     bPlotAdjResults=true;
  //   }
};

union UResultsToDisplay
{
  int nPlotResults[3];
  ResultsToDisplay resultsToDisplay;
};

//using namespace std;
#define CURRENT_YEAR (12)
#define MIN(a,b) ((a)<(b) ?  (a):(b))
#define MAX(a,b) ((a)>(b) ?  (a):(b))
#define ABS(a)   ((a)<(0) ? -(a):(a))
#define PI (3.1415926535) // close-enough for "message-board-science" work...

#define BUFLEN (512)  // more than long enough for input lines

struct ClientSelectFields; // Forward declaration

class GHCN
{
 public:

  // Generic "impossible value" flag
  static const int IMPOSSIBLE_VALUE=-99999;
  
  // All missing temperature fields in the GHCN files are designated
  // by -9999
  static const float GHCN_NOTEMP9999; // = (-9999.0);
  static const float GHCN_NOTEMP999; // = (-999.0);

  // BEST "bad/no sample" designator
  static const float BEST_NOTEMP99; //=(-99.0);

  // CRU "bad/no sample" designator
  static const float CRU4_NOTEMP99; //=(-99.0);

    static constexpr float USHCN_NOTEMP9999=-9999.;

    static constexpr float ALL_NOTEMP99=-99.0;
    
    
  // When comparing floating pt. vals against GHCN_NOTEMP,GHCN_NOTEMP999,
  // make sure that we don't get bitten by floating-pt precision limitations.
  static const float ERR_EPS; // = (0.1);
  
  // Starting year to process -- NASA/GISS doesn't try
  // to compute temperature anomalies prior to this year
  // due to coverage and data quality deficiencies.
  // Results prior to the late 1800's are pretty crappy.
  static const int MIN_GISS_YEAR=1840;  // RWM 2025/02/09 can go back as far back as 1840
  static const int MAX_GISS_YEAR=2025; // RWM 2025/01/15 -- process thru 2025

  // These define the NASA/GISS temperature baseline time period.
  static const int FIRST_BASELINE_YEAR=1951; 
  static const int LAST_BASELINE_YEAR=1980;

  // First stab
  static const int FIRST_ITRDB_BASELINE_YEAR=1700;
  static const int LAST_ITRDB_BASELINE_YEAR=1899;

  // Default minimum number of valid temperature samples during the baseline period
  // for a WMO base-id to be included in the computation.
  static const int DEFAULT_MIN_BASELINE_SAMPLE_COUNT = 15; 

  static const float DEFAULT_GRID_LAT_DEG; //=20.;
  static const float DEFAULT_GRID_LONG_DEG; //=20.;

  static const float MIN_GRID_LAT_DEG; //=20.;
  static const float MIN_GRID_LONG_DEG; //=20.;

  static const float MAX_GRID_LAT_DEG; //=20.;
  static const float MAX_GRID_LONG_DEG; //=20.;

  static const int DEFAULT_AVG_NYEAR=1;
  // Default Length of moving-average smoothing filter in years 

  static const int MAX_AVG_NYEAR=21;
  // Max length of moving-average filter

  GHCN(const char *inFile, const int& avgNyear);
  GHCN(const char *inTempFile, const char* inItrdbList, const int& avgNyear);
  
  virtual ~GHCN();

  std::string GetInputFileName(void);

  void OpenGhcnFile(void);
  void CloseGhcnFile(void);
  void OpenCruFile(void);
  void CloseCruFile(void);
  void OpenUshcnFile(void);
  void CloseUshcnFile(void);
  
  
  bool  IsFileOpen(void);

  // Uses "complete WMO id's -- 8 digits (5-digit base + 3-digit modifier)
  void  ReadGhcnV2TempsAndMergeStations(const std::set<std::size_t>& wmoCompleteIdSet);

  void ReadGhcnV3Temps(const std::set<std::size_t>& stationSet);

  void ReadGhcnV4Temps(const std::set<std::size_t>& stationSet);

    void ReadGhcnDailyTemps(const std::set<std::size_t>& stationSet);
    // Process all stations with Tmax & Tmin data
    bool DailyToMonthlyAllTmaxTminStations(const std::string& strTmaxBuf,
                                           const std::string& strTminBuf,
                                           const int tMode, // 0 for TAVG, 1 for TMIN, 2 for TMAX
                                           const int calcMode, // O for Avg mode, 1 for Min mode, 2 for Max mode.
                                           const int nMinDays, // 1-31 for min # days for a valid monthly result.
                                           float& fMonthlyAvgTemp);
    // Process Tmin/Tmax data only from stations that also have Tavg data (from hourly readings).
    bool DailyToMonthlyTmaxTminFromStationsWithTavg(const std::string& strTmaxBuf,
                                                    const std::string& strTminBuf,
                                                    const std::string& strTavgBuf,
                                                    const int tMode, // 0 for TAVG, 1 for TMIN, 2 for TMAX
                                                    const int calcMode, // O for Avg mode, 1 for Min mode, 2 for Max mode.
                                                    const int nMinDays, // 1-31 for min # days for a valid monthly result.
                                                    float& fMonthlyAvgTemp);
    bool DailyToMonthlyAllTavgStations(const std::string& strTavgBuf,
                                       const int calcMode, // O for Avg mode, 1 for Min mode, 2 for Max mode.
                                       const int nMinDays, // 1-31 for min # days for a valid monthly result.
                                       float& fMonthlyAvgTemp);
    
    void ReadCru3Temps(const std::set<std::size_t>& stationSet);

  int ReadCru4Temps();  // CRU4 data spread into a bunch of individual files.
  int ReadCru4TempFile(const char* cru4File);
  int ParseCru4StationInfo(char *strPtr, const char *strDelim);

  int ReadUshcnTemps();  // CRU4 data spread into a bunch of individual files.
  int ReadUshcnTempFile(const char* cru4File);

    void ReadBestTempsAndMergeStations(void);


  // RWM 2012/09/02
  void AddDataInfoToWmoLatLongMap(void);

  void FDateToYearAndMonth(const float& fDate,
                           int& nYearOut,
                           int& nMonthOut);
  
  bool IsLeapYear(const int& nYear);


  void  CountWmoIds(void);

  // Compute baseline temps for each WMO base (5-digit) id.
  void  ComputeBaselines(const int nMinBaselineCount);

  // Server mode -- speed things up be precomputing all station anomalies
  void PrecomputeStationAnomalies(const int nMinBaselineSampleCount);

  void SelectStationsGlobal(const std::map<std::size_t,GHCNLatLong>& latLongMapIn,
                            std::map<std::size_t,GHCNLatLong>& latLongMapOut);

    // New RWM 2025/11/08 random station selector for bootstrapping.
    void SelectStationsGlobalRandom(const std::vector<std::size_t>& vGhcnStationsIn,
                                    const int nStationsIn,
                                    const std::map<std::size_t,GHCNLatLong>& ghcnLatLongMapIn,
                                    std::map<std::size_t,GHCNLatLong>& ghcnLatLongMapOut);

  void SelectStationsPerGridCell(const float initDelLatDeg,
                                 const float initDelLongDeg,
                                 const std::map<std::size_t,GHCNLatLong>& latLongMapIn,
                                 std::map<std::size_t,GHCNLatLong>& latLongMapOut);


    // Map of station hash (std::size_t) keys, #years per station record (int)
    // Note #years isn't just MaxYear-MinYear -- it's the actual number
    // of years for each station record (accounting for gaps)
    std::map<std::size_t,int> GetStationNyears(void)
    {
        return mTempsMapStationNyears;
    };

    // Need a way to carry WMO ID info to final outputs
    // after we switched from WMO ID to std::hash station ID keys.
    std::map<std::size_t,std::string> GetStationHashvsStationId(void)
    {
        return mStationHashvsStationWmoId;
    };
    

    void AddStationHashandWmoId(const std::size_t& wmoHash, const std::string& strWmoId)
    {
            mStationHashvsStationWmoId[wmoHash]=strWmoId;
    };

    void AddStationHashAndNumYears(const std::size_t& wmoHash, const int& nYears)
    {
            mTempsMapStationNyears[wmoHash]=nYears;
    };
    
// ############ BEGIN ITRDB METHODS ###############
  void OpenItrdbFile(void);
  void CloseItrdbFile(void);

  int ReadMannItrdbList(void);

  int ReadItrdbData(void);
  int ReadItrdbDataFile(const char* itrdbFile);
  int ReadItrdbDataFile(const char* itrdbFile,
                        const std::set<std::string>& itrdbFileSet);

  void ComputeItrdbBaselines(void);

  void NormalizeItrdbData(void);

  std::map<std::size_t,float> ComputeItrdbAvg(void); // const std::set<std::string>& setItrdb);
  std::map<std::size_t,float> ComputeItrdbMannListAvg(void);
  std::map<std::size_t,float> ComputeItrdbAvg(const std::set<std::string>& setItrdb);

  void AverageItrdbGridCells(void);
// ############ END ITRDB METHODS ###############
  

  void Clear(void);
  
  // SERVER MODE
  void GlobalAnomalyServer(const int& minBaselineSampleCount,
                           const float& initDelLatDeg,
                           const float& initDelLongDeg,
                           std::map<std::size_t,GHCNLatLong>& completeLatLongMap);

  SERVER_PLOT_OP ParseWmoIdMessage(std::string& strMsg,
                                   std::set<std::size_t>& selectedWmoIdSet);
  
  UResultsToDisplay ParseResultsToDisplay(const std::string& strCommand);
  
  // "Simple" (rural/urban/suburban) batch plot command
  std::set<std::size_t> SelectStationsByRuralUrbanStatus(const std::string& strMsg);

  // "Custom" (finer-grained) batch plot command
  ClientSelectFields ParseCustomBatchPlotCommand(const std::string& strBatchCommandIn);

  std::set<std::size_t> SelectStationsByClientRequest(const ClientSelectFields& csf);

  std::set<std::size_t> SelectStationsByYearsAnd(const ClientSelectFields& csf,
                                            const std::map<std::size_t,GHCNLatLong>& sMap,
                                            const std::set<std::size_t>& stationSetIn);
  std::set<std::size_t> SelectStationsByYearsOr(const ClientSelectFields& csf,
                                           const std::map<std::size_t,GHCNLatLong>& sMap,
                                           const std::set<std::size_t>& stationSetIn);

    // RRR 2015/04/30 new
  void ComputeGriddedGlobalAverageAnnualAnomalies(const int& minBaselineSampleCount,
                                                  const float& initDelLatDeg,
                                                  const float& initDelLongDeg,
                                                  std::map<std::size_t, GHCNLatLong>& latLongMap, // can't be const -- dunno why
                                                  const std::set<std::size_t>& selectedWmoIdSet);

  // RRR 2015/04/30 new
  void ComputeGriddedGlobalAverageAnnualAnomalies(const int& minBaselineSampleCount,
                                                  const float& initDelLatDeg,
                                                  const float& initDelLongDeg,
                                                  const std::map<std::size_t, GHCNLatLong>& latLongMap);

    // Same as above, but compute global monthly anomalies instead of global annual anomalies.  
  void ComputeGriddedGlobalAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                   const float& initDelLatDeg,
                                                   const float& initDelLongDeg,
                                                   std::map<std::size_t, GHCNLatLong>& latLongMap,// can't be const
                                                   const std::set<std::size_t>& selectedWmoIdSet);

  void ComputeGriddedGlobalAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                   const float& initDelLatDeg,
                                                   const float& initDelLongDeg,
                                                   const std::map<std::size_t, GHCNLatLong>& latLongMap);
    // Accumulate average annual bootstrap runs
    void AccumulateGlobalAverageAnnualBootstrapRuns(void);

    // Accumulate average annual station counts
    void AccumulateGlobalAverageAnnualBootstrapStationCounts(void);


    // The same for monthly average stats (TODO)
    void AccumulateGlobalAverageMonthlyBootstrapStatistics(void);

    // Compute final statistics based on the number of ensembles captured per year.
    void ComputeGlobalAverageAnnualBootstrapStatistics(void);

    // Compute mean station count for all bootstrap ensemble runs
    void ComputeGlobalAverageAnnualBootstrapStationCountMeans(void);


    
    void SetCompleteWmoLatLongMap(const std::map<std::size_t, GHCNLatLong>& latLongMap)
    {

      m_CompleteWmoLatLongMap=latLongMap;

      return;

    };

  std::map<std::size_t,GHCNLatLong> GetCompleteWmoLatLongMap(void)
    {

      return m_CompleteWmoLatLongMap; 

    };

    void AddStationWmoIdStringsToProcessedStationLatLongMap(void)
    {
        for(auto & im: m_ProcessedWmoLatLongMap)
        {
            if(mStationHashvsStationWmoId.find(im.first)!=mStationHashvsStationWmoId.end())
            {
                std::string strWmoId=mStationHashvsStationWmoId[im.first];
                im.second.strWmoId=strWmoId;
            }
        }
    }
    
  std::map<std::size_t, GHCNLatLong> GetProcessedStationLatLongMap(void)
  {
      // WMO string ID keys were replaced with std::size_t hashes as keys
      // in the processed station lat long map. But still want the
      // std::string representations available, so add them to the
      // lat-long map structure entries here.
      AddStationWmoIdStringsToProcessedStationLatLongMap();
      return m_ProcessedWmoLatLongMap;
  };
  
  std::map<std::size_t,double>& GetSmoothedGlobalAverageAnnualAnomaliesMap(void)
    {
      return mSmoothedGlobalAverageAnnualAnomaliesMap;
    };
  
    std::map<std::size_t,float>& GetAverageAnnualStationCountMap(void)
    {
      return mAverageAnnualStationCountMap;
    };
  
  
  // Server mode -- set/get data display mode.
  void SetDisplayMode(const int nDisplayMode)
    {
      m_nDisplayResults=nDisplayMode;
    };
  
  int GetDisplayMode(void)
    {
      return m_nDisplayResults;
    };

  void SetDisplayIndex(const int nDisplayIndex)
    {
      m_nDisplayIndex=nDisplayIndex;
    }
  
  int GetDisplayIndex(void)
    {
      return m_nDisplayIndex; 
    }
  
  int ComputeAvgStationLatAlt(void);

// Moved to GnuPlotter  void GnuPlotResults(int nPlotMode, std::string strTitle);
    friend std::vector<std::size_t> CreateStationIdVector(std::vector<GHCN*>& ghcn, int igh);
    friend void DumpSmoothedAnnualResults(std::fstream& fout, std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpSmoothedMonthlyResults(std::fstream& fout, std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpStationCountResults(std::fstream& fout, std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpStationAvgLatAltResults(std::fstream& fout, std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpAnnualBootstrapResults(std::fstream& fout,std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpAnnualBootstrapEnsemble(std::fstream& fout,std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpAnnualBootstrapStatistics(std::fstream& fout,
                                              std::vector<GHCN*>& ghcn,  int ngh);
    friend void DumpAnnualBootstrapStationCounts(std::fstream& fout,
                                                 std::vector<GHCN*>& ghcn,  int ngh);

  protected:

  std::string mInputFileName;
  std::string mCsvFileName;
  std::string mGnuPlotAnomalyFileName; // SERVER MODE
  std::string mGnuPlotStationCountFileName; // SERVER MODE
 
  std::fstream* mInputFstream;
  std::fstream* mCsvFstream;
  std::fstream* mGnuPlotAnomalyFstream; // SERVER MODE
  std::fstream* mGnuPlotStationCountFstream; // SERVER MODE
   
  FILE* m_pGnuPopen; // SERVER MODE

  bool mbFileIsOpen;
  bool mbCsvFileIsOpen;
  
  int m_nCurrentYear;
  
  char *mCbuf;
  char *mCcountry; // 3 digits
  char *mCwmoId;  // 5 digits
  char *mCmodifier;  // 3 digits
  char *mCduplicate; // 1 digit
  char *mCyear;    // 4 digits
  char *mCtemps;

  // Used by the CRU4 station info parser
  std::string m_strCru4Name;
  std::string m_strCru4Country;
  int  m_iCru4Id;
  float m_fCru4Lat;
  float m_fCru4Long;
  int   m_iCru4StartYear;
  int   m_iCru4EndYear;

    // USHCN station info
  std::string m_strUshcnName;
  int  m_iUshcnId;
  float m_fUshcnLat;
  float m_fUshcnLong;
  int   m_iUshcnStartYear;
  int   m_iUshcnEndYear;
    
  // GHCN data line format
  //
  // (temperature data station ID)
  //   3-digit country code
  //   5-digit WMO base id (station) number
  //   3-digit modifier
  //   1-digit duplicate number
  //
  // (data year follows)
  // 4-digit year
  //
  // (temperature samples follow)
  // 12 5-digit temperature fields
  // 
  // (end of line)


  // All temperature data time-series sharing a common base WMO station
  // (5 digit) number are merged (averaged) to generate a single temperature
  // time-series for that WMO number.

  char mId[16]; // Station id info read into mId (more than long enough).

  std::size_t mIWmoStationId;
  std::size_t mICompleteStationId;
  std::size_t mIyear;
  
  int mIavgNyear;

  std::set<std::size_t> mDebugSet; // RWM DEBUG
  

  std::map<std::size_t, GHCNLatLong> m_CompleteWmoLatLongMap;
  std::map<std::size_t, GHCNLatLong> m_ProcessedWmoLatLongMap;
  
  // WMO (base) station id, year, 12-element temperature vector (1 per month)
  std::map<std::size_t, std::map<std::size_t, std::vector<float> > > mTempsMap;

    // Map of station hash (std::size_t) keys, #years per station record (int)
    // Note #years isn't just MaxYear-MinYear -- it's the actual number
    // of years for each station record (accounting for gaps)
  std::map<std::size_t,int> mTempsMapStationNyears;

    // Need a way to carry WMO ID info to final outputs
    // after we switched from WMO ID to std::hash station ID keys.
  std::map<std::size_t,std::string> mStationHashvsStationWmoId;
    
    
  // Keeps track of the number of stations sharing a WMO id that have
  // temperature data for any year/month.  Used to compute temperature
  // averages during the station merging process.
  std::map<std::size_t, std::map<std::size_t, std::vector<int> > > mTempsMapCount;

  int mMinBaselineSampleCount;
  
  // WMO base id, month:  baseline sample count for each individual month
  // for each WMO base id over the baseline interval 1951-1980
  std::map<std::size_t, std::map<std::size_t, int> > mBaselineSampleCountMap;

  // WMO base id, month: baseline average temperature
  std::map<std::size_t, std::map<std::size_t,float> > mBaselineTemperatureMap;


  // Temperature anomalies indexed by grid-number, year 

  // RRR 2015-04-30 new line below
  std::vector<std::map<std::size_t, std::vector<double> > > mGridCellAverageMonthlyAnomalies;
  std::vector< std::map <int, double > > mGridCellAverageAnnualAnomalies;
  std::vector< double > mGridCellAreasVec;
  std::vector<std::set<std::size_t> > mGridCellYearsWithDataVec;
  
  double mTotalGridCellArea;
  
  bool m_bComputeAvgStationLatAlt;

  // Indexed by year -- number of grid-cells with valid temperature
  //                    data for each year.  Divide the
  //                    summed gridded temperatures by these numbers
  //                    to get the global-average temperature anomalies.
  std::map<std::size_t, int > mGlobalAverageAnnualAnomalyGridCountMap;

  std::map<std::size_t, std::vector<int> > mGlobalAverageMonthlyAnomalyGridCountMap;

  //  Year, month, base-id count.
  //  Number of valid WMO base id's per year, month
  std::map<std::size_t, std::map<std::size_t, int> > mAverageStationCountMap;
  std::map<std::size_t, float> mAverageAnnualStationCountMap;
  
  // Indexed by integer representation of each fractional year by multiplying the fractional year
  // (year+month/12.)  by 1000 & rounding to map global average monthly anomalies.
  // Don't want map keys to be full precision floating pt #s.
  //   
    std::map<int, double > mGlobalAverageYearFractAnomaliesMap;

  // Monthly anomalies indexed by [year][month]
  std::map<std::size_t, std::vector<double> > mGlobalAverageMonthlyAnomaliesMap;

    // Indexed by year -- global-average annual anomalies
  std::map<std::size_t, double >  mGlobalAverageAnnualAnomaliesMap;

  // This will contain the moving-average smoothed global temperature
  // anomalies (1 per year). Map is indexed by year.
    std::map<std::size_t, double> mSmoothedGlobalAverageAnnualAnomaliesMap;
    std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap;
    std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap;
    std::map<std::size_t,float> mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap;
    std::map<std::size_t,float> mAverageAnnualBootstrapStationCountMap;
    std::map<std::size_t,float> mAverageAnnualBootstrapStationCountMeanMap;

    // Each vector entry contains <year,temp_anomaly> entries computed from one global temperature result bootstrap run.
    // key: year
    // value: corresponding global average anomaly (for that key year).
    std::vector<std::map<std::size_t,double>> mSmoothedGlobalAnnualAnomalyEnsembleRunMapVector;
    std::vector<std::map<std::size_t,float>> mAverageAnnualBootstrapStationCountMapVector;
    
    
    // Smoothed global average monthly anomalies indexed by fractional year (year+month/12) * 1000
    // rounded to the nearest int.
    std::map<int, double> mSmoothedGlobalAverageYearFractAnomaliesMap;
    std::map<int,double> mSmoothedGlobalAverageYearFractAnomaliesBootstrapMeanMap;
    std::map<int,double> mSmoothedGlobalAverageYearFractAnomaliesBootstrapStdDevMap;
    std::map<int,float> mSmoothedGlobalAverageYearFractBootstrapEnsembleCountMap;

    

    
  std::map<int,std::pair<float,float> >  m_mapYearStationAvgLatAlt;

    std::map<int,int> m_mapYearStationCountAvgLatAlt;
    
    
  int m_nDisplayResults; // Server mode only: !0 means plot results, 0 means don't.
  int m_nDisplayIndex;   // Each GHCN instance gets an index number -- for plotting consistency.
  

// ################ BEGIN ITRDB MEMBERS #####################
  std::string m_MannItrdbListFileName;
  std::fstream* m_pMannItrdbListFstream;
  std::set<std::string> m_MannItrdbListSet;
  
  // ITRDB data indexed by std::string itrdbId and year
  std::map<std::string, std::map<std::size_t, float> > mItrdbMap;

  // ITRDB baseline count map (may not be needed) indexed by
  // ITRDB id
  std::map<std::string, std::map<std::size_t, int> > mItrdbCountMap;
  std::map<std::string, GHCNLatLong> mItrdbLatLongMap;

  // Itrdb id:  baseline sample count for each Itrdb id 
  // over the Itrdb baseline interval 
  std::map<std::string, int > mBaselineItrdbSampleCountMap;

  // Itrdb id: baseline average itrdb value
  std::map<std::string, float > mBaselineItrdbMap;
  
  // Vector of grid-cell Itrdb averages -- Each grid-cell has avg val per year
  std::vector<std::map<std::size_t,float> > m_vmapItrdbGridCellAvg;
  
  // Final annual average ITRDB values.
  std::map<std::size_t,float> m_mapItrdbAvg;
  // ################ END ITRDB MEMBERS #####################
  
//   int (GHCN::*m_fPtr)();


  bool InLatLongGridElement(const GHCNLatLong& ghcnLatLong, 
                            const LatLongGridElement& latLongEl);

  // Builds a list of all stations within a specific lat/long grid cell.
  std::set<std::size_t> ComputeGridElementStationSet(const std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap,
                                              const LatLongGridElement& latLongEl);


  // For "sparse station" processing -- identifies the station
  // with the longest temperature record in any particular grid-cell.
  std::size_t PickLongestStation(std::set<std::size_t>* ghcnStationSetIn);

  // For "sparse stations" processing -- identify the nStations stations with
  // the longest temperature record.
  std::set<std::size_t> PickLongestNStations(std::set<std::size_t>* ghcnStationSetIn, const int nStations);

  // Customize this to pick the stations U want...
  std::set<std::size_t> MinStationRecordLength(std::set<std::size_t>* ghcnStationSetIn);

    std::set<std::size_t> PickStationsByRecordSpecs(std::set<std::size_t>* ghcnStationSetIn);


    // New 2024/03/10
    // mGlobalAverageMonthlyAnomaliesMap[iyy->first][iMm] is the main grid cell anomaly to accumulate. 
    // iyy is year iterator, iMm is 0-100 month index.
    float ComputeGridCellAnomalyFromSubCells(std::map<std::size_t, std::vector<float> >:: iterator iYy, 
                                             const int iMm,  
                                             LatLongGridElement& gridElIn,
                                             const int& nSubcellLatIn, const int& nSubcellLonIn,
                                             const std::map<std::size_t, GHCNLatLong>& latLongMapIn);

  // Compute average monthly anomalies, averaged over all stations
  // in ghcnStationSet.
  // For each month, include only the stations that have at least minBaselineSampleCount
  // valid temperature samples in the baseline period.
  // RWMM 2012/08/29
  // Returns a list (std::set) of all years for which
  // found data for the stations it has processed. Years not included
  // in the returned set have no station data.
  std::set<std::size_t>  ComputeAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                                   const LatLongGridElement& gridEl,
                                                   const int& nSubcellLat, const int& nSubcellLon,
                                                   std::set<std::size_t>* ghcnStationSet=NULL);


  // Compute average annual anomalies for each lat/long grid-cell.
  void ComputeGridCellAverageAnnualAnomalies(const int& minBaselineSampleCount,
                                             const float& initDelLatDeg,
                                             const float& initDelLongDeg,
                                             const std::map<std::size_t, GHCNLatLong>& latLongMap);


  // RRR 2015/04/30 new
  void ComputeGridCellAverageMonthlyAnomalies(const int& minBaselineSampleCount,
                                             const float& initDelLatDeg,
                                             const float& initDelLongDeg,
                                             const std::map<std::size_t, GHCNLatLong>& latLongMap);

 

  double ComputeGridCellArea(const float minLatDeg, const float maxLatDeg,
                             const float minLonDeg, const float maxLonDeg);

  // RRR 2015/04/30
  // void ComputeGlobalAverageMonthlyAnomaliesFromGridCells(void);

  void  MergeMonthsToYear(void); 

  // RRR 2015/04/30
  void ComputeGlobalAverageAnnualAnomaliesFromGridCellMonthlyAnomalies(void);

    // For global monthly time-series (indexed by year+month/12 with
    // floating pt precision limited to .XXX)  
  void ComputeGlobalAverageMonthlyAnomaliesFromGridCellMonthlyAnomalies(void);

  void ComputeGridCellFullAnnualSums(std::vector<std::map<std::size_t,
                                     std::vector<double> > >::iterator& iCell,
                                     std::map<std::size_t, float>& annualGridXMonthMap);

  void ComputeGridCellYearFractSums(std::vector<std::map<std::size_t,
                                    std::vector<double> > >::iterator& iCell,
                                    std::map<int, float>& gridXYearFractMap);
    
  void ComputeGridCellPartialAnnualSums
    (std::vector<std::map<std::size_t,std::vector<double> > >::iterator& iCell,
     const int& nMinMonthIn0, const int& nMaxMonthIn0,
         std::map<std::size_t, float>& annualGridXMonthMap);


  void  ComputeAnnualMovingAvg(void);
  void  ComputeMonthlyMovingAvg(void);

  // int ComputeAvgStationLatitudes(void);
    
  // Moved to class GnuPlotter
  // void OpenGnuPlotFiles(void);
  // void CloseGnuPlotFiles(void);
  // void WriteDataToGnuPlotFiles(void);
  // void LaunchGnuPlot(int nPlotMode, std::string strTitle);

};

struct ClientSelectFields
{

  int bnDisplayRaw;        // 1 to display raw data results, 0 not to display

  int bnDisplayAdj;        // ditto for adj data results.
  
  int bnSelectAll;         // If=1, then select all stations -- ignore other fields in this struct.
  
  int bnSelectType;        // enable selection by type 0==false, 1==true 
  int  nType;              // 1 for rural, 2 for suburban, 3 for urban
  
  int bnSelectAirport;     // enable selection by airport proximity 0==false, 1==true
  int nAirport;            // 1 for near airport, 2 for not near airport

  int bnSelectAltitude;   // select stations by altitude  0==false, 1==true
  int nAltitudeMode;       // 0 for <= nAltitude, 1 for >= nAltitude
  int nAltitude;           // altitude in meters
  
  int bnSelectMinLatitude;   // 1 means select stations >= minLatitude
  int  nMinLatitude100;         // min latitude (deg) x 100

  int bnSelectMaxLatitude;   // 1 means select stations <= maxLatitude
  int  nMaxLatitude100;      // max latitude (deg) x 100

  int bnSelectStartYear;  // select stations by start year  0==false, 1==true
  int nStartYearMode;      // 0 for <= start year, 1 for >= start year
  int nStartYear;          // start year (19xx or 20xx)

  int bnSelectEndYear;     // select stations by end year  0==false, 1==true
  int nEndYearMode;        // 0 for <= end year, 1 for >= end year
  int nEndYear;            // end year (19xx or 20xx)

  int bnSelectNumYears;    // select stations by #years (record length) 0==false, 1==true
  int nNumYearsMode;       // 0 for <= record length, 1 for >= record length
  int nNumYears;           // Temperature record length (years)

  int nYearOrAnd;          // 0 for "or" of year specs, 1 for "and" of year specs.

    int nPlotStartYear;    // The start year for gnuplot display plots.
    
    
  ClientSelectFields(void)
    {
      // Default -- display raw+adj results
      bnDisplayRaw=1;
      bnDisplayAdj=1;
      
      bnSelectAll=1;   // aka true -- default is use all station data
      bnSelectType=0;   // aka false
      bnSelectAirport=0; // aka false;
      bnSelectAltitude=0;  // aka false;
      bnSelectMinLatitude=0;
      bnSelectMaxLatitude=0;
      bnSelectStartYear=0; // aka false;
      bnSelectEndYear=0; // aka false;
      bnSelectNumYears=0; // aka false;
      
      nType=GHCN::IMPOSSIBLE_VALUE;

      nAirport=GHCN::IMPOSSIBLE_VALUE;
 
      nAltitudeMode=GHCN::IMPOSSIBLE_VALUE;
      nAltitude=GHCN::IMPOSSIBLE_VALUE;
     
      nMinLatitude100=GHCN::IMPOSSIBLE_VALUE;
      nMaxLatitude100=GHCN::IMPOSSIBLE_VALUE;
      
      nStartYearMode=GHCN::IMPOSSIBLE_VALUE;
      nStartYear=GHCN::IMPOSSIBLE_VALUE;
      
      nEndYearMode=GHCN::IMPOSSIBLE_VALUE;
      nEndYear=GHCN::IMPOSSIBLE_VALUE;
      
      nNumYearsMode=GHCN::IMPOSSIBLE_VALUE;
      nNumYears=GHCN::IMPOSSIBLE_VALUE;
      
      nYearOrAnd=GHCN::IMPOSSIBLE_VALUE;

      nPlotStartYear=1880;  // NASA/GISS start year default.
      
    };
  
  
};

