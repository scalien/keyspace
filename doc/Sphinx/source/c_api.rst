.. _c_api:


*****
C API
*****

Installing from source on Linux and other UNIX platforms
========================================================

In order to use the Keyspace C libraries, you need to first build it. The standard ``make`` command also builds the client libraries, or use::

  $ make clientlibs

in the Keyspace folder to explicitly build it. This will place the static library ``libkeyspaceclient.a`` and the dynamic library ``libkeyspaceclient.so`` (``.dylib`` under Darwin) in the ``bin`` folder. The header file ``keyspace_client.h`` is located in ``src/Application/Keyspace/Client``.

Installing from source on Windows
=================================

In order to use the Keyspace C libraries, you need to first build it. We have supplied a Visual C++ 2008 Express Edition project file located in the ``Keyspace.vcproj`` directory. Open the solution file ``Keyspace.sln`` and build the KeyspaceClientLib project. This will generate the static library ``keyspace_client{d}.lib`` in the ``bin`` folder. The header file ``keyspace_client.h`` is located in ``src/Application/Keyspace/Client``.

Connecting to the Keyspace cluster
==================================

First create a ``keyspace_client_t object``::

  #include <stdio.h>
  #include "keyspace_client.h"

  int main()
  {
    keyspace_client_t client = keyspace_client_create();
    if (client == NULL)
    {
      fprintf(stderr, "Failed to create Keyspace client");
      return 1;
    }
    ...
    keyspace_client_destroy(client);
  }

Next, connect to a Keyspace cluster::

  const char* nodes = { "192.168.1.50:7080",
                        "192.168.1.51:7080",
                        "192.168.1.52:7080" };
  int status = keyspace_client_init(client, 3, nodes);
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "Failed to initialize Keyspace client");
    keyspace_client_destroy(client);
    return 1;
  }

Setting timeout values
======================

Next, if you would like to, change the global timeout. The global timeout specifies the maximum time, in msec, that a Keyspace client call will block your application. The default is 120 seconds::

  keyspace_client_set_global_timeout(client, 120*1000);

Next, if you would like to, change the master timeout. The master timeout specifies the maximum time, in msec, that the library will spend trying to find the master node. The default is 21 seconds::

  keyspace_client_set_master_timeout(client, 21*1000);

At this point, you are ready to start issuing commands.

Issuing single write commands
=============================

The Keyspace write commands are: ``set``, ``test_and_set``, ``rename``, ``add``, ``delete``, ``remove`` and ``prune``. Note that all Keyspace commands take and return pointers and a length parameter which specifies the length in bytes; the client libraries **do not assume NULL-terminated strings**.

``set`` command
---------------

The ``set`` command sets a ``key => value`` pair, creating a new pair if ``key`` did not previously exist, overwriting the old value if it did::

  int status = keyspace_client_set(client, "key", strlen("key"),
                                           "value", strlen("value"));
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "set failed");
    ...
  }

``test_and_set`` command
------------------------

The ``test_and_set`` command conditionally and atomically sets a ``key => value`` pair, but only if the current ``value`` matches the user specified value ``test``::

  int status = keyspace_client_test_and_set(client, "key", strlen("key"),
                                                    "test", strlen("test"),
                                                    "value" strlen("value"));
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "test_and_set failed");
    ...
  }

``rename`` command
------------------

The ``rename`` command atomically renames a ``key``, leaving its ``value`` alone::

  int status = keyspace_client_rename(client, "from", strlen("from"),
                                               "to", strlen("to"));
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "rename failed");
    ...
  }

If the database looked like ``from => value`` at the beginning, then it changed to ``to => value`` after the successfull ``rename`` operation.

``add`` command
---------------

The ``add`` command takes the value of the key, parses it as a number and atomically increments it by the given offset::

  int64_t result;
  int status = keyspace_client_add(client, "key", strlen("key"), 3, &result);
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "add failed");
    ...
  }

If the database looked like ``key => 10`` at the beginning, then it changed to ``key => 13`` after the successfull ``add`` operation and the variable ``result`` holds the value 13.

``delete`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key``::

  int status = keyspace_client_delete(client, "key", strlen("key"));
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "delete failed");
    ...
  }

``remove`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key`` and returns the old ``value``::

  int status = keyspace_client_remove(client, "key", strlen("key"));
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "remove failed");
    ...
  }

  // now get the old value
  keyspace_result_t result = keyspace_client_result(client);
  if (result == KEYSPACE_INVALID_RESULT)
  {
    fprintf(stderr, "remove failed");
    ...
  }
  keyspace_result_begin(result);
  char* val;
  unsigned len;
  if (keyspace_result_value(result, (const void**) &val, &len)
      != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "remove failed");
    ...
  }
  // val and len now hold the old value
  ...
  keyspace_result_close(result);

``prune`` command
-----------------

The ``prune`` command deletes all ``key => value`` pairs where the ``key`` starts with the given prefix::

  int status = keyspace_client_prune(client, "prefix", strlen("prefix"));
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "prune failed");
    ...
  }

Issuing single read commands
============================

The only Keyspace single read command is ``get_simple``.

``get_simple`` command
----------------------

The ``get_simple`` command retrieves a single value from the Keyspace cluster. Unlike all other operations, it only works if the returned value is NULL-terminated or its length is otherwise known. The last parameter specifies whether the command is dirty (``0`` for safe, ``1`` for dirty)::

  char buf[1024];
  int vallen = keyspace_client_get_simple(client, "key", strlen("key"),
                                          buf, 1024, 0); // safe
  if (vallen < 0)
  {
    fprintf(stderr, "get_simple failed");
    ...
  }
  // buf now holds the value of length vallen

Issuing list commands
=====================

There are two list commands: ``list_keys`` and ``list_keyvalues`` and one ``count`` command, all have the same set of parameters.

``list_keys`` command
---------------------

The ``list_keys`` command retrieves all keys from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``start_key``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key
- the last parameter specifies whether the command is dirty (``0`` for safe, ``1`` for dirty)

The signature of the function is::

  int keyspace_client_list_keys(keyspace_client_t kc, 
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int backward,
		int skip,
		int dirty);

Since the ``list_keys`` command may return many keys, the result object must be fetched and iterated after the command completed, as shown in the following sample code::

  int status = keyspace_client_list_keys(client, "prefix", strlen("prefix"),
			"", 0,  // start_key
			100,    // count
			0,      // forward list
			0,      // don't skip the first key
			0);     // safe
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "list_keys failed");
    ...
  }
  
  // fetch result
  keyspace_result_t result = keyspace_client_result(client);
  if (result == KEYSPACE_INVALID_RESULT)
  {
    fprintf(stderr, "list_keys failed");
    ...
  }
  for (keyspace_result_begin(result);
       !keyspace_result_is_end(result);
       keyspace_result_next(result))
  {
    char* key;
    unsigned keylen;
    if (keyspace_result_value(result, (const void**) &key,
        &keylen) != KEYSPACE_SUCCESS)
    {
      fprintf(stderr, "list_keys failed");
	  ...
	}
    // key and keylen now hold a key
    ...
  }
  keyspace_result_close(result);

``list_keyvalues`` command
--------------------------

The ``list_keyvalues`` command in nearly identical to ``list_keys``, except it also returns the values. Hence in the result iteration, ``keyspace_result_key`` and ``keyspace_result_value`` may be called.

The ``list_keyvalues`` command retrieves all keys and values from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``start_key``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key
- the last parameter specifies whether the command is dirty (``0`` for safe, ``1`` for dirty)

The signature of the function is::

  int keyspace_client_list_keyvalues(keyspace_client_t kc, 
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int backward,
		int skip,
		int dirty);

Since the ``list_keyvalues`` command may return many key-value pairs, the result object must be fetched and iterated after the command completed, as shown in the following sample code::

  int status = keyspace_client_list_keyvalues(client, "prefix", strlen("prefix"),
			"", 0,  // start_key
			100,    // count
			0,      // forward list
			0,      // don't skip the first key
			0);     // safe
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "list_keyvalues failed");
    ...
  }
  
  // fetch result
  keyspace_result_t result = keyspace_client_result(client);
  if (result == KEYSPACE_INVALID_RESULT)
  {
    fprintf(stderr, "list_keyvalues failed");
    ...
  }
  for (keyspace_result_begin(result);
       !keyspace_result_is_end(result);
       keyspace_result_next(result))
  {
    char* key;
	char* val;
    unsigned keylen, vallen;
    if (keyspace_result_key(result, (const void**) &key,
                            &keylen) != KEYSPACE_SUCCESS ||
        keyspace_result_value(result, (const void**) &val,
                              &vallen) != KEYSPACE_SUCCESS)
    {
      fprintf(stderr, "list_keyvalues failed");
	  ...
	}
    // key, keylen and val, vallen now hold a key-value pair
    ...
  }
  keyspace_result_close(result);

``count`` command
-----------------

The ``count`` command has the same parameters as ``list_keys`` or ``list_keyvalues``, but returns the number of keys (or key-value pairs) that they would return. The signature of the function is::

  int keyspace_client_count(keyspace_client_t kc, 
		uint64_t *res,
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int backward,
		int skip,
		int dirty);

The second ``res`` parameter will hold the number of rows::

  uint64_t num;
  int status = keyspace_client_count(client, "prefix", strlen("prefix"),
			&num,
			"", 0,  // start_key
			100,    // count
			0,      // forward list
			0,      // don't skip the first key
			0);     // safe
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "list_keyvalues failed");
    ...
  }
  // num holds the number of keys

Issuing batched write commands
==============================

For maximum thruput performance, it is possible to issue many write commands together; this is called batched writing. It will be faster then issuing single write commands because

#. The Keyspace cluster will replicate them together
#. The client library will not wait for the previous' write commands response before send the next write command (saves rount-trip times).

In practice batched ``set`` can achieve 5-10x higher throughput than single ``set``.

To send batched write commands, first call ``keyspace_client_begin()`` function, then issue the write commands, and finally call ``keyspace_client_submit()``. The commands are sent on ``keyspace_client_submit()``. After the commands complete, the result object must be fetched and iterated to retrieve the individual return values::

  int status = keyspace_client_begin(client);
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "begin failed");
    ...
  }

  // perform write commands such as set, test_and_set, etc. here

  status = keyspace_client_submit(client);
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "submit failed");
    
    // see which command error'd
    // fetch result
    keyspace_result_t result = keyspace_client_result(client);
    if (result == KEYSPACE_INVALID_RESULT)
    {
      fprintf(stderr, "result failed");
      ...
    }
    for (keyspace_result_begin(result);
        !keyspace_result_is_end(result);
        keyspace_result_next(result))
    {
      status = keyspace_result_command_status(result);
      // status now holds the status of the ith command
      ...
    }
    keyspace_result_close(result);
  }

Issuing batched read commands
=============================

It is possible to issue ``get`` read commands in a batched fashion. Since ``get`` commands are not replicated, only the round-trip time is saved. Nevertheless, batched ``get`` can achieve 3-5x higher throughput than single ``get``.

To send batched ``get`` commands, first call ``keyspace_client_begin()`` function, then issue the ``get`` commands, and finally call ``keyspace_client_submit()``. The commands are sent on ``keyspace_client_submit()``. After the commands complete, the result object must be fetched and iterated to retrieve the individual key-value pairs::

  int status = keyspace_client_begin(client);
  if (status != KEYSPACE_SUCCESS)
  {
    fprintf(stderr, "begin failed");
    ...
  }

  // perform gets here

  keyspace_client_submit(client);
  // fetch result
  keyspace_result_t result = keyspace_client_result(client);
  if (result == KEYSPACE_INVALID_RESULT)
  {
    fprintf(stderr, "result failed");
    ...
  }
  for (keyspace_result_begin(result);
      !keyspace_result_is_end(result);
      keyspace_result_next(result))
  {
    char* key;
    char* val;
    unsigned keylen, vallen;
    if (keyspace_result_key(result, (const void**) &key,
                            &keylen) != KEYSPACE_SUCCESS ||
        keyspace_result_value(result, (const void**) &val,
                              &vallen) != KEYSPACE_SUCCESS)
    {
      fprintf(stderr, "result failed");
	  ...
	}
    // key, keylen and val, vallen now hold a key-value pair
    ...
  }
  keyspace_result_close(result);

Understanding Keyspace status codes
===================================

Keyspace exposes a rich set of status codes through the client library. These are especially useful for batched operations. After issuing command(s), there are four types of status codes which give information about the state of the Keyspace cluster.

``transport_status`` code
-------------------------

``transport_status`` tells the application the portion of commands that were sent to the Keyspace cluster::

  KEYSPACE_SUCCESS: all commands were sent
  KEYSPACE_PARTIAL: only a portion of the commands
                    could be sent before a timeout occured
  KEYSPACE_FAILURE: no commands could be sent

To retrieve the ``transport_status``, use::

  status = keyspace_client_transport_status(client);

``connectivity_status`` code
----------------------------

``connectivity_status`` tells the application the network conditions between the client and the Keyspace cluster::

  KEYSPACE_SUCCESS:      the master could be found
  KEYSPACE_NOMASTER:     some nodes were reachable,
                         but there was no master or it went down
  KEYSPACE_NOCONNECTION: the entire grid was unreachable within timeouts

To retrieve the ``connectivity_status``, use::

  status = keyspace_client_connectivity_status(client);

``timeout_status`` code
----------------------------

``timeout_status`` tells the application what timeouts occured, if any::

  KEYSPACE_SUCCESS:        no timeout occured, everything went fine
  KEYSPACE_MASTER_TIMEOUT: a master could not be found
                           within the master timeout
  KEYSPACE_GLOBAL_TIMEOUT: the blocking client library call
                           returned because the global timeout
                           has expired

To retrieve the ``timeout_status``, use::

  status = keyspace_client_timeout_status(client);

``command_status`` code
-----------------------

``command_status`` is the actual return value of a command::

  KEYSPACE_SUCCESS:   command succeeded
  KEYSPACE_FAILED:    the command was executed, but
                      its return value was FAILED;
                      eg. can happen for test_and_set if the test value
                      does not match or for get if the key does not exist
  KEYSPACE_NOSERVICE: the command was not executed

When using single commands, retrieve the ``command_status`` like::

  status = keyspace_client_command_status(client);

When using batched commands, use::

  // inside the result iteration
  status = keyspace_result_command_status(result);

Note that single operations return the ``command_status``.