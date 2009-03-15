#ifndef PLEASEMSG_H
#define PLEASEMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include "System/Types.h"

// PaxosLease message types:
#define PREPARE_REQUEST				'1'
#define	PREPARE_RESPONSE			'2'
#define PROPOSE_REQUEST				'3'
#define PROPOSE_RESPONSE			'4'
#define LEARN_CHOSEN				'5'

// prepare responses:
#define PREPARE_REJECTED			'r'
#define PREPARE_PREVIOUSLY_ACCEPTED	'a'
#define PREPARE_CURRENTLY_OPEN		'o'

// propose responses:
#define PROPOSE_REJECTED			'r'
#define PROPOSE_ACCEPTED			'a'

class PLeaseMsg
{
public:
	char			type;
	unsigned		nodeID;
	ulong64			proposalID;
	char			response;
	
	ulong64			acceptedProposalID;
	unsigned int	leaseOwner;
	ulong64			expireTime;
	
	void			Init(char type_, unsigned nodeID_);
		
	bool			PrepareRequest(unsigned nodeID_, ulong64 proposalID_);
	bool			PrepareResponse(unsigned nodeID_, ulong64 proposalID_, char response_);
	bool			PrepareResponse(unsigned nodeID_, ulong64 proposalID_, char response_,
						ulong64 acceptedProposalID_, unsigned leaseOwner_, ulong64 expireTime_);
	
	bool			ProposeRequest(unsigned nodeID_, ulong64 proposalID_,
						unsigned leaseOwner_, ulong64 expireTime_);
	bool			ProposeResponse(unsigned nodeID_, ulong64 proposalID_, char response_);
	
	bool			LearnChosen(unsigned nodeID, unsigned leaseOwner_, ulong64 expireTime_);
	
	bool			Read(ByteString& data);
	bool			Write(ByteString& data);
};

#endif
