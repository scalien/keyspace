#ifndef BDBTRANSACTION_H
#define BDBTRANSACTION_H

#include "BDBDatabase.h"
#include "BDBTable.h"

class Transaction
{
	friend class Table;
public:
	Transaction();

	virtual bool	IsActive();
	virtual bool	Begin();
	virtual bool	Commit();
	virtual bool	Rollback();
	
private:
	DbTxn*			txn;
	bool			active;
};

#endif
