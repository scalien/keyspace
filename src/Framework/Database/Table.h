#ifndef TABLE_H
#define TABLE_H

class Db;
class Database;
class ByteString;

class Table
{
public:
	Table(Database &database, const char *name);
	~Table();
	
	const ByteString * Get(const ByteString &key, ByteString &value);
	const ByteString * Put(const ByteString &key, const ByteString &value);
private:
	Database &database;
	Db *db;
};


#endif
