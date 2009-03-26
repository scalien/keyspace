#ifndef CURSOR_HPP
#define CURSOR_HPP

#include "System/Buffer.h"
#include <db_cxx.h>

class Table; // forward

class Cursor
{
friend class Table;

public:
	bool		Next(ByteString &key, ByteString &value);

private:
	Dbc*		cursor;
};

#endif
