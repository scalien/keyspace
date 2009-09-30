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
		if (IsFolder(filename))
		{
			STOP_FAIL(rprintf(
					  "Could not create database file '%s' "
					  "because a folder '%s' exists",
					  filename, filename), 1);
		}
		STOP_FAIL("Could not open database", 1);
	}
	Log_Trace();
	db->set_flags(0);
}

Table::~Table()
{
	db->close(0);
	delete db;
}

bool Table::Iterate(Transaction* tx, Cursor& cursor)
{
	DbTxn* txn = NULL;
	
	if (tx)
		txn = tx->txn;
	
	if (db->cursor(txn, &cursor.cursor, 0) == 0)
		return true;
	else
		return false;
}

bool Table::Get(Transaction* tx,
				const ByteString &key,
				ByteString &value)
{
	Dbt dbtKey;
	Dbt dbtValue;
	DbTxn* txn = NULL;
	int ret;
	
	if (tx)
		txn = tx->txn;

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

bool Table::Get(Transaction* tx, const char* key, ByteString &value)
{
	int len;
	
	len = strlen(key);
	
	ByteString bsKey(len, len, (char*) key);
	
	return Table::Get(tx, bsKey, value);
}

bool Table::Get(Transaction* tx, const char* key, uint64_t& value)
{
	ByteArray<32> ba;
	unsigned nread;
	
	if (!Get(tx, key, ba))
		return false;
	
	value = strntouint64(ba.buffer, ba.length, &nread);
	
	if (nread != ba.length)
		return false;
	
	return true;
}

bool Table::Set(Transaction* tx,
				const ByteString &key,
				const ByteString &value)
{
	Dbt dbtKey(key.buffer, key.length);
	Dbt dbtValue(value.buffer, value.length);
	DbTxn* txn = NULL;
	int ret;

	if (tx)
		txn = tx->txn;
	
	ret = db->put(txn, &dbtKey, &dbtValue, 0);
	if (ret != 0)
		return false;
	
	return true;
}

bool Table::Set(Transaction* tx,
				const char* key,
				const ByteString &value)
{
	int len;
	
	len = strlen(key);
	
	ByteString bsKey(len, len, (char*) key);
	
	return Table::Set(tx, bsKey, value);
}

bool Table::Set(Transaction* tx,
				const char* key,
				const char* value)
{
	int len;
	
	len = strlen(key);
	ByteString bsKey(len, len, (char*) key);
	
	len = strlen(value);
	ByteString bsValue(len, len, (char*) value);
	
	return Table::Set(tx, bsKey, bsValue);
}

bool Table::Delete(Transaction* tx, const ByteString &key)
{
	Dbt dbtKey(key.buffer, key.length);
	DbTxn* txn = NULL;
	int ret;

	if (tx)
		txn = tx->txn;
	
	ret = db->del(txn, &dbtKey, 0);
	if (ret < 0)
		return false;
	
	return true;
}

bool Table::Prune(Transaction* tx, const ByteString &prefix)
{
	Dbc* cursor = NULL;
	u_int32_t flags = DB_NEXT;

	DbTxn* txn;
	
	if (tx == NULL)
		txn = NULL;
	else
		txn = tx->txn;

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

bool Table::Truncate(Transaction* tx)
{
	Log_Trace();

	u_int32_t count;
	u_int32_t flags = 0;
	int ret;
	DbTxn* txn;
	
	txn = tx ? tx->txn : NULL;
	// TODO error handling
	if ((ret = db->truncate(txn, &count, flags)) != 0)
		Log_Trace("truncate() failed");
	return true;
}

bool Table::Visit(TableVisitor &tv)
{
	if (!tv.IsForward())
		return VisitBackward(tv);

	ByteString bsKey, bsValue;
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
		bsKey = ByteString(key.get_size(),
						   key.get_size(),
						   (char*) key.get_data());
		
		bsValue = ByteString(value.get_size(),
							 value.get_size(),
							 (char*) value.get_data());

		Log_Trace("Listing: %.*s %.*s", tv.GetStartKey()->length,
								        tv.GetStartKey()->buffer,
										bsKey.length, bsKey.buffer);

		ret = tv.Accept(bsKey, bsValue);
		if (!ret)
			break;

		flags = DB_NEXT;
	}
	
	cursor->close();	
	tv.OnComplete();
	
	return ret;
}

bool Table::VisitBackward(TableVisitor &tv)
{
	ByteString bsKey, bsValue;
	Dbc* cursor = NULL;
	bool ret = true;
	u_int32_t flags = DB_PREV;

	// TODO call tv.OnComplete() or error handling
	if (db->cursor(NULL, &cursor, 0) != 0)
		return false;
	
	Dbt key, value;
	if (tv.GetStartKey() && tv.GetStartKey()->length > 0)
	{
		key.set_data(tv.GetStartKey()->buffer);
		key.set_size(tv.GetStartKey()->length);
		flags = DB_SET_RANGE;		

		// as DB_SET_RANGE finds the smallest key greater than or equal to the
		// specified key, in order to visit the database backwards, move to the
		// first matching elem, then move backwards
		if (cursor->get(&key, &value, flags) != 0)
		{
			// end of database
			cursor->close();
			if (db->cursor(NULL, &cursor, 0) != 0)
				return false;
		}
		else
		{
			// if there is a match, call the acceptor, otherwise move to the
			// previous elem in the database
			if (memcmp(tv.GetStartKey()->buffer, key.get_data(),
				min(tv.GetStartKey()->length, key.get_size())) == 0)
			{
				bsKey = ByteString(key.get_size(),
								   key.get_size(),
								   (char*) key.get_data());
				
				bsValue = ByteString(value.get_size(),
									 value.get_size(),
									 (char*) value.get_data());

				ret = tv.Accept(bsKey, bsValue);
			}
		}
	}
		
	flags = DB_PREV;
	while (ret && cursor->get(&key, &value, flags) == 0)
	{
		bsKey = ByteString(key.get_size(),
						   key.get_size(),
						   (char*) key.get_data());
		
		bsValue = ByteString(value.get_size(),
							 value.get_size(),
							 (char*) value.get_data());

		ret = tv.Accept(bsKey, bsValue);
		if (!ret)
			break;

		flags = DB_PREV;
	}
	
	cursor->close();	
	tv.OnComplete();
	
	return ret;

}
