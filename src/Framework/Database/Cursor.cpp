#include "Cursor.h"

bool Cursor::Start(ByteString &key)
{
	Dbt dbkey, dbvalue;

	dbkey.set_data(key.buffer);
	dbkey.set_size(key.length);
	
	if (cursor->get(&dbkey, &dbvalue, DB_SET | DB_READ_UNCOMMITTED) == 0)
	{
		if (key.Set((char*)dbkey.get_data(), dbkey.get_size()))
				return true;
		else
			return false;
	}
	return false;
}

bool Cursor::Start(ByteString &key, ByteString &value)
{
	Dbt dbkey, dbvalue;

	dbkey.set_data(key.buffer);
	dbkey.set_size(key.length);
	
	if (cursor->get(&dbkey, &dbvalue, DB_SET_RANGE | DB_READ_UNCOMMITTED) == 0)
	{
		if (key.Set((char*)dbkey.get_data(), dbkey.get_size()) &&
			value.Set((char*)dbvalue.get_data(), dbvalue.get_size()))
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
	
	if (cursor->get(&dbkey, &dbvalue, DB_NEXT | DB_READ_UNCOMMITTED) == 0)
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
	
	if (cursor->get(&dbkey, &dbvalue, DB_PREV | DB_READ_UNCOMMITTED) == 0)
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
