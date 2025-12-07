#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <utility>
#include <map>
#include <set>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

int main(int argc, char** argv)
{
    
    if(argc<3)
    {
        std::cerr<<std::endl;
        std::cerr<<"Usage: "<<argv[0]<<" (char*)Infile (char*)Outfile "<<std::endl;
        std::cerr<<std::endl;
        return 0;
    }

    std::string strInFile=std::string(argv[1]), strOutFile=std::string(argv[2]);
    
    std::ifstream inFd;
    std::ofstream outFd;

    inFd.open(strInFile.c_str(),std::ifstream::in);
    if(!inFd.is_open())
    {
        std::cerr<<std::endl;
        std::cerr<<"Error: failed to open input file "<<strInFile<<std::endl;
        std::cerr<<std::endl;
        return 0;
    }
    
    outFd.open(strOutFile.c_str(),std::ofstream::out);
    if(!outFd.is_open())
    {
        std::cerr<<std::endl;
        std::cerr<<"Error: failed to open output file "<<strInFile<<std::endl;
        std::cerr<<std::endl;
        return 0;
    }
  
    std::map<int,std::set<int> > mapYearCoopIdSet;
    std::string strInBuf;
    std::map<int,int> mapNumStationsPerYear;
    int nLine=0;
    int nBytes=0;
    try
    {
        do {
            nLine++;
            std::cout<<"Processing line "<<nLine<<"      \r";
            char cBuf[513];
            ::memset(cBuf,'\0',513);
            inFd.getline(cBuf,512);
            strInBuf=std::string(cBuf);
            if(strInBuf.size()<260)
                break;
            std::string strTmp;
            strTmp=strInBuf.substr(0,6); // COOP ID: Cols 0-5
            int nCoopId=boost::lexical_cast<int>(strTmp);
            strTmp=strInBuf.substr(6,4); // Year: Cols 6-9
            int nYear=boost::lexical_cast<int>(strTmp);
            strTmp=strInBuf.substr(10,2); // Month: Cols 10-11
            int nMonth=boost::lexical_cast<int>(strTmp); 
            std::string strElement=strInBuf.substr(12,4); // Element: Cols 12-15 (TMIN,TMAX etc.)
            int nTempCount=0;
            if("TMAX"==strElement)
            {
                if(mapYearCoopIdSet.find(nYear)==mapYearCoopIdSet.end())
                {
                    mapYearCoopIdSet[nYear]=std::set<int>();
                }
                mapYearCoopIdSet[nYear].insert(nCoopId);
            }
        } while(inFd.good());   
    } 
    catch(const std::exception& e)
    {
            std::cerr<<e.what()<<'\n';
            std::cerr<<"Thrown while reading line "<<nLine<<std::endl;
            std::cerr<<"Line="<<strInBuf<<std::endl;
    }

    std::cout<<"Creating a map of station coop ids and number of years station was active."<<std::endl<<std::endl;
    for(auto& im: mapYearCoopIdSet)
    {
        mapNumStationsPerYear[im.first]=im.second.size();
    }

    for(auto im: mapNumStationsPerYear)
    {
        std::string strLine=boost::lexical_cast<std::string>(im.first)+std::string(",")
                           +boost::lexical_cast<std::string>(im.second)+std::string("\n");

        outFd.write(strLine.c_str(),strLine.size());
    }

    return 0;
}