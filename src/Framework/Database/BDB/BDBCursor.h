#ifndef BDBCURSOR_HPP
#define BDBCURSOR_HPP

#include "System/Buffer.h"
#include <db_cxx.h>

class Table; // forward

class Cursor
{
friend class Table;

public:
	virtual bool	Next(ByteString &key, ByteString &value);
	virtual bool	Close();

private:
	Dbc*			cursor;
};

#endif
