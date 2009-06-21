int KeyspaceClientTestSuite();
int KeyspaceClientTest2(int argc, char** argv);
extern "C" int keyspace_client_test();

#include <iostream>
#include "Application/Keyspace/Client/KeyspaceClient.h"
#include "System/Buffer.h"

class Test
{
   private:
       Keyspace::Client client;
   public:
       Test();
       void foo(void);
};

Test::Test()
{
   const char *nodes[] = {"94.76.202.195:7080"};
   int status = client.Init(SIZE(nodes), nodes, 3000);
   if (status < 0) {
       std::cerr << "Baj" << std::endl;
   }
}

void
Test::foo(void)
{
   DynArray<128> key;
   DynArray<128> value;

   key.Writef("foo");
   value.Writef("bar");
   int status = client.Set(key, value);
   std::cout << status << std::endl;
}

int
main(int argc, char** argv)
{
	int ret;
	(void) argc;
	(void) argv;
//	ret = KeyspaceClientTestSuite();
//	ret = KeyspaceClientTest2(argc, argv);
	ret = keyspace_client_test();

//   Test t;
//   t.foo();


	return ret;
}
