#ifndef PAXOSCONSTS_H
#define PAXOSCONSTS_H

#include "System/Common.h"

#define	PAXOS_VAL_SIZE				(1*MB + 4*KB)
#define PAXOS_BUF_SIZE				(PAXOS_VAL_SIZE + 1*KB)

#define PAXOS_TIMEOUT				3000 // TODO: I increased this for testing

#endif
