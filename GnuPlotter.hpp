#pragma once
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdio.h>
#include <math.h>

#include "SimpleSocketPlatformDef.h" // RRR 2012/09/29

#ifdef LINUX // RRR 2012/09/29
#include <stdio_ext.h>
#endif

class GHCN;  // Forward declaration

struct GnuPlotElement
{
  std::string m_strInputDataFileName;
  std::string m_strGnuPlotAnomalyFileName;
  std::string m_strGnuPlotStationCountFileName;

  std::fstream* m_pGnuPlotAnomalyFstream;
  std::fstream* m_pGnuPlotStationCountFstream;
  
  GHCN* m_pGhcn;
  
  float m_fXcorr; // crosscorrelation (covariance)
  float m_fR2; // R-squared statistic
  
  GnuPlotElement(void)
    {
      m_pGhcn=NULL;
      m_pGnuPlotAnomalyFstream=NULL;
      m_pGnuPlotStationCountFstream=NULL;
      m_fXcorr=0;
      m_fR2=0;
    }
  
};


class GnuPlotter
{
 public:

  GnuPlotter(void);
  
  GnuPlotter(std::vector<GHCN*>& vGhcn);

  virtual ~GnuPlotter(void)
    {
      if(NULL!=m_pGnuPopen)
        ::pclose(m_pGnuPopen);
    };
  
  void GnuPlotResults(int nPlotMode, const std::string& strNstations);

 private:

    std::vector<GnuPlotElement> m_vGnuPlotElement;
    
    std::vector<GHCN*> m_vpGhcn;
//  std::vector<FILE*> m_vpGnuPopen;
//  std::vector<std::fstream*> m_vpGnuPlotAnomalyFstream;
//  std::vector<std::fstream*> m_vpGnuPlotStationCountFstream;
    
    std::vector<std::string> m_vStrLineColors;
    std::map<int,float> m_mapGissAnomalies;
    
    FILE* m_pGnuPopen;
    
    void OpenGnuPlotFiles(void);
    void CloseGnuPlotFiles(void);
    bool WriteDataToGnuPlotFiles(void);
    void LaunchGnuPlot(int nPlotMode, const std::string& strNstations);

    std::string ComposeGnuPlotAnomalyFileName(const int iFile);
    
    std::map<int,float> ReadGnuPlotFileData(const char* inFile);

    void ServerPlotNew(std::vector<std::string>& vStrGnuCommand, const std::string& strNstations, const std::string& strStartYear="1880");
    void ServerPlotIncrement(std::vector<std::string>& vStrGnuCommand, const std::string& strNstations, const std::string& strStartYear="1880");
    
    float ComputeCrossCorrelation(const std::string& strGnuPlotAnomalyFileName);
    
    // Supersedes above fcn
    int ComputeAnomalyStatistics(const std::map<size_t,double>& mapAnomalies, float& fCorr, float& fR2);

};

  
  
