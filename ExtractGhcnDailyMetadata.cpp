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
    std::string strDaily;

    if(argc<4){
        std::cerr<<std::endl;
        std::cerr<<"Usage: "<<argv[0]<<" (char*)Infile (char*)Outfile (float)fMinLat \\ "<<std::endl;
        std::cerr<<"               [(float)fMaxLat=90 [(float)fMinLon=-180E [(float)fMaxLon=180E [(float)fMinAlt=-300 [(float)fMaxAlt=20000]]]]] "<<std::endl;
        std::cerr<<std::endl;
        return 0;
    }

    std::string strInFile=std::string(argv[1]);
    std::string strOutFile=std::string(argv[2]);
    std::string strMinLat=std::string(argv[3]);

    float fMinLatDeg=-90;
    float fMaxLatDeg=90;
    float fMinLonDeg=-180;
    float fMaxLonDeg=180;
    float fMinAltMet=20000;
    float fMaxAltMet=-300;

    float fAllAverageAltMet=0;
    int nAllStations=0;
    float fSelectedAverageAltMet=0;
    int nSelectedStations=0;
    float fExcludedAverageAltMet=0;

    try {
        fMinLatDeg=boost::lexical_cast<float>(strMinLat);
        if(argc>=5) {
            std::string strMaxLat;
            strMaxLat=std::string(argv[4]);
            fMaxLatDeg=boost::lexical_cast<float>(strMaxLat);
        }
        if(argc>=6) {
            std::string strMinLon;
            strMinLon=std::string(argv[5]);
            fMinLonDeg=boost::lexical_cast<float>(strMinLon);
        }
        if(argc>=7) {
            std::string strMaxLon;
            strMaxLon=std::string(argv[6]);
            fMaxLonDeg=boost::lexical_cast<float>(strMaxLon);
        }
        if(argc>=8)
        {
            std::string strMinAlt;
            strMinAlt=std::string(argv[7]);
            fMinAltMet=boost::lexical_cast<float>(strMinAlt);
        }
        if(argc>=9)
        {
            std::string strMaxAlt;
            strMaxAlt=std::string(argv[8]);
            fMaxAltMet=boost::lexical_cast<float>(strMaxAlt);
        }
  
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << " Invalid min/max lat/long specification."<<'\n';
        return 0;
    }
    
    if(fMaxLatDeg<fMinLatDeg) {
        std::cerr<<"Max latitude must be >= min latitude."<<std::endl;
        return 0;
    }

    if(fMaxLonDeg<fMinLonDeg) {
        std::cerr<<"Max longitude must be >= min longitude."<<std::endl;
        return 0;
    }

    if(fMaxAltMet<fMinAltMet)
    {
        std::cerr<<"Max altitude must be >= min altitude."<<std::endl;
        return 0;
    }
    std::cout<<std::endl;
    std::cout<<"Will extract metadata for all stations with latitudes "<<fMinLatDeg<<"->"<<fMaxLatDeg<<" degrees, "
             <<fMinLonDeg<<"->"<<fMaxLonDeg<<" degrees, "<<fMinAltMet<<"->"<<fMaxAltMet<<" meters."<<std::endl;
    std::cout<<"Hit return to continue ";
    std::string strIn;
    std::getline(std::cin,strIn);
    
    std::ifstream inFd;
    std::ofstream outFd;
    
    inFd.open(strInFile.c_str(),std::ifstream::in);
    if(!inFd.is_open()){
        std::cerr<<std::endl;
        std::cerr<<"Error: failed to open input file "<<strInFile<<std::endl;
        std::cerr<<std::endl;
        return 0;
    }
    
    outFd.open(strOutFile.c_str(),std::ofstream::out);
   if(!outFd.is_open()){
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
        strInBuf=std::string(cBuf);

        if(!inFd.good())
            break;
        
        nStationTot++;
        
        // Will tokenize the station metadata file line strInBuf and
        // extract the lat/lon tokens.
        std::vector<std::string> vStrTokens0,vStrTokens;
        
        // Latitude token should be vStrTokens.at(1), longitude=vStrTokens.at(2)
        try {
            // InFd.getline() doesn't include the terminating delimiter (default "\n"),
            // so must add it back in here to get a properly formatted output file.
            strOutBuf=strInBuf+"\n";
            boost::split(vStrTokens0,strInBuf,boost::is_any_of(" "));
            for(auto iv: vStrTokens0) {
                // boost::split generates zero-length tokens for 2 or more consecutive delimiters.
                // Will ditch 'em here.
                if(iv.size()>0)
                    vStrTokens.push_back(iv);
            }
            if(vStrTokens.size()<3){
                std::cerr<<std::endl;
                std::cerr<<"Not enough data fields -- did you pick the right file?"<<std::endl;
                std::cerr<<std::endl;
                return 1;
            }
            
            float fLatDeg=boost::lexical_cast<float>(vStrTokens.at(1));
            float fLonDeg=boost::lexical_cast<float>(vStrTokens.at(2));
            float fAltMet=boost::lexical_cast<float>(vStrTokens.at(3));
            if((fLatDeg>=fMinLatDeg)&&(fLatDeg<=fMaxLatDeg) &&
               (fLonDeg>=fMinLonDeg)&&(fLonDeg<=fMaxLonDeg) &&
               (fAltMet>=fMinAltMet)&&(fAltMet<=fMaxAltMet))
            {
                // Station within the selected latitude range.
                // Save the metadata line to the new metadata file.
                outFd.write(strOutBuf.c_str(),strOutBuf.size());
                nStationInclude++;
                fSelectedAverageAltMet+=fAltMet;
            }
            else
            {
                fExcludedAverageAltMet+=fAltMet;
                nStationExclude++;
            }
            fAllAverageAltMet+=fAltMet;
            nAllStations++;
        }
        catch(const std::exception& e) {
            std::cerr << e.what() << '\n';
            continue;
        }

    } while(inFd.good());

    inFd.close();
    outFd.close();
    
    std::cout<<std::endl;
    std::cout<<"Total stations processed="<<nStationTot<<std::endl;
    std::cout<<"Stations included="<<nStationInclude<<std::endl;
    std::cout<<"Stations excluded="<<nStationExclude<<std::endl;
    if(nStationInclude>0)
        std::cout<<"Average altitude: selected stations="<<fSelectedAverageAltMet/nStationInclude<<std::endl;
    if(nStationExclude>0)
        std::cout<<"Average altitude: excluded stations="<<fExcludedAverageAltMet/nStationExclude<<std::endl;
    std::cout<<"Average altitude: all stations="<<fAllAverageAltMet/nAllStations<<std::endl;
    std::cout<<std::endl;
}
