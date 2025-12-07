#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <set>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <ctype.h>

int main(int argc, char** argv)
{
    if(argc<4)
    {
        std::cerr<<std::endl;
        std::cerr<<"Usage: "<<argv[0]<<" UHSCN-METADATA USHCN-RURAL-LIST (output)USHCN-RURAL-METADATA"<<std::endl;
        return 0;
    }
    
    fprintf(stderr,"Arg0=%s  Arg1=%s  Arg2=%s  Arg3=%s \n",
            argv[0],argv[1],argv[2],argv[3]);
    
    std::string strUshcnFile=std::string(argv[1]);
    std::string strRuFile=std::string(argv[2]);

    
    // pUshcn --> USHCN metadata file
    // pRuCsv --> Rural/urban csv file
    std::fstream* pUshcnStream;
    std::fstream* pRuStream;

    std::set<std::string> setUshcnStations;

    try
    {
      pUshcnStream = new std::fstream(std::string(strUshcnFile),std::fstream::in);
      pRuStream = new std::fstream(std::string(strRuFile),std::fstream::in);
      if(NULL==pUshcnStream)
          std::cerr<<"Could not open "<<strUshcnFile<<std::endl; 
      if(NULL==pRuStream)
          std::cerr<<"Could not open "<<strRuFile<<std::endl;
    }
    catch(...) // some sort of file open failure
    {
        std::cerr << std::endl << std::endl;
        std::cerr << "Failed to create a stream for "
                  << strUshcnFile << " and/or" << strRuFile << std::endl;

        std::cerr << "Exiting.... " << std::endl;

        std::cerr << std::endl << std::endl;
        return 0;
    }

    char inBuf[512];
    int nStations=0;
    int nRuralStations=0;
    try
    {
        while(!pRuStream->eof())
        {
            pRuStream->getline(inBuf,64);
            nStations++;
            std::vector<std::string> vInBuf;
            boost::split(vInBuf,inBuf,boost::is_any_of(","));
            if(vInBuf.at(1)=="R")
            {
                nRuralStations++;
                char cBuf[64];
                ::snprintf(cBuf,63,"USH%8.8d",boost::lexical_cast<int>(vInBuf.at(0)));
                
                std::string strUshcn=std::string(cBuf);
                
                std::cerr<<"Found rural station "<<strUshcn<<std::endl;
                
                setUshcnStations.insert(strUshcn);
                
            }
        }
    }
    catch(...)
    {
        std::cerr<<"Caught an exception (hopefully EOF)"<<std::endl;
    }
    std::cerr<<std::endl;
    std::cerr<<"Found "<<nRuralStations<<" rural stations out of a total of "<<nStations<<" stations."<<std::endl;
    std::cerr<<std::endl;

    try
    {
        std::cerr<<"Now extract the rural station metadata from "<<strUshcnFile<<std::endl;
        nStations=0;
        nRuralStations=0;
        while(!pUshcnStream->eof())
        {
            pUshcnStream->getline(inBuf,511);
            nStations++;
            std::cerr<<"Line "<<nStations<<":  "<<std::string(inBuf)<<std::endl;
            
            char cStationName[20];
            sscanf(inBuf,"%11s",&cStationName[0]);
            std::string strStationName=std::string(cStationName);
            if(setUshcnStations.find(strStationName)!=setUshcnStations.end())
            {
                nRuralStations++;
                std::cerr<<"Found rural station "<<strStationName<<std::endl;
                std::cout<<inBuf<<std::endl;
            }
            else
            {
                std::cerr<<"Station "<<strStationName<<" is not a rural station."<<std::endl;
            }
        }
    }
    catch(...)
    {
        std::cerr<<"Caught an exception (hopefully EOF)"<<std::endl;
    }

#if(0)
    try
    {
        while(!pUshcnStream->eof())
        {
            pUshcnStream->getline(inBuf,64);
            std::vector<std::string> vInBuf;
            boost::split(vInBuf,inBuf,boost::is_any_of(","));
            if(vInBuf.at(1)=="R")
            {
                char cBuf[64];
                ::snprintf(cBuf,63,"USH%8.8d",boost::lexical_cast<int>(vInBuf.at(0)));
                
                std::string strUshcn=std::string(cBuf);
                
                setUshcnStations.insert(strUshcn);
                
            }
        }
        catch(...)
        {
            if(pRuCsv!=NULL)
            {
                pRuCsv->close(); // make sure it's closed
            }
            pRuCsv=NULL;
            return;
        }
    }
    
  #endif
  
  return 0;
  
}
