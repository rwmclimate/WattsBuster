
// Command line arg Y for annual, y for monthly.
// Default will be annual.
extern int nAnnualAvg_g; // 1 for annual avg output, 0 for monthly avg output.

extern int avgNyear_g;
extern int minBaselineSampleCount_g;

// For statistical bootstrapping runs.
extern int nSeedBootstrap_g; 
extern int nStationsBootstrap_g;
extern int nEnsembleRunsBootstrap_g;

extern char *metadataFile_g;
extern std::string strStationType_g;  // X means "all station types: rural/suburban/urban"

extern float fAirportDistKm_g; // 0 non-aiport stations, 1 airport stations, 2 all stations


extern char *gridSizeStr_g;

extern float gridSizeLatDeg_g;
extern float gridSizeLongDeg_g;

// The ResultsToDisplay struct is for use specifically with the browser/javascript front-end
// in ../BROWSER-FRONTEND.d -- Highly customized -- will work only with the runit.sh script.
// Default is to display raw and adjusted data results.
extern UResultsToDisplay uResultsToDisplay_g;
  

extern STATION_SELECT_MODE eStationSelectMode_g;

// Set to true to enable "sparse station" processing
extern bool bSparseStationSelect_g;

// These specify the min record length
// or max record start and min record end
// times of stations to be processed
extern bool bSetStationRecordSpecs_g;

extern bool bMinStationRecordLength_g;
extern size_t  nMinStationRecordLength_g;

extern bool bMaxStationRecordStartYear_g;
extern size_t  nMaxStationRecordStartYear_g;

extern bool bMinStationRecordEndYear_g;
extern size_t  nMinStationRecordEndYear_g;

extern bool bNHSummerMonthsOnly_g;

extern bool bPartialYear_g; // If true, process only partial year data.
extern int nStartMonth_g;   // Partial year start month 1-12.
extern int nEndMonth_g;     // Partial year end month 1-12.

extern int nLongestStations_g;

extern int nPlotStartYear_g;

// Set to true to adjust for the small variations in grid-cell areas
extern bool bAreaWeighting_g;

// Default GHCN version = 3 (newest)
extern int nGhcnVersion_g;

// Default data version GHCN (vs. BEST or CRU)
extern int nDataVersion_g;

// Default CRU version = 3 or 4(newest) -- default 3
extern int nCruVersion_g;

extern char *cGhcnDailyMode_g;
extern int nGhcnDailyData_g; // Data mode: 0 for TAVG, 1 for TMIN, 2 for TMAX
extern int nGhcnDailyMode_g; // Processing mode: 0 for Avg, 1 for Min, 2 for Max
extern int nGhcnDailyMinDaysPerMonth_g; // Min #days for a valid monthly result.

// If 1, process estimated temps in USHCN
// homogenized data files.
extern int nUshcnEstimatedFlag_g;

extern std::vector <std::string> ensembleElementHeadingStr_g;

extern char *csvFileName_g;
extern char *csvStationCountFileName_g;

extern bool bGenKmlFile_g;
extern bool bGenJavaScriptFile_g;

extern GHCN* pGhcn_g;

extern int nServerPort_g;

extern int nPythonByteSize_g; 

extern bool bComputeAvgStationLatAlt_g;

extern SimpleTcpSocket* pServerSocket_g;
