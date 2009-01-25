#ifndef MEMOCLIENTMSG_H
#define MEMOCLIENTMSG_H

#include "Buffer.h"

#define GET_REQUEST				'1'
#define	GET_RESPONSE			'2'
#define SET_REQUEST				'3'
#define SET_RESPONSE			'4'
#define TESTANDSET_REQUEST		'5'
#define TESTANDSET_RESPONSE		'6'
#define	ERROR					'e'

#define EXEC_OK					'o'
#define EXEC_FAIL				'f'

#define RESPONSE_OK				'o'
#define RESPONSE_FAIL			'f'

class MemoClientMsg
{
public:
	char			type;
	char			exec;
	char			response;
	
	ByteString		key;			// for	get,	set,	testandset
	ByteString		value;			// for			set,	testandset
	ByteString		test;			// for					testandset
	
	void			GetRequest(ByteString key_);
	void			GetResponse(char exec_, char response_, ByteString value_);
	void			GetResponse(char exec_, char response_);

	
	void			SetRequest(ByteString key_, ByteString value_);
	void			SetResponse(char exec_, char response_);
	
	void			TestAndSetRequest(ByteString key_, ByteString test_, ByteString value_);
	void			TestAndSetResponse(char exec_, char response_);
	
	void			Error();
	
	static int		Read(ByteString data, MemoClientMsg* msg);
	static bool		Write(MemoClientMsg* msg, ByteString& data);
};

#endif