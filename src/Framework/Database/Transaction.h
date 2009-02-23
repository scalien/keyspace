#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Database.h"
#include "Table.h"

class Transaction
{
	friend class Table;
public:
	Transaction() {}
	Transaction(Database* database);
	Transaction(Table* table);
	
	void Set(Database* database);	
	void Set(Table* table);
	
	bool Begin();
	bool Commit();
	bool Abort();
	
private:
	Database*	database;
	DbTxn*		txn;
};

#endif