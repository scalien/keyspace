int KeyspaceClientTest(int argc, char** argv);
extern "C" int keyspace_client_test();

#include <iostream>
#include "Application/Keyspace/Client/KeyspaceClient.h"
#include "System/Buffer.h"

int
main(int argc, char** argv)
{
	int ret;
	(void) argc;
	(void) argv;
//	ret = KeyspaceClientTestSuite();
//	ret = KeyspaceClientTest(argc, argv);
	ret = keyspace_client_test();

	return ret;
}
