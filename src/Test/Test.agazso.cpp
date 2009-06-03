int KeyspaceClientTest();
int KeyspaceClientTest2(int argc, char** argv);
extern "C" int keyspace_client_test();

int
main(int argc, char** argv)
{
	int ret;
//	ret = KeyspaceClientTest();
	ret = KeyspaceClientTest2(argc, argv);
//	ret = keyspace_client_test();

	return ret;
}
