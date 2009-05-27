#ifndef PLEASEMSG_H
#define PLEASEMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include <stdint.h>

#define PLEASE_PREPARE_REQUEST				'1'
#define PLEASE_PREPARE_REJECTED				'2'
#define PLEASE_PREPARE_PREVIOUSLY_ACCEPTED	'3'
#define PLEASE_PREPARE_CURRENTLY_OPEN		'4'
#define PLEASE_PROPOSE_REQUEST				'5'
#define PLEASE_PROPOSE_REJECTED				'6'
#define PLEASE_PROPOSE_ACCEPTED				'7'
#define PLEASE_LEARN_CHOSEN					'8'

class PLeaseMsg
{
public:
	char			type;
	unsigned		nodeID;
	uint64_t		proposalID;
	
	uint64_t		acceptedProposalID;
	unsigned		leaseOwner;
	uint64_t		expireTime;
	uint64_t		paxosID; // so only up-to-date nodes can become masters
	
	void			Init(char type_, unsigned nodeID_);
		
	bool			PrepareRequest(unsigned nodeID_,
					uint64_t proposalID_, uint64_t paxosID_);
	bool			PrepareRejected(unsigned nodeID_, uint64_t proposalID_);
	bool			PreparePreviouslyAccepted(unsigned nodeID_,
					uint64_t proposalID_, uint64_t acceptedProposalID_,
					unsigned leaseOwner_, uint64_t expireTime_);
	bool			PrepareCurrentlyOpen(unsigned nodeID_,
					uint64_t proposalID_);	
	bool			ProposeRequest(unsigned nodeID_, uint64_t proposalID_,
					unsigned leaseOwner_, uint64_t expireTime_);
	bool			ProposeRejected(unsigned nodeID_, uint64_t proposalID_);
	bool			ProposeAccepted(unsigned nodeID_, uint64_t proposalID_);
	bool			LearnChosen(unsigned nodeID,
					unsigned leaseOwner_, uint64_t expireTime_);
	
	bool			IsRequest();
	bool			IsPrepareResponse();
	bool			IsProposeResponse();
	bool			IsResponse();
	
	bool			Read(const ByteString& data);
	bool			Write(ByteString& data);	
};

#endif
