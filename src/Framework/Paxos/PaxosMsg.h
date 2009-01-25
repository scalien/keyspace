#ifndef PAXOSMSG_H
#define PAXOSMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"

// paxos message types:
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

#define	VALUE_SIZE					64

// todo: see if static value buffer is necessary, to avoid memcopy()

class PaxosMsg
{
public:
	long					paxosID;
	char					type;
	long					n;
	char					response;
	long					n_accepted;
	ByteArray<VALUE_SIZE>	value;

	/*	if type == PREPARE_RESPONSE and
		response == PREPARE_PREVIOUSLY_ACCEPTED
		then it contains the previously accepted value
	*/
	
	void			Init(long paxosID_, char type_);
		
	void			PrepareRequest(long paxosID_, long n_);
	void			PrepareResponse(long paxosID_, long n_, char response_);
	bool			PrepareResponse(long paxosID_, long n_, char response_,
						long n_accepted_, ByteString value_);
	
	bool			ProposeRequest(long paxosID_, long n_, ByteString value_);
	void			ProposeResponse(long paxosID_, long n_, char response_);
	
	bool			LearnChosen(long paxosID_, ByteString value_);

	static bool		Read(ByteString data, PaxosMsg* msg);
	static bool		Write(PaxosMsg* msg, ByteString& data);
};

#endif