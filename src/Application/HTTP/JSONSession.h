#ifndef JSON_SESSION_H
#define JSON_SESSION_H

#include "System/Buffer.h"

class HttpConn;

class JSONSession
{
public:
	void			Init(HttpConn* conn, const ByteString& jsonCallback);

	void			PrintStatus(const char* status, const char* type = NULL);

	void			PrintString(const char* s, unsigned len);
	void			PrintNumber(double number);
	void			PrintBool(bool b);
	void			PrintNull();
	
	void			PrintObjectStart(const char* name, unsigned len);
	void			PrintObjectEnd();

private:
	ByteString		jsonCallback;
	HttpConn*		conn;
};

#endif
