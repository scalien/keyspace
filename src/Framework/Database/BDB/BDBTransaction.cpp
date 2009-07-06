#include "BDBTransaction.h"

Transaction::Transaction()
{
	txn = NULL;
	active = false;
}

bool Transaction::IsActive()
{
	return active;
}

bool Transaction::Begin()
{
	if (database.env.txn_begin(NULL, &txn, DB_TXN_SYNC) != 0)
			return false;
	
	active = true;
	return true;
}

bool Transaction::Commit()
{
	active = false;
	
	if (txn->commit(0) != 0)
		return false;
	
	return true;
}

bool Transaction::Rollback()
{
	active = false;
	
	if (txn->abort() != 0)
		return false;
	
	return true;
}
