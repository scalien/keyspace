#include <db_cxx.h>

#include "Table.h"
#include "Database.h"
#include "System/Buffer.h"
#include "Transaction.h"

Table::Table(Database* database, const char *name) :
database(database)
{
	DbTxn *txnid = NULL;
	const char *filename = name;
	const char *dbname = NULL;
	DBTYPE type = DB_BTREE;
	u_int32_t flags = DB_CREATE | DB_AUTO_COMMIT;
	int mode = 0;
	
	db = new Db(&database->env, 0);
	db->open(txnid, filename, dbname, type, flags, mode);
}

Table::~Table()
{
	db->close(0);
	delete db;
}

bool Table::Get(Transaction* transaction, const ByteString &key, ByteString &value)
{
	Dbt dbtKey(key.buffer, key.length);
	Dbt dbtValue;
	DbTxn* txn = NULL;
	int ret;
	
	if (transaction)
		txn = transaction->txn;
		
	ret = db->get(txn, &dbtKey, &dbtValue, 0);
	if (ret == DB_KEYEMPTY || ret == DB_NOTFOUND)
		return false;
	
	if (dbtValue.get_size() > value.size)
		return false;

	value.length = dbtValue.get_size();
	memcpy(value.buffer, dbtValue.get_data(), value.length);
	
	return true;
}

bool Table::Get(Transaction* transaction, char* key, ByteString &value)
{
	int len;
	
	len = strlen(key);
	
	ByteString bsKey(len, len, key);
	
	return Table::Get(transaction, bsKey, value);
}

bool Table::Put(Transaction* transaction, const ByteString &key, const ByteString &value)
{
	Dbt dbtKey(key.buffer, key.length);
	Dbt dbtValue(value.buffer, value.length);
	DbTxn* txn = NULL;
	int ret;

	if (transaction)
		txn = transaction->txn;
	
	ret = db->put(txn, &dbtKey, &dbtValue, 0);
	if (ret < 0)
		return false;
	
	return true;
}

bool Table::Put(Transaction* transaction, char* key, const ByteString &value)
{
	int len;
	
	len = strlen(key);
	
	ByteString bsKey(len, len, key);
	
	return Table::Put(transaction, bsKey, value);
}

bool Table::Put(Transaction* transaction, char* key, char* value)
{
	int len;
	
	len = strlen(key);
	ByteString bsKey(len, len, key);
	
	len = strlen(value);
	ByteString bsValue(len, len, value);
	
	return Table::Put(transaction, bsKey, bsValue);
}