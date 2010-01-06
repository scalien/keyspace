#ifndef FD_H
#define FD_H

// File description abstraction

// On Windows the file handle is a pointer, on unices it is a
// growing index in an array in the process, thus it can be used
// as an array index in IOProcessor. We emulate this behavior on
// Windows.


#ifdef PLATFORM_WINDOWS
struct FD
{
	int			index;
	intptr_t	sock;
};

extern const FD INVALID_FD;

#else
typedef int FD;
#define INVALID_FD -1
#endif



#endif
