/****


SAMPLE USHCN LINES
USH00461330 1900 -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999     1688a     724       65   
USH00461330 1901    58a    -381      664b     852     1676b    2245b    2641a    2382     1917b    1305e     326d     -72d  
USH00461330 1902  -119c    -254d     637c    1073b    1856h    2050a    2399d    2206     1892b    1402c    1012a      43   


 ****/



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
      char cWmoId0[10];
      sscanf(&cwmoId[3], "%8s",&cWmoId0[0]);     

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



void GHCN::ReadGhcnV4Temps(const std::set<size_t>& stationSet)
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
      char cCompleteStationId[10];
      
      sscanf(&mCbuf[3],"%8s%4ld",&cCompleteStationId[0],&mIyear); // complete station ID and year

      mICompleteStationId=strHash(std::string(cCompleteStationId));
      

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
            // With V3, only 1 time-series per station ID, so no
            // need to merge temp data 
            // Note -- must divide by 100 to get degrees C (not 10
            // as with V2).
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
