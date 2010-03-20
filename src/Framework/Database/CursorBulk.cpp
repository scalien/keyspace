#include "CursorBulk.h"

CursorBulk::CursorBulk(unsigned bufferSize)
{
	data.set_data(Alloc(bufferSize));
	data.set_ulen(bufferSize);
	data.set_flags(DB_DBT_USERMEM);

	iter = NULL;
}

bool CursorBulk::Start(const ByteString &startKey)
{
	Dbt dbkey, dbvalue;
	int	flags;

	dbkey.set_data(startKey.buffer);
	dbkey.set_size(startKey.length);
	flags = DB_SET_RANGE;
	
	if (cursor->get(&dbkey, &dbvalue, flags) == 0)
		return true;
	
	return false;
}

bool CursorBulk::Delete()
{
	return (cursor->del(0) == 0);
}

bool CursorBulk::Next(ByteString &key, ByteString &value)
{
	Dbt dbkey, dbvalue;

	if (iter)
		goto inside;

outside:
	if (cursor->get(&dummy, &data, DB_MULTIPLE_KEY | DB_NEXT) == 0)
	{
		assert(iter == NULL);
		iter = new DbMultipleKeyDataIterator(data);
		goto inside;
	}
	else
		return false;

inside:
	assert(iter);
	if (iter->next(dbkey, dbvalue))
	{
		if (key.Set((char*)dbkey.get_data(), dbkey.get_size()) &&
			value.Set((char*)dbvalue.get_data(), dbvalue.get_size()))
				return true;
		else
			return false;
	}
	else
	{
		delete iter;
		iter = NULL;
		goto outside;
	}
}

bool CursorBulk::Close()
{
	if (iter)
		delete iter;
	free(data.get_data());
	return (cursor->close() == 0);
}
