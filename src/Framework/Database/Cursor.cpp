#include "Cursor.h"

bool Cursor::Start(ByteString &key)
{
	Dbt dbkey, dbvalue;
	int	flags;

	dbkey.set_data(key.buffer);
	dbkey.set_size(key.length);
	flags = DB_SET_RANGE;
	
	if (cursor->get(&dbkey, &dbvalue, flags) == 0)
	{
		if (key.Set((char*)dbkey.get_data(), dbkey.get_size()))
				return true;
		else
			return false;
	}
	return false;
}

bool Cursor::Delete()
{
	return (cursor->del(0) == 0);
}

bool Cursor::Next(ByteString &key, ByteString &value)
{
	Dbt dbkey, dbvalue;
	
	if (cursor->get(&dbkey, &dbvalue, DB_NEXT) == 0)
	{
		if (key.Set((char*)dbkey.get_data(), dbkey.get_size()) &&
			value.Set((char*)dbvalue.get_data(), dbvalue.get_size()))
				return true;
		else
			return false;
	}
	else
		return false;
}

bool Cursor::Prev(ByteString &key, ByteString &value)
{
	Dbt dbkey, dbvalue;
	
	if (cursor->get(&dbkey, &dbvalue, DB_PREV) == 0)
	{
		if (key.Set((char*)dbkey.get_data(), dbkey.get_size()) &&
			value.Set((char*)dbvalue.get_data(), dbvalue.get_size()))
				return true;
		else
			return false;
	}
	else
		return false;
}

bool Cursor::Close()
{
	return (cursor->close() == 0);
}
