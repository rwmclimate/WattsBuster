#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
int main(int argc, char** argv)
{
    int ProcessV2Metadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg);
    int ProcessV3Metadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg);
    int ProcessV4Metadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg);
    int ProcessV4PriscianMetadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg);
    
    
    if(argc<5)
    {
        std::cerr<<std::endl;
        std::cerr<<"Usage: "<<argv[0]<<" (char*)Infile (char*)Outfile (char*) GhcnDataType (float)fMinLat [(float)fMaxLat=90]"<<std::endl;
        std::cerr<<"  GhcnDataType: 2 for GHCNv2, 3 for GHCNv3, 4 for GHCNv4, 4P for GHCNv4 with Priscian extended data."<<std::endl;
        std::cerr<<std::endl;
        return 0;
    }

    std::string strInFile=std::string(argv[1]);
    std::string strOutFile=std::string(argv[2]);
    std::string strMetadataType=std::string(argv[3]);


    std::string strMinLat=std::string(argv[4]); 
    std::string strMaxLat="90";

    if(argc>=5)
        strMaxLat=std::string(argv[5]);
 
    float fMinLatDeg;
    try
    {
        /* code */
        fMinLatDeg=boost::lexical_cast<float>(strMinLat);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << " Invalid min latitude specification"<<'\n';
        return 0;
    }
    
    float fMaxLatDeg=90;
    if(argc>=4)
    try
    {
        /* code */
        fMaxLatDeg=boost::lexical_cast<float>(strMaxLat);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << " Invalid max latitude specification"<<'\n';
        return 0;
    }
 
    if(fMaxLatDeg<fMinLatDeg)
    {
        std::cerr<<"Max Latitude must be >= min latitude"<<std::endl;
        return 0;
    }
 
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
  
    int nLine=0;
    int nStationTot=0;
    int nStationInclude=0;
    int nStationExclude=0;

    do {
        nLine++;
        std::cout<<"Processing line "<<nLine<<"      \r";
        std::string strInBuf,strOutBuf;
        char cBuf[513];
        ::memset(cBuf,'\0',513);
        inFd.getline(cBuf,512);
        if(inFd.eof())
            break;

        strInBuf=std::string(cBuf);
        if(strInBuf.size()==0)
            break;

        // Get rid of leading blanks, if any.
        // Will guarantee that '#' comment delimiters
        // if present will be the 1st char.
        boost::trim_left(strInBuf);

        if(strInBuf.at(0)=='#')
        {
            // Commented-out line -- skip to the next line
            continue;
        }
        
        int nStatus=0;
        
        if("V2"==strMetadataType)
        {
            nStatus=ProcessV2Metadata(strInBuf,strOutBuf,fMinLatDeg,fMaxLatDeg);
        }
        else if("V3"==strMetadataType)
        {
            nStatus=ProcessV3Metadata(strInBuf,strOutBuf,fMinLatDeg,fMaxLatDeg);
        }
        else if("V4"==strMetadataType)
        {
            nStatus=ProcessV3Metadata(strInBuf,strOutBuf,fMinLatDeg,fMaxLatDeg);
        }
        else if("V4P"==strMetadataType)
        {
            nStatus=ProcessV4PriscianMetadata(strInBuf,strOutBuf,fMinLatDeg,fMaxLatDeg);
        }
        else
        {
            std::cerr<<std::endl;
            std::cerr<<strMetadataType<<": Invalid metadata specifier"<<std::endl;
            std::cerr<<"Valid choices are V2, V3, V4, or V4P"<<std::endl;
            std::cerr<<std::endl;
            inFd.close();
            outFd.close();
            exit(1);
        }
        if(1==nStatus)
        {
            // getline() does not read in the newline chars
            // at the end of the input file lines, so must add
            // that to strOutBuf here.
            strOutBuf=strOutBuf+"\n";
            outFd.write(strOutBuf.c_str(),strOutBuf.size());
        }
        else
            continue;

    } while(inFd.good());

    std::cout<<std::endl;
    std::cout<<"Total stations processed="<<nStationTot<<std::endl;
    std::cout<<"Stations included="<<nStationInclude<<std::endl;
    std::cout<<"Stations excluded="<<nStationExclude<<std::endl;
    std::cout<<std::endl;
}

int ProcessV2Metadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg)
{
    int nStatus=0;
 
    //10160355000 SKIKDA                          36.93    6.95    7   18U  107HIxxCO 1x-9WARM DECIDUOUS  C
    std::string strLatDeg=strInBuf.substr(43,6);
    boost::trim(strLatDeg);
    float fLatDeg;
    try
    {
        fLatDeg=boost::lexical_cast<float>(strLatDeg);
        if((fLatDeg>=fMinLatDeg)&&(fLatDeg<=fMaxLatDeg))
        {
            strOutBuf=strInBuf;
            nStatus=1;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cerr << __FUNCTION__<<"(): Exception thrown parsing a station latitude."<<std::endl;
        std::cerr<<"Check the metadata file format"<<std::endl<<std::endl;
        std::cerr<<"Offending line: \n"<<std::endl;
        std::cerr<<"    "<<strInBuf<<std::endl<<std::endl; 
        exit(1);
    }
    return nStatus;
}

int ProcessV3Metadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg)
{
    //10160355000  36.9300    6.9500    7.0 SKIKDA                           18U  107HIxxCO 1x-9WARM DECIDUOUS  C
    int nStatus=0;

    std::string strLatDeg=strInBuf.substr(12,8);
    boost::trim(strLatDeg);
    float fLatDeg;
    try
    {
        fLatDeg=boost::lexical_cast<float>(strLatDeg);
        if((fLatDeg>=fMinLatDeg)&&(fLatDeg<=fMaxLatDeg))
        {
            strOutBuf=strInBuf;
            nStatus=1;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cerr << __FUNCTION__<<"(): Exception thrown parsing a station latitude."<<std::endl;
        std::cerr<<"Check the metadata file format"<<std::endl<<std::endl;
        std::cerr<<"Offending line: \n"<<std::endl;
        std::cerr<<"    "<<strInBuf<<std::endl<<std::endl; 
        exit(1);
    }
    
    return nStatus;
}

int ProcessV4Metadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg)
{
    //AE000041196  25.3330   55.5170   34.0 SHARJAH_INTER_AIRP
    int nStatus=0;
    std::string strLatDeg=strInBuf.substr(12,8);
    boost::trim(strLatDeg);
    float fLatDeg;
    try
    {
       fLatDeg=boost::lexical_cast<float>(strLatDeg);
       if((fLatDeg>=fMinLatDeg)&&(fLatDeg<=fMaxLatDeg))
       {
           strOutBuf=strInBuf;
           nStatus=1;
       }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cerr << __FUNCTION__<<"(): Exception thrown parsing a station latitude."<<std::endl;
        std::cerr<<"Check the metadata file format"<<std::endl<<std::endl;
        std::cerr<<"Offending line: \n"<<std::endl;
        std::cerr<<"    "<<strInBuf<<std::endl<<std::endl; 
        exit(1);
    }

    return nStatus;
}

int ProcessV4PriscianMetadata(const std::string& strInBuf, std::string& strOutBuf, const float& fMinLatDeg, const float& fMaxLatDeg)
{
    // 0: metadata line not to be written to output file
    // 1: metadata line to be written to output file
    // -1: parsing exception -- no station info, nothing to be written to output file.
    int nStatus=0;
    
    // 1st line in the priscian-extended metadata file are field labels for all following lines.
    if(strInBuf.find("id,latitude,longitude")!=std::string::npos)
    {
        // InFd.getline() doesn't include the terminating delimiter (default "\n"),
        // so must add it back in here to get a properly formatted output file.
        strOutBuf=strInBuf;
        nStatus=1;  // Want to write this line to the output file.
        // outFd.write(strOutBuf.c_str(),strOutBuf.size());
    }
    else
    {
        // Must be a station metadata line.  Grab the latitude value.
        std::vector<std::string> vStrTokens;
        
        // Latitude token should be vStrTokens.at(1)
        try
        {
            boost::split(vStrTokens,strInBuf,boost::is_any_of(","));
            float fLatDeg;           
            fLatDeg=boost::lexical_cast<float>(vStrTokens.at(1));
            if((fLatDeg>=fMinLatDeg)&&(fLatDeg<=fMaxLatDeg))
            {
                // Station within the selected latitude range.
                // Save the metadata line to the new metadata file.
                strOutBuf=strInBuf;
                nStatus=1;  // Write this to the output metadata file
            }
            else
            {
                nStatus=0;   // Don't write this to output metadata file
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            std::cerr << __FUNCTION__<<"(): Exception thrown most likely while parsing a station latitude."<<std::endl;
            std::cerr<<"Check the metadata file format"<<std::endl<<std::endl;
            std::cerr<<"Offending line: \n"<<std::endl;
            std::cerr<<"    "<<strInBuf<<std::endl<<std::endl;
            exit(1);
            // nStatus=-1; // Parse error -- do nothing and process the next line.
        }
    }

    return nStatus;
    
}
