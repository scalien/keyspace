#include <db_cxx.h>

#include "Table.h"
#include "Database.h"
#include "System/Buffer.h"
#include "Transaction.h"

Table::Table(Database* database, const char *name, int pageSize) :
database(database)
{
	DbTxn *txnid = NULL;
	const char *filename = name;
	const char *dbname = NULL;
	DBTYPE type = DB_BTREE;
	u_int32_t flags = DB_CREATE | DB_AUTO_COMMIT | DB_NOMMAP;
	int mode = 0;
	
	db = new Db(&database->env, 0);
	if (pageSize != 0)
		db->set_pagesize(pageSize);
		
	Log_Trace();
	if (db->open(txnid, filename, dbname, type, flags, mode) != 0)
	{
		db->close(0);
		STOP_FAIL("could not open database", 1);
	}
	Log_Trace();
}

Table::~Table()
{
	db->close(0);
	delete db;
}

bool Table::Iterate(Transaction* transaction, Cursor& cursor)
{
	DbTxn* txn = NULL;
	
	if (transaction)
		txn = transaction->txn;
	
	if (db->cursor(txn, &cursor.cursor, 0) == 0)
		return true;
	else
		return false;
}

bool Table::Get(Transaction* transaction, const ByteString &key, ByteString &value)
{
	Dbt dbtKey;
	Dbt dbtValue;
	DbTxn* txn = NULL;
	int ret;
	
	if (transaction)
		txn = transaction->txn;

	dbtKey.set_flags(DB_DBT_USERMEM);
	dbtKey.set_data(key.buffer);
	dbtKey.set_ulen(key.length);
	dbtKey.set_size(key.length);
	
	dbtValue.set_flags(DB_DBT_USERMEM);
	dbtValue.set_data(value.buffer);
	dbtValue.set_ulen(value.size);
	
	ret = db->get(txn, &dbtKey, &dbtValue, 0);
	if (ret == DB_KEYEMPTY || ret == DB_NOTFOUND)
		return false;
	
	if (dbtValue.get_size() > (size_t) value.size)
		return false;

	value.length = dbtValue.get_size();
	
	return true;
}

bool Table::Get(Transaction* transaction, const char* key, ByteString &value)
{
	int len;
	
	len = strlen(key);
	
	ByteString bsKey(len, len, (char*) key);
	
	return Table::Get(transaction, bsKey, value);
}

bool Table::Get(Transaction* transaction, const char* key, uint64_t& value)
{
	ByteArray<32> ba;
	unsigned nread;
	
	if (!Get(transaction, key, ba))
		return false;
	
	value = strntouint64(ba.buffer, ba.length, &nread);
	
	if (nread != ba.length)
		return false;
	
	return true;
}

bool Table::Set(Transaction* transaction, const ByteString &key, const ByteString &value)
{
	Dbt dbtKey(key.buffer, key.length);
	Dbt dbtValue(value.buffer, value.length);
	DbTxn* txn = NULL;
	int ret;

	if (transaction)
		txn = transaction->txn;
	
	ret = db->put(txn, &dbtKey, &dbtValue, 0);
	if (ret != 0)
		return false;
	
	return true;
}

bool Table::Set(Transaction* transaction, const char* key, const ByteString &value)
{
	int len;
	
	len = strlen(key);
	
	ByteString bsKey(len, len, (char*) key);
	
	return Table::Set(transaction, bsKey, value);
}

bool Table::Set(Transaction* transaction, const char* key, const char* value)
{
	int len;
	
	len = strlen(key);
	ByteString bsKey(len, len, (char*) key);
	
	len = strlen(value);
	ByteString bsValue(len, len, (char*) value);
	
	return Table::Set(transaction, bsKey, bsValue);
}

bool Table::Delete(Transaction* transaction, const ByteString &key)
{
	Dbt dbtKey(key.buffer, key.length);
	DbTxn* txn = NULL;
	int ret;

	if (transaction)
		txn = transaction->txn;
	
	ret = db->del(txn, &dbtKey, 0);
	if (ret < 0)
		return false;
	
	return true;
}

bool Table::Prune(Transaction* transaction, const ByteString &prefix)
{
	Dbc* cursor = NULL;
	u_int32_t flags = DB_NEXT;

	DbTxn* txn;
	
	if (transaction == NULL)
		txn = NULL;
	else
		txn = transaction->txn;

	if (db->cursor(txn, &cursor, 0) != 0)
		return false;
	
	Dbt key, value;
	if (prefix.length > 0)
	{
		key.set_data(prefix.buffer);
		key.set_size(prefix.length);
		flags = DB_SET_RANGE;		
	}
	
	while (cursor->get(&key, &value, flags) == 0)
	{
		if (key.get_size() < prefix.length)
			break;
		
		if (strncmp(prefix.buffer, (char*)key.get_data(), prefix.length) != 0)
			break;
		
		// don't delete keys starting with @@
		if (!(key.get_size() >= 2 &&
		 ((char*)key.get_data())[0] == '@' &&
		 ((char*)key.get_data())[1] == '@'))
			cursor->del(0);

		flags = DB_NEXT;
	}
	
	if (cursor)
		cursor->close();
	
	return true;
}

/*bool Table::Truncate(Transaction* transaction)
{
	u_int32_t count;
	u_int32_t flags = 0;
	int ret;
	DbTxn* txn;
	
	txn = transaction ? transaction->txn : NULL;
	// TODO error handling
	Log_Trace();
	if ((ret = db->truncate(txn, &count, flags)) != 0)
	{
		Log_Trace("truncate() failed");
	}
	Log_Trace();
	return true;
}*/

bool Table::Visit(TableVisitor &tv)
{
	Dbc* cursor = NULL;
	bool ret = true;
	u_int32_t flags = DB_NEXT;

	// TODO call tv.OnComplete() or error handling
	if (db->cursor(NULL, &cursor, 0) != 0)
		return false;
	
	Dbt key, value;
	if (tv.GetStartKey() && tv.GetStartKey()->length > 0)
	{
		key.set_data(tv.GetStartKey()->buffer);
		key.set_size(tv.GetStartKey()->length);
		flags = DB_SET_RANGE;		
	}
	
	while (cursor->get(&key, &value, flags) == 0)
	{
		ByteString bsKey(key.get_size(), key.get_size(), (char*) key.get_data());
		ByteString bsValue(value.get_size(), value.get_size(), (char*) value.get_data());

		ret = tv.Accept(bsKey, bsValue);
		if (!ret)
			break;

		flags = DB_NEXT;
	}
	
	cursor->close();	
	tv.OnComplete();
	
	return ret;
}
