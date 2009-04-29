int KeyspaceClientTest();
extern "C" int keyspace_client_test();

int
main(int, char**)
{
	int ret;
	ret = KeyspaceClientTest();
//	ret = keyspace_client_test();

	return ret;
}
