#include "Cursor.h"

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
