#include "JSONSession.h"
#include "HttpConn.h"
#include "HttpConsts.h"

void JSONSession::Init(HttpConn* conn_, const ByteString& jsonCallback_)
{
	conn = conn_;
	jsonCallback = jsonCallback_;
}

void JSONSession::PrintStatus(const char* status, const char* type_)
{
	conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
	"Content-type: application/json" HTTP_CS_CRLF);
	if (jsonCallback.length)
	{
		conn->Write(jsonCallback.buffer, jsonCallback.length, false);
		conn->Print("(");
	}
	conn->Print("{\"status\":\"");
	conn->Print(status);
	if (type_)
	{
		conn->Print("\",\"type\":\"");
		conn->Print(type_);
	}
	conn->Print("\"}");

	if (jsonCallback.length)
		conn->Print(")");
	
	conn->Flush(true);
}

void JSONSession::PrintString(const char *s, unsigned len)
{
	conn->Write("\"", 1, false);
	for (unsigned i = 0; i < len; i++)
	{
		if (s[i] == '"')
			conn->Write("\\", 1, false);
		conn->Write(s + i, 1, false);
	}
	conn->Write("\"", 1, false);
}

void JSONSession::PrintNumber(double number)
{
	ByteArray<32>	text;
	
	text.length = snprintf(text.buffer, text.length, "%lf", number);
	conn->Write(text.buffer, text.length, false);
}

void JSONSession::PrintBool(bool b)
{
	if (b)
		conn->Write("true", 4, false);
	else
		conn->Write("false", 5, false);
}

void JSONSession::PrintNull()
{
	conn->Write("null", 4, false);
}

void JSONSession::PrintObjectStart(const char* /*name*/, unsigned /*len*/)
{
	// TODO
}

void JSONSession::PrintObjectEnd()
{
	// TODO
}
