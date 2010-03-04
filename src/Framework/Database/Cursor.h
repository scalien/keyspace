#ifndef CURSOR_HPP
#define CURSOR_HPP

#include "System/Buffer.h"
#include <db_cxx.h>

class Table; // forward

class Cursor
{
friend class Table;

public:
	bool	Start(const ByteString &key);
	bool	Delete();

	bool	Next(ByteString &key, ByteString &value);
	bool	Prev(ByteString &key, ByteString &value);

	bool	Close();

private:
	Dbc*	cursor;
};

#endif
