#ifndef PAXOSCONSTS_H
#define PAXOSCONSTS_H

#include "System/Common.h"

#define	VALUE_SIZE					(512*KB)
#define PAXOS_BUFSIZE				(512*KB + 1*KB)

#define PAXOS_PORT_OFFSET			0

#define PAXOS_TIMEOUT				1000 // TODO: I increased this for testing

#define WIDTH_NODEID				8
#define WIDTH_RESTART_COUNTER		16

#endif
