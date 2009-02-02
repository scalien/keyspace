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
	u_int32_t flags = DB_CREATE;
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
		false;
	
	if (dbtValue.get_size() > value.size)
		false;

	value.length = dbtValue.get_size();
	memcpy(value.buffer, dbtValue.get_data(), value.length);
	
	return true;
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
	Dbt dbtKey(key, strlen(key));
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

bool Table::Put(Transaction* transaction, char* key, char* value)
{
	Dbt dbtKey(key, strlen(key));
	Dbt dbtValue(value, strlen(value));
	DbTxn* txn = NULL;
	int ret;

	if (transaction)
		txn = transaction->txn;
	
	ret = db->put(txn, &dbtKey, &dbtValue, 0);
	if (ret < 0)
		return false;
	
	return true;
}