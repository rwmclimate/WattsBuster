#include "GnuPlotter.hpp"
#include "GHCN.hpp"

extern int nPlotStartYear_g;

GnuPlotter::GnuPlotter(std::vector<GHCN*>& v_pGhcn)
{
  
    m_vpGhcn=v_pGhcn;
    m_pGnuPopen=NULL;

  // Read and save GISS anomaly results here -- just do it once in the
  // constructor -- no need to repeat this.
  

    // NOTE: The GISS.dat plot color is hard-coded to #FA0000 (lc rgb \'#FA0000\'), line 456 below.
  // "One-off" app -- expect to see the GISS data in the
  // directory where this app is launched.
    m_mapGissAnomalies=ReadGnuPlotFileData("GISS.dat");
    
    m_vStrLineColors.push_back("#000000"); // Just a placeholder
    m_vStrLineColors.push_back("#000000"); // Just a placeholder
    m_vStrLineColors.push_back("#008F21"); // dull green (raw curve) 
    m_vStrLineColors.push_back("#0000FF"); // Mostly blue (adjusted curve)
}

void GnuPlotter::GnuPlotResults(int nPlotMode, const std::string& strNstations)
{
  bool bStatus;
  
  OpenGnuPlotFiles();

  bStatus=WriteDataToGnuPlotFiles();

  if(false==bStatus)
    nPlotMode=SERVER_PLOT_RESET;
  
  LaunchGnuPlot(nPlotMode,strNstations);
  
  CloseGnuPlotFiles();


}

void GnuPlotter::OpenGnuPlotFiles(void)
{
  GnuPlotElement gPe;
  
  std::vector<GHCN*>::iterator iGh;

  m_vGnuPlotElement.clear();
  
  // Want to remove old files so as not to confuse the "show data"
  // desktop feature.
  ::remove("v3_raw1AnomalyData.dat");
  ::remove("v3_raw2AnomalyData.dat");
  ::remove("v3_adj1AnomalyData.dat");
  ::remove("v3_adj2AnomalyData.dat");
  ::remove("v3_raw1StationCount.dat");
  ::remove("v3_raw2StationCount.dat");
  ::remove("v3_adj1StationCount.dat");
  ::remove("v3_adj2StationCount.dat");
  
  // A quick patch to make sure that the old GHCN v2 file naming
  // convention (same prefix, different suffixes) don't break
  // this app.
  int iFile=1; 
  
  for(iGh=m_vpGhcn.begin(); iGh!=m_vpGhcn.end(); iGh++)
  {

    size_t nPos;


    // A quick patch to make sure that the old GHCN v2 file naming
    // convention (same prefix, different suffixes) doesn't break
    // this app.
    std::stringstream sstrIfile;
    sstrIfile<<iFile;

   if((*iGh)->GetDisplayMode()!=0)
   {
        // Compute some simple statistical measures
        float fXcorr=0, fR2=0;
        int status=ComputeAnomalyStatistics((*iGh)->GetSmoothedGlobalAverageAnnualAnomaliesMap(),fXcorr,fR2);
        gPe.m_fXcorr=fXcorr;
        gPe.m_fR2=fR2;
        
        gPe.m_pGhcn=(*iGh);
    
        gPe.m_strInputDataFileName=(*iGh)->GetInputFileName();
        
        nPos=gPe.m_strInputDataFileName.find_last_of('.');
        if(std::string::npos!=nPos)
        {
          // Everything up to (but not including) the last '.' in the input file name.
          gPe.m_strGnuPlotAnomalyFileName=gPe.m_strInputDataFileName.substr
            (0,gPe.m_strInputDataFileName.find_last_of('.'));
        }
        else
        {
          gPe.m_strGnuPlotAnomalyFileName=gPe.m_strInputDataFileName;
        }
        gPe.m_strGnuPlotStationCountFileName=gPe.m_strGnuPlotAnomalyFileName;
        
        gPe.m_strGnuPlotAnomalyFileName+=(sstrIfile.str()+std::string("AnomalyData.dat"));
        
        gPe.m_strGnuPlotStationCountFileName+=(sstrIfile.str()+std::string("StationCount.dat"));
        
        iFile++;
    
        try
        {     
          gPe.m_pGnuPlotAnomalyFstream
            =new std::fstream(gPe.m_strGnuPlotAnomalyFileName.c_str(), 
                              std::fstream::out);
          
          // Oops -- want multiple station count lists after all
          gPe.m_pGnuPlotStationCountFstream=
            new std::fstream(gPe.m_strGnuPlotStationCountFileName.c_str(), 
                             std::fstream::out);
        }
        catch(...) // some sort of file open failure
        {
          std::cerr << std::endl << std::endl;
          std::cerr << "Failed to create a gnuplot streams for " 
                    << (*iGh)->GetInputFileName() << std::endl;
          std::cerr << "Exiting.... " << std::endl;
          std::cerr << std::endl << std::endl;
          if(NULL!=gPe.m_pGnuPlotAnomalyFstream)
          {
            gPe.m_pGnuPlotAnomalyFstream->close();
            delete gPe.m_pGnuPlotAnomalyFstream;
          }
          if(NULL!=gPe.m_pGnuPlotStationCountFstream)
          {
            gPe.m_pGnuPlotStationCountFstream->close();
            delete gPe.m_pGnuPlotStationCountFstream;
          }
          exit(1);
        }
  
        if(!gPe.m_pGnuPlotStationCountFstream->is_open())
        {
          std::cerr << std::endl << std::endl;
          std::cerr << "Failed to open " 
                    << gPe.m_strGnuPlotStationCountFileName << std::endl;
          std::cerr << "Exiting.... " << std::endl;
          std::cerr << std::endl << std::endl;
          exit(1);
        }

        if(!gPe.m_pGnuPlotAnomalyFstream->is_open())
        {
          std::cerr << std::endl << std::endl;
          std::cerr << "Failed to open " 
                    << gPe.m_strGnuPlotAnomalyFileName << std::endl;
          std::cerr << "Exiting.... " << std::endl;
          std::cerr << std::endl << std::endl;
          exit(1);
        }

        // gPe.m_strDataFileName=strInputDataFileName;
        // gPe.m_strGnuPlotAnomalyFileName=strGnuPlotAnomalyFileName;
        // gPe.m_strGnuPlotStationCountFileName=strGnuPlotStationCountFileName;
        // gPe.m_pGnuPlotAnomalyFstream=pGnuPlotAnomalyFstream;
        // gPe.m_pGnuPlotStationCountFstream=pGnuPlotStationCountFstream;
        
        m_vGnuPlotElement.push_back(gPe);
      }
  }
  

  return;
  
}

void GnuPlotter::CloseGnuPlotFiles(void)
{

  std::vector<GnuPlotElement>::iterator iGpe;

  for(iGpe=m_vGnuPlotElement.begin(); iGpe!=m_vGnuPlotElement.end(); iGpe++)
  {
  
    if((iGpe->m_pGnuPlotAnomalyFstream)!=NULL)
    {
      (iGpe)->m_pGnuPlotAnomalyFstream->close();
      delete (iGpe)->m_pGnuPlotAnomalyFstream;
      (iGpe)->m_pGnuPlotAnomalyFstream=NULL;
    }

    if((iGpe)->m_pGnuPlotStationCountFstream!=NULL)
    {
      (iGpe)->m_pGnuPlotStationCountFstream->close();
      delete (iGpe)->m_pGnuPlotStationCountFstream;
      (iGpe)->m_pGnuPlotStationCountFstream=NULL;
    }
  }

  m_vGnuPlotElement.clear(); // RRR 2012/09/29
  
  return;
}

#if(0)
void GnuPlotter::WriteDataToGnuPlotFiles(void)
{
  std::vector<GHCN*>::iterator iGhcn;
  
  int iCount=0;
  for(iGhcn=m_vpGhcn.begin(); iGhcn!=m_vpGhcn.end(); iGhcn++)
  {
    
    std::map<int,double>::iterator imyd;
    std::map<int,float>::iterator imyf;
  
    if((*iGhcn)->GetSmoothedGlobalAverageAnnualAnomaliesMap().size()<2)
    {
      // Not enough data to plot -- do nothing and return
      return;
    }
    
    imyd=(*iGhcn)->GetSmoothedGlobalAverageAnnualAnomaliesMap().begin();
    

    imyf=(*iGhcn)->GetAverageAnnualStationCountMap().begin();
    
    // Current year typically does not have a complete year's worth of data.
    // My code doesn't deal with that quite correctly -- take the lazy-boy
    // way out and chop off that last year...
    // RWM 2015/01/15 -- current data has a complete final year, so this kludge
    //                   has been disabled.
    int iCount=0;
    int finalYear=(*iGhcn)->GetAverageAnnualStationCountMap().rbegin()->first;

    for(imyf=(*iGhcn)->GetAverageAnnualStationCountMap().begin();
        imyf!=(*iGhcn)->GetAverageAnnualStationCountMap().end(); imyf++)
    {
      //      if(imyf->first<finalYear) -- RWM 2015/01/15 disabled -- want to process last full year
        *(m_vpGnuPlotStationCountFstream[iCount])<<imyf->first<<"  "<<imyf->second<<std::endl;
      iCount++;
    }
    
    
    iCount=0;
    for(imyd=(*iGhcn)->GetSmoothedGlobalAverageAnnualAnomaliesMap().begin(); 
        imyd!=(*iGhcn)->GetSmoothedGlobalAverageAnnualAnomaliesMap().end(); imyd++)
    {
      // if(imyd->first<finalYear) -- RWM 2015/01/15 disabled -- want to process last full year
      {
        *(m_vpGnuPlotAnomalyFstream[iCount])<<imyd->first<<"  "<<imyd->second<<"  "<<std::endl;
      }
      iCount++;
    }

  }
  return;
}
#endif

bool GnuPlotter::WriteDataToGnuPlotFiles(void)
{

  std::vector<GnuPlotElement>::iterator iGnu;
  
  for(iGnu=m_vGnuPlotElement.begin(); iGnu!=m_vGnuPlotElement.end();
      iGnu++)
  {
    
    std::map<int,double>::iterator imyd;
    std::map<int,float>::iterator imyf;
  
    if(iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap().size()<2)
    {
      // Nothing to plot -- skip to the next gnuplot element
      continue;
    }
    
    if((iGnu->m_pGhcn->GetAverageAnnualStationCountMap().size()<2) ||
       (iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap().size()<2))
    {
      return false ;
    }
    
    // Current year typically does not have a complete year's worth of data.
    // My code doesn't deal with that quite correctly -- take the lazy-boy
    // way out and chop off that last year...
    int finalYear=iGnu->m_pGhcn->GetAverageAnnualStationCountMap().rbegin()->first;

    int iYear;

    // RWM 2015/01/15 -- now packaged with data-set that has a complete final year,
    //                   so don't chop off that last year.  iYear<finalYear changed
    //                   to iYear<=finalYear
    for(iYear=GHCN::MIN_GISS_YEAR; iYear<=finalYear; iYear++)
    {
            if(iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap().find(iYear) ==
               iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap().end())
            {
                    // No data for this year -- put in a blank line
                    *(iGnu->m_pGnuPlotAnomalyFstream)<<std::endl;
            }
            else
            {
                    *(iGnu->m_pGnuPlotAnomalyFstream)
                            <<iYear<<"  "
                            <<iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap()[iYear]
                            <<"  "<<std::endl;
            }

            if(iGnu->m_pGhcn->GetAverageAnnualStationCountMap().find(iYear) ==
               iGnu->m_pGhcn->GetAverageAnnualStationCountMap().end())
            {
                    // No data for this year -- put in a blank line
                    *(iGnu->m_pGnuPlotStationCountFstream)<<std::endl;
            }
            else
            {
                    *(iGnu->m_pGnuPlotStationCountFstream)
                            <<iYear<<"  "
                            <<iGnu->m_pGhcn->GetAverageAnnualStationCountMap()[iYear]
                            <<"  "<<std::endl;
            }
    }
    

#if(0) // ORIGINAL VERSION DISABLED
    int finalYear=iGnu->m_pGhcn->GetAverageAnnualStationCountMap().rbegin()->first;
    for(imyf=iGnu->m_pGhcn->GetAverageAnnualStationCountMap().begin(); 
        imyf!=iGnu->m_pGhcn->GetAverageAnnualStationCountMap().end(); imyf++)
    {
      if(imyf->first<finalYear)
        *(iGnu->m_pGnuPlotStationCountFstream)<<imyf->first<<"  "<<imyf->second<<std::endl;
    }
    
    
    for(imyd=iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap().begin(); 
        imyd!=iGnu->m_pGhcn->GetSmoothedGlobalAverageAnnualAnomaliesMap().end(); imyd++)
    {
      if(imyd->first<finalYear)
      {
        *(iGnu->m_pGnuPlotAnomalyFstream)<<imyd->first<<"  "<<imyd->second<<"  "<<std::endl;
      }
    }
#endif
  
  }
  
  return true;
  
}


void GnuPlotter::LaunchGnuPlot(int nPlotMode, const std::string& strNstations)
{
  std::vector<GnuPlotElement>::iterator iGnu;
  
  std::vector<std::string> vStrGnuCommand;
  std::string strGnuCommand;

  struct timespec tsleep;
  tsleep.tv_sec=0;
  tsleep.tv_nsec=static_cast<long>((static_cast<double>(0.005*1.e9)));

  vStrGnuCommand.clear();
  
//  if(mSmoothedGlobalAverageAnnualAnomaliesMap.size()<2)
//    return;


  std::cerr<<"nPlotMode="<<nPlotMode<<std::endl;

  std::string strColorSpec=std::string("red");

  std::string strPlotStartYear="1880";
  if(nPlotStartYear_g>=1840)
      strPlotStartYear=boost::lexical_cast<std::string>(nPlotStartYear_g);

  std::cout<<"RWM: "<<__FILE__<<":"<<__LINE__<<"  nPlotStartYear_g="<<nPlotStartYear_g<<std::endl;
  std::cout<<"RWM: "<<__FILE__<<":"<<__LINE__<<"  strPlotStartYear="<<strPlotStartYear<<std::endl;
  
  
  switch(nPlotMode)
  {
    case SERVER_PLOT_RESET:
      vStrGnuCommand.clear();
      strGnuCommand="clear\n";
      vStrGnuCommand.push_back(strGnuCommand);
      strGnuCommand="unset mouse\n";
      vStrGnuCommand.push_back(strGnuCommand);
      break;
      
    case SERVER_PLOT_NEW:
        ServerPlotNew(vStrGnuCommand,strNstations,strPlotStartYear);
        break;
      
    case SERVER_PLOT_INCREMENT:
        ServerPlotIncrement(vStrGnuCommand,strNstations,strPlotStartYear);
        break;

      // Do nothing
    case SERVER_PLOT_NONE:
    default:
      break;
      
  }

  if(vStrGnuCommand.size()<1)
    return;
  

  if(NULL==m_pGnuPopen)
  {
//    m_pGnuPopen=popen("gnuplot -persistent", "w");
    m_pGnuPopen=::popen("gnuplot", "w");
    if(0!=errno)
    {
      ::perror("popen()");
      if(NULL==m_pGnuPopen)
      {
        std::cerr<<"popen() returned a NULL ptr"<<std::endl;
        return;
      }
    }
  }

  if(NULL!=m_pGnuPopen)   // RRR 2012/09/29
  {
#ifdef LINUX
    ::__fpurge(m_pGnuPopen); // RRR 2012/02/29 (2)
#else
    ::fpurge(m_pGnuPopen);  // BSD/OS-X version
#endif
  }
  
  std::vector<std::string>::iterator icmd;
  
  for(icmd=vStrGnuCommand.begin(); icmd!=vStrGnuCommand.end(); icmd++)
  {
    int nbytes;
    nbytes=::fwrite(icmd->c_str(),sizeof(char),strlen(icmd->c_str()), m_pGnuPopen);
    // perror("fwrite() to popen()");
    // std::cerr<<"Just wrote "<<nbytes<<" bytes to popen() pipe..."<<std::endl;
    // std::cerr<<"popen() command: "<<*icmd<<std::endl;
    ::nanosleep(&tsleep,NULL);
  }
  
  ::fflush(m_pGnuPopen);
  
  return;
  
}

void GnuPlotter::ServerPlotNew(std::vector<std::string>& vStrGnuCommand, const std::string& strNstations, const std::string& strStartYear)
{
    std::string strGnuCommand;
    
    strGnuCommand="clear\n";
    vStrGnuCommand.push_back(strGnuCommand);

    strGnuCommand="unset mouse\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set key noenhanced\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set key left top Left\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set multiplot layout 2,1\n";
    vStrGnuCommand.push_back(strGnuCommand);

    // RWM 2025/07/02
    strGnuCommand="set xrange ["+strStartYear+" : 2025] \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set xtics auto\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set format y \"%6.1f\"; set ytics auto\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    // strGnuCommand="set grid xtics 20. \n";
    // vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set grid  \n";
//      strGnuCommand="set grid ytics 0.2 \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set size 1., 0.65 \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set origin 0., 0.35 \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand+=" set ylabel \'Temperature Anomalies\'\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="plot ";
    int iCount=0;


    std::cerr<<"m_vGnuPlotElement size = " << m_vGnuPlotElement.size()<<std::endl;

    std::vector<GnuPlotElement>::iterator iGnu;

    for(iGnu=m_vGnuPlotElement.begin(); iGnu!=m_vGnuPlotElement.end(); iGnu++)
    {

        // Compute cross-correlation between these results and the NASA/GISS results.

        // Replaced with ComputeAnomalyStatistics call above: 
        //     iGnu->m_fXcorr=ComputeCrossCorrelation(iGnu->m_strGnuPlotAnomalyFileName);

        std::cerr<<"################################"<<std::endl;
        std::cerr<<"Correlation for "<<iGnu->m_strInputDataFileName<<"   "<<iGnu->m_fXcorr<<std::endl;
        std::cerr<<"R2 statistic for "<<iGnu->m_strInputDataFileName<<"   "<<iGnu->m_fR2<<std::endl;
        std::cerr<<"################################"<<std::endl;

        char str_lc[4];
        snprintf(str_lc, 3, "%d",  
                 static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex())); // Want to keep raw, adj temp line colors consistent.

        char str_xcorr[32];
        snprintf(str_xcorr, 31, " (Corr=%5.3f,  ", iGnu->m_fXcorr);

        char str_R2[32];
        snprintf(str_R2, 31, "Pseudo R2=%5.3f)", iGnu->m_fR2);
 
        int iColor=static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex());
        std::string strColorSpec=std::string(" lc rgb \'")+m_vStrLineColors.at(iColor)+std::string("\'");
        std::cout<<"GHCN object index "<<iColor<<": "<<strColorSpec<<std::endl;

        // Get rid of the path portion of the input data file name
        // so as to minimize "label-clutter"
        std::string strFileName=iGnu->m_strInputDataFileName;
        size_t nLastSlash=strFileName.find_last_of("/");
        if(nLastSlash!=std::string::npos)
            strFileName=strFileName.substr(nLastSlash+1);

        strGnuCommand+="\""+iGnu->m_strGnuPlotAnomalyFileName+"\" title \'"
              +strFileName+std::string(str_xcorr)+std::string(str_R2)
              +"\' with lines lw 2" + strColorSpec + ",";
              // orig +"\' with lines lw 2 lc " + std::string(str_lc)+ ",";
               iCount++;
      }

      strGnuCommand+=" \"GISS.dat\" using 1:2 with lines lw 2 lc rgb \'#FA0000\'  title \'NASA/GISS\' \n"; // lc 1\n";   // Plots 1-year-avg GISS results
      vStrGnuCommand.push_back(strGnuCommand);

      strGnuCommand="set size 1., 0.35 \n";
      vStrGnuCommand.push_back(strGnuCommand);
      
      strGnuCommand="set origin 0., 0.0 \n";
      vStrGnuCommand.push_back(strGnuCommand);

      strGnuCommand=" set ylabel \'#Stations\' \n";
      vStrGnuCommand.push_back(strGnuCommand);
 
      strGnuCommand="plot ";
      iCount=0;
      for(iGnu=m_vGnuPlotElement.begin(); iGnu!=m_vGnuPlotElement.end(); iGnu++)
      {
        char str_lc[4];
        snprintf(str_lc, 3, "%d",  static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex())); // Want to keep raw, adj temp line colors consistent.
        
        int iColor=static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex());
        std::string strColorSpec=std::string(" lc rgb \'")+m_vStrLineColors.at(iColor)+std::string("\'");
        // Get rid of the path portion of the input data file name
        // so as to minimize "label-clutter"
        std::string strFileName=iGnu->m_strInputDataFileName;
        size_t nLastSlash=strFileName.find_last_of("/");
        if(nLastSlash!=std::string::npos)
            strFileName=strFileName.substr(nLastSlash+1);

        //Display station count for just the first data-set 
        if(0==iCount)
        {
            strGnuCommand+="\""+iGnu->m_strGnuPlotStationCountFileName+"\" title \'"
                + strFileName+strNstations+"\' with lines lw 2 "  
                + strColorSpec;
            
        }
        else
        {
            strGnuCommand+="\""+iGnu->m_strGnuPlotStationCountFileName+"\" title \'"
                +strFileName+"\' with lines lw 2 "  
                + strColorSpec;
        }
        
        // Don't want the last gnuplot data filen item followed by a trailing comma
        if(iCount<(int)m_vGnuPlotElement.size()-1)
            strGnuCommand+=",";
        
        iCount++;
        
      }

      strGnuCommand+="\n"; // Don't forget the terminating CR!

      // \""+mGnuPlotStationCountFileName+"\" using 1:2 with lines lw 2\n";
      vStrGnuCommand.push_back(strGnuCommand);

      strGnuCommand="unset multiplot \n";
      vStrGnuCommand.push_back(strGnuCommand);

      return;
}

void GnuPlotter::ServerPlotIncrement(std::vector<std::string>& vStrGnuCommand, const std::string& strNstations, const std::string& strStartYear)
{
    std::string strGnuCommand;
    
    strGnuCommand="clear\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="unset mouse\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set key noenhanced\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set key left top Left\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set multiplot layout 2,1\n";
    vStrGnuCommand.push_back(strGnuCommand);

    // RWM 2025/07/02
    strGnuCommand="set xrange ["+strStartYear+" : 2025] \n";
    vStrGnuCommand.push_back(strGnuCommand);

    strGnuCommand="set xtics auto \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set format y \"%6.1f\"; set ytics auto \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set grid  \n";
//      strGnuCommand="set grid xtics 20. \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
      // strGnuCommand="set grid ytics 0.2 \n";
      // vStrGnuCommand.push_back(strGnuCommand);

    strGnuCommand="set size 1., 0.65 \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand="set origin 0., 0.35 \n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    strGnuCommand+=" set ylabel \'Temperature Anomalies\'\n";
    vStrGnuCommand.push_back(strGnuCommand);
    
    std::cerr<<"m_vGnuPlotElement size = " << m_vGnuPlotElement.size()<<std::endl;
    
    std::vector<GnuPlotElement>::iterator iGnu;
    
    strGnuCommand="plot ";
    for(iGnu=m_vGnuPlotElement.begin(); iGnu!=m_vGnuPlotElement.end(); iGnu++)
    {

        int iColor=static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex());
        std::string strColorSpec=std::string(" lc rgb \'")+m_vStrLineColors.at(iColor)+std::string("\'");

        // Compute cross-correlation between these results and the NASA/GISS results.
        // Replaced with ComputeAnomalyStastics() call above
        // iGnu->m_fXcorr=ComputeCrossCorrelation(iGnu->m_strGnuPlotAnomalyFileName);

        std::cerr<<"################################"<<std::endl;
        std::cerr<<"Correlation for "<<iGnu->m_strInputDataFileName<<"   "<<iGnu->m_fXcorr<<std::endl;
        std::cerr<<"################################"<<std::endl;

        char str_lc[4];
        snprintf(str_lc, 3, "%d",  static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex())); // Want to keep raw, adj temp line colors consistent.

        char str_xcorr[32];
        snprintf(str_xcorr, 31, " (Corr=%5.3f,  ", iGnu->m_fXcorr);
        
        char str_R2[32];
        snprintf(str_R2, 31, "Pseudo R2=%5.3f)", iGnu->m_fR2);

        std::string strFileName=iGnu->m_strInputDataFileName;
        // Get rid of the path portion of the input data file name
        // so as to minimize "label-clutter"
        size_t nLastSlash=strFileName.find_last_of("/");
        if(nLastSlash!=std::string::npos)
          strFileName=strFileName.substr(nLastSlash+1);
        
        strGnuCommand+="\""+iGnu->m_strGnuPlotAnomalyFileName+"\" title \'"
            +strFileName+std::string(str_xcorr)+std::string(str_R2)
            +"\' with lines lw 2 "+ strColorSpec + ",";

      }

      strGnuCommand+=" \"GISS.dat\" using 1:2 with lines lw 2  lc rgb \'#FA0000\' title \'NASA/GISS\' \n";    // Plots 1-year-avg GISS results
      vStrGnuCommand.push_back(strGnuCommand);

      strGnuCommand="set size 1., 0.35 \n";    // Vertical Plot Size
      vStrGnuCommand.push_back(strGnuCommand);
      
      strGnuCommand="set origin 0., 0.0 \n";
      vStrGnuCommand.push_back(strGnuCommand);

//      strGnuCommand="plot \""+mGnuPlotStationCountFileName+"\" using 1:2 with lines lw 2\n";

      strGnuCommand=" set ylabel \'#Stations\' \n";
      vStrGnuCommand.push_back(strGnuCommand);
      
      strGnuCommand="plot ";
      int iCount=0;
      for(iGnu=m_vGnuPlotElement.begin(); iGnu!=m_vGnuPlotElement.end(); iGnu++)
      {
          char str_lc[4];
          snprintf(str_lc, 3, "%d",  static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex())); // Want to keep raw, adj temp line colors consistent.
       
          int iColor=static_cast<int>(iGnu->m_pGhcn->GetDisplayIndex());
          std::string strColorSpec=std::string(" lc rgb \'")+m_vStrLineColors.at(iColor)+std::string("\'");
          
          // Get rid of the path portion of the input data file name
          // so as to minimize "label-clutter"
          std::string strFileName=iGnu->m_strInputDataFileName;
          size_t nLastSlash=strFileName.find_last_of("/");
          if(nLastSlash!=std::string::npos)
              strFileName=strFileName.substr(nLastSlash+1);

          //Display station count for just the first data-set 
          if(0==iCount)
          {
              strGnuCommand+="\""+iGnu->m_strGnuPlotStationCountFileName+"\" title \'"
                  +strFileName+strNstations+"\' with lines lw 2 " + strColorSpec;
          }
          else
          {
              strGnuCommand+="\""+iGnu->m_strGnuPlotStationCountFileName+"\" title \'"
                  +strFileName+"\' with lines lw 2  " + strColorSpec;
          }

            // Don't want the last gnuplot data filen item followed by a trailing comma
          if(iCount<(int)m_vGnuPlotElement.size()-1)
              strGnuCommand+=",";
          iCount++;
      }
      strGnuCommand+=" \n"; // Don't forget the terminating CR!

      vStrGnuCommand.push_back(strGnuCommand);

      strGnuCommand="unset multiplot \n";
      vStrGnuCommand.push_back(strGnuCommand);

      return;
}




std::string GnuPlotter::ComposeGnuPlotAnomalyFileName(const int iFile)
{
  std::string strGhcnDataFileName;
  std::string strGnuPlotAnomalyFileName;
  size_t nPos;
  
  if((iFile>=1) && (iFile<=static_cast<int>(m_vpGhcn.size())))
  {

    std::stringstream sstrIfile;
    sstrIfile<<iFile;

    strGhcnDataFileName=m_vpGhcn[iFile-1]->GetInputFileName();
        
    nPos=strGhcnDataFileName.find_last_of('.');
    if(std::string::npos!=nPos)
    {
      // Everything up to (but not including) the last '.' in the input file name.
      strGnuPlotAnomalyFileName=strGhcnDataFileName.substr
        (0,strGhcnDataFileName.find_last_of('.'));
    }
    else
    {
      strGnuPlotAnomalyFileName=strGhcnDataFileName;
    }
    
    strGnuPlotAnomalyFileName+=(sstrIfile.str()+std::string("AnomalyData.dat"));

  }
  else
  {
    return std::string("InvalidFile");
  }
  
  return strGnuPlotAnomalyFileName;
}

std::map<int,float> GnuPlotter::ReadGnuPlotFileData(const char* inFile)
{
  std::map<int,float> mapGnuPlotData;
  std::fstream* pInStream;
  
  pInStream=NULL;

  try
  {
    pInStream = new std::fstream(inFile,std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << inFile << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(pInStream!=NULL)
    {
      pInStream->close(); // make sure it's closed
      return mapGnuPlotData;  // Return an empty map
    }
  }

  if(NULL==pInStream)
  {
    std::cerr<<"Null file ptr for "<<inFile<<std::endl;
    return mapGnuPlotData; // Return an empty map -- don't crash
  }
  
  pInStream->exceptions(std::fstream::badbit 
                       | std::fstream::failbit 
                       | std::fstream::eofbit);

  char inBuf[512];
  
  try
  {
    while(!pInStream->eof())
    {
      int nYear;
      float fAnomaly;
      int nEl;
      
      pInStream->getline(inBuf,511);
      nEl=sscanf(inBuf,"%d %f",&nYear,&fAnomaly);

      if(2==nEl)
      {
        mapGnuPlotData[nYear]=fAnomaly;
      }
      
    }
  }
  catch(...)
  {
    // Caught an exception -- carry on for now...
  }
  
  pInStream->close();

  delete pInStream;
  
  return mapGnuPlotData;
}

// Replaced with ComputeAnomalyStatistics() below
float GnuPlotter::ComputeCrossCorrelation(const std::string& strGnuPlotAnomalyFileName)
{
  double dCorrVal;

  double gissMean=0;
  double myMean=0;
  double gissVar=0;
  double myVar=0;
  double gissXme=0;
  
  std::map<int,float> mapMyAnomalies=ReadGnuPlotFileData(strGnuPlotAnomalyFileName.c_str());

  std::map<int,float>::iterator iMap;
  
  int iCount=0;
  
  for(iMap=mapMyAnomalies.begin(); iMap!=mapMyAnomalies.end(); iMap++)
  {
    if(m_mapGissAnomalies.find(iMap->first) != m_mapGissAnomalies.end())
    {
      
      double gissVal, myVal;
      // Got a matching value
      myVal=iMap->second;
      gissVal=m_mapGissAnomalies[iMap->first];

      gissMean+=gissVal;
      myMean+=myVal;
      
      gissVar+=gissVal*gissVal;
      myVar+=myVal*myVal;

      gissXme+=gissVal*myVal;
      
      iCount+=1;
      
    }
  }

  if(iCount<=0)
    return 0.;
  
  gissXme=gissXme/iCount-myMean/iCount*gissMean/iCount;
  gissVar=gissVar/iCount-gissMean/iCount*gissMean/iCount;
  myVar=myVar/iCount-(myMean/iCount)*(myMean/iCount);
  
  dCorrVal=gissXme/(sqrt(myVar)*sqrt(gissVar));
  
  return static_cast<float>(dCorrVal);
  
}
    
int GnuPlotter::ComputeAnomalyStatistics(const std::map<size_t,double>& mapMyAnomalies, 
                                         float& fCorr, float& fR2)
{
  double dCorrVal;

  double gissMean=0;
  double myMean=0;
  double gissVar=0;
  double myVar=0;
  double gissXme=0;
  double msqError=0;
  
  std::map<size_t,double>::const_iterator iMap;
  
  int iCount=0;
  
  for(iMap=mapMyAnomalies.begin(); iMap!=mapMyAnomalies.end(); iMap++)
  {
    if(m_mapGissAnomalies.find(iMap->first) != m_mapGissAnomalies.end())
    {
      double gissVal, myVal;
      // Got a matching value
      myVal=iMap->second;
      gissVal=m_mapGissAnomalies[iMap->first];

      gissMean+=gissVal;
      myMean+=myVal;
      
      gissVar+=gissVal*gissVal;
      myVar+=myVal*myVal;

      gissXme+=gissVal*myVal;
      
      msqError += (gissVal-myVal)*(gissVal-myVal);

      iCount+=1;
      
    }
  }

  // No data -- return error status
  if(iCount<=0)
    return -1.;

  // Normalize by #samples and subtract out mean-offsets
  gissXme=gissXme/iCount-myMean/iCount*gissMean/iCount;
  gissVar=gissVar/iCount-gissMean/iCount*gissMean/iCount;
  myVar=myVar/iCount-(myMean/iCount)*(myMean/iCount);

  // Don't forget to normalize by #samples
  msqError = msqError/iCount;
  
  // Covariance
  dCorrVal=gissXme/(sqrt(myVar)*sqrt(gissVar));
  fCorr=static_cast<float>(dCorrVal);
  
  // Hacked R-squared statistic -- divide
  // msqError by variance of "model" (GISS results)
  // instead of "measurement" (my results).
  // Not a proper R2 statistic, but it gives a better indication 
  // of mismatch between my results and the GISS results.
  fR2=static_cast<float>(1.-msqError/gissVar);
  
  return 0; 
  
}
    
    
