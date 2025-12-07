#include "USCRN.hpp"

USCRN::USCRN(const std::string& strInSubHourlyDir, const std::string& strOutMonthlyDir)
{
    m_strInSubHourlyDir=strInSubHourlyDir;
    m_strOutMonthlyDir=strOutMonthlyDir;

    /**
          ID                 1-11        Integer
          YEAR              13-16        Integer
          VALUE1            17-22        Integer
          DMFLAG1           23-23        Character
          QCFLAG1           24-24        Character
          DSFLAG1           25-25        Character
            .                 .             .
            .                 .             .
            .                 .             .
          VALUE12          116-121       Integer
          DMFLAG12         122-122       Character
          QCFLAG12         123-123       Character
          DSFLAG12         124-124       Character


          ID: 11 character alfanumeric identifier:
              characters 1-2=Country Code ('US' for all USHCN stations)
              character 3=Network code ('H' for Historical Climatology Network)
              digits 4-11='00'+6-digit Cooperative Observer Identification Number 

          

                             11-digit ID  YEAR
                             USH00123456 1999         
               Value (monthly avg temp) increment: 9 chars                    
               Value01: 17-22
               Value02: 26-31
               Value03: 35-40
               Value04: 44-49
               Value05: 53-58
               Value06: 62-67
               Value07: 71-76
               Value08: 80-85
               Value09: 89-94
               Value10: 98-103
               Value11: 107-112
               Value12: 116-121
    */
    m_strUshcnLineTemplate1=
"USH00NNNNNN 2019 -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999   ";
    m_strUshcnLineTemplate2=
"USH00NNNNNN 2019 -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999    -9999   ";

}

void USCRN::OpenInSubHourlyStream(const std::string& strFileName, const int& nYear)
{
    std::string strYear=boost::lexical_cast<std::string>(nYear);

    m_pInSubHourlyStream=_OpenInSubHourlyStream(strFileName,strYear);

    return;
}


std::fstream* USCRN::_OpenInSubHourlyStream(const std::string& strInSubHourlyFileName, const std::string& strYear)
{
  std::string strInSubHourlyFullPath=std::string(m_strInSubHourlyDir+"/"+strYear+"/"+strInSubHourlyFileName);

  if(strInSubHourlyFullPath.size()==0)
  {
    std::cerr<<"Need to supply a valid input filename "
             <<std::endl<<std::endl;
    exit(1);
  }

  std::fstream* pInSubHourlyFstream=NULL;
  try
  {
    pInSubHourlyFstream = new std::fstream(strInSubHourlyFullPath.c_str(),std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << strInSubHourlyFullPath << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(pInSubHourlyFstream!=NULL)
        pInSubHourlyFstream->close(); // make sure it's closed
    delete pInSubHourlyFstream;
    pInSubHourlyFstream=NULL;
  }

  if(!pInSubHourlyFstream->is_open())
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to open " << strInSubHourlyFullPath << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    delete pInSubHourlyFstream;
    pInSubHourlyFstream=NULL;
  }

  return pInSubHourlyFstream;
}

std::fstream* USCRN::_OpenOutMonthlyStream(const std::string& strOutMonthlyDir, const std::string& strOutMonthlyFile)
{

  std::string strOutMonthlyFullPath=std::string(strOutMonthlyDir+"/"+strOutMonthlyFile);

  if(strOutMonthlyFullPath.size()<=std::string(strOutMonthlyDir+"/").size())
  {
    std::cerr<<"Need to supply a valid output filename "
             <<std::endl<<std::endl;
    exit(1);
  }

  std::fstream inp;
  inp.open(strOutMonthlyFullPath.c_str(), std::fstream::out);
  inp.close();
  if(!inp.fail())
  {
    std::cerr<<"File "<< strOutMonthlyFullPath
             << " already exists -- will not overwrite." << std::endl;
    return NULL;
  }

  std::fstream* pOutMonthlyFstream=NULL;
  try
  {
    pOutMonthlyFstream = new std::fstream(strOutMonthlyFullPath.c_str(),std::fstream::in);
  }
  catch(...) // some sort of file open failure
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to create a stream for " << strOutMonthlyFullPath << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    if(pOutMonthlyFstream!=NULL)
        pOutMonthlyFstream->close(); // make sure it's closed
    delete pOutMonthlyFstream;
    pOutMonthlyFstream=NULL;
  }

  if(!pOutMonthlyFstream->is_open())
  {
    std::cerr << std::endl << std::endl;
    std::cerr << "Failed to open " << strOutMonthlyFullPath << std::endl;
    std::cerr << "Exiting.... " << std::endl;
    std::cerr << std::endl << std::endl;
    delete pOutMonthlyFstream;
    pOutMonthlyFstream=NULL;
  }


  return pOutMonthlyFstream;

}

void USCRN::ProcessInSubHourlyFiles(const std::string& strInSubHourlyFileName, int nStartYear, int nEndYear)
{
    nStartYear=std::max(nStartYear,MIN_USCRN_YEAR);
    nEndYear=std::min(nEndYear,MAX_USCRN_YEAR);

    for(int iY=nStartYear; iY<=nEndYear; ++iY)
    {
        OpenInSubHourlyStream(strInSubHourlyFileName,iY);
        ProcessInSubHourlyFile();
        CloseInSubHourlyStream();
    }

    return;
}

// When we hit day 01, have a new month. Need to open a new monthly average
// output file.
void USCRN::ProcessInSubHourlyFile(const std::string & strOutMonthlyFileName)
{
    do
    {
        char cBuf[256];
        m_pInSubHourlyStream->getline(cBuf,255);

        std::string strBuf=std::string(cBuf);

        std::string strWbanId=strBuf.substr(0,5);
        std::string strYear=strBuf.substr(20,4);
        std::string strMonth=strBuf.substr(24,2);
        std::string strDay=strBuf.substr(26,2);
        std::string strMinute=strBuf.substr(31,2);

        if("01"==strDay)
        {
            // Start a new monthly file.
            OpenOutMonthlyStream(strOutMonthlyFileName);
        }
        
    } while(m_pInSubHourlyStream->good());
}
