#include "test.h"
#include "Framework/Database/Table.h"

class ListTableVisitor : public TableVisitor
{
public:	
	virtual void Accept(const ByteString &key, const ByteString &value)
	{
		TEST_LOG("\"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
	}
};

int TableVisitorTest()
{
	Table* table;
	ListTableVisitor ltv;
	
	table = database.GetTable("test");
	table->Visit(ltv);
	
	return TEST_SUCCESS;
}

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

class TableSelector : public TableVisitor
{
public:
	const char* what;
	const char* where;
	
	TableSelector(const char* what_, const char* where_) : what(what_), where(where_) {}
	
	virtual void Accept(const ByteString &key, const ByteString &value)
	{
		int whatlen = strlen(what);
		int wherelen = strlen(where);
		
		if (strncmp(what, key.buffer, min(whatlen, key.length)) == 0)
		{
			if (strncmp(where, value.buffer, min(wherelen, value.length)) == 0)
			{
				TEST_LOG("\"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
			}
		}
	}
};

int TableSelectorTest()
{
	Table* table;
	// select all user where name starts with 'd'
	TableSelector selector("user", "d");
	
	table = database.GetTable("test");

	// populate table with test data
	table->Put(NULL, "user:1", "dopey");
	table->Put(NULL, "user:2", "grumpy");
	table->Put(NULL, "user:3", "doc");
	table->Put(NULL, "user:4", "happy");
	table->Put(NULL, "user:5", "bashful");
	table->Put(NULL, "user:6", "sneezy");
	table->Put(NULL, "user:7", "sleepy");
	
	table->Visit(selector);
	
	return TEST_SUCCESS;
	
}

TEST_MAIN(TableVisitorTest, TableSelectorTest);
