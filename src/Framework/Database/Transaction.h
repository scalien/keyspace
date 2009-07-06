#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Database.h"
#include "Table.h"

class Transaction
{
	friend class Table;
public:

	virtual ~Transaction() {}

	virtual void	Set(Database* database) = 0;
	virtual void	Set(Table* table) = 0;
	virtual bool	IsActive() = 0;
	virtual bool	Begin() = 0;
	virtual bool	Commit() = 0;
	virtual bool	Rollback() = 0;
};

#endif
