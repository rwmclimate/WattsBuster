#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "GnuPlotter.hpp"
#include "SimpleSocket.h"

#include "GHCN.hpp"
#include "GHCN_externs.hpp"

int nBrightnessValue_g=999;

std::vector<std::string> g_vStrCmdArgs;

int main(int argc, char **argv)
{
    bMaxStationRecordStartYear_g=false;
    bMinStationRecordLength_g=false;
    bMinStationRecordEndYear_g=false;

    int status;
  
  void ProcessOptions(int argc, char **argv);
  
  int MainBatchMode(int argc, char **argv);
  int MainServerMode(int argc, char **argv);

  cStationType_g="XXX"; // 'X' means process all stations

  for(int ic=0; ic<argc; ic++)
  {
      g_vStrCmdArgs.push_back(argv[ic]);
  }
  
  ProcessOptions(argc,argv);
  
  if(nServerPort_g<=1024) // Server mode only for non-privileged ports (>1024)
  {
    status=MainBatchMode(argc,argv);
  }
  else
  {
    status=MainServerMode(argc,argv);
  }
  
  std::cerr << "All finished!"<<std::endl<<std::endl;
  
  return status;

}

int MainServerMode(int argc, char** argv)
{
  std::vector<GHCN*> ghcn;

  std::set<size_t> wmoCompleteIdSet;
  
  std::map<size_t, GHCNLatLong> ghcnLatLongMap;

  std::set<size_t> selectedWmoIdSet;


  void ReadMetadataFile(std::set<size_t>& wmoCompleteIdSet,
                        std::map<size_t,GHCNLatLong>& ghcnLatLongMap);

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<size_t>& wmoCompleteIdSet,
                           std::map<size_t, GHCNLatLong>& ghcnLatLongMap);

  void ServerPrecomputeAnomalies(std::vector<GHCN*>& ghcn, 
                                 int argc, char**& argv,
                                 std::map<size_t,GHCNLatLong>& ghcnLatLongMap,
                                 std::set<size_t>& wmoCompleteIdSet);
  
  cStationType_g="XXX"; // Default to all stations.
  
  ReadMetadataFile(wmoCompleteIdSet,
                   ghcnLatLongMap);
  

  // Read command message from client (QGIS or browser)
  // Return true if the message came from a browser (i.e. is html)
  // Return false if the message came from QGIS (not html)
  bool ReadCommandMessage(SimpleTcpSocket *pClientConnectSocket,
                          std::string& strMsg);

  void ServerProcessLoop(std::vector<GHCN*>& ghcn,
                         int argc, char** argv,
                         std::map<size_t,GHCNLatLong>& ghcnLatLongMap,
                         std::set<size_t>& wmoCompleteIdSet);

  std::cerr << std::endl;
  std::cerr << "Smoothing filter length = " << avgNyear_g 
            << std::endl << std::endl; 
  
  std::cerr << "Will crunch " << argc-optind << " temperature data set(s). " 
            << std::endl;


  ghcn.resize(argc-optind);
  
  ServerPrecomputeAnomalies(ghcn, argc, argv,
                            ghcnLatLongMap, wmoCompleteIdSet);

  ServerProcessLoop(ghcn, argc, argv, ghcnLatLongMap, wmoCompleteIdSet);
  
  
  return 0;

}

  
void ServerPrecomputeAnomalies(std::vector<GHCN*>& ghcn, 
                               int argc, char**& argv,
                               std::map<size_t,GHCNLatLong>& ghcnLatLongMap,
                               std::set<size_t>& wmoCompleteIdSet)
{

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<size_t>& wmoCompleteIdSet,
                           std::map<size_t, GHCNLatLong>& ghcnLatLongMap);

  // Loop through the GHCN file command-line args.
  for(int igh=0; igh<argc-optind; igh++)
  {
    
    ghcn[igh] = new GHCN(argv[igh+optind],avgNyear_g);
    
    pGhcn_g=ghcn[igh];
    
    ghcn[igh]->SetCompleteWmoLatLongMap(ghcnLatLongMap);
    
    
    std::cerr << std::endl;
    std::cerr << "Reading data from " 
              << argv[igh+optind] << std::endl;
    
    // Read in temperature data -- also finish "populating" the
    // ghcnLatLong map with station data info not available in
    // the metadata file (i.e. data start/stop years...)
    ReadTemperatureData(ghcn,igh,wmoCompleteIdSet,ghcnLatLongMap);

    // Add data-specific info to the WMO lat/long map
    // (i.e. info not available in the metadata file,
    // like data start/end years)
    ghcn[igh]->AddDataInfoToWmoLatLongMap();
    
    ghcn[igh]->CountWmoIds();
    
    // Single-result -- no ensemble processing
    std::cerr << "Precomputing baseline temperatures for " 
              << argv[igh+optind]<< std::endl;
    std::cerr << std::endl;
    ghcn[igh]->ComputeBaselines();
    
    std::cerr << "Precomputing station anomalies for " 
              << argv[igh+optind]<< std::endl;
    std::cerr << std::endl;
    ghcn[igh]->PrecomputeStationAnomalies();
    
  }
}

void ServerProcessLoop(std::vector<GHCN*>& ghcn,
                  int argc, char** argv,
                  std::map<size_t,GHCNLatLong>& ghcnLatLongMap,
                  std::set<size_t>& wmoCompleteIdSet)
{
    
  SimpleTcpSocket *pServerSocket=NULL;
  SimpleTcpSocket *pClientConnectSocket=NULL;
  
  std::set<size_t> selectedWmoIdSet;
  
  GnuPlotter gnuPlotter(ghcn);
  
  int ComputeResultsToDisplay(int argc,
                              std::vector<GHCN*>& ghcn,
                              std::set<size_t>& selectedWmoIdSet);

  bool ReadCommandMessage(SimpleTcpSocket *pClientConnectSocket,
                          std::string& strMsg);

  void ManageReconnection(bool bHtmlMode,
                          SimpleTcpSocket* pServerSocket,
                          SimpleTcpSocket*& pClientConnectSocket);

  pServerSocket=new SimpleTcpSocket(nServerPort_g);
  pServerSocket_g=pServerSocket;

  // All prepped and ready to go into server mode
  std::cerr<<"Entering server mode..."<<std::endl;
  pClientConnectSocket = pServerSocket->WaitForClient();
    
  std::cerr<<"Connected to the client..."<<std::endl;

  bool bHtmlMode=false;
  
  while(true)
  {
    std::string strMsg="";

    try
    {
      bHtmlMode=ReadCommandMessage(pClientConnectSocket,
                                   strMsg); // strMsg will be modified
    std::cerr<<"CalledReadCommandMessage(): "<<strMsg<<std::endl;
    }
    catch(SimpleSocketException* pSex)
    {
        strMsg="";  // RWM new 2014/10/28
      delete pSex;
      std::cerr<<"Comm problem -- try to reconnect..."<<std::endl;
      pClientConnectSocket
        =pClientConnectSocket->ReconnectToClient(pServerSocket,
                                                 pClientConnectSocket);
      continue;
    }

    std::cerr<<"Just received message: "<<strMsg<<std::endl;
    
    SERVER_PLOT_OP nPlotMode=SERVER_PLOT_RESET;

    // New for 2013/02/03
    // Parse out fields for raw/adj data display options
    uResultsToDisplay_g = ghcn[0]->ParseResultsToDisplay(strMsg);
    
    // Borrow this from a GHCN instance
    nPlotMode=ghcn[0]->ParseWmoIdMessage(strMsg,selectedWmoIdSet);
    
    if(SERVER_PLOT_NONE==nPlotMode) // Invalid/Incomplete message? skip it.
      continue;

    if(SERVER_PLOT_QUIT==nPlotMode) // Shutdown code
    {
      pClientConnectSocket->CloseSocket();
      exit(1);
    }
    
    // If we got this far, we've received and parsed a valid/complete message --
    // So reset the message string.
    strMsg.clear();

    int nMaxPointsToPlot=ComputeResultsToDisplay(argc,ghcn, selectedWmoIdSet);

    std::string strNstations;

// Corrected version 2012/12/03
    if(nMaxPointsToPlot>0) 
    {
      std::stringstream sstrNstations;
      sstrNstations<<"("<<nMaxPointsToPlot<<")"; 
      strNstations=sstrNstations.str();
    }
    else
    {
      strNstations=std::string("(0)");
      nPlotMode=SERVER_PLOT_RESET;
    }

    gnuPlotter.GnuPlotResults(nPlotMode,strNstations);
    
    ManageReconnection(bHtmlMode, pServerSocket, pClientConnectSocket);

  }
}
 
int ComputeResultsToDisplay(int argc,
                            std::vector<GHCN*>& ghcn,
                            std::set<size_t>& selectedWmoIdSet)
{
      
                            
  // Must loop over all ghcn instances and compute gridded avg anomalies.
  // Must loop over all ghcn instances and compute the max #points to plot.
  // Will plot something if just one instance has enough points to plot.
  unsigned int nMaxPointsToPlot=0;
  for(int igh=0; igh<argc-optind; igh++)
  {
    // Don't compute results that won't get plotted
    if(igh>=2) // A max of two data sets can be handled in this mode.
      break;
    if(uResultsToDisplay_g.nPlotResults[igh]>0)
    {
      std::cerr<<std::endl<<"Computing results for file "<<igh+1<<std::endl;
        
      std::map<size_t,GHCNLatLong> completeLatLongDegMap;
      completeLatLongDegMap=ghcn[igh]->GetCompleteWmoLatLongMap();
      
      ghcn[igh]->Clear();
      
      ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                            gridSizeLatDeg_g,
                                                            gridSizeLongDeg_g,
                                                            completeLatLongDegMap,
                                                            selectedWmoIdSet);
      ghcn[igh]->SetDisplayMode(1);

      if(ghcn[igh]->GetProcessedStationLatLongMap().size()>nMaxPointsToPlot)
      {
        nMaxPointsToPlot=ghcn[igh]->GetProcessedStationLatLongMap().size();
      }
        
    }
    else
    {
      ghcn[igh]->SetDisplayMode(0);
      std::cerr<<std::endl<<"Skipping file "<<igh+1<<std::endl;
    }

    // Want consistent data plot curve colors for all
    // combos of display selections.
    // NASA/GISS color index = 1.  Raw,Adj indices always 2 and 3 respectively.
    ghcn[igh]->SetDisplayIndex(igh+2);
      
  }
  std::cerr<<std::endl;

  return nMaxPointsToPlot;
    
}

void ManageReconnection(bool bHtmlMode,
                        SimpleTcpSocket* pServerSocket,
                        SimpleTcpSocket*& pClientConnectSocket)
{
      
  // If the client is QGIS (i.e. not html/browser mode)
  // send back the ACK that the QGIS plugin expects...
  if(false==bHtmlMode)
  {
    try
    {
        
      std::string strAck=std::string("Got It!\n");
      int nBytesSend=0;
        
      nBytesSend=pClientConnectSocket->SendData(strAck.c_str(),
                                                static_cast<int>(strlen(strAck.c_str())));
        
      std::cerr<<"Just sent an ACK ("<<nBytesSend<<" bytes)"<<std::endl;
        
    }
    catch(SimpleSocketException* pSex)
    {
      delete pSex;
      std::cerr<<"Comm problem -- try to reconnect..."<<std::endl;
      pClientConnectSocket
        =pClientConnectSocket->ReconnectToClient(pServerSocket,
                                                 pClientConnectSocket);
      return;
    }
  }
  else
  {
    // HTML mode -- must disconnect and reconnect after every operation
    pClientConnectSocket->CloseSocket();
    pClientConnectSocket
      =pClientConnectSocket->ReconnectToClient(pServerSocket,
                                               pClientConnectSocket);
  }
}
   
int MainBatchMode(int argc, char **argv)
{

//  int igh;
    
  std::vector<GHCN*> ghcn;

  std::set<size_t> wmoShortIdSet;
  std::set<size_t> wmoCompleteIdSet;
  
  std::map<size_t, GHCNLatLong> ghcnLatLongMap;

  std::stringstream headingStrBase;
  std::stringstream headingStr;

  // void ReadMetadataFile(std::set<size_t>& wmoCompleteIdSet,
  //                       std::map<size_t,GHCNLatLong>& ghcnLatLongMap);

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<size_t>& wmoCompleteIdSet,
                           std::map<size_t, GHCNLatLong>& ghcnLatLongMap);

  void ProcessOptions(int argc, char **argv);

  void ReadMetadataFile(std::set<size_t>& wmoCompleteIdSet,
                        std::map<size_t,GHCNLatLong>& ghcnLatLongMap);

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<size_t>& wmoCompleteIdSet,
                           std::map<size_t, GHCNLatLong>& ghcnLatLongMap);
  

  std::fstream* OpenCsvFile(const char* filename);
  std::fstream* OpenCsvStationCountFile(const char* filename);
 
  std::fstream* OpenKmlFile(const char* infilename, const char* csvfilename);

  void DumpRunInfo(std::fstream& fout);
  
  void DumpSpreadsheetHeader(std::fstream& fout);

  void DumpSmoothedResults(std::fstream& fout,
                           std::vector<GHCN*> ghcn, int nghcn);

  
  void DumpLatLongInfoAsKml(std::fstream& fout,
                            const std::map<size_t,GHCNLatLong>& latLongMap);
  
  void WriteKmlData(char** argv, std::vector<GHCN*>& ghcn, int igh);

  void WriteJavaScriptCode(char** argv, std::vector<GHCN*>& ghcn, int igh);
  
  
  wmoCompleteIdSet.clear();
  

  std::fstream* csvOut=NULL;
  std::fstream* csvStationCountOut=NULL;

  ReadMetadataFile(wmoCompleteIdSet,
                   ghcnLatLongMap);
  

  std::cerr << std::endl;
  std::cerr << "Smoothing filter length = " << avgNyear_g 
            << std::endl << std::endl; 
  
  std::cerr << "Will crunch " << argc-optind << " temperature data set(s). " 
            << std::endl;


  ghcn.resize(argc-optind);
  

  // Loop through the GHCN file command-line args.
  for(int igh=0; igh<argc-optind; igh++)
  {
    
    ghcn[igh] = new GHCN(argv[igh+optind],avgNyear_g);
    
    pGhcn_g=ghcn[igh];
    
    ghcn[igh]->SetCompleteWmoLatLongMap(ghcnLatLongMap);

    
    std::cerr << std::endl;
    std::cerr << "Reading data from " 
         << argv[igh+optind] << std::endl;

    // Read in temperature data and also finish "populating" the
    // ghcnLatLong map with station data info not available in
    // the metadata file (i.e. data start/stop years...)
    ReadTemperatureData(ghcn,igh,wmoCompleteIdSet,ghcnLatLongMap);
    
    // Add data-specific info to the WMO lat/long map
    // (i.e. info not available in the metadata file,
    // like data start/end years)
    ghcn[igh]->AddDataInfoToWmoLatLongMap();
    
    ghcn[igh]->CountWmoIds();

    // Single-result -- no ensemble processing
    std::cerr << "Computing baseline temps for " 
         << argv[igh+optind]<< std::endl;
    std::cerr << std::endl;
    ghcn[igh]->ComputeBaselines();

    ghcn[igh]->PrecomputeStationAnomalies();

    // Just a straight run with all stations...
    std::cerr << "Computing average anomalies for " 
         << argv[igh+optind] << std::endl;
    std::cerr << std::endl;
          
    headingStrBase.str("");
    headingStrBase<<argv[igh+optind]<<",";
    headingStr.str("");
    headingStr<<headingStrBase.str(); 
    ensembleElementHeadingStr_g.push_back(headingStr.str());
          
    if(true==bSparseStationSelect_g)
    {
            std::map<size_t, GHCNLatLong> selectedGhcnLatLongMap;

            ghcn[igh]->SelectStationsPerGridCell(gridSizeLatDeg_g,
                                                 gridSizeLongDeg_g,
                                                 ghcnLatLongMap,
                                                 selectedGhcnLatLongMap);
  
            ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                                  gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                  selectedGhcnLatLongMap);
    }
    else
    {
      if(SELECT_ALL==eStationSelectMode_g)
      {
          ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                                gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                ghcnLatLongMap);
      }
      else
      {
        std::map<size_t, GHCNLatLong> selectedGhcnLatLongMap;

        ghcn[igh]->SelectStationsGlobal(ghcnLatLongMap,selectedGhcnLatLongMap);

        ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                              gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                              selectedGhcnLatLongMap);

      }
      
    }
    
    std::cerr<<std::endl;
    std::cerr << argv[igh+optind]<<": Number of stations = "
              << ghcnLatLongMap.size() << std::endl;
    std::cerr<<std::endl;
    

    WriteKmlData(argv, ghcn, igh);

    WriteJavaScriptCode(argv, ghcn, igh);

  }
  
  std::cerr <<std::endl<<std::endl;
  std::cerr << "Writing results to " << csvFileName_g <<"..."
            << std::endl<<std::endl<<std::endl;

  
  
#if(1)
  std::cerr << "######## BEGIN CSV COLUMN HEADING DUMP ###########"<<std::endl;
  std::vector <std::string>::iterator istr;
  for(istr=ensembleElementHeadingStr_g.begin(); 
      istr!=ensembleElementHeadingStr_g.end();
      istr++)
  {
    std::cerr << *istr << std::endl;
  }
  std::cerr << "######### END CSV COLUMN HEADING DUMP ############"<<std::endl<<std::endl;
#endif

  csvOut=OpenCsvFile(csvFileName_g);
  if(csvOut==NULL)
  {
      std::cerr<<std::endl<<"Error opening output CSV file "
               <<csvFileName_g<<std::endl<<std::endl;
      exit(1);
  }

  DumpRunInfo(*csvOut);
  
  DumpSpreadsheetHeader(*csvOut);
  
  DumpSmoothedResults(*csvOut, ghcn, argc-optind);

  csvOut->close();
  delete csvOut;
  
  csvStationCountOut=OpenCsvStationCountFile(csvFileName_g);
  if(csvStationCountOut==NULL)
  {
      std::cerr<<std::endl<<"Error opening output companion station count file for "
               <<csvFileName_g<<std::endl<<std::endl;
      exit(1);
  }

  DumpRunInfo(*csvStationCountOut);
  
  DumpSpreadsheetHeader(*csvStationCountOut);
  
  DumpStationCountResults(*csvStationCountOut, ghcn, argc-optind);
  
  csvStationCountOut->close();
  delete csvStationCountOut;
  
  return 0;
  
}

void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                         std::set<size_t>& wmoCompleteIdSet,
                         std::map<size_t, GHCNLatLong>& ghcnLatLongMap)
{
      
  switch(nDataVersion_g)
  {
    case DATA_CRU:
      switch(nCruVersion_g)
      {
        case 3:
          ghcn[igh]->ReadCru3Temps(wmoCompleteIdSet);
          break;
            
        case 4:
          ghcn[igh]->ReadCru4Temps(); // wmoCompleteIdSet);
          // For CRUTEM4 data, no separate metadata file.
          // Metadata already compiled by reading data file.
          // Need to extract the internal station lat/long list,
          // put it into ghcnLatLongMap, and then
          // pass it back in to the GHCN object below.
          // (For other data formats, lat/long lists are compiled
          // in an external "read metadata" fcns, so we already
          // have the ghcnLatLongMap for those cases). 
          // If we don't do this for this case, we'll overwrite
          // the station metadata list with an uninitialized 
          // ghcnLatLongMap.
          ghcnLatLongMap=ghcn[igh]->GetCompleteWmoLatLongMap();
          break;
      }
      break;
        
    case DATA_BEST:
      ghcn[igh]->ReadBestTempsAndMergeStations();
      break;

      case DATA_USHCN:
          ghcn[igh]->ReadUshcnTemps();
          break;
          
      case DATA_GHCN:
      default:
          switch(nGhcnVersion_g)
          {
              case 2:
                  ghcn[igh]->ReadGhcnV2TempsAndMergeStations(wmoCompleteIdSet);
                  break;
                  
              default:
                  ghcn[igh]->ReadGhcnV4Temps(wmoCompleteIdSet);
                  break;
          }
          
  }

}


void WriteKmlData(char** argv, std::vector<GHCN*>& ghcn, int igh)
{
  std::fstream* OpenKmlFile(const char* inFileName, const char* csvFileName);

  void DumpLatLongInfoAsKml(std::fstream& fout,
                            const std::map<size_t,GHCNLatLong>& latLongMap);
  

  if(true==bGenKmlFile_g)
  {
    std::fstream* kmlOut;
    kmlOut=OpenKmlFile(argv[igh+optind],csvFileName_g);
    if(kmlOut==NULL)
    {
      std::cerr<<std::endl<<"Error opening output KML file "
               <<csvFileName_g<<std::endl<<std::endl;
      exit(1);
    }
    
    std::cerr << argv[igh+optind]<<": Number of stations processed = "
              << ghcn[igh]->GetProcessedStationLatLongMap().size() << std::endl;
    
    DumpLatLongInfoAsKml(*kmlOut,ghcn[igh]->GetProcessedStationLatLongMap());
    
    kmlOut->close();
    
  }
}

void WriteJavaScriptCode(char** argv, std::vector<GHCN*>& ghcn, int igh)
{
  std::fstream* OpenJavaScriptFile(const char* inFileName, const char* csvFileName);
  void DumpJavaScriptCode(std::fstream& fout,
                          const std::map<size_t,GHCNLatLong>& latLongMap);

  if(true==bGenJavaScriptFile_g)
  {
    std::fstream* jsOut;
    jsOut=OpenJavaScriptFile(argv[igh+optind],csvFileName_g);
    if(jsOut==NULL)
    {
      std::cerr<<std::endl<<"Error opening output JavaScript file for "
               <<csvFileName_g<<std::endl<<std::endl;
      exit(1);
    }
    
    std::cerr << argv[igh+optind]<<": Number of stations processed = "
              << ghcn[igh]->GetProcessedStationLatLongMap().size() << std::endl;
    
    DumpJavaScriptCode(*jsOut,ghcn[igh]->GetProcessedStationLatLongMap());

    jsOut->close();
    
  }
}
    
bool ReadCommandMessage(SimpleTcpSocket* pClientConnectSocket,
                        std::string& strMsg)
{
  bool bHtmlMode=false;

  char cDataBuf[2048];  // way more than big enough
  int nDataBuflen=2048;

  int nBytesRecv=0;
  int lenUnstripped;
  int lenStripped;

  std::string StripHtml(const std::string& strHtml, 
                        const std::string& strDelim);

  memset(cDataBuf,'\0',nDataBuflen);
  
  // Keep reading data until we get both start '$' and end '*'
  // delimiters.
  while((strMsg.find("<GHCN_COMMAND>")==std::string::npos) ||
        (strMsg.find("</GHCN_COMMAND>")==std::string::npos))
    {
      memset(cDataBuf,'\0',nDataBuflen);
      
      nBytesRecv+=pClientConnectSocket->RecvData(cDataBuf,nDataBuflen-nBytesRecv);
      
      strMsg+=std::string(cDataBuf);
      
    }
  
  lenUnstripped=strMsg.length();
  
  strMsg=StripHtml(strMsg,"GHCN_COMMAND");

  lenStripped=strMsg.length();
  
  if(lenUnstripped>lenStripped)
    bHtmlMode=true;
  else
    bHtmlMode=false;

  pClientConnectSocket->CloseSocket(); // RWM 2014/10/28 -- accommodate updated
  //                                      firefox tcp connection behavior.
  
  return bHtmlMode;
}

std::string StripHtml(const std::string& strHtml, const std::string& strDelim)
{
  std::string strCommand;
  std::string strDelimBegin;
  std::string strDelimEnd;
  
  size_t iStart=0;
  size_t iEnd=0;
  
  strDelimBegin=std::string("<")+strDelim+std::string(">");
  strDelimEnd=std::string("</")+strDelim+std::string(">");
  

  if((strHtml.find(strDelimBegin)!=std::string::npos) &&
     (strHtml.find(strDelimEnd)!=std::string::npos))
  {
    iStart=strHtml.find(strDelimBegin)+std::string(strDelimBegin).length();
    iEnd=strHtml.find(strDelimEnd);
  }
  else
  {
    return strHtml;
  }
  
  strCommand=strHtml.substr(iStart,iEnd-iStart);
  
  return strCommand;
}



std::string GenHtmlResponse(void)
{
  
        std::stringstream sstrHeaderHtml;
        
        std::stringstream sstrBodyHtml;
        
        std::string strHtml;
        
//      sstrHtml<<"HTTP/1.0 200 OK\n";
        sstrBodyHtml<<"<!DOCTYPE html>\n";
        sstrBodyHtml<<"<html>\n";
        sstrBodyHtml<<"<body>\n";
        sstrBodyHtml<<"</body>\n";
        sstrBodyHtml<<"</html>\n";
        
        sstrHeaderHtml<<"HTTP/1.0 200 OK\n";
        sstrHeaderHtml<<"Content-Type: text/html\n";
        sstrHeaderHtml<<"Content-Length:  "<<sstrBodyHtml.str().length()<<"\n";
        

        strHtml=sstrHeaderHtml.str()+sstrBodyHtml.str();

        return strHtml; 
}

void DumpLatLongInfoAsKml(std::fstream& fout,
                          const std::map<size_t,GHCNLatLong>& latLongMap)
{
  
  std::string strBoilerplate1="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  std::string strBoilerplate2="<kml xmlns=\"http://www.opengis.net/kml/2.2\">";
  
  std::map<size_t,GHCNLatLong>::const_iterator im;
  
  fout<<strBoilerplate1;
  fout<<strBoilerplate2;
  
  fout<<"<Document>"<<std::endl;
  fout<<"<Folder>"<<std::endl;
  
  for(im=latLongMap.begin(); im!=latLongMap.end(); im++)
  {
    fout<<"<Placemark>"<<std::endl;

    std::stringstream sstrInfo;
    std::string strInfo;

    sstrInfo<<"Station ID: "<<im->second.strWmoId<<"\n"
            <<"Station Name: "<<im->second.strStationName<<"\n"
            <<im->second.first_year<<"/"<<im->second.last_year<<"/"
            <<im->second.num_years<<"  "<<im->second.strStationType;

    // Original version.
    // sstrInfo<<im->first<<"  "<<im->second.strStationName<<":  "
    //         <<im->second.first_year<<"/"<<im->second.last_year<<"/"
    //         <<im->second.num_years<<"  "<<im->second.strStationType;
    strInfo=sstrInfo.str();
    

    // '&' chars cause the GoogleEarth xml parser to gag,
    // so remove them.
    std::string::iterator iss;
    for(iss=strInfo.begin(); iss!=strInfo.end(); iss++)
    {
      if(*iss=='&')
      {
        *iss=' ';
      }
    }

    fout<<"       <name>"<<strInfo<<"</name>"<<std::endl;
//    fout<<"     <name>"<<im->first<<"</name>"<<std::endl;

    fout<<"     <description>"<<im->first<<"</description>"<<std::endl;
    fout<<"     <Point>"<<std::endl;
    // Dont forget to translate back to standard coords
    fout<<"          <coordinates>"<<im->second.long_deg-180<<","<<im->second.lat_deg-90
        <<"</coordinates>"<<std::endl;
    fout<<"     </Point>"<<std::endl;
    fout<<"     <Style>"<<std::endl;
    fout<<"     <IconStyle>"<<std::endl;
    fout<<"        <color>ff0000ff</color>"<<std::endl;
    fout<<"        <scale>3.0</scale>"<<std::endl;
    fout<<"        <Icon>"<<std::endl;
    fout<<"         <href>http://maps.google.com/mapfiles/kml/paddle/wht-blank.png</href>"<<std::endl;
    fout<<"        </Icon>"<<std::endl;
    fout<<"     </IconStyle>"<<std::endl;
    fout<<"     </Style>"<<std::endl;
    fout<<"</Placemark>"<<std::endl;
  }

  fout<<"</Folder>"<<std::endl;
  fout<<"</Document>"<<std::endl;

  fout<<"</kml>"<<std::endl;
  
  return;
  
}

void DumpJavaScriptCode(std::fstream& fout,
                          const std::map<size_t,GHCNLatLong>& latLongMap)
{
  // Not very efficient, but what the hell...
  int ic;
  int nel;
  std::map<size_t,GHCNLatLong>::const_iterator im;

  nel=latLongMap.size();

  fout<<"{ \n";

  fout<<"lat=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.lat_deg-90; // Don't forget to translate back to original coords
    if(ic<nel-1)
      fout<<","; 
    
    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";
    
  }
  fout<<"];\n";
  
  fout<<"lon=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.long_deg-180; // Don't forget to translate back to original coords
    if(ic<nel-1)
       fout<<","; 

  // Friendlier formatting for easier viewing in text editors
       if((ic+1)%20==0)
         fout<<"\n";

  }
  fout<<"];\n";

  fout<<"alt=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.altitude; 
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  std::string strStationName;
  size_t ii;

  fout<<"nam=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    strStationName=im->second.strStationName;
    for(ii=0; ii<strStationName.length(); ii++)
    {
      // & char is potentially problematic w GoogleEarth/GoogleMaps
      // get rid of it
      if(strStationName[ii]=='&')
        strStationName[ii]=' ';
    }
    fout<<"\""<<im->second.strStationName<<"\""; // Don't forget to translate back to original coords
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";
  

  fout<<"startyr=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.first_year; // Don't forget to translate back to original coords
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  fout<<"endyr=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.last_year;
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  fout<<"length=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.num_years;
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  fout<<"airpt=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->second.isNearAirport;
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  fout<<"urban=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {
    int iurban=0;
    
    if(im->second.strStationType==std::string("RURAL"))
      iurban=1;
    else if(im->second.strStationType==std::string("SUBURBAN"))
      iurban=2;
    else if(im->second.strStationType==std::string("URBAN"))
      iurban=3;
    
    fout<<iurban;
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  fout<<"wmoid=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

    fout<<im->first;
    if(ic<nel-1)
      fout<<","; 

    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";

  }
  fout<<"];\n";

  // Begin yearsegs 2013/02/21
  std::vector<std::pair <int, int> >::const_iterator iseg;
  fout<<"yearseg=[\n";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++, ic++)
  {
    int isegCount=0;
    int nSeg=im->second.year_seg.size();

    if(nSeg>1)
    {
      std::cerr<<"########## WMOID="<<im->first<<"    #Segs="<<nSeg<<std::endl;
    }

      fout<<" [";
      for(iseg=im->second.year_seg.begin(); iseg!=im->second.year_seg.end(); iseg++)
      {
        fout<<"["<<iseg->first<<","<<iseg->second<<"]";
        if(isegCount<nSeg-1)
          fout<<",";
        isegCount++;
      }
      fout<<"]";

      if(ic<nel-1)
        fout<<",";

      fout<<"\n";
      
    // Friendlier formatting for easier viewing in text editors
    // if((ic+1)%20==0)
    //   fout<<"\n";

  }
  fout<<"]; \n";
  
  fout<<"};\n";
  
}


void ProcessOptions(int argc, char **argv)
{

  int optRtn;

  void ParseMaxReportingYearStr(void);
  void ParseGridSizeStr(void);
  void ParseRecordSpecsStr(char *optArg);
  void DumpUsage(char *argv0);

  if(argc<2)
  {
    DumpUsage(argv[0]);
    exit(1);
  }
  std::cerr << std::endl;
  
  minBaselineSampleCount_g=GHCN::DEFAULT_MIN_BASELINE_SAMPLE_COUNT;
  avgNyear_g=GHCN::DEFAULT_AVG_NYEAR;
  metadataFile_g=NULL;

  
  gridSizeLatDeg_g=GHCN::DEFAULT_GRID_LAT_DEG;
  gridSizeLongDeg_g=GHCN::DEFAULT_GRID_LONG_DEG;

  // Include stations only within this distance to nearest airport.
  // If <0, process stations greater than the abs value
  // If the default GHCN::IMPOSSIBLE_VALUE, don't filter by airport status.
  fAirportDistKm_g=GHCN::IMPOSSIBLE_VALUE;
  
  
  nUshcnEstimatedFlag_g=0;
  
  while ((optRtn=getopt(argc,argv,"gGcCbuUWsjkEa:A:B:M:S:o:t:P:R:L:"))!=-1)
  {
    switch(optRtn)
    {

      case 'A':
        avgNyear_g=atoi(optarg);
        break;
        
      case 'B':
        minBaselineSampleCount_g=atoi(optarg);
        break;
        
      case 'M':
        metadataFile_g=optarg;
        break;
        
      case 'S':
        ParseGridSizeStr(); // Want to parse out the lat/long grid-size values here
        break;
        
      case 's':
        bSparseStationSelect_g=true;
        eStationSelectMode_g=SELECT_GREATEST_DURATION;
        break;
        
      case 'R':
        bSetStationRecordSpecs_g=true;
        ParseRecordSpecsStr(optarg);
        break;
        
      case 'W':
        bAreaWeighting_g=true;
        break;;
        
      case 'o':
        csvFileName_g=optarg;
        break;

      case 'j':
        bGenJavaScriptFile_g=true;
        break;

      case 'k':
        bGenKmlFile_g=true;
        break;
        
      case 'g':
        nGhcnVersion_g=2;
        nDataVersion_g=DATA_GHCN;
        break;
        
      case 'G':
        nGhcnVersion_g=4;
        nDataVersion_g=DATA_GHCN;
        break;

      case 'c':
        nCruVersion_g=3;
        nDataVersion_g=DATA_CRU;
        break;

      case 'C':
        nCruVersion_g=4;
        nDataVersion_g=DATA_CRU;
        break;

      case 'b':
        nDataVersion_g=DATA_BEST;
        break;

        case 'U':
        case 'u':
            nDataVersion_g=DATA_USHCN;
            break;

        case 'E':
            nUshcnEstimatedFlag_g=1;
            break;

        case 'a':
            // Include stations only within this distance to nearest airport.
            // If <0, then don't filter by distance to airport.
            fAirportDistKm_g=atof(optarg);
            break;
            
      case 't':
        // Rural/Urban/Suburban designator (GHCN only)
          cStationType_g=std::string(optarg);
        break;
        
      case 'P':
        nServerPort_g=atoi(optarg);
        break;
        
      case 'L':
        nPythonByteSize_g=atoi(optarg);
        break;
        
      default:
        std::cerr << std::endl;
        DumpUsage(argv[0]);
        exit(1);
    }
  }
  avgNyear_g=MAX(1,MIN(GHCN::MAX_AVG_NYEAR,avgNyear_g));
  minBaselineSampleCount_g=MAX(1,
     MIN(GHCN::LAST_BASELINE_YEAR-GHCN::FIRST_BASELINE_YEAR+1,
         minBaselineSampleCount_g));
      

  if(true==bSetStationRecordSpecs_g)
  {
      if((nDataVersion_g!=DATA_GHCN)&&(nDataVersion_g!=DATA_USHCN))
      {
          std::cerr<<"The -R option is currently supported only for GHCN V3 data."<<std::endl;
          std::cerr<<std::endl;
          exit(1);
      }
  }
  
   
  if(argc-optind<=0)
  {
    std::cerr << std::endl;
    std::cerr << "Did you forget to supply a data file name? " << std::endl;
    std::cerr << std::endl;
    exit(1);
  }
  
}


void ParseGridSizeStr()
{
  char *tok=NULL;
  
  gridSizeStr_g=optarg;
  
  tok=strtok(gridSizeStr_g,":");
  if(NULL==tok)
  {
    std::cerr<<std::endl;
    std::cerr<<"Error specifying grid size: format is Lat:Long"<<std::endl;
    std::cerr<<std::endl;
    exit(1);
  }
  gridSizeLatDeg_g=atof(tok);

  std::cerr << std::endl;
  std::cerr << "Grid size (lat) = " << gridSizeLatDeg_g << std::endl;
  std::cerr << std::endl;
  
  if((gridSizeLatDeg_g<GHCN::MIN_GRID_LAT_DEG)||
     (gridSizeLatDeg_g>GHCN::MAX_GRID_LAT_DEG))
  {
    std::cerr << "Error - valid grid lat size range: "
              <<GHCN::MIN_GRID_LAT_DEG<<"-"
              <<GHCN::MAX_GRID_LAT_DEG<<" deg. " 
              << std::endl;
    exit(1);
  }

  tok=strtok(NULL,":");

  if(NULL==tok)
  {
    std::cerr<<std::endl;
    std::cerr<<"Error specifying grid size: format is Lat:Long"<<std::endl;
    std::cerr<<std::endl;
    exit(1);
  }
  
  gridSizeLongDeg_g=atof(tok);
  std::cerr << std::endl;
  std::cerr << "Grid size (long) = " << gridSizeLongDeg_g << std::endl;
  std::cerr << std::endl;

  if((gridSizeLongDeg_g<GHCN::MIN_GRID_LONG_DEG)||
     (gridSizeLatDeg_g>GHCN::MAX_GRID_LONG_DEG))
  {
    std::cerr << "Error - valid grid long size range: " 
              <<GHCN::MIN_GRID_LONG_DEG<<"-"
              <<GHCN::MAX_GRID_LONG_DEG<<" deg. " 
              << std::endl;
    exit(1);
  }

  return;
  
}

void ParseRecordSpecsStr(char *optarg)
{
  std::string strRecordLength;
  char *tok;
  bool bValidFormat=true;
  size_t nStrPos;
  size_t nRevStrPos;
  
  bSetStationRecordSpecs_g=true;
  
  // FirstYear:LastYear
  strRecordLength=std::string(optarg);
  nStrPos=strRecordLength.find(':');
  nRevStrPos=strRecordLength.rfind(':');

  if(nStrPos!=nRevStrPos)
  {
      std::vector<std::string> vStrRecs;
      boost::split(vStrRecs,strRecordLength,boost::is_any_of(":"));
      if(vStrRecs.size()==3)
      {
          // Have max_start_year,min_rec_length,min_start_year
          try
          {
              eStationSelectMode_g=SELECT_MAX_START_MIN_END_DATE_MIN_DURATION;
              bMaxStationRecordStartYear_g=true;
              nMaxStationRecordStartYear_g=boost::lexical_cast<int>(vStrRecs.at(0));
              bMinStationRecordLength_g=true;
              nMinStationRecordLength_g=boost::lexical_cast<int>(vStrRecs.at(1));
              bMinStationRecordEndYear_g=true;
              nMinStationRecordEndYear_g=boost::lexical_cast<int>(vStrRecs.at(2));
          }
          catch(...)
          {
              std::cerr<<"###################################################################################"<<std::endl;
              std::cerr<<__FILE__<<":"<<__LINE__<<"(): (boost::split) Invalid station rec specification "<<strRecordLength<<std::endl;
              std::cerr<<"###################################################################################"<<std::endl;
              exit(1);
          }
      }
      else
      {
          std::cerr<<"#########################################################################"<<std::endl;
          std::cerr<<__FILE__<<":"<<__LINE__<<"(): Invalid station rec specification "<<strRecordLength<<std::endl;
          std::cerr<<"#########################################################################"<<std::endl;
          exit(1);
      }
  }
  else
  {
      // No ':' delimitor
      // So this is just a minimum record length 
      // (no specified beginning/end times)
      if(nStrPos==std::string::npos)
      {
          tok=strtok(optarg," ");
          if(NULL==tok)
          {
              bValidFormat=false;
          }
          else
          {
              eStationSelectMode_g=SELECT_MIN_DURATION;
              bMinStationRecordLength_g=true;
              nMinStationRecordLength_g=atoi(tok);
              std::cerr<<std::endl;
              std::cerr<<"Processing only stations with record len >= "
                       <<nMinStationRecordLength_g<<std::endl;
              std::cerr<<std::endl;
          }
      }
      else if(nStrPos==0)
      {
          // Only have end year specified
          
          // skip past the':' char
          tok=strtok(optarg+1," ");
          if(NULL==tok)
          {
              bValidFormat=false;
          }
          else
          {
              eStationSelectMode_g=SELECT_MIN_END_DATE;
              bMinStationRecordEndYear_g=true;
              nMinStationRecordEndYear_g=atoi(tok);
              std::cerr<<std::endl;
              std::cerr<<"Processing only stations with records that end on/after "
                       <<nMinStationRecordEndYear_g<<std::endl;
              std::cerr<<std::endl;
          }
      }
      else if(nStrPos==strRecordLength.length()-1)
      {
          // Only have start year specified
          //
          tok=strtok(optarg,":");
          if(NULL==tok)
          {
              bValidFormat=false;
          }
          else
          {
              eStationSelectMode_g=SELECT_MAX_START_DATE;
              bMaxStationRecordStartYear_g=true;
              nMaxStationRecordStartYear_g=atoi(tok);
              std::cerr<<std::endl;
              std::cerr<<"Processing only stations with records that begin on/before "
                       <<nMaxStationRecordStartYear_g<<std::endl;
              std::cerr<<std::endl;
          }
      }
      else
      {
          // Have first year and last year
          // Get first year
          tok=strtok(optarg,":");
          if(NULL==tok)
          {
              bValidFormat=false;
          }
          else
          {
              bMaxStationRecordStartYear_g=true;
              nMaxStationRecordStartYear_g=atoi(tok);
          }
    
          // Get last year
          tok=strtok(NULL,":");
          if(NULL==tok)
          {
              bValidFormat=false;
          }
          else
          {
              bMinStationRecordEndYear_g=true;
              nMinStationRecordEndYear_g=atoi(tok);
              eStationSelectMode_g=SELECT_MAX_START_MIN_END_DATE;
              std::cerr<<std::endl;
              std::cerr<<"Processing only stations with records that begin on/before "
                       <<nMaxStationRecordStartYear_g<<std::endl;
              std::cerr<<"and end on/after "
                       <<nMinStationRecordEndYear_g<<std::endl;
              std::cerr<<std::endl;
          }
      }
  }


  if(false==bValidFormat)
  {
      std::cerr<<std::endl;
      std::cerr<<"Invalid record specification: format is one of: "<<std::endl;
      std::cerr<<"     BeginYear:EndYear, :EndYear, BeginYear:, or NumYears"<<std::endl;
      std::cerr<<std::endl;
      exit(1);
      
  }

  return;
  
}

void DumpUsage(char *argv0)
{
  std::cerr << std::endl;
  std::cerr << "DISCLAIMER -- This info is not necessarily up to date."<<std::endl;
  
  std::cerr << std::endl 
            << "Usage: " << argv0  << "  \\ " << std::endl
            << "         -M (char*)temp-station-metadata-file \\" << std::endl
            << "         -o (char*)csv-output-file \\" << std::endl
            << "         [-j] " <<std::endl
            << "         [-k] " <<std::endl
            << "         [-gGcCBuUE] " << std::endl
            << "         [-t A/B/C] for rural/mixed/urban station type to process \\" << std::endl
            << "         [-A (int)smoothing-filter-length-years] \\ " << std::endl
            << "         [-B (int)min-baseline-sample-count] \\ "     << std::endl
            << "         [-S grid-size-lat-deg:grid-size-long-deg] \\ " << std::endl
            << "         [-W] [-s] \\ " << std::endl
            << "         [-R  (char*)StationRecordSpec] \\ " << std::endl 
            << "         [-P (int)server-listening-port] \\ " <<std::endl
            << "         [-L (int)network-char-length ] (no longer needed)\\ " << std::endl
            << "         (char*)data-source1 (char*)data-source2... " << std::endl
            << "         For GHCN, CRU3, and BEST data, the data-sourceX sources are files."<<std::endl
            << "         For USHCN and CRU4 data, the data-sourceX sources are directories"<<std::endl
            << "         that contain USHCN or CRU4 temperature data files."<<std::endl;
  std::cerr << std::endl;

  std::cerr << "   Options/Data-filenames can be intermingled (possibly not on MacOS)."<<std::endl;
  std::cerr << std::endl;

 
  std::cerr << "The -M flag (mandatory except for HADCrut4): " << std::endl << std::endl;
  std::cerr << "     Specifies the metadata file name. " << std::endl << std::endl;
  std::cerr << "     The metadata-file must be the appropriate metadata format." << std::endl;
  std::cerr << "     For GHCN v4 metadata, must used Jim Java's supplentary GHCNv4 metadata file,"<<std::endl;
  std::cerr << "     available at:"<<std::endl;
  std::cerr << "     https://raw.githubusercontent.com/priscian/climeseries/master/inst/extdata/instrumental/GHCN/v4.temperature.inv%2Bpopcss.csv"<<std::endl;
  std::cerr << "     Copy and paste to your web-browser URL bar."<<std::endl;
  std::cerr << std::endl; 
  
  std::cerr << std::endl << std::endl;
  std::cerr << "The -o flag (mandatory except for server-mode): " << std::endl << std::endl;
  std::cerr << "     Specifies the output .csv file. The .csv file contains the spreadsheet plottable "<< std::endl
       << "     temperature anomaly results." << std::endl;

  std::cerr << std::endl << std::endl;
  std::cerr << "The -j flag: " << std::endl << std::endl;
  std::cerr << "     Generate a JavaScript (.js) file. The .js file name will be a composite of the "<< std::endl
            << "     corresponding data file name and .csv file name -- used with the new Google Map frontend." << std::endl;

  std::cerr << std::endl << std::endl;
  std::cerr << "The -k flag: " << std::endl << std::endl;
  std::cerr << "     Generate a GoogleEarth .kml file. The .kml file name will be a composite of the "<< std::endl
            << "     corresponding data file name and .csv file name." << std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << "Only one of -g, -G -c, -C, -b, or -U may be specified. " <<std::endl;
  std::cerr << std::endl;
  std::cerr << "     The '-g' or '-G' flag specifies GHCN version 2 or 4 data, respectively. " 
            << std::endl
            << "     The '-c' or '-C' flag specifies CRU v3 or v4 \"climategate\" data, respectively. " 
            << std::endl
            << "     The '-b' flag specifies BEST data. " << std::endl;
  std::cerr << "     The '-u' or '-U' flag specifies USHCN data. "<<std::endl;
  std::cerr << "     The default value is -G (for GHCN V4). " <<std::endl;
  std::cerr << std::endl;
  std::cerr << "The '-E' flag (optional): "<<std::endl;
  std::cerr << "     Enables the use of estimated temps in USHCN data."<<std::endl;
  std::cerr << "     (The default is not to use estimated USHCN temp data)"<<std::endl;
  std::cerr << std::endl << std::endl;

// std::cerr << "     (Metadata file available at ftp://ftp.ncdc.noaa.gov/pub/data/ghcn/v2/v2.temperature.inv) " 
  //        << std::endl 
  //        << std::endl;

  std::cerr << "The -t flag (GHCN only): "<<std::endl;
  std::cerr << "    Specifies rural/urban/mixed/all stations. " << std::endl
            << "    'A' for rural, 'B' for mixed, 'C' for urban, 'X' for all." << std::endl
            << "    Default is 'X' (all) " <<std::endl;
  std::cerr<<std::endl<<std::endl;
  
  std::cerr << "The -A option: " << std::endl << std::endl;
  std::cerr << "     Specifies the moving-average smoothing filter length in years." << std::endl;
  std::cerr << "     (Default length: " << GHCN::DEFAULT_AVG_NYEAR << ")"<<std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << "The -B option: " << std::endl << std::endl;
  std::cerr << "     Specifies the minimum #samples a station must report during the " << std::endl
            << "     "<<GHCN::FIRST_BASELINE_YEAR<<"-"<<GHCN::LAST_BASELINE_YEAR
            <<" baseline period to be included in the anomaly calculations. "<<std::endl
            <<"     (Default value: "<< GHCN::DEFAULT_MIN_BASELINE_SAMPLE_COUNT <<")"<<std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << "The -S option: " << std::endl << std::endl;
  std::cerr << "     Specifies the grid size in degrees lat and degrees longitude at the equator. " 
            << std::endl
            << "     Grid size values can be floating pt numbers: i.e. '-S 5.5:6.5'. " << std::endl
            << "     The grid longitude size will be adjusted automatically to keep the grid " << std::endl
            << "     cell areas approximately equal as you move away from the equator (in latitude). " 
            << std::endl
            << "     (Default Lat/Long values: "<<GHCN::DEFAULT_GRID_LAT_DEG<<"/"
            <<GHCN::DEFAULT_GRID_LONG_DEG
            <<")"<<std::endl;
  std::cerr <<std::endl<<std::endl;
  std::cerr <<"The -a option: "<<std::endl<<std::endl;
  std::cerr <<"    Airport status option: "<<std::endl;
  std::cerr <<"    0: process non-airport stations only."<<std::endl;
  std::cerr <<"    1: process airport stations only."<<std::endl;
  std::cerr <<"    2: process all (airport & non-airport) stations."<<std::endl;
  std::cerr << std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << " The -s option: "<<std::endl<<std::endl;
  std::cerr << "      Enable sparse station selection (1 station per grid-cell) " << std::endl; 
  std::cerr << std::endl << std::endl;
  std::cerr << " The -R option: "<<std::endl<<std::endl;
  std::cerr << "      Process data from stations with a records spec'd per one of the following: " << std::endl;
  std::cerr << "        StationRecordSpec: nYears -- temp. record length must be at least nYears long."<<std::endl;
  std::cerr << "        StationRecordSpec: StartYear: -- temp. record must start no later than StartYear."<<std::endl;
  std::cerr << "        StationRecordSpec: :EndYear -- temp. record must end no earlier than EndYear."<<std::endl;
  std::cerr << "        StationRecordSpec: StartYear:EndYear -- temp. record must start no later than StartYear."<<std::endl;
  std::cerr << "                                                and end no earlier than EndYear."<<std::endl;
  std::cerr << " Fully functional only with GHCN V3 data."<<std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << "      Incompatible with the -s option."<<std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << " The -W option:  "<<std::endl<<std::endl;
  std::cerr << "       Enable corrections for grid-cell area variations " << std::endl;
  std::cerr << "       Combining -s and -W options not recommended." <<std::endl;
  std::cerr << std::endl << std::endl;
  std::cerr << "The -P option: " << std::endl << std::endl;
  std::cerr << "     For server-mode: Specifies the listening port -- use with hacked QGIS. "<<std::endl;
  std::cerr << "     Server-mode is supported for GHCN V3 data only. " <<std::endl;
  std::cerr << std::endl << std::endl;
  
  std::cerr << std::endl;

  std::cerr << "Example (for GHCN V2 data): "<< std::endl << std::endl;
  std::cerr << "  ./anomaly.exe -g -M v2-md.dat -B 20 -S 10.5:15.5 -A 5 v2.mean v2.mean_adj -o output.csv " << std::endl << std::endl
       << "      Processes v2.mean and v2.mean_adj GHCN V2 files. " << std::endl << std::endl
       << std::endl 
       << "      -M: Reads GHCN v2-formatted metadata file v2-md.dat. " << std::endl
       << std::endl
       << "      -B: Require at least 20 years of valid data during the baseline period " << std::endl
       << std::endl
       << "      -A: Outputs will be smoothed with a moving-average filter of length 5 years. " << std::endl
       << std::endl 
       << "      -S: Initial (at the equator) grid-cell size will be 10.5 deg lat, 15.5 deg long. " << std::endl
       << std::endl
       << "      -o: Data will be written to output.csv in csv, with 1 column per set of global annual " << std::endl
       << "          anomalies. " << std::endl;

  
  std::cerr << std::endl << std::endl;

  std::cerr << "To page this message for easier reading, do this: " << std::endl << std::endl
       << "     "<<argv0<<" 2>&1 | less " << std::endl << std::endl;
  
  return;
    
}


void ReadMetadataFile(std::set<size_t>& wmoCompleteIdSetOut,
                      std::map<size_t,GHCNLatLong>& ghcnLatLongMapOut)
{
  void ReadGhcnMetadataFile(const char* infile,
                            const std::string& cStationType,
                            std::set<size_t>& wmoCompleteIdSetOut,
                            std::set<size_t>& wmoGhcnV2DeprecatedIdSet,
                            std::map<size_t,GHCNLatLong>& latLongMap);

  void ReadGhcnMetadataFileV3(const char* infile,
                              const std::string& cStationType,
                              std::set<size_t>& wmoCompleteIdSetOut,
                              std::map<size_t,GHCNLatLong>& latLongMap);
  // Reads standard GHCNv4 metadata file.
  void ReadGhcnMetadataFileV4(const char* infile,
                              const std::string& cStationType,
                              std::set<size_t>& wmoCompleteIdSetOut,
                              std::map<size_t,GHCNLatLong>& latLongMap);

  // Reads extended GHCNv4 metadata file with rural/mixed/urban classifications.
  // https://raw.githubusercontent.com/priscian/climeseries/master/inst/extdata/instrumental/GHCN/v4.temperature.inv%2Bpopcss.csv
  void ReadGhcnMetadataFileV4b_BU(const char* infile,
                               const std::string& cStationType,
                               const float& fAirportDistKm, // Max distance to nearest airport. If<0 ignore
                               std::set<size_t>& wmoCompleteIdSetOut,
                               std::map<size_t,GHCNLatLong>& latLongMap);

  // Process stations with brightness index >= nBrightnessIndex
  void ReadGhcnMetadataFileV4b2(const char* inFile,
                               const int& nBrightnessIndex, // satellite night brightness index.
                               std::set<size_t>& wmoIdSetOut,
                               std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut);

  void ReadCru3MetadataFile(const char* inFile,
                            std::set<size_t>& wmoCompleteIdSetOut,
                            std::map<size_t, GHCNLatLong >& ghcnLatLongMapOutMap);

  void ReadBestMetadataFile(const char* inFile,
                            std::set<size_t>& wmoCompleteIdSetOut,
                            std::map<size_t, GHCNLatLong >& bestLatLongMap);

  void ReadUshcnMetadataFile(const char* inFile,
                             const std::string& cStationType,
                             std::set<size_t>& wmoCompleteIdSetOut,
                             std::map<size_t, GHCNLatLong >& ghcnLatLongMapOutMap);

  // old GHCN V2 WMO id list that is no longer used
  // just read this in and throw away...
  std::set<size_t> wmoGhcnV2DeprecatedIdSet; 
  
  if(metadataFile_g!=NULL)
  {
    // GHCN V2/V3, HadCRUT3, and BEST
    // have separate metadata files.
    // HadCRUT4 doesn't.
    switch(nDataVersion_g)
    {
      case DATA_CRU:
        switch(nCruVersion_g)
        {
          case 3:
            ReadCru3MetadataFile(metadataFile_g,
                                 wmoCompleteIdSetOut,
                                 ghcnLatLongMapOut);
          default:
            ;
            // CRU4 -- no separate metadata file.
        }
        break;
        

      case DATA_BEST:
        ReadBestMetadataFile(metadataFile_g,
                             wmoCompleteIdSetOut,
                             ghcnLatLongMapOut);
        break;

        case DATA_USHCN:
            ReadUshcnMetadataFile(metadataFile_g,
                                  cStationType_g,
                                  wmoCompleteIdSetOut,
                                  ghcnLatLongMapOut);
            break;
            
      case DATA_GHCN:
      default:

        switch(nGhcnVersion_g)
        {
          case 2:
            ReadGhcnMetadataFile(metadataFile_g,
                                 cStationType_g,
                                 wmoGhcnV2DeprecatedIdSet, /* no longer used */
                                 wmoCompleteIdSetOut,/* was wmShortIdSet, */
                                 ghcnLatLongMapOut);
            break;

            case 3:
                ReadGhcnMetadataFileV3(metadataFile_g,
                                       cStationType_g,
                                       wmoCompleteIdSetOut,
                                       ghcnLatLongMapOut);
                break;
                
          default:
//              Original V4 metadata processing disabled 2025/06/22
#if(0)
              ReadGhcnMetadataFileV4(metadataFile_g,
                                     cStationType_g,
                                     wmoCompleteIdSetOut, 
                                     ghcnLatLongMapOut);
#endif
#if(1) // For priscian extended metadata w/airport info.
              ReadGhcnMetadataFileV4b_BU(metadataFile_g,
                                      cStationType_g,
                                      fAirportDistKm_g, // max distance to nearest airport. If <0, then
                                                        // don't filter by airport distance.
                                      wmoCompleteIdSetOut, 
                                      ghcnLatLongMapOut);
#endif
              break;
        }
        break;
    }
  }
  else
  {
    // All data formats except CRU V4 use a separate
    // metadata file (which must be specified).
    if((DATA_CRU!=nDataVersion_g)&&(4!=nCruVersion_g))
    {
      std::cerr << std::endl;
      std::cerr << "Must supply a GHCN, HadCRUT3, or BEST metadata file (command-line flag -M)" 
                << std::endl;
      std::cerr << std::endl;
      exit(1);
    }
    
  }
  return;
}
void DumpStationCountResults(std::fstream& fout,
                             std::vector<GHCN*> ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<size_t, float >::iterator iyy;
  int igh;
  
  // Iterate over years
  for(iyy=ghcn[0]->mAverageAnnualStationCountMap.begin();
      iyy!=ghcn[0]->mAverageAnnualStationCountMap.end();
      iyy++)
  {
    // Results for first input file
    // First 
    fout << iyy->first << ","
         << iyy->second;

    if(ngh>1)
      fout << ",";

    // Results for additional input files
    for(igh=1; igh<ngh; igh++)
    {
      if(ghcn[igh]->mAverageAnnualStationCountMap.find(iyy->first) != 
         ghcn[igh]->mAverageAnnualStationCountMap.end())
      {
        fout << ghcn[igh]->mAverageAnnualStationCountMap[iyy->first];
        // Don't need a trailing comma after the last field
        if(igh<ngh-1)
             fout << ",";
      }
      else
      {
        // No valid data for this year -- put in a blank/null csv placeholder
        // Also, don't need a trailing comma after the last field
        if(igh<ngh-1)
          fout << ",";
      }

    }
    fout << std::endl; // end of this csv line..

  }

  return;
  
}


void SigintHandler(int param)
{
  printf("\n######### User pressed Ctrl+C #########\n\n");

  if(pServerSocket_g!=NULL)
  {
      delete pServerSocket_g; // destructor closes socket cleanly
  }

  exit(1);
}

// ########### END OF GHCN CLASS METHODS #################




// For v4b_ABC.inv, 'A'=rural, 'B'=suburban, 'C'=urban
void ReadGhcnMetadataFile(const char* inFile,
                          const std::string& cStationType, // GHCNv3: R(ural), U(rban), S(uburban), or A(ll)
                                                   // GHCNv4: A(rural),  B(suburban ),  C(urban)
                          std::set<size_t>& wmoCompleteIdSetOut,  // remapped to wmoCompleteIdSetOut
                          std::set<size_t>& wmoGhcnV2DeprecatedIdSet, // old GHCN V2 wmo id's that are no longer used
                          std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[512];
  
  std::map<size_t,int> wmoCount;
  std::map<size_t,float> wmoLatSum;
  std::map<size_t,float> wmoLongSum;
  
  wmoCount.clear();
  
  ghcnLatLongMapOut.clear();
  wmoCompleteIdSetOut.clear();  // Make sure the list is empty
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);
  int ichar;
  char *cwmoId;
  char *cLat;
  char *cLong;
  
  size_t ssl;
  int ww;
  
  GHCNLatLong latLong;
  

  //ccountry = &inBuf[ichar];
  ichar=0;
  ichar+=3;
  cwmoId = &inBuf[ichar];

#if(0) // VERSION 3
  cLat=&inBuf[12]; // Latitude field location 
  
  cLong=&inBuf[21]; // Longitude field location
#endif

  cLat=&inBuf[43]; // Latitude field location 
  
  cLong=&inBuf[50]; // Longitude field location

  std::hash<std::string> str_hash;
  
  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  
  int icount=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);
      
      if(cStationType != "XXX") // "XXX" means process all stations
      {
          if(cStationType.find(inBuf[67])==std::string::npos) // Rural/Urban/Suburban designator -- V2 col 67.
        {
          continue;
        }
      }
      
      switch(inBuf[67])
      {
        case 'R':
          latLong.strStationType=std::string("RURAL");
          latLong.iStationType=1;
          break;
          
        case 'S':
          latLong.strStationType=std::string("SUBURBAN");
          latLong.iStationType=2;
          break;
          
        case 'U':
          latLong.strStationType=std::string("URBAN");
          latLong.iStationType=3;
          break;

        default:
          latLong.strStationType=std::string(" ");
          latLong.iStationType=0;
          
          break;
      }
      

      // Scan in complete station id (wmo#(5-digits) + 
      // modifier(3-digits)+duplicate(3-digits))
      char cssl[10],cww[10];
      
      sscanf(cwmoId, "%8s", &cssl[0]);  // complete station id: wmo-base-id+modifier
      sscanf(cwmoId, "%5s",&cww[0]);     // wmo-base-id only

      ssl=str_hash(std::string(cssl));
      ww=str_hash(std::string(cww));
    
      // sscanf(cwmoId, "%8ld", &ssl);  // complete station id: wmo-base-id+modifier
      // sscanf(cwmoId, "%5ld",&ww);     // wmo-base-id only
      
      sscanf(cLat,"%f",&latLong.lat_deg);
      sscanf(cLong,"%f",&latLong.long_deg);

      
      // Translate to positive values only (for easier location comparisons)
      latLong.lat_deg+=90;  
      latLong.long_deg+=180;
      
      wmoCompleteIdSetOut.insert(ww);
      wmoGhcnV2DeprecatedIdSet.insert(ssl);
      if(ghcnLatLongMapOut.find(ww)==ghcnLatLongMapOut.end())
      {
        std::string strStationName = std::string(&inBuf[12],30);
        int ichar;
        for(ichar=strStationName.length()-1; ichar>=0; ichar--)
        {
          if(strStationName[ichar]!=' ')
            break;
          else
            strStationName[ichar]='\0';
        }
        strStationName=static_cast<std::string>(strStationName.c_str());
        
        latLong.strStationName=strStationName;

        ghcnLatLongMapOut[ww]=latLong;
        wmoCount[ww]=1;
        wmoLatSum[ww]=latLong.lat_deg;
        wmoLongSum[ww]=latLong.long_deg;
      }
      else
      {
        wmoCount[ww]+=1;
        wmoLatSum[ww]+=latLong.lat_deg;
        wmoLongSum[ww]+=latLong.long_deg;
      }
      icount++;
      
    }
  }
  catch(...)
  {
    std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
    std::cerr<<std::endl;
  }

  
  // Now compute average lat/long for each wmo-base-id from the above sums
  std::map<size_t, GHCNLatLong >::iterator ighcn;
  size_t wmoid;
  for(ighcn=ghcnLatLongMapOut.begin(); ighcn!=ghcnLatLongMapOut.end(); ighcn++)
  {
    wmoid=ighcn->first;
    latLong.lat_deg=wmoLatSum[wmoid]/wmoCount[wmoid];
    latLong.long_deg=wmoLongSum[wmoid]/wmoCount[wmoid];
    ghcnLatLongMapOut[wmoid].wmo_id=wmoid;
    ghcnLatLongMapOut[wmoid].lat_deg=latLong.lat_deg;
    ghcnLatLongMapOut[wmoid].long_deg=latLong.long_deg;

  }

  if(instream!=NULL)
  {
    instream->close();
  }

  std::cerr << "Number of short WMO IDs = " << wmoCompleteIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  std::cerr << "Number of long WMO IDs = " << wmoGhcnV2DeprecatedIdSet.size() << std::endl;
  std::cerr << std::endl;
  
  
  std::cerr << "Now have a lat/long list with " << ghcnLatLongMapOut.size() 
            <<" unique short WMO ids" << std::endl;

  return;
  
}

void ReadGhcnMetadataFileV3(const char* inFile,
                            const std::string& cStationType,
                            std::set<size_t>& wmoIdSetOut,
                            std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[512];
  
  ghcnLatLongMapOut.clear();
  wmoIdSetOut.clear();
  
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
    return;
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);

  char *cwmoId;
  char *cLat;
  char *cLong;
  char *cAltitude;
  char *cAirport;
  
  GHCNLatLong latLong;
  

  cwmoId = &inBuf[0];

  cLat=&inBuf[12]; // Latitude field location 
  cLong=&inBuf[21]; // Longitude field location

  cAltitude=&inBuf[31]; // Altitude field location
  cAirport=&inBuf[87];
  
  
  
  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  size_t nWmoId=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);
      
      if(cStationType != "XXXX") // 'X' means process all stations
      {
          if(cStationType.find(inBuf[106])==std::string::npos) // Rural/Urban/Suburban designator -- V2 col 73.
        {
          continue;
        }
      }

      switch(inBuf[106])
      {
        case 'A':
          latLong.strStationType=std::string("RURAL");
          latLong.iStationType=1;
          break;
          
        case 'B':
          latLong.strStationType=std::string("SUBURBAN");
          latLong.iStationType=2;
          break;
          
        case 'C':
          latLong.strStationType=std::string("URBAN");
          latLong.iStationType=3;
          break;

        default:
          latLong.strStationType=std::string(" ");
          latLong.iStationType=0;
          break;

      }
      

      // Scan in complete station id excluding country code (wmo#(8-digits)) 
      // First 3 digits in each line are country code -- including them makes the 
      // wmo id too big to store in a 32-bit int. So skip 'em.

      char cWmoId0[10];
      sscanf(&cwmoId[3], "%8s",&cWmoId0[0]);
      std::hash<std::string> strHash;
      nWmoId=strHash(std::string(cWmoId0));
      
      //sscanf(&cwmoId[3], "%8d",&nWmoId);     
      
      sscanf(cLat,"%f",&latLong.lat_deg);
      sscanf(cLong,"%f",&latLong.long_deg);
      // Translate to positive values only (for easier location comparisons)
      latLong.lat_deg+=90;  
      latLong.long_deg+=180;

      sscanf(cAltitude, "%f", &latLong.altitude);
      
      if(cAirport[0]=='A')
        latLong.isNearAirport=1;
      else
        latLong.isNearAirport=2;
      
  
      std::string strStationName = std::string(&inBuf[38],30);
      int ichar;
      for(ichar=strStationName.length()-1; ichar>=0; ichar--)
      {
        if(strStationName[ichar]!=' ')
          break;
        else
          strStationName[ichar]='\0';
      }
      strStationName=static_cast<std::string>(strStationName.c_str());

      latLong.strStationName=strStationName;
      
      ghcnLatLongMapOut[nWmoId]=latLong;

      wmoIdSetOut.insert(nWmoId);
      
    }
  }
  catch(...)
  {
    std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
    std::cerr<<std::endl;
  }


  if(instream!=NULL)
  {
    instream->close();
  }

  std::cerr << std::endl;
  std::cerr << "Number of unique WMO IDs read from metadata file = " 
            << wmoIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  
  
  return;
  
}

void ReadGhcnMetadataFileV4b(const char* inFile,
                             const std::string & cStationTypeIn,
                             const float& fAirportDistKmIn, // max distance to nearest airport. If <0, ignoere
                             std::set<size_t>& wmoIdSetOut,
                             std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[512];
  
  ghcnLatLongMapOut.clear();
  wmoIdSetOut.clear();
  
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
    return;
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);

  char *cwmoId;
  size_t nWmoId=0;
  const char *cLat;
  const char *cLong;
  const char *cAltitude;
  const char *cStationName;
  std::string cStationType;
  std::string strStationNameConst;
  std::string strStationName;
  float fAirportDistKm;
  

  GHCNLatLong latLong;

  std::string strStream;
  std::vector<std::string> vStrTokens;

  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  nWmoId=0;
  try
  {
      while(!instream->eof())
      {
        
        instream->getline(inBuf,511);
        strStream=std::string(inBuf);


        try
        {
            boost::split(vStrTokens,strStream,boost::is_any_of(","));
            // vStrTokens[0] -- wmoid, taken care of by sscanf below:
            // vStrTokens[1] -- float latitude
            // vStrTokens[2] -- float longitude,
            // vStrTokens[3] -- float altitude
            // vStrTokens[4] -- char[] Station name, padded with lots of spaces.
            //                  Don't need to do anything with this here.
            // vStrTokens[5] -- Sat night light score (?)
            // vStrTokens[6] -- A/B/C rural/suburban/urban(?) Or izzit the other way around?
            
            // Scan in complete station id excluding country code (wmo#(8-digits)) 
            // First 3 digits in each line are country code -- including them makes the 
            // wmo id too big to store in a 32-bit int. So skip 'em.
            // sscanf(&cwmoId[3], "%8d",&nWmoId);     
        }
        catch(...)
        {
            continue;
        }
        
        cwmoId = &inBuf[0];

        char cWmoId0[16];
        ::memset(cWmoId0,'\0',16);
        sscanf(&cwmoId[0], "%11s",&cWmoId0[0]);     
        std::string strWmoId=std::string(cWmoId0);
        
        std::hash<std::string> nStrWmoIdHash;
        
        nWmoId=nStrWmoIdHash(std::string(cWmoId0));

        cLat=vStrTokens.at(1).c_str();
        cLong=vStrTokens.at(2).c_str();
        cAltitude=vStrTokens.at(3).c_str();
        cStationName=vStrTokens.at(4).c_str();

        // If a rural/mixed/urban classification is absent,
        // vStrTokens.at(5) will have zero length. Set
        // the corresponding station type to '?' for unknown
        if(vStrTokens.at(5).size()>0)
             cStationType=std::string(vStrTokens.at(5));
        else
                cStationType.push_back('?');

        // Use popcss to identify the header line in metadata file
        // Want to skip to the next line in the file to start picking up station metadata.
        if("popcss"==cStationType)
                continue;

        strStationNameConst=std::string(cStationName);
        strStationName=strStationNameConst;
        boost::algorithm::trim(strStationName);

        std::string strAirportDistKm=std::string(vStrTokens.at(9));
        boost::algorithm::trim(strAirportDistKm);

        float fAirportDistKm=9999;
        
        try
        {
            fAirportDistKm=boost::lexical_cast<float>(strAirportDistKm);
            latLong.lat_deg=boost::lexical_cast<float>(std::string(cLat));
            latLong.long_deg=boost::lexical_cast<float>(std::string(cLong));
            latLong.altitude=boost::lexical_cast<float>(std::string(cAltitude));
        }
        catch(...)
        {
            continue;
        }
        
        // If arg fAirportDistKmIn>=0, then
        // exclude all stations that are not within fAirPortDistKmIn of an airport.
        // If fAirportDistKmIn<0, then don't bother with airport filtering.
        // Toss in an floating pt ERR_EPS margin out of an abundance of caution.
        // GHCN::IMPOSSIBLE_VALUE is very large and negative.
        if(fAirportDistKmIn>=GHCN::IMPOSSIBLE_VALUE+GHCN::ERR_EPS)
        {
            if(fAirportDistKmIn>0)
            {
                // Discard all stations that are > fAirportDistKmIn from the
                // nearest airport.
                if(fAirportDistKm>fAirportDistKmIn)
                    continue;
            }
            else if(fAirportDistKmIn<0)
            {
                // if fAirportDistKm<0, then discard stations
                // are that are less than ABS(fAirportDistKmIn)
                // from the nearest airport.
                if(fAirportDistKm<=ABS(fAirportDistKmIn))
                    continue;
            }
            
        }
        
      
        if(cStationTypeIn != "XXX") // 'X' means process all stations
        {
                // Not all stations have ABC rural/mixed/urban designators.
                // For those stations, those designators will be zero length tokens
                // replaced with '?' per above.
                if(cStationType.at(0)=='?')
                        continue;

            if(cStationTypeIn.find(cStationType)==std::string::npos) // Rural/Urban/Suburban designator -- V2 col 73.
            {
                continue;
            }
        }
      
//  std::cout<<strStationName<<" Lat
  
        char ncStationType=(char)(cStationType[0]);
        switch(ncStationType)
        {
            case 'A':
                latLong.strStationType=std::string("RURAL");
                latLong.iStationType=1;
                break;
                
            case 'B':
                latLong.strStationType=std::string("SUBURBAN");
                latLong.iStationType=2;
                break;
                
            case 'C':
                latLong.strStationType=std::string("URBAN");
                latLong.iStationType=3;
                break;
                
            default:
                latLong.strStationType=std::string("UNKNOWN");
                latLong.iStationType=0;
                break;
                
        }

        
        // Translate to positive values only (for easier location comparisons)
        latLong.lat_deg+=90;  
        latLong.long_deg+=180;
        latLong.strStationName=strStationName;
        latLong.wmo_id=nWmoId;
        
        ghcnLatLongMapOut[nWmoId]=latLong;
        
        wmoIdSetOut.insert(nWmoId);

      }

  }
  catch(...)
  {
      std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
      std::cerr<<std::endl;
  }


  if(instream!=NULL)
  {
      instream->close();
  }
  
  std::cerr << std::endl;
  std::cerr << "Number of unique WMO IDs read from metadata file = " 
            << wmoIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  
}

void ReadGhcnMetadataFileV4b_BU(const char* inFile,
                             const std::string & cStationTypeIn,
                             const float& fAirportDistKmIn, // max distance to nearest airport. If <0, ignoere
                             std::set<size_t>& wmoIdSetOut,
                             std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[2049];
  
  ghcnLatLongMapOut.clear();
  wmoIdSetOut.clear();
  
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
    return;
  }

//  instream->exceptions(std::fstream::badbit 
//                       | std::fstream::failbit 
//                       | std::fstream::eofbit);

  char *cwmoId;
  size_t nWmoId=0;
  const char *cLat;
  const char *cLong;
  const char *cAltitude;
  const char *cStationName;
  std::string cStationType;
  std::string strStationNameConst;
  std::string strStationName;
  float fAirportDistKm;
  

  GHCNLatLong latLong;

  std::string strStream;
  std::vector<std::string> vStrTokens;

  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  nWmoId=0;
  try
  {
      while(!instream->eof())
      {
        
        instream->getline(inBuf,2048);
        strStream=std::string(inBuf);


        try
        {
            boost::split(vStrTokens,strStream,boost::is_any_of(","));
            // vStrTokens[0] -- wmoid, taken care of by sscanf below:
            // vStrTokens[1] -- station name
            // vStrTokens[2] -- N or Y (is it USCRN?)
            // vStrTokens[3] -- float latitude
            // vStrTokens[4] -- float longitude,
            // vStrTokens[5] -- float altitude
            // vStrTokens[6] -- NOT USED HERE: char[] Station name, padded with lots of spaces.
            //                  Don't need to do anything with this here.
            // vStrTokens[5] -- NOT USED HERE: Sat night light score (?) UNUSED
            // vStrTokens[6] -- NOT USED HERE: A/B/C rural/suburban/urban(?) Or izzit the other way around? UNUSED
            
            // Scan in complete station id excluding country code (wmo#(8-digits)) 
            // First 3 digits in each line are country code -- including them makes the 
            // wmo id too big to store in a 32-bit int. So skip 'em.
            // sscanf(&cwmoId[3], "%8d",&nWmoId);     
        }
        catch(...)
        {
            continue;
        }
        
        // Use here only to identify and skip the header (first line)
      
        if("Station"==vStrTokens.at(1))
             continue;

        cwmoId = &inBuf[0];
        char cWmoId0[16];
        ::memset(cWmoId0,'\0',16);
        sscanf(&cwmoId[0], "%11s",&cWmoId0[0]);     
        std::string strWmoId=std::string(cWmoId0);
        
        std::hash<std::string> nStrWmoIdHash;
        
        nWmoId=nStrWmoIdHash(std::string(cWmoId0));
        // vStrTokens.at(1): USCRN, "Y" or "N": not used
        cStationName=vStrTokens.at(1).c_str();
        cLat=vStrTokens.at(3).c_str();
        cLong=vStrTokens.at(4).c_str();
        cAltitude=vStrTokens.at(5).c_str();
        
        
        strStationNameConst=std::string(vStrTokens.at(1));
        strStationName=strStationNameConst;
        boost::algorithm::trim(strStationName);

        //std::string strAirportDistKm=std::string(vStrTokens.at(9));
        //boost::algorithm::trim(strAirportDistKm);

        float fAirportDistKm=9999;
        
        try
        {
        //    fAirportDistKm=boost::lexical_cast<float>(strAirportDistKm);
            latLong.lat_deg=boost::lexical_cast<float>(std::string(cLat));
            latLong.long_deg=boost::lexical_cast<float>(std::string(cLong));
            latLong.altitude=boost::lexical_cast<float>(std::string(cAltitude));
        }
        catch(...)
        {
            continue;
        }
        
        // If arg fAirportDistKmIn>=0, then
        // exclude all stations that are not within fAirPortDistKmIn of an airport.
        // If fAirportDistKmIn<0, then don't bother with airport filtering.
        // Toss in an floating pt ERR_EPS margin out of an abundance of caution.
        // GHCN::IMPOSSIBLE_VALUE is very large and negative.
        
        
        // Translate to positive values only (for easier location comparisons)
        latLong.lat_deg+=90;  
        latLong.long_deg+=180;
        latLong.strStationName=strStationName;
        latLong.wmo_id=nWmoId;
        
        ghcnLatLongMapOut[nWmoId]=latLong;
        
        wmoIdSetOut.insert(nWmoId);

      }

  }
  catch(...)
  {
      std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
      std::cerr<<std::endl;
  }


  if(instream!=NULL)
  {
      instream->close();
  }
  
  std::cerr << std::endl;
  std::cerr << "Number of unique WMO IDs read from metadata file = " 
            << wmoIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  
}

void ReadGhcnMetadataFileV4b2(const char* inFile,
                              const int& nBrightnessIndexIn,
                              // satellite night brightness index: if positive, process all stations with
                              // brightness index >= nBrightnessIndexIn. If negative, process all stations with
                              // brightness index <= nBrignessIndexIn.
                              std::set<size_t>& wmoIdSetOut,
                              std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[512];
  
  ghcnLatLongMapOut.clear();
  wmoIdSetOut.clear();
  
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
    return;
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);

  char *cwmoId;
  size_t nWmoId=0;
  const char *cLat;
  const char *cLong;
  const char *cAltitude;
  const char *cStationName;
  std::string cStationType;
  std::string strStationNameConst;
  std::string strStationName;
  float fAirportDistKm;
  int nBrightnessIndex;

  GHCNLatLong latLong;

  std::string strStream;
  std::vector<std::string> vStrTokens;

  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  nWmoId=0;
  try
  {
      while(!instream->eof())
      {
        
        instream->getline(inBuf,511);
        strStream=std::string(inBuf);


        try
        {
            boost::split(vStrTokens,strStream,boost::is_any_of(","));
            // vStrTokens[0] -- wmoid, taken care of by sscanf below:
            // vStrTokens[1] -- float latitude
            // vStrTokens[2] -- float longitude,
            // vStrTokens[3] -- float altitude
            // vStrTokens[4] -- char[] Station name, padded with lots of spaces.
            //                  Don't need to do anything with this here.
            // vStrTokens[5] -- A/B/C rural/suburban/urban(?) Or izzit the other way around?
            // vStrTokens[6] -- Sat night light score (?)
            
            // Scan in complete station id excluding country code (wmo#(8-digits)) 
            // First 3 digits in each line are country code -- including them makes the 
            // wmo id too big to store in a 32-bit int. So skip 'em.
            // sscanf(&cwmoId[3], "%8d",&nWmoId);     
        }
        catch(...)
        {
            continue;
        }

        // If a rural/mixed/urban classification is absent,
        // vStrTokens.at(5) will have zero length. Set
        // the corresponding station type to '?' for unknown
        if(vStrTokens.at(5).size()>0)
        {
            cStationType=std::string(vStrTokens.at(5));
        }
        else
            cStationType.push_back('?');

        // Use popcss to identify the header line in metadata file
        // Want to skip to the next line in the file to start picking up station metadata.
        if("popcss"==cStationType)
                continue;

        cwmoId = &inBuf[0];

        char cWmoId0[16];
        ::memset(cWmoId0,'\0',16);
        sscanf(&cwmoId[0], "%11s",&cWmoId0[0]);     
        std::string strWmoId=std::string(cWmoId0);
        
        std::hash<std::string> nStrWmoIdHash;
        
        nWmoId=nStrWmoIdHash(std::string(cWmoId0));
        
        cLat=vStrTokens.at(1).c_str();
        cLong=vStrTokens.at(2).c_str();
        cAltitude=vStrTokens.at(3).c_str();
        cStationName=vStrTokens.at(4).c_str();

        if(vStrTokens.at(6).size()>0)
            nBrightnessIndex=boost::lexical_cast<int>(vStrTokens.at(6));
        else // Set to impossibly high figure.
            nBrightnessIndex=999;

        strStationNameConst=std::string(cStationName);
        strStationName=strStationNameConst;
        boost::algorithm::trim(strStationName);

        if(nBrightnessIndex>nBrightnessIndexIn)
            continue;
        
        char ncStationType=(char)(cStationType[0]);
        switch(ncStationType)
        {
            case 'A':
                latLong.strStationType=std::string("RURAL");
                latLong.iStationType=1;
                break;
                
            case 'B':
                latLong.strStationType=std::string("SUBURBAN");
                latLong.iStationType=2;
                break;
                
            case 'C':
                latLong.strStationType=std::string("URBAN");
                latLong.iStationType=3;
                break;
                
            default:
                latLong.strStationType=std::string("UNKNOWN");
                latLong.iStationType=0;
                break;
                
        }
        
        // Translate to positive values only (for easier location comparisons)
        latLong.lat_deg+=90;  
        latLong.long_deg+=180;
        latLong.strStationName=strStationName;
        latLong.wmo_id=nWmoId;
        
        ghcnLatLongMapOut[nWmoId]=latLong;
        
        wmoIdSetOut.insert(nWmoId);

      }
  }
  catch(...)
  {
      std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
      std::cerr<<std::endl;
  }


  if(instream!=NULL)
  {
      instream->close();
  }
  
  std::cerr << std::endl;
  std::cerr << "Number of unique WMO IDs read from metadata file = " 
            << wmoIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  
}


void ReadGhcnMetadataFileV4(const char* inFile,
                            const std::string& cStationType,
                            std::set<size_t>& wmoIdSetOut,
                            std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[512];
  
  ghcnLatLongMapOut.clear();
  wmoIdSetOut.clear();
  
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
    return;
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);

  char *cwmoId;
  char *cLat;
  char *cLong;
  char *cAltitude;

  GHCNLatLong latLong;
  

  cwmoId = &inBuf[0];

  cLat=&inBuf[12]; // Latitude field location 
  cLong=&inBuf[21]; // Longitude field location

  cAltitude=&inBuf[31]; // Altitude field location
  
  
  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  size_t nWmoId=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);


      if(cStationType != "XXX") // 'A' means process all stations
      {
          if(cStationType.find(inBuf[106])==std::string::npos)  // Rural/Urban/Suburban designator -- V2 col 73.
          {
              continue;
          }
      }

      switch(inBuf[106])
      {
        case 'A':
          latLong.strStationType=std::string("RURAL");
          latLong.iStationType=1;
          break;
          
        case 'B':
          latLong.strStationType=std::string("SUBURBAN");
          latLong.iStationType=2;
          break;
          
        case 'C':
          latLong.strStationType=std::string("URBAN");
          latLong.iStationType=3;
          break;

        default:
          latLong.strStationType=std::string(" ");
          latLong.iStationType=0;
          break;

      }

      // Scan in complete station id excluding country code (wmo#(8-digits)) 
      // First 3 digits in each line are country code -- including them makes the 
      // wmo id too big to store in a 32-bit int. So skip 'em.
      // sscanf(&cwmoId[3], "%8d",&nWmoId);     
      char cWmoId0[16];
      ::memset(cWmoId0,'\0',16);
      sscanf(&cwmoId[0], "%11s",&cWmoId0[0]);     
      std::string strWmoId=std::string(cWmoId0);

      std::hash<std::string> nStrWmoIdHash;

      nWmoId=nStrWmoIdHash(std::string(cWmoId0));
      
      sscanf(cLat,"%f",&latLong.lat_deg);
      sscanf(cLong,"%f",&latLong.long_deg);
      // Translate to positive values only (for easier location comparisons)
      latLong.lat_deg+=90;  
      latLong.long_deg+=180;

      sscanf(cAltitude, "%f", &latLong.altitude);
      
      std::string strStationName = std::string(&inBuf[38],30);
      int ichar;
      for(ichar=strStationName.length()-1; ichar>=0; ichar--)
      {
        if(strStationName[ichar]!=' ')
          break;
        else
          strStationName[ichar]='\0';
      }
      strStationName=static_cast<std::string>(strStationName.c_str());

      latLong.strStationName=strStationName;
      
      ghcnLatLongMapOut[nWmoId]=latLong;
      ghcnLatLongMapOut[nWmoId].wmo_id=nWmoId;
      
      wmoIdSetOut.insert(nWmoId);
      
    }
  }
  catch(...)
  {
    std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
    std::cerr<<std::endl;
  }


  if(instream!=NULL)
  {
    instream->close();
  }

  std::cerr << std::endl;
  std::cerr << "Number of unique WMO IDs read from metadata file = " 
            << wmoIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  
  
  return;
  
}

void ReadCru3MetadataFile(const char* inFile,
                         std::set<size_t>& wmoIdSetOut,
                         std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{

/*
   1. World Meteorological Organization (WMO) Station Number with a single 
      additional character making a field of 6 integers. WMO numbers comprise 
      a 5 digit sub-field, where the first two digits are the country code and 
      the next three digits designated by the National Meteorological Service 
      (NMS). Some country codes are not used. If the additional sixth digit is 
      a zero, then the WMO number is or was an official WMO number. Two examples 
      are given at the end. Many additional stations are grouped beginning 99****. 
      Station numbers in the blocks 72**** to 75**** are additional stations in the 
      United States.
   2. Station latitude in degrees and tenths (-999 is missing), with negative 
      values in the Southern Hemisphere
   3. Station longitude in degrees and tenths (-1999 is missing), with negative 
      values in the Eastern Hemisphere
   4. Station Elevation in metres (-999 is missing)
   5. Station Name
   6. Country
   7. First year of monthly temperature data
   8. Last year of monthly temperature data
   9. Data Source (see below)
  10. First reliable year (generally the same as the first year) 
 */

  std::fstream* instream=NULL;

  char inBuf[512];
  
  std::map<size_t,int> wmoCount;
  

  wmoCount.clear();
  
  ghcnLatLongMapOut.clear();
  wmoIdSetOut.clear();  // Make sure lists, maps are empty
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
  }

  instream->exceptions(std::fstream::badbit 
                            | std::fstream::failbit 
                            | std::fstream::eofbit);
  char *cWmoId;
  char *cLat;
  char *cLong;

  size_t wmoId;
  int lat10ths;
  int long10ths;
  
  
  GHCNLatLong latLong;


  /*
    wmoId begins at column, 6 chars
    latitude in 10ths starts at column 6   (4 chars including sign)
                        -999 is invalid latitude

    longitude in 10ths starts at column 10 (5 chars including sign)
                        -1999 is invalid longitude
  */

  cWmoId=&inBuf[0];
  cLat=&inBuf[6]; // Latitude field location 
  cLong=&inBuf[10]; // Longitude field location
  
  int icount=0;
  
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);

      // sscanf(cWmoId, "%6d", &wmoId);   // WMO ID -- up to 6 digits
      char cWmoId0[10];
      sscanf(cWmoId,"%8s",&cWmoId0[0]);
      std::hash<std::string> strHash;
      wmoId=strHash(std::string(cWmoId0));

      sscanf(cLat, "%4d",&lat10ths);   // latitude in 10ths
      sscanf(cLong, "%5d",&long10ths); // longitude in 10ths

      latLong.lat_deg=lat10ths/10.0+90;  
      latLong.long_deg=(-1.*long10ths/10.)+180; // #$%@! Brits reversed the longitudes!

      latLong.wmo_id=wmoId;
      
      if((latLong.lat_deg<=180.0)&&(latLong.lat_deg>=0.))
      {
        if((latLong.long_deg<=360)&&(latLong.long_deg>=0))
        {
          // Got a valid lat/long coord pair -- save it
          ghcnLatLongMapOut[wmoId]=latLong;
          wmoIdSetOut.insert(wmoId);
          icount++;
        }
      }


      std::string strStationName = std::string(&inBuf[21],20);
      std::string strStationCountry = std::string(&inBuf[42],14);
      int ichar;
      for(ichar=strStationName.length()-1; ichar>=0; ichar--)
      {
        if(strStationName[ichar]!=' ')
          break;
        else
          strStationName[ichar]='\0';
      }
      strStationName=static_cast<std::string>(strStationName.c_str());

      for(ichar=strStationCountry.length()-1; ichar>=0; ichar--)
      {
        if(strStationCountry[ichar]!=' ')
          break;
        else
          strStationCountry[ichar]='\0';
      }
      strStationCountry=static_cast<std::string>(strStationCountry.c_str());

      latLong.strStationName=strStationName+std::string(",")+strStationCountry;
      latLong.strStationType=std::string(" ");
      
    }

  }
  catch(...)
  {
    std::cerr << "Probably reached end of file -- this is ok..." 
              << std::endl;
    std::cerr << "Found " << icount 
         << " stations with valid lat/long coords."<<std::endl;
    
    std::cerr<<std::endl;
  }

  if(instream!=NULL)
  {
    instream->close();
  }
  
  return;
}



void ReadBestMetadataFile(const char* inFile,
                          std::set<size_t>& wmoIdSetOut,
                          std::map<size_t, GHCNLatLong >& bestLatLongMapOut)
{


  std::fstream* instream=NULL;

  char inBuf[512];
  
  std::map<size_t,int> wmoCount;
  

  wmoCount.clear();
  
  bestLatLongMapOut.clear();
  wmoIdSetOut.clear();  // Make sure the list is empty
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);
  
  GHCNLatLong latLong;


  int icount=0;
  
  size_t nWmoId;
  float fLat;
  float fLong;
  
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);

      if(inBuf[0]=='%') // comment line -- skip it.
        continue;
      
      sscanf(inBuf,"%lu %f %f", &nWmoId,&fLat,&fLong);

      latLong.lat_deg=fLat+90;  
      latLong.long_deg=fLong+180;

      // BEST metadata file used -- site_summary.txt
      //      has minimal info (only id,lat,long), so use 
      //      the station id as the station name.
      std::stringstream sstrStationName;
      sstrStationName<<nWmoId;
      latLong.strStationName=sstrStationName.str();
      latLong.wmo_id=nWmoId;
      if((latLong.lat_deg<=180.0)&&(latLong.lat_deg>=0.))
      {
        if((latLong.long_deg<=360)&&(latLong.long_deg>=0))
        {
          // Got a valid lat/long coord pair -- save it
          bestLatLongMapOut[nWmoId]=latLong;
          wmoIdSetOut.insert(nWmoId);
          icount++;
        }
      }
      

    }

  }
  catch(...)
  {
    std::cerr << "Probably reached end of file -- this is ok..." 
              << std::endl;
    std::cerr << "Found " << icount 
         << " stations with valid lat/long coords."<<std::endl;
    
    std::cerr<<std::endl;
  }

  if(instream!=NULL)
  {
    instream->close();
  }
  
  return;

}

void ReadUshcnMetadataFile(const char* inFile,
                           const std::string& cStationType,
                           std::set<size_t>& wmoCompleteIdSetOut,
                           std::map<size_t, GHCNLatLong >& ghcnLatLongMapOut)
{

  std::fstream* instream=NULL;

  char inBuf[512];
  
  ghcnLatLongMapOut.clear();
  wmoCompleteIdSetOut.clear();
  
  try
  {
    instream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(instream!=NULL)
    {
      instream->close(); // make sure it's closed
    }
    return;
  }

  instream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);

  char *cwmoId;
  char *cLat;
  char *cLong;
  char *cAltitude;

  GHCNLatLong latLong;

  cwmoId = &inBuf[0];

  cLat=&inBuf[12]; // Latitude field location 
  cLong=&inBuf[21]; // Longitude field location

  cAltitude=&inBuf[31]; // Altitude field location
  
  
  // curs=&inBuf[67]; location of rural/suburban/urban station designator
  size_t nWmoId=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);


#if(0) //  No USHCN rural/urban station status (yet)
      if(cStationType != 'X') // 'A' means process all stations
      {
        if(cStationType != inBuf[106]) // Rural/Urban/Suburban designator -- V2 col 73.
        {
          continue;
        }
      }

      switch(inBuf[106])
      {
        case 'A':
          latLong.strStationType=std::string("RURAL");
          latLong.iStationType=1;
          break;
          
        case 'B':
          latLong.strStationType=std::string("SUBURBAN");
          latLong.iStationType=2;
          break;
          
        case 'C':
          latLong.strStationType=std::string("URBAN");
          latLong.iStationType=3;
          break;

        default:
          latLong.strStationType=std::string(" ");
          latLong.iStationType=0;
          break;

      }
#endif
      // USHCN -- use entire 11 char ID string, including country code and station id (including leading zeros)
      //          per GHCN::ReadUshcnTempFile(const char* ushcnFile) in GHCN.cpp::1216
      std::string strWmoId=std::string(inBuf).substr(0,11);
      std::hash<std::string> nStrWmoIdHash;
      nWmoId=nStrWmoIdHash(strWmoId);

      std::string strLatDeg=std::string(inBuf).substr(12,8); // (+/-)DD.DDDD (-90.0000->+90.0000)
      std::string strLonDeg=std::string(inBuf).substr(21,9); // (+/-)DDD.DDDD (-180.0000->+180.0000)
      std::string strAltMet=std::string(inBuf).substr(31,6);

      boost::trim(strLatDeg);
      boost::trim(strLonDeg);
      boost::trim(strAltMet);
      try
      {
          latLong.lat_deg=boost::lexical_cast<float>(strLatDeg);
          latLong.long_deg=boost::lexical_cast<float>(strLonDeg);
          latLong.altitude=boost::lexical_cast<float>(strAltMet);
      }
      catch(...)
      {
          std::cerr<<__FUNCTION__<<"(): Error parsing lat,long,or alt."<<std::endl;
          continue;
      }

      // No urban/rural info available for USHCN stations.
      latLong.strStationType=std::string(" ");
      latLong.iStationType=0;

      // Translate to positive values only (for easier location comparisons)
      latLong.lat_deg+=90;  
      latLong.long_deg+=180;

      std::string strStationName = std::string(inBuf).substr(41,30);
      int ichar;
      for(ichar=strStationName.length()-1; ichar>=0; ichar--)
      {
        if(strStationName[ichar]==' ')
            strStationName[ichar]='_';
      }

      latLong.strStationName=strStationName;
      
      ghcnLatLongMapOut[nWmoId]=latLong;

      wmoCompleteIdSetOut.insert(nWmoId);
      
    }
  }
  catch(...)
  {
    std::cerr << "Just finished parsing metadata file " << inFile << std::endl;
    std::cerr<<std::endl;
  }


  if(instream!=NULL)
  {
    instream->close();
  }

  std::cerr << std::endl;
  std::cerr << "Number of unique WMO IDs read from metadata file = " 
            << wmoCompleteIdSetOut.size() << std::endl;
  std::cerr << std::endl;
  
  
  return;

}

std::fstream* OpenCsvStationCountFile(const char* csvFileName)
{

  std::fstream* OpenCsvFile(const char* csvFileName);

  // Check for a .suffix -- if it exists, chop it off
  std::string strFileName=std::string(csvFileName);
  size_t dotIndex;
  dotIndex=strFileName.find_last_of(".");
  if(std::string::npos==dotIndex)
  {
    // No suffix -- just append this:
    strFileName+="_StationCount";
  }
  else 
  {
    // Suffix present -- insert "StationCount" before suffix.
    std::string strSuffix;
    strSuffix=strFileName.substr(dotIndex);    // grab suffix including '.'
    strFileName=strFileName.substr(0,dotIndex); // grab prefix excluding '.'
    strFileName+=("_StationCount")+std::string(strSuffix);
  }
  
  std::fstream* csvStationCountFstream;
  
  csvStationCountFstream=OpenCsvFile(strFileName.c_str());
  
  return csvStationCountFstream;
  
}


std::fstream* OpenCsvFile(const char* csvFileName) 
{
  if(csvFileName==NULL)
  {
    std::cerr<<"Need to supply an output filename (-o option) or a port# (-P option) "
             <<std::endl<<std::endl;
    exit(1);
  }

  std::fstream inp;
  inp.open(csvFileName, std::fstream::in);
  inp.close();
  if(!inp.fail())  
  {
    std::cerr<<"File "<< csvFileName 
             << " already exists -- will not overwrite." << std::endl;
    return NULL;
  }
  
  std::fstream* csvFstream=NULL;
  try
  {
    csvFstream = new std::fstream(csvFileName,std::fstream::out);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << csvFileName << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(csvFstream!=NULL)
        csvFstream->close(); // make sure it's closed
    delete csvFstream;
    csvFstream=NULL;
  }

  if(!csvFstream->is_open())
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to open " << csvFileName << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    delete csvFstream;
    csvFstream=NULL;
  }
 
  return csvFstream;
  
}

std::fstream* OpenKmlFile(const char* inFileName, const char* csvFileName) 
{
  // kml file name will be a composite of the input file name and csv output file name
  std::string strInFileName;
  std::string strCsvFileName;
  std::string strKmlFileName;

  std::fstream* fout;
  
  std::fstream* OpenCsvFile(const char* kmlFileName);
  
  // Modify csvFileName to make a KML file name;
  if((NULL==csvFileName) || (NULL==inFileName))
  {
    return NULL;
  }
  
  strInFileName=static_cast<std::string>(inFileName);
  
  strCsvFileName=static_cast<std::string>(csvFileName);
  

  size_t dotPos;
  dotPos=strInFileName.find_last_of('.');
  if(dotPos!=std::string::npos)
  {
    // Chop off suffix, resize accordingly
    strInFileName[dotPos]='\0';
    strInFileName=std::string(strInFileName.c_str());
  }

  dotPos=strCsvFileName.find_last_of('.');
  if(dotPos!=std::string::npos)
  {
    strCsvFileName[dotPos]='\0';
    strCsvFileName=std::string(strCsvFileName.c_str());
  }
  
  strKmlFileName=strInFileName+std::string("_")+strCsvFileName+".kml";
  
  fout = OpenCsvFile(strKmlFileName.c_str());
  
  return fout;

}

std::fstream* OpenJavaScriptFile(const char* inFileName, const char* csvFileName) 
{
  // kml file name will be a composite of the input file name and csv output file name
  std::string strInFileName;
  std::string strCsvFileName;
  std::string strJsFileName;

  std::fstream* fout;
  
  std::fstream* OpenCsvFile(const char* jsFileName);
  
  // Modify csvFileName to make a KML file name;
  if((NULL==csvFileName) || (NULL==inFileName))
  {
    return NULL;
  }
  
  strInFileName=static_cast<std::string>(inFileName);
  
  strCsvFileName=static_cast<std::string>(csvFileName);
  

  size_t dotPos;
  dotPos=strInFileName.find_last_of('.');
  if(dotPos!=std::string::npos)
  {
    // Chop off suffix, resize accordingly
    strInFileName[dotPos]='\0';
    strInFileName=std::string(strInFileName.c_str());
  }

  dotPos=strCsvFileName.find_last_of('.');
  if(dotPos!=std::string::npos)
  {
    strCsvFileName[dotPos]='\0';
    strCsvFileName=std::string(strCsvFileName.c_str());
  }
  
  strJsFileName=strInFileName+std::string("_")+strCsvFileName+".js";
  
  fout = OpenCsvFile(strJsFileName.c_str());
  
  return fout;

}

void DumpRunInfo(std::fstream& fout)
{
    time_t csv_t=time(NULL);
    std::string strCsvTime=ctime(&csv_t);
    
    std::string strCmdArgs;
    for(auto& iv: g_vStrCmdArgs)
    {
        strCmdArgs+=iv+" ";
    }

    fout<<"Creation_Date "<<strCsvTime;
    fout<<"Cmd args "<<strCmdArgs;
    fout<<"\n\n";

}

void DumpSpreadsheetHeader(std::fstream& fout)
{
  std::vector<std::string>::iterator istr;
  unsigned int ii=0;
  int ipos;
  
  // Silly boy, the first column is "Year"
  fout << "Year,";
  if(fout.bad())    //bad() function will check for badbit
  {
       std::cout<<__FUNCTION__<<"(): Writing to file failed"<<std::endl;
  }

  for(istr=ensembleElementHeadingStr_g.begin();
      istr!=ensembleElementHeadingStr_g.end();
      istr++,ii++)
  {
    if(ii==ensembleElementHeadingStr_g.size()-1)
    {
      // Last element (spreadsheet column)
      // Trim off the trailing comma.
        if(istr->find(',')!=std::string::npos)
        {
            ipos=istr->find(',');
            (*istr)[ipos]=' ';
        }
    }
    fout << *istr;
    if(fout.bad())    //bad() function will check for badbit
    {
         std::cout<<__FUNCTION__<<"(): Writing to file failed"<<std::endl;
    }
  }
  fout << std::endl;

}



// Dumps results from multiple GHCN files in csv form
void DumpSmoothedResults(std::fstream& fout,
                         std::vector<GHCN*> ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<size_t, double >::iterator iyy;
  int igh;
  
  // Iterate over years
  for(iyy=ghcn[0]->mSmoothedGlobalAverageAnnualAnomaliesMap.begin();
      iyy!=ghcn[0]->mSmoothedGlobalAverageAnnualAnomaliesMap.end();
      iyy++)
  {
    // Results for first input file
    // First 
    fout << iyy->first << ","
         << iyy->second;

    if(ngh>1)
      fout << ",";

    // Results for additional input files
    for(igh=1; igh<ngh; igh++)
    {
      if(ghcn[igh]->mSmoothedGlobalAverageAnnualAnomaliesMap.find(iyy->first) != 
         ghcn[igh]->mSmoothedGlobalAverageAnnualAnomaliesMap.end())
      {
        fout << ghcn[igh]->mSmoothedGlobalAverageAnnualAnomaliesMap[iyy->first];
        // Don't need a trailing comma after the last field
        if(igh<ngh-1)
             fout << ",";
      }
      else
      {
        // No valid data for this year -- put in a blank/null csv placeholder
        // Also, don't need a trailing comma after the last field
        if(igh<ngh-1)
          fout << ",";
      }

    }
    fout << std::endl; // end of this csv line..

  }

  return;
  
}





 

   







