#include "Transaction.h"

Transaction::Transaction(Database* database_)
{
	database = database_;
}

Transaction::Transaction(Table* table)
{
	database = table->database;
}

bool Transaction::Begin()
{
	if (database->env.txn_begin(NULL, &txn, DB_TXN_SYNC) != 0)
			return false;
	
	return true;
}

bool Transaction::Commit()
{
	if (txn->commit(0) != 0)
		return false;
	
	return true;
}

bool Transaction::Abort()
{
	if (txn->abort() != 0)
		return false;
	
	return true;
}
