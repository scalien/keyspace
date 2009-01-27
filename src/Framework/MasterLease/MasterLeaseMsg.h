#ifndef MASTERLEASEMSG_H
#define MASTERLEASEMSG_H

#include "System/Buffer.h"

#define EXTEND_LEASE		'l'
#define YIELD_LEASE			'y'

class MasterLeaseMsg
{
public:
	char			type;
	int				nodeID;
	int				epochID;
	
	void			ExtendLease(int nodeID_, int epochID_);
	void			YieldLease(int nodeID_, int epochID_);

	static bool		Read(ByteString data, MasterLeaseMsg* msg);
	static bool		Write(MasterLeaseMsg* msg, ByteString& data);
};

#endif
