#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "System/Common.h"

#define MAX_TCP_MESSAGE_SIZE (1*MB + 1*KB + 10)
// 1MB for data, 1KB for application headers, 10 bytes for the message envelope

#define MAX_UDP_MESSAGE_SIZE (64*KB + 1*KB)
// 64K for data, 1KB for application headers

#endif
