#include <db_cxx.h>

#include "Table.h"
#include "Database.h"
#include "System/Buffer.h"

Table::Table(Database &database, const char *name) :
database(database)
{
	DbTxn *txnid = NULL;
	const char *filename = name;
	const char *dbname = NULL;
	DBTYPE type = DB_BTREE;
	u_int32_t flags = DB_CREATE;
	int mode = 0;
	
	db = new Db(&database.env, 0);
	db->open(txnid, filename, dbname, type, flags, mode);
}

Table::~Table()
{
	db->close(0);
	delete db;
}

const ByteString * Table::Get(const ByteString &key, ByteString &value)
{
	Dbt dbtKey(key.buffer, key.length);
	Dbt dbtValue;
	int ret;
	
	ret = db->get(NULL, &dbtKey, &dbtValue, 0);
	if (ret == DB_KEYEMPTY || ret == DB_NOTFOUND)
		return NULL;
	
	if (dbtValue.get_size() > value.size)
		return NULL;

	value.length = dbtValue.get_size();
	memcpy(value.buffer, dbtValue.get_data(), value.length);
	
	return &value;
}

const ByteString * Table::Put(const ByteString &key, const ByteString &value)
{
	Dbt dbtKey(key.buffer, key.length);
	Dbt dbtValue(value.buffer, value.length);
	int ret;
	
	ret = db->put(NULL, &dbtKey, &dbtValue, 0);
	if (ret < 0)
		return NULL;
	
	return &value;
}
