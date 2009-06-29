//===================================================================
// 
// Changing binary name in Xcode:
//
// BDBTool target - Get info - Build - Packaging - Product name
//
// Valid names are: bdbdump, bdbrestore, bdblist
//
//===================================================================
#include <string.h>
#include <unistd.h>
#include "Framework/Database/Database.h"
#include "Framework/Database/Table.h"
#include "Application/Keyspace/Database/KeyspaceMsg.h"

class TablePrinter : public TableVisitor
{
public:
	KeyspaceMsg		msg;
	DynArray<1024>	out;
	bool			dumpFormat;
	
	TablePrinter(bool dumpFormat_)
	{
		dumpFormat = dumpFormat_;
	}

	virtual bool Accept(const ByteString &key, const ByteString &value)
	{
		if (!dumpFormat)
			printf("%.*s %.*s\n", key.length, key.buffer, value.length, value.buffer);
		else
		{
			msg.Set(key, value);
			while (!msg.Write(out))
				out.Reallocate(out.size * 2, false);

			printf("%.*s\n", out.length, out.buffer);
		}

		return true;
	}
};

char* filepart(char* path)
{
	char* slash;
	
	slash = strrchr(path, '/');
	if (slash)
		return slash + 1;
	
	return NULL;
}

int bdbdump(int argc, char* argv[], bool dumpFormat)
{
	char*			slash;
	char*			filename;
	Table*			table;
	TablePrinter	tp(dumpFormat);
	bool			ret;
	DatabaseConfig	dbConfig;

	if (argc < 2)
	{
		printf("usage: bdbdump db-file\n");
		exit(1);
	}
	
	dbConfig.dir = ".";
	filename = argv[1];
	if ((slash = strrchr(filename, '/')) != NULL)
	{
		dbConfig.dir = argv[1];
		*slash = '\0';
		filename = slash + 1;
		if (chdir(dbConfig.dir) < 0)
		{
			printf("cannot find database directory!\n");
			exit(1);
		}
		dirname = ".";
	}
	
	ret = database.Init(dbConfig);
	if (!ret)
	{
		printf("cannot initialise database!\n");
		exit(1);
	}
	
	table = new Table(&database, filename);
	table->Visit(tp);
	
	delete table;
	
	return 0;
}

int bdbrestore(int argc, char* argv[])
{
	return 0;
}

int main(int argc, char* argv[])
{
	char*	filename;

	filename = filepart(argv[0]);

	if (strcmp(filename, "bdbdump") == 0)
		return bdbdump(argc, argv, true);
	else if (strcmp(filename, "bdblist") == 0)
		return bdbdump(argc, argv, false);
	else if (strcmp(filename, "bdbrestore") == 0)
		return bdbrestore(argc, argv);
	
	return 1;
}
