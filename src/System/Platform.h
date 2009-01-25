#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __APPLE__
#ifdef __MACH__
#define PLATFORM_DARWIN
#endif
#endif

#ifdef __linux__
#define PLATFORM_LINUX
#endif

#endif