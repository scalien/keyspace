#ifndef CURSORBULK_HPP
#define CURSORBULK_HPP

#include "System/Buffer.h"
#include <db_cxx.h>

class Table; // forward

class CursorBulk
{
friend class Table;

public:
	CursorBulk(unsigned bufferSize);

	bool	Start(const ByteString &key);
	bool	Delete();

	bool	Next(ByteString &key, ByteString &value);

	bool	Close();

private:
	Dbc*	cursor;
	Dbt		dummy, data;
	DbMultipleKeyDataIterator* iter;
};

#endif
