#pragma once

// Enable #define WINDOWS_XP or #define UNIX, depending on platform to build for

// For non-Linux Unix-type systems (i.e. some or all versions
// of OSX), you will want to enable #define UNIX and disable #define LINUX.
// For Linux systems, #define UNIX and #define LINUX must both be enabled
// Comment out the line below if your compilation fails
// with a "MSG_NOSIGNAL was not declared..." error.
// Experimentation may be needed with certain systems

#define UNIX
#define LINUX

// #define WINDOWS_XP

#if defined(UNIX) && defined(WINDOWS_XP)
#error "Cannot define both UNIX and WINDOWS_XP"
#endif

#if !defined(UNIX) && !defined(WINDOWS_XP)
#error "Must define either UNIX or WINDOWS_XP"
#endif

#if !defined(WINDOWS_XP)

#pragma message " "
#pragma message "########################################################"

#if defined(LINUX)

#pragma message  "Configured to build on non-OSX platforms."
#pragma message  "To build on OSX platforms, comment out"
#pragma message  "the \'#define LINUX\' line in SimpleSocketPlatformDef.h"

#else

#pragma message  "Configured to build on OSX platforms"
#pragma message  "To build on Linux platforms, enable the"
#pragma message  "the \'#define LINUX\' line in SimpleSocketPlatformDef.h"

#endif

#pragma message "########################################################"
#pragma message " "

#endif
