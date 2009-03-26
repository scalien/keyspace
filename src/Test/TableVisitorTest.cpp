#include "test.h"
#include "System/Containers/List.h"
#include "Framework/Database/Table.h"

class ListTableVisitor : public TableVisitor
{
public:	
	virtual bool Accept(const ByteString &key, const ByteString &value)
	{
		TEST_LOG("\"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
		return true;
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
	ByteArray<1024> what;
	ByteArray<1024> where;
	
	TableSelector(const char* what_, const char* where_)
	{
		what.Set(what_);
		where.Set(where_);
	}
	
	virtual bool Accept(const ByteString &key, const ByteString &value)
	{
		TEST_LOG("\"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
		
		if (strncmp(what.buffer, key.buffer, min(what.length, key.length)) == 0)
		{
			if (strncmp(where.buffer, value.buffer, min(where.length, value.length)) == 0)
			{
				TEST_LOG(" matched: \"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
			}
		}
		
		return true;
	}
	
	virtual const ByteString* GetKeyHint()
	{
		return &what;
	}
};

int TableSelectorTest()
{
	Table* table;
	// select all user where name starts with 'd'
	TableSelector selector("user", "d");
	
	table = database.GetTable("test");

	// populate table with test data
	table->Set(NULL, "user:1", "dopey");
	table->Set(NULL, "user:2", "grumpy");
	table->Set(NULL, "user:3", "doc");
	table->Set(NULL, "user:4", "happy");
	table->Set(NULL, "user:5", "bashful");
	table->Set(NULL, "user:6", "sneezy");
	table->Set(NULL, "user:7", "sleepy");
	
	table->Visit(selector);
	
	return TEST_SUCCESS;
	
}

/*
 * User 
 * {
 *		id: primary,
 *		nick: index,
 *		password,
 *		status,
 * }
 * 
 * Event 
 * {
 *		user_id foreign,
 *		name primary,
 *		date,
 *		note,
 * }
 *
 */

class QuerySelector : public TableVisitor
{
public:
	ByteArray<1024> result;
	ByteArray<1024> keyhint;
	ByteArray<1024> where;
	List<ByteString> keys;
	
	QuerySelector(const char* query)
	{
		where.Set("");
		ParseQuery(query);
	}
	
	const char* SkipWhitespace(const char* p)
	{
		while (p && *p && *p <= ' ') p++;
		
		return p;			
	}
	
	const char* SkipKey(const char* p, int *len)
	{
		bool isKey = true;
		*len = 0;
		
		while (p && *p)
		{
			if (isKey)
			{
				if (*p != ' ' && *p != ',')
				{
					(*len)++;
					p++;
				}
				else
					isKey = false;
			}
			else if (*p == ' ')
				p++;
			else
				break;
		}
			   
		return p;
	}
	
	bool ParseQuery(const char* query)
	{
		const char SELECT[] = "SELECT";
		const char FROM[] = "FROM";
		const char WHERE[] = "WHERE";
		const char* p;
		const char* start;
		int len;
		
		if (strncmp(query, SELECT, sizeof(SELECT) - 1))
			return false;
		
		p = query + sizeof(SELECT) - 1;
		p = SkipWhitespace(p);
		if (!p)
			return false;
		
		p = ParseQueryKeys(p);
		
		if (strncmp(p, FROM, sizeof(FROM) - 1))
			return false;
		p = p + sizeof(FROM) - 1;
		p = SkipWhitespace(p);
		start = p;
		p = SkipKey(start, &len);
		if (!len)
			return false;
		
		keyhint.Printf("%.*s", len, start);

		if (!strncmp(p, WHERE, sizeof(WHERE) - 1))
		{
			p = p + sizeof(WHERE) - 1;
			p = SkipWhitespace(p);
			p = ParseConditions(p);
		}
		
		return true;
	}
	
	const char* ParseQueryKeys(const char* p)
	{
		const char* keyStart;
		int keyLen;

		p = SkipWhitespace(p);
		
		keyStart = p;
		while (keyStart)
		{
			p = SkipKey(p, &keyLen);
			if (keyLen)
			{
				ByteString key;
				
				key.buffer = (char*) keyStart;
				key.length = keyLen;
				key.size = keyLen;
				
				keys.Add(key);
				
			}

			if (*p == ',')
			{
				p++;
				p = SkipWhitespace(p);
				keyStart = p;
			}
			else
			{
				p = SkipWhitespace(p);
				break;
			}
		}
		
		return p;
	}
	
	const char* ParseConditions(const char* p)
	{
		// TODO we need to know the schema here
		return p;
	}
	
	bool GetResult(const char* key, ByteString &value)
	{
		ByteString data;
		
		data.buffer = (char*) key;
		data.length = strlen(key);
		data.size = data.length;
		
		// TODO
		
		return false;
	}
	virtual const ByteString* GetKeyHint()
	{
		return &keyhint;
	}

	virtual bool Accept(const ByteString &key, const ByteString &value)
	{
		TEST_LOG("\"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
		
		if (strncmp(keyhint.buffer, key.buffer, min(keyhint.length, key.length)) == 0)
		{
			if (strncmp(where.buffer, value.buffer, min(where.length, value.length)) == 0)
			{
				TEST_LOG(" matched: \"%.*s\": \"%.*s\"", key.length, key.buffer, value.length, value.buffer);
				//return false;
			}
		}
		
		return true;
	}
	
};

int InsertUser(Table* table, const char* nick, const char* password, const char* status)
{
	int user_id;
	ByteArray<1024> key;
	ByteArray<1024> value;
	int nread;
	
	// this should be an atomic Incr operation
	table->Get(NULL, "user:id", value);
	user_id = strntol(value.buffer, value.length, &nread);
	user_id++;
	value.Printf("%d", user_id);
	table->Set(NULL, "user:id", value);
	
	key.Printf("user:%d", user_id);
	value.Printf("nick:%s:password:%s:status:%s", nick, password, status);
	table->Set(NULL, key, value);
	
	// nick is indexed, we have to make a reverse reference
	key.Printf("user:nick:%s", nick);
	value.Printf("%d", user_id);
	table->Set(NULL, key, value);

	return user_id;
}

void InsertEvent(Table* table, int user_id, const char* name, const char* note, const char* date)
{
	ByteArray<1024> key;
	ByteArray<1024> value;

	key.Printf("event:user:%d:%s", user_id, name);
	value.Printf("date:%s:note:%s", date, note);
	table->Set(NULL, key, value);
}

int StructuredDatabaseTest()
{
	Table* table;
	int user_id;
	DynArray<1024> what;
	DynArray<1024> where;

	table = new Table(&database, "structured_test");
	table->Truncate();
	user_id = InsertUser(table, "dopey", "dopeypass", "online");
	InsertEvent(table, user_id, "dinner", "cheezburger", "2009-03-24 01:09:00");
	InsertEvent(table, user_id, "meeting", "important", "2009-03-24 11:12:00");

	// SELECT * FROM event WHERE user=%d AND date > 2009-03-24 01
	what.Printf("event:user:%d", user_id);
	where.Printf("date:2009-03-24 01");
	//TableSelector selector(what.buffer, where.buffer);
//	QuerySelector selector("SELECT date, note FROM event WHERE user_id=%d", user_id);
//	table->Visit(selector);

	ListTableVisitor visitor;
	table->Visit(visitor);
	
	return TEST_SUCCESS;
}

TEST_MAIN(/*TableVisitorTest, TableSelectorTest*/ StructuredDatabaseTest);
