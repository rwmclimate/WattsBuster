#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>

int main(int argc, char** argv)
{
  int nIdOffset=3;
  int nIdLen=8;

  if((argc<2) && (argc!=4))
    {
      std::cerr<<"Usage:"<<argv[0]<<" vanilla-ghcnv4-inv-file-name [ID-offset=3 ID-len=8]"<<std::endl;
      return 1;
    }
  
  if(4==argc)
    {
      nIdOffset=boost::lexical_cast<int>(argv[2]);
      nIdLen=boost::lexical_cast<int>(argv[3]);
    }
  
    std::ifstream inFstr;
    std::vector<std::string> vstrNumId;
    std::vector<std::string> vstrDupIds;
    std::vector<std::string> vstrNames;
    try
    {
        /* code */
        inFstr.open(argv[1]);

        do
        {
           char buf[513];
            std::string strBuf;
            inFstr.getline(buf,512);
            strBuf=std::string(buf);
            if(strBuf.size()==0)
                break;
            std::string strWmoId=strBuf.substr(nIdOffset,nIdLen);
            std::string strName=strBuf.substr(38);
            vstrNumId.push_back(strWmoId);
            vstrNames.push_back(strName);
            // std::cout<<"Station ID: "<<strWmoId<<std::endl;
        } while(inFstr.good());
        
        inFstr.close();

        int ii1,ii2;
        int iCount=0;
        for(ii1=0; ii1<vstrNumId.size(); ++ii1)
        {
            for(ii2=ii1+1; ii2<vstrNumId.size(); ++ii2)
            {
                if(vstrNumId.at(ii1)==vstrNumId.at(ii2))
                {
                    iCount++;
                    vstrDupIds.push_back(vstrNumId.at(ii1));
                    std::cout<<iCount<<": Found Duplicate wmoId="<<vstrNumId.at(ii1)<<std::endl;
                    std::cout<<"     Dup station names:"<<std::endl;
                    std::cout<<"       1: "<<vstrNames.at(ii1)<<std::endl;
                    std::cout<<"       2: "<<vstrNames.at(ii2)<<std::endl<<std::endl;
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr<<"Caught an exception"<<std::endl;
        std::cerr << e.what() << '\n';
    }

 #if(0)   
    std::cout<<"Found "<<vstrDupIds.size() <<" duplicate station ids"<<std::endl<<std::endl;

    int ii=1;
    for(auto& iv:vstrDupIds)
    {
        std::cout<<"Duplicate id #"<<ii<<":  "<<iv<<std::endl;
    }
#endif
    return 0;
}
