#ifndef PAXOSMSG_H
#define PAXOSMSG_H

#include <unistd.h>
#include <string.h>
#include "System/Buffer.h"
#include "System/Types.h"

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

// TODO: see if static value buffer is necessary, to avoid memcopy()

class PaxosMsg
{
public:
	ulong64					paxosID;
	char					type;
	ulong64					n;
	char					response;
	ulong64					n_accepted;
	ByteArray<VALUE_SIZE>	value;

	/*	if type == PREPARE_RESPONSE and
		response == PREPARE_PREVIOUSLY_ACCEPTED
		then it contains the previously accepted value
	*/
	
	void			Init(ulong64 paxosID_, char type_);
		
	bool			PrepareRequest(ulong64 paxosID_, ulong64 n_);
	bool			PrepareResponse(ulong64 paxosID_, ulong64 n_, char response_);
	bool			PrepareResponse(ulong64 paxosID_, ulong64 n_, char response_,
						ulong64 n_accepted_, ByteString value_);
	
	bool			ProposeRequest(ulong64 paxosID_, ulong64 n_, ByteString value_);
	bool			ProposeResponse(ulong64 paxosID_, ulong64 n_, char response_);
	
	bool			LearnChosen(ulong64 paxosID_, ByteString value_);

	bool			Read(ByteString& data);
	bool			Write(ByteString& data);
};

#endif
