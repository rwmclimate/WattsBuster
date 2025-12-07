//-----------------------------------------------------------------------------------------
/* 

  Stolen shamelessly from Liyang Yu (although he mentions that mentioning him, the original
  article and his work is enough for licensing, see:
  http://beta.codeproject.com/Messages/3793113/Re-Once-again-License.aspx and the licenses
  are at:
  http://beta.codeproject.com/info/Licenses.aspx so we could probably assume the CPOL).

  This is to implement the domain and IP address resolution. 

  3 cases are considered:

  1. a host name is given (a host name looks like "www.delta.com"), query
     the IP address of the host

  2. an IP address is given (an IP address looks like 10.6.17.184), query
     the host name

  in the above two cases, the IP address and the host name are the same thing: 
  since IP address is hard to remember, they are usually aliased by a name, and this 
  name is known as the host name.

  3. nothing is given. in other words, we don't know the host name or the IP address.
     in this case, the standard host name for the current processor is used
     
*/
//-------------------------------------------------------------------------------------------

#ifndef SimpleSocketHostInfo_H
#define SimpleSocketHostInfo_H

#include "SimpleSocketPlatformDef.h"


#include <string>
// using namespace std;

// this version is for both Windows and UNIX, the following line
// specifies that this is for WINDOWS
/* #ifndef WINDOWS_XP */
/*      #define WINDOWS_XP */
/* #endif */

// #include <XPCException.h>  // add this later

#ifdef UNIX
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#else
    #include <winsock2.h>
#endif
#include <stdio.h>
    
enum hostType {NAME, ADDRESS};
const int HOST_NAME_LENGTH = 64;

class SimpleSocketHostInfo
{

private:

#ifdef UNIX
        char m_cSearchHostDB;     // search the host database flag
#endif

        struct hostent *mp_cHostPtr;    // Entry within the host address database

public:

    // Default constructor
    SimpleSocketHostInfo();

    // Retrieves the host entry based on the host name or address
    SimpleSocketHostInfo(const std::string& hostName, hostType type);
 
    // Destructor.  Closes the host entry database.
    ~SimpleSocketHostInfo()
    {
#ifdef UNIX
            endhostent();
#endif
    };
    

#ifdef UNIX
    
    // Retrieves the next host entry in the database
    char GetNextHost(void);
    
    // Opens the host entry database
    void OpenHostDb(void)
    {
            endhostent();
            m_cSearchHostDB = 1;
            sethostent(1);
    };
    
    
#endif

    // Retrieves the hosts IP address
    char* GetHostIPAddress(void) 
    {
      if(NULL==mp_cHostPtr)
        return NULL;
      if(NULL==mp_cHostPtr->h_addr_list)
         return NULL;

        struct in_addr *addr_ptr;
                // the first address in the list of host addresses
        addr_ptr = (struct in_addr *)*mp_cHostPtr->h_addr_list;
                // changed the address format to the Internet address in standard dot notation
        return inet_ntoa(*addr_ptr);
    };
    
    
    // Retrieves the hosts name
    const char* GetHostName(void)
    {
      if(NULL==mp_cHostPtr)
        return "No hostname info.";
      
      if(NULL==mp_cHostPtr->h_name)
      {
          return "No hostname info.";
      }
      
      return mp_cHostPtr->h_name;

    };
    

private:

        #ifdef WINDOWS_XP
    void DetectErrorGethostbyname(int*,std::string&);
    void DetectErrorGethostbyaddr(int*,std::string&);
        #endif
};

#endif
