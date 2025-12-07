#include <spawn.h>
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <X11/Xlib.h>

#include "GnuPlotter.hpp"
#include "SimpleSocket.h"

#include "GHCN.hpp"
#include "GHCN_externs.hpp"
#include "AnomalyVersion.hpp"

int nBrightnessValue_g=999;

pid_t nYadServerBusyPid_g=-1;

std::vector<std::string> g_vStrCmdArgs;

int main(int argc, char **argv)
{
    bMaxStationRecordStartYear_g=false;
    bMinStationRecordLength_g=false;
    bMinStationRecordEndYear_g=false;

    int status;
  
  void ProcessOptions(int argc, char **argv);
  
  int MainBatchMode(int argc, char **argv);
  int MainBatchBootstrapMode(int argc, char **argv);
  int MainServerMode(int argc, char **argv);

  strStationType_g="X"; // 'X' means process all stations

  for(int ic=0; ic<argc; ic++)
  {
      g_vStrCmdArgs.push_back(argv[ic]);
  }
  
  ProcessOptions(argc,argv);

  if(nServerPort_g>1024) 
  {
      // Server mode only for non-privileged ports (>1024)
      status=MainServerMode(argc,argv);
  }
  else if(0==nEnsembleRunsBootstrap_g)
  {
      // Standard batch run mode
      status=MainBatchMode(argc,argv);
  }
  else
  {
      // Statistical bootstraping mode
      status=MainBatchBootstrapMode(argc,argv);
  }

  std::cerr << "All finished!"<<std::endl<<std::endl;
  
  return status;

}

int MainServerMode(int argc, char** argv)
{
  std::vector<GHCN*> ghcn;

  std::set<std::size_t> wmoCompleteIdSet;
  
  std::map<std::size_t, GHCNLatLong> ghcnLatLongMap;

  std::set<std::size_t> selectedWmoIdSet;


  void ReadMetadataFile(std::set<std::size_t>& wmoCompleteIdSet,
                        std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap);

  void ReadGhcnDailyMetadataFile(const char* inFile,
                                 std::set<std::size_t>& wmoIdSetOut,
                                 std::map<std::size_t, GHCNLatLong >& ghcnLatLongMap);
  
  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<std::size_t>& wmoCompleteIdSet,
                           std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap);

  void ServerPrecomputeAnomalies(std::vector<GHCN*>& ghcn, 
                                 int argc, char**& argv,
                                 std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap,
                                 std::set<std::size_t>& wmoCompleteIdSet);
  
  void ShowServerStatus(const std::string& strMessage);

  strStationType_g="X"; // Default to all stations.
  
  ReadMetadataFile(wmoCompleteIdSet,
                   ghcnLatLongMap);
  

  // Read command message from client (QGIS or browser)
  // Return true if the message came from a browser (i.e. is html)
  // Return false if the message came from QGIS (not html)
  bool ReadCommandMessage(SimpleTcpSocket *pClientConnectSocket,
                          std::string& strMsg);

  void ServerProcessLoop(std::vector<GHCN*>& ghcn,
                         int argc, char** argv,
                         std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap,
                         std::set<std::size_t>& wmoCompleteIdSet);

  std::cerr << std::endl;
  std::cerr << "Smoothing filter length = " << avgNyear_g 
            << std::endl << std::endl; 
  
  std::cerr << "Will crunch " << argc-optind << " temperature data set(s). " 
            << std::endl;


  ghcn.resize(argc-optind);

  std::string strStatusMessage
      =std::string("--text=<b>\nPrecalculating all station anomalies.\n\nThis may take some time.\n</b>");

  ShowServerStatus(strStatusMessage);
  
  ServerPrecomputeAnomalies(ghcn, argc, argv,
                            ghcnLatLongMap, wmoCompleteIdSet);

  kill(nYadServerBusyPid_g,SIGTERM);
  
  ServerProcessLoop(ghcn, argc, argv, ghcnLatLongMap, wmoCompleteIdSet);
  
  
  return 0;

}

/**
WidthOfScreen

WidthOfScreen(screen)

int XWidthOfScreen(screen)
      Screen *screen;

screen  Specifies the appropriate Screen structure.

Both return the width of the specified screen in pixels.
HeightOfScreen

HeightOfScreen(screen)

int XHeightOfScreen(screen)
      Screen *screen;

 **/

void ShowServerBusy(const std::string& strSplashScreenFileName)
{
    Display* d = XOpenDisplay(NULL);
    Screen*  s = DefaultScreenOfDisplay(d);

    int nPopupWidth=320;
    int nPopupHeight=75;
    
    int nDispWidth=XWidthOfScreen(s);
    int nDispHeight=XHeightOfScreen(s);

    int nXLoc=nDispWidth/2.-nPopupWidth/2.;
    int nYLoc=nDispHeight/2-nPopupHeight/2.;
    
    std::cerr<<"nDispWidth="<<nDispWidth<<"  nDispHeight="<<nDispHeight<<std::endl;
    
    std::string strXLoc=boost::lexical_cast<std::string>(nXLoc);
    std::string strXLocOption_1=std::string("--posx=")+strXLoc;
    std::string strYLoc=boost::lexical_cast<std::string>(nYLoc);
    std::string strYLocOption_2=std::string("--posy=")+strYLoc;

    std::string strPopupWidth=boost::lexical_cast<std::string>(nPopupWidth);
    std::string strPopupWidthOption_3=std::string("--width=")+strPopupWidth;
    std::string strPopupHeight=boost::lexical_cast<std::string>(nPopupHeight);
    std::string strPopupHeightOption_4=std::string("--height=")+strPopupHeight;
    std::string strMessageAlignOption_5=std::string("--text-align=center");
    std::string strUndecorated_6=std::string("--undecorated");
    std::string strOnTop_7=std::string("--on-top");
    std::string strFixed_8=std::string("--fixed");

    std::string strMessage_9=
        std::string("--text=\"<b>\nStation anomaly precalculations finished.\n\nPress OK to continue.\n</b>\"");

    std::vector<const char*> vArgv;

    vArgv.push_back(strXLocOption_1.c_str());
    vArgv.push_back(strYLocOption_2.c_str());
    vArgv.push_back(strPopupWidthOption_3.c_str());
    vArgv.push_back(strPopupHeightOption_4.c_str());
    vArgv.push_back(strMessageAlignOption_5.c_str());
    vArgv.push_back(strUndecorated_6.c_str());
    vArgv.push_back(strOnTop_7.c_str());
    vArgv.push_back(strFixed_8.c_str());
    vArgv.push_back(strMessage_9.c_str());

    const char** argv;
    std::unique_ptr<const char*[]> appArgv(new const char* [10]);
    argv=appArgv.get();

    int iarg=0;
    for(int iarg=0; iarg<9; iarg++)
    {
        argv[iarg]=vArgv.at(iarg);
    }

    // spawn a child, non-blocking,
    // guaranteeing that the child will
    // never turn into a zombie.
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_NOCLDWAIT;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    int nPidStatus=posix_spawnp(&nYadServerBusyPid_g,"yad",NULL,NULL,(char* const*)argv,NULL);
    
    return;
}

void ShowServerStatus(const std::string& strMessage)
{
    int status;

    Display* d = XOpenDisplay(NULL);
    Screen*  s = DefaultScreenOfDisplay(d);

    int nPopupWidth=320;
    int nPopupHeight=75;
    
    int nDispWidth=XWidthOfScreen(s);
    int nDispHeight=XHeightOfScreen(s);

    int nXLoc=nDispWidth/2.-nPopupWidth/2.;
    int nYLoc=nDispHeight/2-nPopupHeight/2.;
    
    std::cerr<<"nDispWidth="<<nDispWidth<<"  nDispHeight="<<nDispHeight<<std::endl;
    
    std::string strXLoc=boost::lexical_cast<std::string>(nXLoc);
    std::string strXLocOption_1=std::string("--posx=")+strXLoc;
    std::string strYLoc=boost::lexical_cast<std::string>(nYLoc);
    std::string strYLocOption_2=std::string("--posy=")+strYLoc;

    std::string strPopupWidth=boost::lexical_cast<std::string>(nPopupWidth);
    std::string strPopupWidthOption_3=std::string("--width=")+strPopupWidth;
    std::string strPopupHeight=boost::lexical_cast<std::string>(nPopupHeight);
    std::string strPopupHeightOption_4=std::string("--height=")+strPopupHeight;
    std::string strMessageAlignOption_5=std::string("--text-align=center");
    std::string strUndecorated_6=std::string("--undecorated");
    std::string strNoButtons_7=std::string("--no-buttons");
    std::string strOnTop_8=std::string("--on-top");
    std::string strFixed_9=std::string("--fixed");

    std::string strMessage_10=strMessage;
    
    std::vector<const char*> vArgv;
    vArgv.push_back("yad");
    vArgv.push_back(strXLocOption_1.c_str());
    vArgv.push_back(strYLocOption_2.c_str());
    vArgv.push_back(strPopupWidthOption_3.c_str());
    vArgv.push_back(strPopupHeightOption_4.c_str());
    vArgv.push_back(strMessageAlignOption_5.c_str());
    vArgv.push_back(strUndecorated_6.c_str());
    vArgv.push_back(strNoButtons_7.c_str());
    vArgv.push_back(strOnTop_8.c_str());
    vArgv.push_back(strFixed_9.c_str());
    vArgv.push_back(strMessage_10.c_str());

    const char** argv;
    std::unique_ptr<const char*[]> appArgv(new const char* [vArgv.size()+1]);
    argv=appArgv.get();

    int iarg=0;
    for(iarg=0; iarg<vArgv.size(); iarg++)
    {
        argv[iarg]=vArgv.at(iarg);
    }
    argv[iarg]=NULL;

    iarg=0;
    while(argv[iarg++]!=NULL)
    {
        std::cout<<"Arg "<<iarg<<": "<<argv[iarg]<<std::endl;
    }

    // YAD likes to steal standard output, so save stdout, and std::cout
    int saved_cout_fd= dup(STDOUT_FILENO);
    // Save original buffer
    std::streambuf* originalBuf = std::cout.rdbuf();

    int nPidStatus=posix_spawnp(&nYadServerBusyPid_g,"yad",NULL,NULL,(char* const*)argv,environ);
    if(0==nPidStatus)
    {
        std::cerr<<"posix_spawnp: spawned process "<<nYadServerBusyPid_g<<std::endl;
        std::cerr<<"Should not block from here"<<std::endl;
    }
    else
    {
        perror("posix_spawnp");
    }

    // Restore the original stdout and std::cout
    dup2(saved_cout_fd, STDOUT_FILENO);
    // Restore original buffer
    std::cout.rdbuf(originalBuf);    

    return;
}

void ShowServerIsReady(const std::string& strSplashScreenFileName)
{
    Display* d = XOpenDisplay(NULL);
    Screen*  s = DefaultScreenOfDisplay(d);

    int nPopupWidth=320;
    int nPopupHeight=75;
    
    int nDispWidth=XWidthOfScreen(s);
    int nDispHeight=XHeightOfScreen(s);

    int nXLoc=nDispWidth/2.-nPopupWidth/2.;
    int nYLoc=nDispHeight/2-nPopupHeight/2.;
    
    std::cerr<<"nDispWidth="<<nDispWidth<<"  nDispHeight="<<nDispHeight<<std::endl;
    
    std::string strXLoc=boost::lexical_cast<std::string>(nXLoc);
    std::string strXLocOption=std::string(" --posx=")+strXLoc+std::string(" ");
    std::string strYLoc=boost::lexical_cast<std::string>(nYLoc);
    std::string strYLocOption=std::string(" --posy=")+strYLoc+std::string(" ");

    std::string strPopupWidth=boost::lexical_cast<std::string>(nPopupWidth);
    std::string strPopupWidthOption=std::string(" --width=")+strPopupWidth+std::string(" ");
    std::string strPopupHeight=boost::lexical_cast<std::string>(nPopupHeight);
    std::string strPopupHeightOption=std::string(" --height=")+strPopupHeight+std::string(" ");
    
    
    std::string strMessage=std::string("\"<b>\nAnomaly server precalculations finished.\n\nPress OK to continue.\n</b>\"");
    std::string strMessageOption=std::string(" --text-align=center --text=")+strMessage+std::string(" ");

    std::string strButtonOption=std::string(" --button=OK:1 --buttons-layout=center ");
    
    std::string strCmd=std::string("yad --undecorated --on-top --fixed")
            +strXLocOption+strYLocOption+strPopupWidthOption+strPopupHeightOption
            +strButtonOption+strMessageOption;
        ::system(strCmd.c_str());
    return;
}

  
void ServerPrecomputeAnomalies(std::vector<GHCN*>& ghcn, 
                               int argc, char**& argv,
                               std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap,
                               std::set<std::size_t>& wmoCompleteIdSet)
{

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<std::size_t>& wmoCompleteIdSet,
                           std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap);

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
    ghcn[igh]->ComputeBaselines(minBaselineSampleCount_g);
    
    std::cerr << "Precomputing station anomalies for " 
              << argv[igh+optind]<< std::endl;
    std::cerr << std::endl;
    ghcn[igh]->PrecomputeStationAnomalies(minBaselineSampleCount_g);
    
  }
}

void ServerProcessLoop(std::vector<GHCN*>& ghcn,
                  int argc, char** argv,
                  std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap,
                  std::set<std::size_t>& wmoCompleteIdSet)
{
    
  SimpleTcpSocket *pServerSocket=NULL;
  SimpleTcpSocket *pClientConnectSocket=NULL;
  
  std::set<std::size_t> selectedWmoIdSet;
  selectedWmoIdSet.clear();

  GnuPlotter gnuPlotter(ghcn);
  
  int ComputeResultsToDisplay(int argc,
                              std::vector<GHCN*>& ghcn,
                              std::set<std::size_t>& selectedWmoIdSet);

  bool ReadCommandMessage(SimpleTcpSocket *pClientConnectSocket,
                          std::string& strMsg);

  void ManageReconnection(bool bHtmlMode,
                          SimpleTcpSocket* pServerSocket,
                          SimpleTcpSocket*& pClientConnectSocket);

  void ShowServerBusy(const std::string& strServerIsReadyPngFile);
  void ShowServerIsReady(const std::string& strServerIsReadyPngFile);
  
  pServerSocket=new SimpleTcpSocket(nServerPort_g);
  pServerSocket_g=pServerSocket;

  // All prepped and ready to go into server mode
  ShowServerIsReady(std::string("server_is_ready.png"));
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

    if(selectedWmoIdSet.size()>1000)
    {
        std::string strNumStations=boost::lexical_cast<std::string>(selectedWmoIdSet.size());
        std::string strStatusMessage
            =std::string("--text=<b>\nScanning/processing "+strNumStations+" stations.\n\nThis may take some time.\n</b>");

        ShowServerStatus(strStatusMessage);
    }
    
    int nMaxPointsToPlot=ComputeResultsToDisplay(argc,ghcn, selectedWmoIdSet);

    kill(nYadServerBusyPid_g,SIGTERM);

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
                            std::set<std::size_t>& selectedWmoIdSet)
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
        
      std::map<std::size_t,GHCNLatLong> completeLatLongDegMap;
      completeLatLongDegMap=ghcn[igh]->GetCompleteWmoLatLongMap();
      
      ghcn[igh]->Clear();
      
      ghcn[igh]->ComputeGriddedGlobalAverageAnnualAnomalies(minBaselineSampleCount_g,
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
  std::vector<GHCN*> ghcn;

  std::set<std::size_t> wmoShortIdSet;
  std::set<std::size_t> wmoCompleteIdSet;
  
  std::map<std::size_t, GHCNLatLong> ghcnLatLongMap;

  std::stringstream headingStrBase;
  std::stringstream headingStr;

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<std::size_t>& wmoCompleteIdSet,
                           std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap);

  void ProcessOptions(int argc, char **argv);

  void ReadMetadataFile(std::set<std::size_t>& wmoCompleteIdSet,
                        std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap);

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<std::size_t>& wmoCompleteIdSet,
                           std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap);
  

  std::fstream* OpenCsvFile(const char* filename);
  std::fstream* OpenCsvGenericFile(const char* filename, const std::string & strFileAppend);
  std::fstream* OpenCsvStationCountFile(const char* filename);
  std::fstream* OpenCsvStationAvgLatAltFile(const char* filename);
 
  std::fstream* OpenKmlFile(const char* infilename, const char* csvfilename);

  void DumpBuildRunInfo(std::fstream& fout);
  
  void DumpSpreadsheetHeader(std::fstream& fout);

  void DumpSmoothedAnnualResults(std::fstream& fout,
                                 std::vector<GHCN*>& ghcn, int nghcn);
  void DumpSmoothedMonthlyResults(std::fstream& fout,
                                 std::vector<GHCN*>& ghcn, int nghcn);

  
  void DumpLatLongInfoAsKml(std::fstream& fout,
                            const std::map<std::size_t,GHCNLatLong>& latLongMap);
  
  void WriteKmlData(char** argv, std::vector<GHCN*>& ghcn, int igh);

  void WriteJavaScriptCode(char** argv, std::vector<GHCN*>& ghcn, int igh);
  
  
  wmoCompleteIdSet.clear();
  

  std::fstream* csvOut=NULL;
  std::fstream* csvStationCountOut=NULL;
  std::fstream* csvStationAvgLatAltOut=NULL;

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
    ghcn[igh]->ComputeBaselines(minBaselineSampleCount_g);

    ghcn[igh]->PrecomputeStationAnomalies(minBaselineSampleCount_g);

    std::cerr << "Computing average anomalies for " 
         << argv[igh+optind] << std::endl;
    std::cerr << std::endl;
          
    headingStrBase.str("");
    headingStrBase<<argv[igh+optind]<<",";
    headingStr.str("");
    headingStr<<headingStrBase.str(); 
    ensembleElementHeadingStr_g.push_back(headingStr.str());

    std::map<std::size_t, GHCNLatLong> selectedGhcnLatLongMap;
          
    if(true==bSparseStationSelect_g)
    {
            ghcn[igh]->SelectStationsPerGridCell(gridSizeLatDeg_g,
                                                 gridSizeLongDeg_g,
                                                 ghcnLatLongMap,
                                                 selectedGhcnLatLongMap);
  
            if(nAnnualAvg_g>0) // Compute annual global averages.
            {
                ghcn[igh]->ComputeGriddedGlobalAverageAnnualAnomalies(minBaselineSampleCount_g,
                                                                      gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                      selectedGhcnLatLongMap);
            }
            else // Compute monthly global averages.
            {
                ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                                       gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                       selectedGhcnLatLongMap);
            }
    }
    else
    {
      if(SELECT_ALL==eStationSelectMode_g)
      {
          if(nAnnualAvg_g>0) // Compute annual global averages.
          {
              ghcn[igh]->ComputeGriddedGlobalAverageAnnualAnomalies(minBaselineSampleCount_g,
                                                                    gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                    ghcnLatLongMap);
          }
          else // Compute monthly global averages.
          {
              ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                                     gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                     ghcnLatLongMap);
          }
      }
      else
      {
        std::map<std::size_t, GHCNLatLong> selectedGhcnLatLongMap;

        ghcn[igh]->SelectStationsGlobal(ghcnLatLongMap,selectedGhcnLatLongMap);

        if(nAnnualAvg_g>0) // Compute annual global averages.
        {
            ghcn[igh]->ComputeGriddedGlobalAverageAnnualAnomalies(minBaselineSampleCount_g,
                                                                  gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                  selectedGhcnLatLongMap);
        }
        else // Compute monthly global averages.
        {
            ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                                   gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                   selectedGhcnLatLongMap);
        }
      }
    
      if(bComputeAvgStationLatAlt_g)
         ghcn[igh]->ComputeAvgStationLatAlt();
        
    }
    
    std::cerr<<std::endl;
    std::cerr << argv[igh+optind]<<": Number of stations = "
              << ghcnLatLongMap.size() << std::endl;
    std::cerr<<std::endl;
    

    WriteKmlData(argv, ghcn, igh);

    WriteJavaScriptCode(argv, ghcn, igh);

#if(0) // Enable for debugging, if desired.
    if(true==bSparseStationSelect_g)
    {
        std::cerr<<std::endl;
        std::cerr<<"Sparse Station Dump"<<std::endl;
        for(auto& im: selectedGhcnLatLongMap)
        {
            std::cerr<<"Station WMO_ID="<<im.second.strWmoId
                     <<" lat="<<im.second.lat_deg-90.<<" lon="<<im.second.long_deg<<std::endl;
        }
    }
#endif
  }
  
  std::cerr <<std::endl<<std::endl;
  std::cerr << "Writing results to " << csvFileName_g <<"..."
            << std::endl<<std::endl<<std::endl;

  csvOut=OpenCsvFile(csvFileName_g);
  if(csvOut==NULL)
  {
      std::cerr<<std::endl<<"Error opening output CSV file "
               <<csvFileName_g<<std::endl<<std::endl;
      exit(1);
  }

  DumpBuildRunInfo(*csvOut);
  
  DumpSpreadsheetHeader(*csvOut);

  if(nAnnualAvg_g>0)
      DumpSmoothedAnnualResults(*csvOut, ghcn, argc-optind);
  else
      DumpSmoothedMonthlyResults(*csvOut, ghcn, argc-optind);
  
  csvOut->close();
  delete csvOut;
  
  csvStationCountOut=OpenCsvGenericFile(csvFileName_g,std::string("_StationCount"));
  if(csvStationCountOut==NULL)
  {
      std::cerr<<std::endl<<"Error opening output companion station count file for "
               <<csvFileName_g<<std::endl<<std::endl;
      exit(1);
  }

  DumpBuildRunInfo(*csvStationCountOut);
  
  DumpSpreadsheetHeader(*csvStationCountOut);
  
  DumpStationCountResults(*csvStationCountOut, ghcn, argc-optind);
  
  csvStationCountOut->close();
  delete csvStationCountOut;

  if(bComputeAvgStationLatAlt_g)
  {
      csvStationAvgLatAltOut=OpenCsvGenericFile(csvFileName_g,std::string("_LatAlt"));
  
      DumpBuildRunInfo(*csvStationAvgLatAltOut);
  
      DumpSpreadsheetHeader(*csvStationAvgLatAltOut);

      DumpStationAvgLatAltResults(*csvStationAvgLatAltOut, ghcn, argc-optind);

      csvStationAvgLatAltOut->close();
      delete csvStationCountOut;
  }

  
  return 0;
  
}

// Generate an ensemble of results -- select stations with replacement
int MainBatchBootstrapMode(int argc, char **argv)
{
  std::vector<GHCN*> ghcn;

  std::set<std::size_t> wmoShortIdSet;
  std::set<std::size_t> wmoCompleteIdSet;
  
  std::map<std::size_t, GHCNLatLong> ghcnLatLongMap;

  std::stringstream headingStrBase;
  std::stringstream headingStr;

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<std::size_t>& wmoCompleteIdSet,
                           std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap);

  void ProcessOptions(int argc, char **argv);

  void ReadMetadataFile(std::set<std::size_t>& wmoCompleteIdSet,
                        std::map<std::size_t,GHCNLatLong>& ghcnLatLongMap);

  void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                           std::set<std::size_t>& wmoCompleteIdSet,
                           std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap);
  
  std::vector<std::size_t> CreateStationIdVector(std::vector<GHCN*>& ghcn, int igh);

  std::fstream* OpenCsvGenericFile(const char* filename, const std::string & strFileAppend);
 
  std::fstream* OpenKmlFile(const char* infilename, const char* csvfilename);

  void DumpBuildRunInfo(std::fstream& fout);
  
  void DumpSpreadsheetHeader(std::fstream& fout);

  void DumpSmoothedAnnualResults(std::fstream& fout,
                                 std::vector<GHCN*>& ghcn, int nghcn);
  void DumpSmoothedMonthlyResults(std::fstream& fout,
                                 std::vector<GHCN*>& ghcn, int nghcn);

  
  void DumpLatLongInfoAsKml(std::fstream& fout,
                            const std::map<std::size_t,GHCNLatLong>& latLongMap);
  
  void WriteKmlData(char** argv, std::vector<GHCN*>& ghcn, int igh);

  void WriteJavaScriptCode(char** argv, std::vector<GHCN*>& ghcn, int igh);

  void WriteBootstrapEnsembleToFile(std::fstream* csvOut,std::vector<GHCN*>&ghcn,int igh);

  void WriteBootstrapStatisticsToFile(std::fstream* csvOut,std::vector<GHCN*>&ghcn,int igh);

  void WriteBootstrapStationCountsToFile(std::fstream* csvOut,std::vector<GHCN*>&ghcn,int igh);
  

  void WriteStationCountToFile(std::fstream* csvStationCountOut,
                               std::vector<GHCN*>&ghcn, int igh);

  
  wmoCompleteIdSet.clear();
  

  std::fstream* csvOut=NULL;
  std::fstream* csvStationCountOut=NULL;
  std::fstream* csvStationAvgLatAltOut=NULL;

  ReadMetadataFile(wmoCompleteIdSet,
                   ghcnLatLongMap);
  

  std::cerr << std::endl;
  std::cerr << "Smoothing filter length = " << avgNyear_g 
            << std::endl << std::endl; 
  
  std::cerr << "Will crunch " << argc-optind << " temperature data set(s). " 
            << std::endl;


  ghcn.resize(argc-optind);
  
  unsigned int unRandSeed=nSeedBootstrap_g;
  int nRandomStationCount=nStationsBootstrap_g;
  int nEnsembleCount=nEnsembleRunsBootstrap_g;
    

  // Loop through the GHCN file command-line args.
  // Note: In bootstrapping mode, can can currently process only 1 temperature data file per run,
  // so will exit at the end of this loop after the first iteration. Leave the loop infrastructure
  // in place in case we decide to extend this capability to multi data file processing.
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
    ghcn[igh]->ComputeBaselines(minBaselineSampleCount_g);

    std::cerr << "Computing average anomalies for " 
         << argv[igh+optind] << std::endl;
    std::cerr << std::endl;
          
    ghcn[igh]->PrecomputeStationAnomalies(minBaselineSampleCount_g);

    headingStrBase.str("");
    headingStrBase<<argv[igh+optind]<<",";
    headingStr.str("");
    headingStr<<headingStrBase.str(); 
    ensembleElementHeadingStr_g.push_back(headingStr.str());

    ::srand(unRandSeed);
    
    // Need to put station map ids into a vector for random selections
    // Use mBaselineSampleCountMap because it references only stations
    // currently in use for this run.

    std::vector<std::size_t>vGhcnLatLongMapIds=CreateStationIdVector(ghcn,igh);
    
    for(int iBootstrap=1; iBootstrap<=nEnsembleCount; ++iBootstrap)
    {
     
        std::map<std::size_t, GHCNLatLong> selectedGhcnLatLongMap;

        ghcn[igh]->SelectStationsGlobalRandom(vGhcnLatLongMapIds,nRandomStationCount,
                                              ghcnLatLongMap,selectedGhcnLatLongMap);

        if(nAnnualAvg_g>0) // Compute annual global averages.
        {
            ghcn[igh]->ComputeGriddedGlobalAverageAnnualAnomalies(minBaselineSampleCount_g,
                                                                  gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                  selectedGhcnLatLongMap);
            // RWM 2025/11/09: Want to accumulate bootstrap statistics from
            // ghcn[igh]->mSmoothedGlobalAverageAnnualAnomaliesMap.
            // Note: the above is cleared internally prior to each ensemble member computation.
            ghcn[igh]->AccumulateGlobalAverageAnnualBootstrapRuns();

            ghcn[igh]->AccumulateGlobalAverageAnnualBootstrapStationCounts();
            
        }
        else // Compute monthly global averages.
        {
            std::cerr<<std::endl;
            std::cerr<<"Monthly-average processing not supported in bootstrap mode. Exiting..."<<std::endl;
            std::cerr<<std::endl;
            exit(1);
            

            ghcn[igh]->ComputeGriddedGlobalAverageMonthlyAnomalies(minBaselineSampleCount_g,
                                                                   gridSizeLatDeg_g,gridSizeLongDeg_g, 
                                                                   selectedGhcnLatLongMap);
            // RWM 2025/11/09: Want to accumulate bootstrap statistics from
            // ghcn[igh]->mSmoothedGlobalAverageYearFractAnomaliesMap.
            // Note: the above is cleared internally prior to each ensemble member computation.

        }
    
        std::cerr<<std::endl;
        std::cerr<<"Bootstrap ensemble run "<<iBootstrap<<" of "
                 <<nEnsembleCount<<" for "<< argv[igh+optind]<<": Number of stations = "
                 << selectedGhcnLatLongMap.size() << std::endl;
        std::cerr<<std::endl;
  
    }

    ghcn[igh]->ComputeGlobalAverageAnnualBootstrapStatistics();
    
    std::cerr <<std::endl<<std::endl;
    std::cerr << "Writing results to " << csvFileName_g <<"..."
              << std::endl<<std::endl<<std::endl;
    
    WriteBootstrapEnsembleToFile(csvOut,ghcn,argc-optind);
    
    WriteBootstrapStatisticsToFile(csvOut,ghcn,argc-optind);

    WriteBootstrapStationCountsToFile(csvStationCountOut,ghcn,argc-optind);

    
    break; // Just one data file iteration for now.
    
  }
  
  return 0;
  
}

void WriteBootstrapEnsembleToFile(std::fstream* csvOut,std::vector<GHCN*>&ghcn,int igh)
{
    std::fstream* OpenCsvGenericFile(const char* csvFileName, const std::string & strNameAppend);
    void DumpBuildRunInfo(std::fstream& fout);
    void DumpSpreadsheetHeader(std::fstream& fout);
    void DumpAnnualBootstrapEnsemble(std::fstream& fout,
                                     std::vector<GHCN*>& ghcn,  int ngh);

    csvOut=OpenCsvGenericFile(csvFileName_g,"_Bootstrap_Ensemble");
    if(csvOut==NULL)
    {
        std::cerr<<std::endl<<"Error opening output CSV file "
                 <<csvFileName_g<<std::endl<<std::endl;
        return;
    }
    
    DumpBuildRunInfo(*csvOut);
    
    DumpSpreadsheetHeader(*csvOut);
    
    if(nAnnualAvg_g>0)
        DumpAnnualBootstrapEnsemble(*csvOut, ghcn, igh);
    else
    {
        std::cerr<<std::endl;
        std::cerr<<"Bootstrapping for monthly average temps not implemented (yet) "<<std::endl;
        std::cerr<<std::endl;
    }
    csvOut->close();
    delete csvOut;
}

void WriteBootstrapStationCountsToFile(std::fstream* csvOut,std::vector<GHCN*>&ghcn,int igh)
{
    std::fstream* OpenCsvGenericFile(const char* csvFileName, const std::string & strNameAppend);
    void DumpBuildRunInfo(std::fstream& fout);
    void DumpSpreadsheetHeader(std::fstream& fout);
    void DumpAnnualBootstrapEnsemble(std::fstream& fout,
                                     std::vector<GHCN*>& ghcn,  int ngh);

    csvOut=OpenCsvGenericFile(csvFileName_g,"_Bootstrap_StationCounts");
    if(csvOut==NULL)
    {
        std::cerr<<std::endl<<"Error opening output CSV file "
                 <<csvFileName_g<<std::endl<<std::endl;
        return;
    }
    
    DumpBuildRunInfo(*csvOut);
    
    DumpSpreadsheetHeader(*csvOut);
    
    if(nAnnualAvg_g>0)
        DumpAnnualBootstrapStationCounts(*csvOut, ghcn, igh);
    else
    {
        std::cerr<<std::endl;
        std::cerr<<"Bootstrapping for monthly average temps not implemented (yet) "<<std::endl;
        std::cerr<<std::endl;
    }
    csvOut->close();
    delete csvOut;
}

void WriteBootstrapStatisticsToFile(std::fstream* csvOut,std::vector<GHCN*>&ghcn,int igh)
{
    std::fstream* OpenCsvGenericFile(const char* csvFileName, const std::string & strNameAppend);
    void DumpBuildRunInfo(std::fstream& fout);
    void DumpSpreadsheetHeader(std::fstream& fout);
    void DumpAnnualBootstrapStationCounts(std::fstream& fout,
                                          std::vector<GHCN*>& ghcn,  int ngh);
    
    csvOut=OpenCsvGenericFile(csvFileName_g,"_Bootstrap_Statistics");
    if(csvOut==NULL)
    {
        std::cerr<<std::endl<<"Error opening output CSV file "
                 <<csvFileName_g<<std::endl<<std::endl;
        return;
    }
    
    DumpBuildRunInfo(*csvOut);
    
    DumpSpreadsheetHeader(*csvOut);
    
    if(nAnnualAvg_g>0)
        DumpAnnualBootstrapStatistics(*csvOut, ghcn, igh);
    else
    {
        std::cerr<<std::endl;
        std::cerr<<"Bootstrapping for monthly average temps not implemented (yet) "<<std::endl;
        std::cerr<<std::endl;
    }
    csvOut->close();
    delete csvOut;

    return;
}

void WriteStationCountToFile(std::fstream* csvStationCountOut,
                             std::vector<GHCN*>&ghcn, int igh)
{
    std::fstream* OpenCsvGenericFile(const char* csvFileName, const std::string & strNameAppend);
    void DumpBuildRunInfo(std::fstream& fout);
    void DumpSpreadsheetHeader(std::fstream& fout);

    csvStationCountOut=OpenCsvGenericFile(csvFileName_g,std::string("_StationCount"));
    if(csvStationCountOut==NULL)
    {
        std::cerr<<std::endl<<"Error opening output companion station count file for "
                 <<csvFileName_g<<std::endl<<std::endl;
        exit(1);
    }
    
    DumpBuildRunInfo(*csvStationCountOut);
    
    DumpSpreadsheetHeader(*csvStationCountOut);
    
    DumpStationCountResults(*csvStationCountOut, ghcn, igh);
    
    csvStationCountOut->close();
    delete csvStationCountOut;
}

std::vector<std::size_t> CreateStationIdVector(std::vector<GHCN*>& ghcn, int igh)
{
    std::vector<std::size_t> vGhcnLatLongMapIds;
    for(auto & im:ghcn[igh]->mBaselineSampleCountMap)
    {
        vGhcnLatLongMapIds.push_back(im.first);
    }
    return vGhcnLatLongMapIds;
};



void ReadTemperatureData(std::vector<GHCN*>& ghcn, int igh, 
                         std::set<std::size_t>& wmoCompleteIdSet,
                         std::map<std::size_t, GHCNLatLong>& ghcnLatLongMap)
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

      case DATA_GHCN_DAILY:
          ghcn[igh]->ReadGhcnDailyTemps(wmoCompleteIdSet);
          break;
          
      case DATA_GHCN:
      default:
          switch(nGhcnVersion_g)
          {
              case 2:
                  ghcn[igh]->ReadGhcnV2TempsAndMergeStations(wmoCompleteIdSet);
                  break;

              case 3:
                  ghcn[igh]->ReadGhcnV3Temps(wmoCompleteIdSet);

              case 4:
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
                            const std::map<std::size_t,GHCNLatLong>& latLongMap);
  

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
                          const std::map<std::size_t,GHCNLatLong>& latLongMap);

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
  
  std::size_t iStart=0;
  std::size_t iEnd=0;
  
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
                          const std::map<std::size_t,GHCNLatLong>& latLongMap)
{
  
  std::string strBoilerplate1="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  std::string strBoilerplate2="<kml xmlns=\"http://www.opengis.net/kml/2.2\">";
  
  std::map<std::size_t,GHCNLatLong>::const_iterator im;
  
  fout<<strBoilerplate1;
  fout<<strBoilerplate2;
  
  fout<<"<Document>"<<std::endl;
  fout<<"<Folder>"<<std::endl;
  
  for(im=latLongMap.begin(); im!=latLongMap.end(); im++)
  {
    fout<<"<Placemark>"<<std::endl;

    std::stringstream sstrInfo;
    std::string strInfo;

    std::string strStationName=im->second.strStationName;

    // Get rid of the underscores & leading/trailing blanks for KML label display
    for(auto& iss:strStationName)
    {
        if(iss=='_')
            iss=' ';
    }
    boost::trim(strStationName);

    // Google Earth gags on embedded blanks in a tag, so
    // replace with underscores.
    // for(auto& iss: strStationName)
    // {
    //     if(iss==' ')
    //         iss='_';
    // }

    sstrInfo<<"Station ID: "<<im->second.strWmoId<<" \n"
            <<"Station Name: "<<strStationName<<"  \n"
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
    fout<<"        <scale>1.0</scale>"<<std::endl;
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
                          const std::map<std::size_t,GHCNLatLong>& latLongMap)
{
  // Not very efficient, but what the hell...
  int ic;
  int nel;
  std::map<std::size_t,GHCNLatLong>::const_iterator im;

  nel=latLongMap.size();

  fout<<"{ \n";

  fout<<"strwmoid=[";
  for(im=latLongMap.begin(),ic=0; im!=latLongMap.end(); im++,ic++)
  {

      fout<<"\""<<im->second.strWmoId<<"\""; 
      if(ic<nel-1)
          fout<<","; 
    
    // Friendlier formatting for easier viewing in text editors
    if((ic+1)%20==0)
      fout<<"\n";
    
  }
  fout<<"];\n";

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
  std::size_t ii;

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

      fout<<"\""<<im->first<<"\"";
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

  void ParseBootstrapMode(char* optarg);

  void ParseMaxReportingYearStr(void);
  void ParseGridSizeStr(void);
  void ParseRecordSpecsStr(char *optArg);
  void ParseStartEndMonthStr(char *optarg);
  void ParseGhcnDailyMode(char *optarg);
  
  void DumpUsage(char *argv0);

  if(argc<2)
  {
    DumpUsage(argv[0]);
    exit(1);
  }
  std::cerr << std::endl;
  
  minBaselineSampleCount_g=0; // GHCN::DEFAULT_MIN_BASELINE_SAMPLE_COUNT;
  avgNyear_g=GHCN::DEFAULT_AVG_NYEAR;
  metadataFile_g=NULL;

  
  gridSizeLatDeg_g=GHCN::DEFAULT_GRID_LAT_DEG;
  gridSizeLongDeg_g=GHCN::DEFAULT_GRID_LONG_DEG;

  // Include stations only within this distance to nearest airport.
  // If <0, process stations greater than the abs value
  // If the default GHCN::IMPOSSIBLE_VALUE, don't filter by airport status.
  fAirportDistKm_g=GHCN::IMPOSSIBLE_VALUE;
  
  
  nUshcnEstimatedFlag_g=0;
  
  while ((optRtn=getopt(argc,argv,"G:D:b:FcCuUWsjJkELyYm:a:A:B:M:S:o:t:P:R:"))!=-1)
  {
    switch(optRtn)
    {
        case 'F':
            minBaselineSampleCount_g=0;
            gridSizeLatDeg_g=180;
            gridSizeLongDeg_g=360;
            break;

        case 'b':
            ParseBootstrapMode(optarg);
            break;
            
        case 'Y':
            nAnnualAvg_g=1;
            break;

        case 'y':
            nAnnualAvg_g=0;
            break;
            
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

      case 'J':
          bNHSummerMonthsOnly_g=true;
          break;

      case 'm':
          bPartialYear_g=true;
          ParseStartEndMonthStr(optarg);
          break;
            
      case 'k':
        bGenKmlFile_g=true;
        break;
        
      case 'G':
          nGhcnVersion_g=atoi(optarg);
          nDataVersion_g=DATA_GHCN;
          if((nGhcnVersion_g<2)||(nGhcnVersion_g>4))
          {
              std::cerr<<std::endl;
              std::cerr<<"Valid G options are 2, 3, or 4 (for GHCN versions 2, 3, or 4)";
              std::cerr<<std::endl;
              exit(1);
          }
        break;

      case 'D':
      nGhcnVersion_g=1;
      nDataVersion_g=DATA_GHCN_DAILY;
      ParseGhcnDailyMode(optarg);
      std::cerr<<std::endl;
      std::cerr<<"Processing GHCN Daily data: nGhcnDailyMode_g="<<nGhcnDailyMode_g<<std::endl;
      std::cerr<<std::endl;
      break;

      case 'c':
        nCruVersion_g=3;
        nDataVersion_g=DATA_CRU;
        std::cerr<<std::endl;
        std::cerr<<"CRUTEM 3 is no longer actively supported."<<std::endl;
        std::cerr<< "Only GHCNv4 (-G) and USHCN (-U) are supported."<<std::endl;
        std::cerr<<std::endl;
        exit(1);
        break;

      case 'C':
        nCruVersion_g=4;
        nDataVersion_g=DATA_CRU;
        std::cerr<<std::endl;
        std::cerr<<"CRUTEM 4 is no longer actively supported."<<std::endl;
        std::cerr<< "Only GHCNv4 (-G) and USHCN (-U) are supported."<<std::endl;
        std::cerr<<std::endl;
        exit(1);
        break;

#if(0) // Appropriating '-b' for bootstrapping mode
      case 'b':
        nDataVersion_g=DATA_BEST;
        std::cerr<<std::endl;
        std::cerr<<"BEST is no longer actively supported."<<std::endl;
        std::cerr<< "Only GHCNv4 (-G) and USHCN (-U) are supported."<<std::endl;
        std::cerr<<std::endl;
        exit(1);
        break;
#endif
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
          // Rural/Urban/Suburban designator (GHCNv2/3, GHCNv4 with priscian supplemental metadata,
          // and USHCN with hausfather supplemental metadata only)
          strStationType_g=std::string(optarg);
        break;
        
      case 'P':
        nServerPort_g=atoi(optarg);
        break;
        
      case 'L':
        bComputeAvgStationLatAlt_g=true;
        break;
        
      default:
        std::cerr << std::endl;
        DumpUsage(argv[0]);
        exit(1);
    }
  }

  avgNyear_g=MAX(1,MIN(GHCN::MAX_AVG_NYEAR,avgNyear_g));

  // If minBaselineSampleCount<=0, this will disable baselining,
  // so don't want to impose this baseline sample count check for this case.
  if(minBaselineSampleCount_g>0)
  {
      minBaselineSampleCount_g=MAX(1,
                                   MIN(GHCN::LAST_BASELINE_YEAR-GHCN::FIRST_BASELINE_YEAR+1,
                                       minBaselineSampleCount_g));
  }

  if(true==bSetStationRecordSpecs_g)
  {
      if((nDataVersion_g!=DATA_GHCN)&&(nDataVersion_g!=DATA_USHCN)&&(nDataVersion_g!=DATA_GHCN_DAILY))
      {
          std::cerr<<"The -R option is currently supported only for GHCN&USHCN data."<<std::endl;
          std::cerr<<std::endl;
          exit(1);
      }
  }

  if((nServerPort_g<=1024) && (NULL==csvFileName_g))
  {
    std::cerr << std::endl;
    std::cerr << "This is a batch-mode processing run. " << std::endl;
    std::cerr << "Did you forget to supply an output csv file name? (-o command-line arg) " << std::endl;
    std::cerr << std::endl;
    exit(1);
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

void ParseBootstrapMode(char *optarg)
{
    std::string strBootstrapMode=std::string(optarg);

    std::vector<std::string> vStrBootstrapMode;
    boost::split(vStrBootstrapMode,strBootstrapMode, boost::is_any_of(":"));
    if(vStrBootstrapMode.size()!=3)
    {
        std::cerr<<std::endl;
        std::cerr<<"Bootstrap (-b) arg must be nSeed:nStations:nEnsemble"<<std::endl;
        std::cerr<<std::endl;
        exit(1);
    }
    try
    {
        nSeedBootstrap_g=boost::lexical_cast<int>(vStrBootstrapMode.at(0));
        nStationsBootstrap_g=boost::lexical_cast<int>(vStrBootstrapMode.at(1));
        nEnsembleRunsBootstrap_g=boost::lexical_cast<int>(vStrBootstrapMode.at(2));
    }
    catch(...)
    {
        std::cerr<<std::endl;
        std::cerr<<"Bootstrap args must be (int)seed:(int)nStations:(int)nEnsembleRuns"<<std::endl;
        std::cerr<<std::endl;
        exit(1);
    }
    return;
}

void ParseGhcnDailyMode(char *optarg)
{
    std::string strGhcnDailyMode=std::string(optarg);

    void DumpGhcnDailyModeInfo(void);

    try
    {
        std::string strDailyDataMode=std::string(optarg);

        std::vector<std::string> vStrDailyDataMode;
        boost::split(vStrDailyDataMode,strDailyDataMode, boost::is_any_of(":"));
        if((vStrDailyDataMode.size()!=2)&&(vStrDailyDataMode.size()!=3))
        {
            // Throw an appropriate exception here.
            std::exception e;
            throw(e);
        }

        std::string strDailyData;
        std::string strDailyMode;
        
        strDailyData=vStrDailyDataMode.at(0);
        strDailyMode=vStrDailyDataMode.at(1);
        
        boost::trim(strDailyData);
        boost::trim(strDailyMode);

        nGhcnDailyData_g=boost::lexical_cast<int>(strDailyData);
        nGhcnDailyMode_g=boost::lexical_cast<int>(strDailyMode);

        if(vStrDailyDataMode.size()==3)
        {
            std::string strDaysMonth=vStrDailyDataMode.at(2);
            boost::trim(strDaysMonth);
            // Have an nDaysMonth arg
            nGhcnDailyMinDaysPerMonth_g=boost::lexical_cast<int>(strDaysMonth);
        }
        if(nGhcnDailyMinDaysPerMonth_g<1)
            nGhcnDailyMinDaysPerMonth_g=1;
        else if(nGhcnDailyMinDaysPerMonth_g>28)
            nGhcnDailyMinDaysPerMonth_g=28;
        
        if((nGhcnDailyData_g<0)||(nGhcnDailyData_g>4) ||
           (nGhcnDailyMode_g<0)||(nGhcnDailyMode_g>2))
        {
            std::exception e;
            throw e;
        }
    }
    catch(...)
    {
        DumpGhcnDailyModeInfo();
        exit(1);
    }
}

void DumpGhcnDailyModeInfo(void)
{
    std::cerr<<std::endl;
    std::cerr<<"Must provide a valid TDATA:TMODE[:nMinDaysPerMonth] specs per below."<<std::endl;
    std::cerr<<" TDATA must be 0 (for (TMIN+TMAX)/2), 1 (for TMIN), 2 (for TMAX), 3 (for TAVG from hourly),"<<std::endl;
    std::cerr<<" or 4 (for (TMAX+TMIN)/2 only from stations that also have TAVG from hourly data)."<<std::endl;
    std::cerr<<" MODE must be 0 (for AVG), 1 (for MIN), or 2 (for MAX)."<<std::endl;
    std::cerr<<" Examples below: "<<std::endl;
    std::cerr<<"    0:0:25 Set each monthly val to the average of all daily TAVG temps, min 25 days/month."<<std::endl;
    std::cerr<<"    0:1 Set each monthly val to the mininum of all daily TAVG temps, default min 15 days/month."<<std::endl;
    std::cerr<<"    0:2 Set each monthly val to the maximum of all daily TAVG temps, default min 15 days/month."<<std::endl;
    std::cerr<<std::endl;
    std::cerr<<"    1:0 Set each monthly val to the average of all daily TMIN temps, default min 15 days/month."<<std::endl;
    std::cerr<<"    1:1 Set each monthly val to the minimum of all daily TMIN temps, default min 15 days/month."<<std::endl;
    std::cerr<<"    1:2 Set each monthly val to the maximum of all daily TMIN temps, default min 15 days/month."<<std::endl;
    std::cerr<<std::endl;
    std::cerr<<"    2:0:10 Set each monthly val to the average of all daily TMAX temps, min 10 days/month."<<std::endl;
    std::cerr<<"    2:1:19 Set each monthly val to the minimum of all daily TMAX temps, min 19 days/month."<<std::endl;
    std::cerr<<"    2:2:25 Set each monthly val to the maximum of all daily TMAX temps, min 25 days/mpnth."<<std::endl;
    std::cerr<<std::endl;
}


void ParseStartEndMonthStr(char *optarg)
{
    std::string strStartEndMonth=std::string(optarg);
    std::vector<std::string> vStrStartEndMonth;
    try
    {
        boost::split(vStrStartEndMonth,strStartEndMonth,boost::is_any_of(":"));
        if(vStrStartEndMonth.size()!=2) 
        {
            throw std::invalid_argument("Invalid start:end month:[num-days] arg");
        }
        std::string strStartMonth=vStrStartEndMonth.at(0);
        boost::trim(strStartMonth);
        
        std::string strEndMonth=vStrStartEndMonth.at(1);
        boost::trim(strEndMonth);
        
        nStartMonth_g=boost::lexical_cast<int>(strStartMonth);
        nEndMonth_g=boost::lexical_cast<int>(strEndMonth);

        if((nStartMonth_g<1) || (nStartMonth_g>12) || (nEndMonth_g<1) || (nEndMonth_g>12))
        {
            throw std::invalid_argument("Start and end months must be 1-12");
        }
    }
    catch(...)
    {
        std::cerr<<std::endl;
        std::cerr<<"Must provide a valid start:end month pair (values 1-12) with the -m option."<<std::endl;
        std::cerr<<std::endl;
    }
    return;
}

void ParseRecordSpecsStr(char *optarg)
{
  std::string strRecordLength;
  char *tok;
  bool bValidFormat=true;
  std::size_t nStrPos;
  std::size_t nRevStrPos;
  
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
    ::system("clear");
    std::cerr<<std::endl;
    AnomalyVersion::DumpVersionInfo(std::cerr);
    std::cerr<<std::endl;
    char* ptr=::readline("Hit Enter to continue -->");
    ::free(ptr);
    std::cerr << std::endl;
    std::cerr << "DISCLAIMER -- The info in this usage: dump is not necessarily up to date."<<std::endl;
    std::cerr<<std::endl;
    std::cerr << "              GHCN monthly v2-v4, GHCN daily and USHCN monthly data formats"<<std::endl;
    std::cerr << "              have been recently tested. Support for CRU 3/4 and BEST is not guaranteed."<<std::endl; 
    std::cerr << std::endl;

    std::cerr << std::endl 
              << "Usage: " << argv0  << "  \\ " << std::endl
              << "         [-F] Fuckup mode. Disable baselining and area-weighting."<<std::endl
              << "         [-G 2, 3, or 4] Process GHCN monthly version 2, 3, or 4. " <<std::endl
              << "         [-D TData:TMode[:MinDays]] TData=0 for GHCND Tavg, 1 for GHCND TMin, or 2 for GHCND TMax." <<std::endl
              << "                  Tmode=0 to compute the monthly average, 1 to find the minimum, or 2 to find the maximum." <<std::endl
              << "                  MinDays=1-28 for min number of days/month for valid monthly result."<<std::endl
              << "         [-u or -U] Process USCHN data.  " << std::endl
              << "         -M (char*)temp-station-metadata-file. " << std::endl
              << "         -o (char*)csv-output-file  " << std::endl
              << "         [-b nSeed:nStations:nRuns] Bootstrap mode. nSeed=random# seed, nStations=#random stations, nRuns=#bootstrap runs."<<std::endl
              << "         [-j] Generate station javascript metadata array file.  " <<std::endl
              << "         [-J] Process June/July/August temps only (GHCN V4 & GHCN Daily only).  " <<std::endl
              << "         [-m start-month:end-month] Process partial year data from start-month to end-month 1-12."<<std::endl
              << "         [-k] Generate a .kml file -- written to the input data file directory.  " <<std::endl
              << "         [-c] Process CRU v3 data.   "<<std::endl
              << "         [-C] Process CRU v4 data.  "<<std::endl
              << "         [-E] Include interpolated/estimated USHCN monthly temp data. " << std::endl
              << "         [-t station_type] A/B/C:rural/suburban/urban for GHCNv3/4, R/S/U:rural/suburban/urban for GHCN v2, R/U:rural/urban for USHCN csv." << std::endl
              << "         [-Y or -y ] optional, -Y for annual average output, -y for monthly average output. Default -Y for annual output."<<std::endl 
              << "         [-A (int)smoothing-filter-length-years]  " << std::endl
              << "         [-B (int)min-baseline-sample-count]  "     << std::endl
              << "         [-E ] USHCN adjusted only -- include estimated values "<<std::endl
              << "         [-S grid-size-lat-deg:grid-size-long-deg]  " << std::endl
              << "         [-W] Normalize grid-cell averages by grid-cell size -- this has a small impact. "<< std::endl 
              << "         [-s] Enable sparse station processing: use the single longest-record station per grid cell. " << std::endl
              << "         [-R  (char*)StationRecordSpec]  " << std::endl 
              << "         [-L ] Calculate average latitude & altitude of active stations.  " << std::endl
              << "         [-P (int)server-listening-port]  " <<std::endl
              << "         (char*)data-source1 (char*)data-source2... " << std::endl
              << "         For GHCN, CRU3, and BEST data, the data-sourceX sources are files."<<std::endl
              << "         For USHCN and CRU4 data, the data-sourceX sources are directories"<<std::endl
              << "         that contain USHCN or CRU4 temperature data files."<<std::endl;
    std::cerr << std::endl;
    
    std::cerr << "   Options/Data-filenames can be intermingled (possibly not on MacOS)."<<std::endl;
    std::cerr << std::endl;

    ptr=::readline("Hit Enter to continue --> ");
    ::free(ptr);
    
    std::cerr<<std::endl;
    std::cerr << "The -M flag (mandatory except for HADCrut4): " << std::endl << std::endl;
    std::cerr << "     Specifies the metadata file name. " << std::endl << std::endl;
    std::cerr << "     The metadata-file must be the appropriate metadata format." << std::endl;
    std::cerr << "     For GHCN v4 metadata, must use Jim Java's supplentary GHCNv4 metadata file,"<<std::endl;
    std::cerr << "     available at:"<<std::endl;
    std::cerr << "     https://raw.githubusercontent.com/priscian/climeseries/master/inst/extdata/instrumental/GHCN/v4.temperature.inv%2Bpopcss.csv"<<std::endl;
    std::cerr<<std::endl;
    std::cerr << "     For USHCN data, use the metadata file with rural/urban classifications available at: "<<std::endl;
    std::cerr << "     https://www.ncei.noaa.gov/pub/data/ushcn/papers/hausfather-etal2013-suppinfo/metadata/"<<std::endl;
    std::cerr << "     Copy and paste the above URLs to your web-browser URL bar."<<std::endl;
    std::cerr << std::endl; 
    
    std::cerr << std::endl;
    std::cerr << "The -o flag (mandatory except for server-mode): " << std::endl << std::endl;
    std::cerr << "     Specifies the output .csv file. The .csv file contains the spreadsheet plottable "<< std::endl
              << "     temperature anomaly results." << std::endl;
    
    std::cerr << std::endl << std::endl;
    std::cerr << "The -j flag: " << std::endl << std::endl;
    std::cerr << "     Generate a JavaScript (.js) file. The .js file name will be a composite of the "<< std::endl
              << "     corresponding data file name and .csv file name -- used with the new Google Map frontend." << std::endl;

    std::cerr << std::endl << std::endl;
    std::cerr << "The -J flag: " << std::endl << std::endl;
    std::cerr << "     Process NH summer months only (JJA). "<<std::endl;
    
    std::cerr << std::endl << std::endl;
    std::cerr << "The -k flag: " << std::endl << std::endl;
    std::cerr << "     Generate a GoogleEarth .kml file. The .kml file name will be a composite of the "<< std::endl
              << "     corresponding data file name and .csv file name." << std::endl;
    std::cerr << std::endl << std::endl;
    ptr=::readline("Hit Enter to continue --> ");
    ::free(ptr);
    std::cerr<<std::endl;

    std::cerr << "Only one of -G -D,-c, -C, -b, or -U may be specified. " <<std::endl;
    std::cerr << "IMPORTANT NOTE: Only -G 2/3/4 (GHCNv2-4 monthly), -D(GHCN daily), and -U(USHCN monthly) are currently supported. "<<std::endl;
    std::cerr << std::endl;
    std::cerr << "-G 4: GHCNv4 monthly data note. Customized metadata file reqired."<<std::endl;
    std::cerr << "      For GHCN v4 metadata, must use Jim Java's supplentary GHCNv4 metadata file,"<<std::endl;
    std::cerr << "      available at:"<<std::endl;
    std::cerr << "     https://raw.githubusercontent.com/priscian/climeseries/master/inst/extdata/instrumental/GHCN/v4.temperature.inv%2Bpopcss.csv"<<std::endl;
        std::cerr<<std::endl;
    std::cerr << "-D:[0-3]:[0-2][minDays] GHCN daily data. "<<std::endl;
    std::cerr << "    -D 0:0 for Tavg average,-D 0:1 for Tavg min,-D 0:2 for Tavg max,-D 1:0 for Tmin avg, "<<std::endl;
    std::cerr << "    -D 1:1 for Tmin min, -D 1:2 for Tavg max, -D 2:0 for Tmax avg, -D 2:1 for Tmax min, -D 2:2 for Tmax max."<<std::endl;
    std::cerr << "    -D 2:0 for TAVG (from hourly) min, etc. "<<std::endl;
    std::cerr <<std::endl;
    std::cerr << "    Note: To create a usable GHCN daily data file, preprocess the NOAA ghcnd_all.tar.gz file to extract the "<<std::endl;
    std::cerr << "    desired temperature data per the steps below."<<std::endl;
    std::cerr << "      1) Use ftp to connect to ftp.ncdc.noaa.gov (use 'ftp' as the login ID)"<<std::endl;
    std::cerr << "      2) Then cd to pub/data/ghcn/daily."<<std::endl;
    std::cerr << "      3) Download the ghcnd_all.tar.gz(data) and ghcnd-stations.txt(metadata) files."<<std::endl;
    std::cerr << "      4) Extract the TMIN/TMAX temperature data from the ghcnd_all.tar.gz file via this script: "<<std::endl;
    std::cerr << "          $ pv ghcnd_all.tar.gz | tar zxf - -O | grep 'TMIN\\|TMAX' >ghcnd_all_tmin_tmax.dly "<<std::endl;
    std::cerr << "         The ghcnd_all_tmin_tmax.dly file will contain the extracted TMIN & TMAX data used by this app."<<std::endl;
    std::cerr << "      5) To extract TAVG (from hourly readings) data from ghcnd_all.tar.gz, use this command scripg: "<<std::endl;
    std::cerr << "          $ pv ghcnd_all.tar.gz | tar zxf - -O | grep 'TAVG' >ghcnd_all_tavg.dly "<<std::endl;
    std::cerr << "        "<<std::endl;
    std::cerr <<std::endl;
    std::cerr << "    The '-U' flag specifies USHCN data. "<<std::endl;
    std::cerr << "    USHCN metadata with Hausfather 2013 et al. rural/urban classifications available from: "<<std::endl;
    std::cerr << "    https://www.ncei.noaa.gov/pub/data/ushcn/papers/hausfather-etal2013-suppinfo/metadata/"<<std::endl;
    std::cerr << std::endl;
    ptr=::readline("Hit Enter to continue -->");
    #if(0)
    std::cerr << "     The '-G' flag + specifiers 2, 3, or 4 specify GHCN version 2, 3, or 4 data, respectively. " 
              << std::endl
              << "     The '-c' or '-C' flag specifies CRU v3 or v4 \"climategate\" data, respectively. " 
              << std::endl
              << "     The '-b' flag specifies BEST data. " << std::endl;
    std::cerr << "     The '-u' or '-U' flag specifies USHCN data. "<<std::endl;
    std::cerr << "     USHCN metadata with Hausfather 2013 et al. rural/urban classifications available from: "<<std::endl;
    std::cerr << "     https://www.ncei.noaa.gov/pub/data/ushcn/papers/hausfather-etal2013-suppinfo/metadata/"<<std::endl;
    std::cerr<<std::endl;
   #endif
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
    std::cerr <<"The -a option: (GHCN monthly only)"<<std::endl<<std::endl;
    std::cerr <<"    Airport status option: "<<std::endl;
    std::cerr <<"    0: process non-airport stations only."<<std::endl;
    std::cerr <<"    1: process airport stations only."<<std::endl;
    std::cerr <<"    2: process all (airport & non-airport) stations."<<std::endl;
    std::cerr << std::endl;
    std::cerr << std::endl << std::endl;
    std::cerr << " The -s option: "<<std::endl<<std::endl;
    std::cerr << "      Enable sparse station selection (single longest-record station per grid-cell) " << std::endl; 
    std::cerr<<std::endl;

    ptr=::readline("Hit Enter to continue --> ");
    ::free(ptr);
  
    std::cerr << std::endl << std::endl;
    std::cerr << " The -R option: "<<std::endl<<std::endl;
    std::cerr << "      Process data from stations with a records spec'd per one of the following: " << std::endl;
    std::cerr << "        StationRecordSpec: nYears -- temp. record length must be at least nYears long."<<std::endl;
    std::cerr << "        StationRecordSpec: StartYear: -- temp. record must start no later than StartYear."<<std::endl;
    std::cerr << "        StationRecordSpec: :EndYear -- temp. record must end no earlier than EndYear."<<std::endl;
    std::cerr << "        StationRecordSpec: StartYear:EndYear -- temp. record must start no later than StartYear."<<std::endl;
    std::cerr << "                                                and end no earlier than EndYear."<<std::endl;
    std::cerr << " Fully functional only with GHCN & USHCN data."<<std::endl;
    std::cerr << std::endl << std::endl;
    std::cerr << "      Incompatible with the -s option."<<std::endl;
    std::cerr << std::endl << std::endl;
    std::cerr << " The -W option:  "<<std::endl<<std::endl;
    std::cerr << "       Enable corrections for grid-cell area variations " << std::endl;
    std::cerr << "       Combining -s and -W options not recommended." <<std::endl;
    std::cerr << std::endl << std::endl;
    std::cerr << "The -P option: " << std::endl << std::endl;
    std::cerr << "     For server-mode: Specifies the listening port -- use with hacked QGIS. "<<std::endl;
    std::cerr << "     Server-mode is supported for GHCN V4 data only. " <<std::endl;
    std::cerr << std::endl << std::endl;
  
    std::cerr << std::endl;
 
    ptr=::readline("Hit Enter to continue --> ");
    ::free(ptr);
  
    std::cerr << "Example for GHCN V4 data: "<< std::endl << std::endl;
    std::cerr << "  ./anomaly.exe -G 4 -M v4_ABC_airports.csv -B 20 -S 10:15 -A 5 v4_raw.dat v4_adj.dat -o output.csv " << std::endl << std::endl
              << "      Processes v4_raw.dat and v4_adj.dat GHCN V4 files. " << std::endl << std::endl
              << std::endl 
              << "      -M: Reads GHCN v4_ABC_airports.inv metadata file (includes Priscian supplemental info)." << std::endl<<std::endl
              << "      Priscian supplemental metadata file must be used with GHCNv4 and is available at:"<<std::endl
              << "      https://raw.githubusercontent.com/priscian/climeseries/master/inst/extdata/instrumental/GHCN/v4.temperature.inv%2Bpopcss.csv"<<std::endl
              << "      Copy and paste to your web-browser URL bar."<<std::endl          
              << std::endl
              << "      -B: Require at least 20 years of valid data during the baseline period " << std::endl
              << std::endl
              << "      -A: Outputs will be smoothed with a moving-average filter of length 5 years. " << std::endl
              << std::endl 
              << "      -S: Initial (at the equator) grid-cell size will be 10 deg lat, 15 deg long. " << std::endl
              << std::endl
              << "      -o: Data will be written to output.csv in csv, with 1 column per set of global annual " << std::endl
              << "          anomalies. " << std::endl<<std::endl;
    std::cerr << "Example for USHCN data:"<<std::endl<<std::endl;
    std::cerr<<"  ./anomaly.exe -U -M  ushcn-proxy-metadata.csv -m 6:8 -t R -B 1 -A 9 -S 2.5:2.5 -W -o results.csv $3 ushcn.v2.5.5.YYYYMMDD"<<std::endl;
    std::cerr<<"  This computes 9 yr moving averages for months 6-8 (Jun-Aug) from rural USHCN station data in the ushcn.v2.5.5.YYYYMMDD directory."<<std::endl;
    std::cerr<<"  The USHCN metadata file ushcn-proxy-metadata.csv (with Hausfather 2013 et al. rural/urban classifications) "<<std::endl;
    std::cerr<<"  is available from: "<<std::endl;
    std::cerr << "     https://www.ncei.noaa.gov/pub/data/ushcn/papers/hausfather-etal2013-suppinfo/metadata/"<<std::endl;
    std::cerr<<std::endl;

  
    std::cerr << std::endl << std::endl;

 //   std::cerr << "To page this message for easier reading, do this: " << std::endl << std::endl
 //             << "     "<<argv0<<" 2>&1 | less " << std::endl << std::endl;
  
  return;
    
}


void ReadMetadataFile(std::set<std::size_t>& wmoCompleteIdSetOut,
                      std::map<std::size_t,GHCNLatLong>& ghcnLatLongMapOut)
{
  void ReadGhcnMetadataFileV2(const char* infile,
                              const std::string& strStationType,
                              std::set<std::size_t>& wmoCompleteIdSetOut,
                              std::set<std::size_t>& wmoGhcnV2DeprecatedIdSet,
                              std::map<std::size_t,GHCNLatLong>& latLongMap);

  void ReadGhcnMetadataFileV3(const char* infile,
                              const std::string& strStationType,
                              std::set<std::size_t>& wmoCompleteIdSetOut,
                              std::map<std::size_t,GHCNLatLong>& latLongMap);
  // Reads standard GHCNv4 metadata file.
  void ReadGhcnMetadataFileV4(const char* infile,
                              const std::string& strStationType,
                              std::set<std::size_t>& wmoCompleteIdSetOut,
                              std::map<std::size_t,GHCNLatLong>& latLongMap);

  // Reads extended GHCNv4 metadata file with rural/mixed/urban classifications.
  // https://raw.githubusercontent.com/priscian/climeseries/master/inst/extdata/instrumental/GHCN/v4.temperature.inv%2Bpopcss.csv
  void ReadGhcnMetadataFileV4b(const char* infile,
                               const std::string& strStationType,
                               const float& fAirportDistKm, // Max distance to nearest airport. If<0 ignore
                               std::set<std::size_t>& wmoCompleteIdSetOut,
                               std::map<std::size_t,GHCNLatLong>& latLongMap);

  void ReadGhcnDailyMetadataFile(const char* infile,
                               std::set<std::size_t>& wmoCompleteIdSetOut,
                               std::map<std::size_t,GHCNLatLong>& latLongMap);

  // Process stations with brightness index >= nBrightnessIndex
  void ReadGhcnMetadataFileV4b2(const char* inFile,
                               const int& nBrightnessIndex, // satellite night brightness index.
                               std::set<std::size_t>& wmoIdSetOut,
                               std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut);

  void ReadCru3MetadataFile(const char* inFile,
                            std::set<std::size_t>& wmoCompleteIdSetOut,
                            std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOutMap);

  void ReadBestMetadataFile(const char* inFile,
                            std::set<std::size_t>& wmoCompleteIdSetOut,
                            std::map<std::size_t, GHCNLatLong >& bestLatLongMap);

  void ReadUshcnMetadataFile(const char* inFile,
                             const std::string& strStationType,
                             std::set<std::size_t>& wmoCompleteIdSetOut,
                             std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOutMap);

  // Hausfather et al. 2013 USHCN metadata with rural/urban classifications.                           
  void ReadUshcnMetadataCsvFile(const char* inFile,
                              const std::string& strStationType,
                              std::set<std::size_t>& wmoCompleteIdSetOut,
                              std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut);


  // old GHCN V2 WMO id list that is no longer used
  // just read this in and throw away...
  std::set<std::size_t> wmoGhcnV2DeprecatedIdSet; 
  
  if(metadataFile_g!=NULL)
  {
    // GHCN V2/V3, HadCRUT3, and BEST
    // have separate metadata files.
    // HadCRUT4 doesn't.
      std::string strMetadataFile=std::string(metadataFile_g);
      
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
            if((strMetadataFile.find(".csv")==std::string::npos)&&
               (strMetadataFile.find(".CSV")==std::string::npos))
                ReadUshcnMetadataFile(metadataFile_g,
                                      strStationType_g,
                                      wmoCompleteIdSetOut,
                                      ghcnLatLongMapOut);
            else
                ReadUshcnMetadataCsvFile(metadataFile_g,
                                         strStationType_g,
                                         wmoCompleteIdSetOut,
                                         ghcnLatLongMapOut);
            break;

        case DATA_GHCN_DAILY:
            ReadGhcnDailyMetadataFile(metadataFile_g,
                                      wmoCompleteIdSetOut,
                                      ghcnLatLongMapOut);
            break;
            
      case DATA_GHCN:
      default:

        switch(nGhcnVersion_g)
        {
          case 2:
            ReadGhcnMetadataFileV2(metadataFile_g,
                                   strStationType_g,
                                   wmoGhcnV2DeprecatedIdSet, /* no longer used */
                                   wmoCompleteIdSetOut,/* was wmShortIdSet, */
                                   ghcnLatLongMapOut);
            break;

            case 3:
                ReadGhcnMetadataFileV3(metadataFile_g,
                                       strStationType_g,
                                       wmoCompleteIdSetOut,
                                       ghcnLatLongMapOut);
                break;
            case 4:
            default:
//              Original V4 metadata processing disabled 2025/06/22
//              This is the original GHCNv4 metadata file as supplied by NOAA.
//              No rural/urban/ or airport/non-airport status info.             
#if(0)
              ReadGhcnMetadataFileV4(metadataFile_g,
                                     strStationType_g,
                                     wmoCompleteIdSetOut, 
                                     ghcnLatLongMapOut);
#endif
#if(1) // For priscian extended metadata w/airport info. Has urban/rural & airport/non-airport status info.
              ReadGhcnMetadataFileV4b(metadataFile_g,
                                      strStationType_g,
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
//    if((DATA_CRU!=nDataVersion_g)&&(4!=nCruVersion_g))
    {
      std::cerr << std::endl;
      std::cerr << "Must supply an appropriate metadata file (command-line flag -M)" 
                << std::endl;
      std::cerr << std::endl;
      exit(1);
    }
    
  }

  if(ghcnLatLongMapOut.size()==0)
  {
      std::cerr<<std::endl;
      std::cerr<<"Did not extract any station metadata."<<std::endl;
      std::cerr<<"Did you supply the right metadata file?"<<std::endl;
      std::cerr<<"If you are processing GHCNv4 data, make sure that you "<<std::endl;
      std::cerr<<"using the right version (standard vs Jim Jave extended)."<<std::endl;
      std::cerr<<std::endl;
      exit(1);
  }
  
  return;
}

void DumpStationCountResults(std::fstream& fout,
                             std::vector<GHCN*>& ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<std::size_t, float >::iterator iyy;
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

void DumpStationAvgLatAltResults(std::fstream& fout,
                                 std::vector<GHCN*>& ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<int,std::pair<float,float> >::iterator iyy;
  int igh;

  fout<<"Year,"<<"#Stations,"<<"AvgLat,"<<"AvgAlt"<<std::endl;
  
  // Iterate over years
  for(iyy=ghcn[0]->m_mapYearStationAvgLatAlt.begin();
      iyy!=ghcn[0]->m_mapYearStationAvgLatAlt.end();
      iyy++)
  {
    // Results for first input file
    // First 
      fout << iyy->first << ","
           <<ghcn[0]->m_mapYearStationCountAvgLatAlt[iyy->first]<<","
           << iyy->second.first<< ","
           <<iyy->second.second;

    if(ngh>1)
      fout << ",";

    // Results for additional input files
    for(igh=1; igh<ngh; igh++)
    {
      if(ghcn[igh]->m_mapYearStationAvgLatAlt.find(iyy->first) != 
         ghcn[igh]->m_mapYearStationAvgLatAlt.end())
      {
          int nYear=iyy->first;
          fout <<ghcn[igh]->m_mapYearStationCountAvgLatAlt[iyy->first]<<","
               <<ghcn[igh]->m_mapYearStationAvgLatAlt[iyy->first].first<<","
               <<ghcn[igh]->m_mapYearStationAvgLatAlt[iyy->first].second;
        // Don't need a trailing comma after the last field
        if(igh<ngh-1)
             fout << ",";
      }
      else
      {
        // No valid data for this year -- put in a blank/null csv placeholder
        // Also, don't need a trailing comma after the last field
        if(igh<ngh-1)
            fout << ",";;
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




void ReadGhcnMetadataFileV2(const char* inFile,
                            const std::string& strStationType, // GHCNv3: R(ural), U(rban), S(uburban), or A(ll)
                            // GHCNv4: A(rural),  B(suburban ),  C(urban)
                            std::set<std::size_t>& wmoCompleteIdSetOut,  // remapped to wmoCompleteIdSetOut
                            std::set<std::size_t>& wmoGhcnV2DeprecatedIdSet, // old GHCN V2 wmo id's that are no longer used
                            std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
{
  
  std::fstream* instream=NULL;

  char inBuf[512];
  
  std::map<std::size_t,int> wmoCount;
  std::map<std::size_t,float> wmoLatSum;
  std::map<std::size_t,float> wmoLongSum;
  
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
  
  std::size_t ssl;
  
  GHCNLatLong latLong;
  

  //ccountry = &inBuf[ichar];
  ichar=0;
  ichar+=3;
  cwmoId = &inBuf[ichar];

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

      std::string strWmoId=std::string(inBuf).substr(0,11);
      size_t nWmoStationId=(std::size_t)str_hash(strWmoId);

      if(strStationType != "X") // "X" means process all stations
      {
          if(strStationType.find(inBuf[67])==std::string::npos) // Rural/Urban/Suburban designator -- V2 col 67.
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
      
      
      sscanf(cLat,"%f",&latLong.lat_deg);
      sscanf(cLong,"%f",&latLong.long_deg);
      
      // Translate to positive values only (for easier location comparisons)
      latLong.lat_deg+=90;  
      latLong.long_deg+=180;
      
      wmoCompleteIdSetOut.insert(nWmoStationId);
      wmoGhcnV2DeprecatedIdSet.insert(nWmoStationId);
      if(ghcnLatLongMapOut.find(nWmoStationId)==ghcnLatLongMapOut.end())
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

        ghcnLatLongMapOut[nWmoStationId]=latLong;
        wmoCount[nWmoStationId]=1;
        wmoLatSum[nWmoStationId]=latLong.lat_deg;
        wmoLongSum[nWmoStationId]=latLong.long_deg;
      }
      else
      {
        wmoCount[nWmoStationId]+=1;
        wmoLatSum[nWmoStationId]+=latLong.lat_deg;
        wmoLongSum[nWmoStationId]+=latLong.long_deg;
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
  std::map<std::size_t, GHCNLatLong >::iterator ighcn;
  std::size_t wmoid;
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
                            const std::string& strStationType,
                            std::set<std::size_t>& wmoIdSetOut,
                            std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  std::size_t nWmoId=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);

      // Add ability to skip over commented-out lines.
      std::string strStreamTmp=std::string(inBuf);
      boost::trim_left(strStreamTmp);
      if(strStreamTmp.at(0)=='#')
          continue;

      std::hash<std::string> str_hash;
      std::string strWmoId=std::string(inBuf).substr(0,11);
      size_t nWmoId=(std::size_t)str_hash(strWmoId);

      
      if(strStationType != "X") // 'X' means process all stations
      {
          if(strStationType.find(inBuf[106])==std::string::npos) // Rural/Urban/Suburban designator -- V2 col 73.
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

// For v4b_ABC.inv, 'A'=rural, 'B'=suburban, 'C'=urban
void ReadGhcnMetadataFileV4b(const char* inFile,
                             const std::string & strStationTypeIn,
                             const float& fAirportDistKmIn, // max distance to nearest airport. If <0, ignoere
                             std::set<std::size_t>& wmoIdSetOut,
                             std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  std::size_t nWmoId=0;
  const char *cLat;
  const char *cLong;
  const char *cAltitude;
  const char *cStationName;
  std::string strStationType;
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

        // Add ability to skip over commented-out lines.
        std::string strStreamTmp=strStream;
        // Don't trim the strStream string that we want to parse.
        boost::trim_left(strStreamTmp);
        if(strStreamTmp.at(0)=='#')
            continue;
        
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

        latLong.strWmoId=strWmoId;
        
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
             strStationType=std::string(vStrTokens.at(5));
        else
                strStationType.push_back('?');

        // Use popcss to identify the header line in metadata file
        // Want to skip to the next line in the file to start picking up station metadata.
        if("popcss"==strStationType)
                continue;

        strStationNameConst=std::string(cStationName);
        strStationName=strStationNameConst;
        boost::algorithm::trim(strStationName);

        std::string strAirportDistKm=std::string(vStrTokens.at(9));
        boost::algorithm::trim(strAirportDistKm);

        // Some v4_ABC_airport airport name fields have one or more embedded commas,
        // fooling this routine into attmpting to parse the post-comma airport name
        // characters as a float. boost::lexical_cast<float> will throw an exception here.
        // In that case, parse the next token vStrTokens.at(10) as the airport distance string.
        try
        {
            boost::lexical_cast<float>(strAirportDistKm);
        }
        catch(...)
        {
            // strAirportDistKm aka vStrTokens.at(9) threw an exception,
            // is the post-comma portion of the airport name string.
            // so let's try vStrTokens.at(10);
            strAirportDistKm=std::string(vStrTokens.at(10));
        }
        
        float fAirportDistKm=9999;
        try
        {
            fAirportDistKm=boost::lexical_cast<float>(strAirportDistKm);
            //std::cout<<"RWM_DEBUG: Airport Distance="<<fAirportDistKm<<std::endl;
            latLong.lat_deg=boost::lexical_cast<float>(std::string(cLat));
            latLong.long_deg=boost::lexical_cast<float>(std::string(cLong));
            if(std::string(cAltitude).size()!=0)
                latLong.altitude=boost::lexical_cast<float>(std::string(cAltitude));
            else
                latLong.altitude=0;
            if(fAirportDistKm<=1) // 2025/04/08: If <=1 km, then "near airport"
                latLong.isNearAirport=1;
            else
                latLong.isNearAirport=2; // If > 1 km, then not "near airport"
        }
        catch(...)
        {
            std::cerr<<std::endl;
            std::cerr<<"Station Name: "<<strStationName<<std::endl;
            std::cerr<<"Airport Distance Info: "<<strAirportDistKm<<std::endl;
            std::cerr<<"Lat/Long/Alt="<<std::string(cLat)<<"/"<<std::string(cLong)<<"/"<<std::string(cAltitude)<<std::endl;
            std::cerr<<"Exception thrown parsing station lat/long/alt values,"<<std::endl;
            std::cerr<<"Are you using the extended v4_ABC GHCN metatadata file with airport status?"<<std::endl;
            std::cerr<<"If not, abort this run by pressing ctrl-c."<<std::endl;
            std::cerr<<std::endl;
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
        
      
        if(strStationTypeIn != "X") // 'X' means process all stations
        {
                // Not all stations have ABC rural/mixed/urban designators.
                // For those stations, those designators will be zero length tokens
                // replaced with '?' per above.
                if(strStationType.at(0)=='?')
                        continue;

            if(strStationTypeIn.find(strStationType)==std::string::npos) // Rural/Urban/Suburban designator -- V2 col 73.
            {
                continue;
            }
        }
      
//  std::cout<<strStationName<<" Lat
  
        char nstrStationType=(char)(strStationType[0]);
        switch(nstrStationType)
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


void ReadGhcnMetadataFileV4b2(const char* inFile,
                              const int& nBrightnessIndexIn,
                              // satellite night brightness index: if positive, process all stations with
                              // brightness index >= nBrightnessIndexIn. If negative, process all stations with
                              // brightness index <= nBrignessIndexIn.
                              std::set<std::size_t>& wmoIdSetOut,
                              std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  std::size_t nWmoId=0;
  const char *cLat;
  const char *cLong;
  const char *cAltitude;
  const char *cStationName;
  std::string strStationType;
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

        std::string strStreamTmp=strStream;
        // Add ability to skip over commented-out lines.
        boost::trim_left(strStreamTmp);
        if(strStreamTmp.at(0)=='#')
            continue;

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
            strStationType=std::string(vStrTokens.at(5));
        }
        else
            strStationType.push_back('?');

        // Use popcss to identify the header line in metadata file
        // Want to skip to the next line in the file to start picking up station metadata.
        if("popcss"==strStationType)
                continue;

        cwmoId = &inBuf[0];

        char cWmoId0[16];
        ::memset(cWmoId0,'\0',16);
        sscanf(&cwmoId[0], "%11s",&cWmoId0[0]);     
        std::string strWmoId=std::string(cWmoId0);

        latLong.strWmoId=strWmoId;

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
        
        char nstrStationType=(char)(strStationType[0]);
        switch(nstrStationType)
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

// Original GHCNv4 metadata file
void ReadGhcnMetadataFileV4(const char* inFile,
                            const std::string& strStationType,
                            std::set<std::size_t>& wmoIdSetOut,
                            std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  std::size_t nWmoId=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);

      // Add ability to skip over commented-out lines.
      std::string strStreamTmp=std::string(inBuf);
      boost::trim_left(strStreamTmp);
      if(strStreamTmp.at(0)=='#')
          continue;

      if(strStationType != "X") // 'A' means process all stations
      {
          if(strStationType.find(inBuf[106])==std::string::npos)  // Rural/Urban/Suburban designator -- V2 col 73.
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

      latLong.strWmoId=strWmoId;

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

// Original GHCNv4 metadata file
void ReadGhcnDailyMetadataFile(const char* inFile,
                               std::set<std::size_t>& wmoIdSetOut,
                               std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  std::size_t nWmoId=0;
  try
  {
      int iLine=1;
    while(!instream->eof())
    {
        std::cerr<<"GHCND metadata: reading line "<<iLine++<<"         \r";
        
      instream->getline(inBuf,511);

      // Add ability to skip over commented-out lines.
      std::string strStreamTmp=std::string(inBuf);
      boost::trim_left(strStreamTmp);
      if(strStreamTmp.at(0)=='#')
          continue;

      // There are duplicate WMO IDS in larger GHCN data-sets,
      // so include the country code to ensure a unique station ID.
      char cWmoId0[16];
      ::memset(cWmoId0,'\0',16);
      sscanf(&cwmoId[0], "%11s",&cWmoId0[0]);     
      std::string strWmoId=std::string(cWmoId0);

      latLong.strWmoId=strWmoId;

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
                         std::set<std::size_t>& wmoIdSetOut,
                         std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  
  std::map<std::size_t,int> wmoCount;
  

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

  std::size_t wmoId;
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

      // Add ability to skip over commented-out lines.
      std::string strStreamTmp=std::string(inBuf);
      boost::trim_left(strStreamTmp);
      if(strStreamTmp.at(0)=='#')
          continue;
      
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
                          std::set<std::size_t>& wmoIdSetOut,
                          std::map<std::size_t, GHCNLatLong >& bestLatLongMapOut)
{


  std::fstream* instream=NULL;

  char inBuf[512];
  
  std::map<std::size_t,int> wmoCount;
  

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
  
  std::size_t nWmoId;
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

void ReadUshcnMetadataCsvFile(const char* inFile,
                              const std::string& strStationTypeIn,
                              std::set<std::size_t>& wmoCompleteIdSetOut,
                              std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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

  char cBuf[513];
  do{
      // instream->read(cBuf,512);
      if(!instream->good())
      {
        break;
      }
      try
      {
        /* code */
        instream->getline(cBuf,512);
      }
      catch(const std::exception& e)
      {
        std::cerr << e.what() << '\n';
        break;
      }
      std::string strLine=std::string(cBuf);
      std::vector<std::string> vStrTok;
      
      // # begins a commented-out line. Skip it
      if(strLine.at(0)=='#')
        continue;

      // Split comma-separated value line into a vector of tokens.
      // The Hausfather et al. 2013 file contains literal " chars that
      // must be removed. so include them as delimeter chars in boost::is_any_of().
      // boost::split generates zero-length tokens when there are consecutive
      // delimiter chars, so will remove those zero-length tokens in the 
      // for(auto & iv: vStrTok0) loop below.

      std::string strLineCleaned;
      
      // The .csv metadata file does have empty comma-separated fields (for zero-length csv values),
      // so can't filter those out.  But must clean the metadata lines of literal " chars".
      for(auto & ichar: strLine)
        if(ichar!='\"')
          strLineCleaned+=(ichar);

      boost::split(vStrTok,strLineCleaned,boost::is_any_of(",\""));

      // Is this a header line? then skip it.
      if(vStrTok.at(0)=="station_id")
        continue;
      
      // Check the for zero-length csv tokens and make sure that
      // none should have info that we need.  
      int iv_count=1;
      for(auto& iv: vStrTok)
      {
        if(iv.size()==0)
          break;
        iv_count++;
      }

      // Don't have critical metadata for this station: skip it
      if(iv_count<6)
          continue;
      // USHCN csv metadata file format:
      //     0        1     2   3   4    5      6              7                    8       9        10      11
      // station_id,state,name,lat,lon,GRUMP,sensor_type,sensor_transition_year,pop_2000,pop_1990,pop_1980,pop_1970,
      //     12      13       14      15       16         17        18                19            20             
      // pop_1960,pop_1950,pop_1940,pop_1930,nightlights,isa,nightlight_urbanity,GRUMP_urbanity,pop_growth_urbanity,
      //         21                  22            23
      // low_pop_growth_urbanity,ISA_urbanity,low_ISA_urbanity 

      // Prepend standard USHCN station id prefix to abbreviated station id provided in the Hausfather et al.
      // metadata csv file.
      std::string strCompleteId;
      std::string strIdNum;
      strIdNum=vStrTok.at(0);

      // Prepend zeros to make all station id numbers 6 chars in length.
      for(int ic=6-strIdNum.size(); ic>0; --ic)
        strIdNum=std::string("0")+strIdNum;

      // Prepend this to station ID numbers padded to length 6
      // to create a complete station ID.  
      strCompleteId=std::string("USH00")+strIdNum;

      std::hash<std::string> nStrCompleteIdHash;
      size_t nWmoId=nStrCompleteIdHash(strCompleteId);
      
      std::string strLatDeg=vStrTok.at(3);
      std::string strLonDeg=vStrTok.at(4);
      std::string strAltMet="0"; // No altitude info in this metadata file, so set altitude to 0.
      std::string strStationName=vStrTok.at(2);
      std::string strStationType="UNK";
      if(vStrTok.size()>=20)
      {
        if(vStrTok.at(19).size()>0)
           strStationType=vStrTok.at(19);  // Use GRUMP Rural/Urban criteria.
      }
      else
        continue;

      // The default strStationTypeIn value is "X", which means
      // add metadata for all station types (station types are rural,urban,
      // and depending on the dataset, may include suburban.
      if(strStationTypeIn!="X")
      {
          if(strStationType==strStationTypeIn)
          {
              // if desired station type strStationTypeIn matches
              // strStationType from the metadata file, add the metadata
              // info to the GHCNLatLong structure below.
              // Valid station types are "R" for rural and "U" for urban.
              // 
          }
          else
              continue;
      }
      
      
      // boost::lexical_cast<> will throw an exception at the drop of a hat,
      // i.e. if there are any leading/trailing blanks.  There shouldn't be any,
      // but trim these just in case.
      boost::trim(strLatDeg);
      boost::trim(strLonDeg);
      boost::trim(strAltMet);

      
      GHCNLatLong latLong;
      try
      {
          latLong.lat_deg=boost::lexical_cast<float>(strLatDeg);
          latLong.long_deg=boost::lexical_cast<float>(strLonDeg);
          latLong.altitude=boost::lexical_cast<float>(strAltMet);
          latLong.strWmoId=strCompleteId;
          latLong.strStationName=strStationName;
          if("R"==strStationType)
          {
              latLong.iStationType=1;
              latLong.strStationType=std::string("RURAL");
          }
          else if("U"==strStationType)
          {
              latLong.iStationType=3;
              latLong.strStationType=std::string("URBAN");
          }
          else
          {
              latLong.iStationType=0;
              latLong.strStationType=std::string("UNK");
          }
      }
      catch(...)
      {
          std::cerr<<__FUNCTION__<<"(): Error parsing lat,long,or alt."<<std::endl;
          continue;
      }

      // This app works with pos lat/lon degs, so
      // adjust accordingly.
      latLong.lat_deg+=90;
      latLong.long_deg+=180;

      wmoCompleteIdSetOut.insert(nWmoId);
      ghcnLatLongMapOut[nWmoId]=latLong;

    } while(instream->good());

  return;
}


void ReadUshcnMetadataFile(const char* inFile,
                           const std::string& strStationType,
                           std::set<std::size_t>& wmoCompleteIdSetOut,
                           std::map<std::size_t, GHCNLatLong >& ghcnLatLongMapOut)
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
  std::size_t nWmoId=0;
  try
  {
    while(!instream->eof())
    {
        
      instream->getline(inBuf,511);

      // Add ability to skip over commented-out lines.
      std::string strStreamTmp=std::string(inBuf);
      boost::trim_left(strStreamTmp);
      if(strStreamTmp.at(0)=='#')
          continue;

#if(0) //  No USHCN rural/urban station status (yet)
      if(strStationType != 'X') // 'A' means process all stations
      {
        if(strStationType != inBuf[106]) // Rural/Urban/Suburban designator -- V2 col 73.
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
          latLong.strWmoId=strWmoId;
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

std::fstream* OpenCsvGenericFile(const char* csvFileName, const std::string & strNameAppend)
{

  std::fstream* OpenCsvFile(const char* csvFileName);

  // Check for a .suffix -- if it exists, chop it off
  std::string strFileName=std::string(csvFileName);
  std::size_t dotIndex;
  dotIndex=strFileName.find_last_of(".");
  if(std::string::npos==dotIndex)
  {
    // No suffix -- just append this:
    strFileName+="_StationCount";
  }
  else 
  {
    // .csv suffix present -- insert "StationCount" before suffix.
      std::string strSuffix=strFileName.substr(dotIndex);    // grab suffix including '.'
      strFileName=strFileName.substr(0,dotIndex); // grab prefix excluding '.'
      strFileName+=strNameAppend+std::string(strSuffix);
  }
  
  std::fstream* csvStationCountFstream;
  
  csvStationCountFstream=OpenCsvFile(strFileName.c_str());
  
  return csvStationCountFstream;
  
}

std::fstream* OpenCsvStationCountFile(const char* csvFileName)
{

  std::fstream* OpenCsvFile(const char* csvFileName);

  // Check for a .suffix -- if it exists, chop it off
  std::string strFileName=std::string(csvFileName);
  std::size_t dotIndex;
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

std::fstream* OpenCsvBootstrapFile(const char* csvFileName)
{

  std::fstream* OpenCsvFile(const char* csvFileName);

  // Check for a .suffix -- if it exists, chop it off
  std::string strFileName=std::string(csvFileName);
  std::size_t dotIndex;
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
    strFileName+=("_Bootstrap")+std::string(strSuffix);
  }
  
  std::fstream* csvBootstrapFstream;
  
  csvBootstrapFstream=OpenCsvFile(strFileName.c_str());
  
  return csvBootstrapFstream;
  
}

std::fstream* OpenCsvBootstrapStationCountFile(const char* csvFileName)
{

  std::fstream* OpenCsvFile(const char* csvFileName);

  // Check for a .suffix -- if it exists, chop it off
  std::string strFileName=std::string(csvFileName);
  std::size_t dotIndex;
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
    strFileName+=("_Bootstrap_StationCount")+std::string(strSuffix);
  }
  
  std::fstream* csvBootstrapStationCountFstream;
  
  csvBootstrapStationCountFstream=OpenCsvFile(strFileName.c_str());
  
  return csvBootstrapStationCountFstream;
  
}

std::fstream* OpenCsvStationAvgLatAltFile(const char* csvFileName)
{

  std::fstream* OpenCsvFile(const char* csvFileName);

  // Check for a .suffix -- if it exists, chop it off
  std::string strFileName=std::string(csvFileName);
  std::size_t dotIndex;
  dotIndex=strFileName.find_last_of(".");
  if(std::string::npos==dotIndex)
  {
    // No suffix -- just append this:
    strFileName+="_StationAvgLatAlt";
  }
  else 
  {
    // Suffix present -- insert "StationCount" before suffix.
    std::string strSuffix;
    strSuffix=strFileName.substr(dotIndex);    // grab suffix including '.'
    strFileName=strFileName.substr(0,dotIndex); // grab prefix excluding '.'
    strFileName+=("_StationAvgLatAlt")+std::string(strSuffix);
  }
  
  std::fstream* csvStationAvgLatAltFstream;
  
  csvStationAvgLatAltFstream=OpenCsvFile(strFileName.c_str());
  
  return csvStationAvgLatAltFstream;
  
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
  

  std::size_t dotPos;
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
  

  std::size_t dotPos;
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

void DumpBuildRunInfo(std::fstream& fout)
{
    time_t csv_t=time(NULL);
    std::string strCsvTime=ctime(&csv_t);
    
    std::string strCmdArgs;
    for(auto& iv: g_vStrCmdArgs)
    {
        strCmdArgs+=iv+" ";
    }
    fout<<"App Build Date/Time: "<<AnomalyVersion::ms_strDateTime<<std::endl;
    fout<<"App Git Branch:Rev: "<<AnomalyVersion::ms_strGitBranch
        <<":"<<AnomalyVersion::ms_strGitRevNum<<std::endl;
    fout<<"CSV File Creation Date/Time: "<<strCsvTime;
    fout<<"Cmd args: "<<strCmdArgs;
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
void DumpSmoothedAnnualResults(std::fstream& fout,
                               std::vector<GHCN*>& ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<std::size_t, double >::iterator iyy;
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

void DumpSmoothedMonthlyResults(std::fstream& fout,
                               std::vector<GHCN*>& ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<int, double >::iterator iyy;
  int igh;
  
  // Iterate over years
  for(iyy=ghcn[0]->mSmoothedGlobalAverageYearFractAnomaliesMap.begin();
      iyy!=ghcn[0]->mSmoothedGlobalAverageYearFractAnomaliesMap.end();
      iyy++)
  {
    // Results for first input file
    // First 
    fout << iyy->first/1000. << ","
         << iyy->second;

    if(ngh>1)
      fout << ",";

    // Results for additional input files
    for(igh=1; igh<ngh; igh++)
    {
      if(ghcn[igh]->mSmoothedGlobalAverageYearFractAnomaliesMap.find(iyy->first) != 
         ghcn[igh]->mSmoothedGlobalAverageYearFractAnomaliesMap.end())
      {
        fout << ghcn[igh]->mSmoothedGlobalAverageYearFractAnomaliesMap[iyy->first];
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

void DumpAnnualBootstrapStatistics(std::fstream& fout,
                                   std::vector<GHCN*>& ghcn,  int ngh) 
{
  std::string headingStr;
  
  std::map<std::size_t, float >::iterator iyy;
  int igh;

//    std::map<std::size_t, double> mSmoothedGlobalAverageAnnualAnomaliesMap;
//    std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap;
//    std::map<std::size_t,double> mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap;

  // Iterate over years
  for(iyy=ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.begin();
      iyy!=ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.end();
      iyy++)
  {
    // Results for first input file
    // First 
      float fMean=ghcn[0]->mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[iyy->first];
      float fSdev=ghcn[0]->mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap[iyy->first];
      fout << iyy->first << ","<<fMean<<","<<fMean-2*fSdev<<","<<fMean+2*fSdev;

    if(ngh>1)
      fout << ",";

    // Results for additional input files
    for(igh=1; igh<ngh; igh++)
    {
      if(ghcn[igh]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.find(iyy->first) != 
         ghcn[igh]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.end())
      {
          float fMean=ghcn[igh]->mSmoothedGlobalAverageAnnualAnomaliesBootstrapMeanMap[iyy->first];
          float fSdev=ghcn[igh]->mSmoothedGlobalAverageAnnualAnomaliesBootstrapStdDevMap[iyy->first];
          fout << iyy->first << ","<<fMean<<","<<fMean-fSdev<<","<<fMean+fSdev;
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

void DumpAnnualBootstrapEnsemble(std::fstream& fout,
                                 std::vector<GHCN*>& ghcn,  int ngh) 
{
    std::string headingStr;
    
    std::map<std::size_t, float >::iterator iyy;
    int igh;

  // Iterate over years
    int icol=1;
    int ncols=(int)ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.size();
    for(auto iyy: ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap)
    {
        std::vector<float> vfEnsembleTemps;
        fout<<iyy.first<<",";
        
        for(auto ive: ghcn[0]->mSmoothedGlobalAnnualAnomalyEnsembleRunMapVector)
        {
            if(ive.find(iyy.first)!=ive.end())
            {
                fout<<ive.find(iyy.first)->second<<",";
            }
            else
            {
              if(icol<=ncols)
                fout<<",";
            }
            ++icol;
        }
        
        fout << std::endl; // end of this csv line..
    }

    return;
  
}

void DumpAnnualBootstrapStationCounts(std::fstream& fout,
                                      std::vector<GHCN*>& ghcn,  int ngh) 
{
    std::string headingStr;
    
    std::map<std::size_t, float >::iterator iyy;
    int igh;

    // We can use mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap for
    // ensemble count info.
    int icol=1;
    int ncols=(int)ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.size(); // Ensemble count
    for(auto iyy: ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap) // Use this to get year info.
    {
        fout<<iyy.first<<",";
        
        for(auto ive: ghcn[0]->mAverageAnnualBootstrapStationCountMapVector)
        {
            if(ive.find(iyy.first)!=ive.end())
            {
                fout<<ive.find(iyy.first)->second<<",";
            }
            else
            {
              if(icol<=ncols)
                fout<<",";
            }
            ++icol;
        }
        
        fout << std::endl; // end of this csv line..
    }

    return;
  
}

#if(0)
void DumpAnnualBootstrapStatistics(std::fstream& fout,
                                   std::vector<GHCN*>& ghcn,  int ngh) 
{
    std::string headingStr;
    
    std::map<std::size_t, float >::iterator iyy;
    int igh;

  // Iterate over years
    int icol=1;
    int ncols=(int)ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap.size();
    for(auto iyy: ghcn[0]->mSmoothedGlobalAverageAnnualBootstrapEnsembleCountMap)
    {
        std::vector<float> vfEnsembleTemps;
        fout<<iyy.first<<",";
        
        for(auto ive: ghcn[0]->mSmoothedGlobalAnnualAnomalyEnsembleRunMapVector)
        {
            if(ive.find(iyy.first)!=ive.end())
            {
                fout<<ive.find(iyy.first)->second<<",";
            }
            else
            {
              if(icol<=ncols)
                fout<<",";
            }
            ++icol;
        }
        
        fout << std::endl; // end of this csv line..
    }
    return;
  
}
#endif
