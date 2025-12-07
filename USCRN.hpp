#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <ftw.h>
#include <time.h>
#include <errno.h>

#include <stdio.h>
#include <signal.h>

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <functional>

#include <iostream>
#include<fstream>
#include <filesystem>

#include <boost/function.hpp>
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

class USCRN
{
 
    public:
    
    USCRN(const std::string& strFileName, const int& nYear);
    
    void OpenInSubHourlyStream(const std::string& strInSubHourlyFileName, const int& nYear);    
    void OpenOutMonthlyStream(const std::string& strOutMonthlyFullPath);
        
    void ProcessInSubHourlyFiles(const std::string& strInSubHourlyFileName, 
                                int nStartYear, int nEndYear);

    void ProcessInSubHourlyFile(const std::string & strOutMonthlyFileName);

    void ReadAndParseSubHourlyData(void);

    static const int MIN_USCRN_YEAR=2006;
    static const int MAX_USCRN_YEAR=2024;
    
    // Directory where monthly average files will be written.
    std::string m_strInSubHourlyDir;
    std::string m_strOutMonthlyDir;

    std::string m_strInSubHourlyFileName;
    std::string m_strOutMonthlyFileName;

    int m_nWbanId; // cols 0-4
    int m_nYear;   // cols 19-22
    int m_nMonth;  // cols 23-24 
    int m_nDay;    // cols 25-26
    int m_nHour;   // cols 28-29
    float m_fTemp; // cols 57-63

    protected:

    std::string m_strUshcnLineTemplate1;
    std::string m_strUshcnLineTemplate2;
    std::fstream* m_pInSubHourlyStream;
    std::fstream* m_pOutMonthlyStream;

    std::fstream* _OpenInSubHourlyStream(const std::string& strFileName, const std::string& strYear);
    std::fstream* _OpenOutMonthlyStream(const std::string& strOutMonthlyDir, const std::string& strOutMonthlyFullPath);

    void CloseInSubHourlyStream(void)
    {   
        m_pInSubHourlyStream->close();
        m_pInSubHourlyStream=NULL;
    }

    void CloseOutMonthlyStream(void)
    {   
        m_pOutMonthlyStream->close();
        m_pOutMonthlyStream=NULL;
    }

    void _ReadAndParseSubHourlyData(void);

}