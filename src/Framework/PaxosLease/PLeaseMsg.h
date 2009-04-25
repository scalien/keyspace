#ifndef PLEASEMSG_H
#define PLEASEMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include <stdint.h>

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
	uint64_t		proposalID;
	char			response;
	
	uint64_t		acceptedProposalID;
	unsigned int	leaseOwner;
	uint64_t		expireTime;
	uint64_t		paxosID; // so only up-to-date nodes can become masters
	
	void			Init(char type_, unsigned nodeID_);
		
	bool			PrepareRequest(unsigned nodeID_, uint64_t proposalID_, uint64_t paxosID_);
	bool			PrepareResponse(unsigned nodeID_, uint64_t proposalID_, char response_);
	bool			PrepareResponse(unsigned nodeID_, uint64_t proposalID_, char response_,
						uint64_t acceptedProposalID_, unsigned leaseOwner_, uint64_t expireTime_);
	
	bool			ProposeRequest(unsigned nodeID_, uint64_t proposalID_,
						unsigned leaseOwner_, uint64_t expireTime_);
	bool			ProposeResponse(unsigned nodeID_, uint64_t proposalID_, char response_);
	
	bool			LearnChosen(unsigned nodeID, unsigned leaseOwner_, uint64_t expireTime_);
	
	bool			Read(ByteString& data);
	bool			Write(ByteString& data);
};

#endif
