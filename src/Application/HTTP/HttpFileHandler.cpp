#include "HttpFileHandler.h"
#include "HttpConsts.h"
#include "Mime.h"

#include <stdio.h>

typedef DynArray<128> HeaderArray;
void HttpHeaderAppend(HeaderArray& ha, const char* k, size_t klen, const char* v, size_t vlen)
{
	ha.Append(k, klen);
	ha.Append(": ", 2);
	ha.Append(v, vlen);
	ha.Append(HTTP_CS_CRLF, 2);
}

HttpFileHandler::HttpFileHandler(const char* docroot, const char* prefix_)
{
	documentRoot = docroot;
	prefix = prefix_;
}

bool HttpFileHandler::HandleRequest(HttpConn* conn, const HttpRequest& request)
{
	DynArray<128>	path;
	DynArray<128>	tmp;
	DynArray<128>	ha;
	char			buf[128 * 1024];
	FILE*			fp;
	size_t			nread;
	size_t			fsize;
	const char*		mimeType;
	
	if (strncmp(request.line.uri, prefix, strlen(prefix)))
		return false;
	
	path.Writef("%s%s", documentRoot, request.line.uri);
	path.Append("", 1);
	
	mimeType = MimeTypeFromExtension(strrchr(path.buffer, '.'));
	
	fp = fopen(path.buffer, "rb");
	if (!fp)
		return false;
	
	fseek(fp, 0L, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	
	HttpHeaderAppend(ha, 
		HTTP_HEADER_CONTENT_TYPE, sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,
		mimeType, strlen(mimeType));
	
	tmp.Writef("%u", fsize);
	HttpHeaderAppend(ha,
		HTTP_HEADER_CONTENT_LENGTH, sizeof(HTTP_HEADER_CONTENT_LENGTH) - 1,
		tmp.buffer, tmp.length);

	// zero-terminate
	ha.Append("", 1);
	
	conn->ResponseHeader(HTTP_STATUS_CODE_OK, ha.buffer);
	
	while (true)
	{
		nread = fread(buf, 1, sizeof(buf), fp);
		if (feof(fp))
		{
			fclose(fp);
			conn->Write(buf, nread, true);
			break;
		}
		if (ferror(fp))
		{
			fclose(fp);
			break;
		}
		
		conn->Write(buf, nread);
	}

	conn->Flush();
	return true;
}

