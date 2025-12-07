#pragma once

/**

   Use this to extract and print embedded version, build date-time info.

 */

#ifndef _BUILD_DATE_TIME_
#define _BUILD_DATE_TIME_ "Undefined_Date_Time"
#endif

#ifndef _BUILD_GIT_BRANCH_
#define _BUILD_GIT_BRANCH_ "Undefined_Git_Branch"
#endif

#ifndef _BUILD_GIT_HASH_
#define _BUILD_GIT_HASH_ "Undefined_Git_Hash"
#endif

#ifndef _BUILD_GIT_REV_NUM_
#define _BUILD_GIT_REV_NUM_ "Undefined_Git_Rev_Num"
#endif

#ifndef _BUILD_GIT_FULL_REV_STRING
#define _BUILD_GIT_FULL_REV_STRING "Undefined_Full_Rev_String"
#endif

#include <iostream>
#include <boost/lexical_cast.hpp>

class AnomalyVersion
{
  public:
    AnomalyVersion(void){};
    
    virtual ~AnomalyVersion(void){};

    static const std::string ms_strDateTime;

    static const std::string ms_strGitBranch;
    static const std::string ms_strGitHash;
    static const std::string ms_strGitRevNum;

    static const std::string ms_strGitFullRevString;

    
    static void DumpBuildInfo(std::ostream& ostr)
    {
        ostr<<std::endl;
        ostr<<"Build date-time: "<<AnomalyVersion::ms_strDateTime<<std::endl;
        ostr<<"Git branch & commit number:hash "<<AnomalyVersion::ms_strGitFullRevString<<std::endl;
        ostr<<std::endl;
        return;
    };
    
    // Another name for DumpBuildInfo()
    static void DumpVersionInfo(std::ostream& ostr)
    {
       DumpBuildInfo(ostr);
       return;
    };

    static std::string GetGitBranch(void)
    {
       return ms_strGitBranch;
    }
    
    static std::string GetGitHash(void)
    {
        return ms_strGitHash;
    }

    static std::string GetGitRevNum(void)
    {
        return ms_strGitRevNum;
    }

    
};

const std::string AnomalyVersion::ms_strDateTime=std::string(_BUILD_DATE_TIME_);

const std::string AnomalyVersion::ms_strGitBranch=std::string(_BUILD_GIT_BRANCH_);

const std::string AnomalyVersion::ms_strGitHash
     =std::string(boost::lexical_cast<std::string>(_BUILD_GIT_HASH_));

const std::string AnomalyVersion::ms_strGitRevNum
      =std::string(boost::lexical_cast<std::string>(_BUILD_GIT_REV_NUM_));

const std::string AnomalyVersion::ms_strGitFullRevString
     =std::string(_BUILD_GIT_FULL_REV_STRING_);
