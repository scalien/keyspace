#ifndef KEYSPACE_CLIENT_H
#define KEYSPACE_CLIENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Status values. 
 * When using as a return value, negative values mean error.
 *
 * NOTMASTER is returned when a write or safe read operation was sent to a
 *	non-master server.
 * FAILED is returned when the operation failed, e.g. when there is no value to
 *	the key.
 * ERROR is returned when the connection closes unexpectedly or timeout occurs.
 */
#define KEYSPACE_OK					0
#define KEYSPACE_NOTMASTER			-1
#define KEYSPACE_FAILED				-2
#define KEYSPACE_ERROR				-3

#define KEYSPACE_INVALID_RESULT		NULL

typedef void * keyspace_client_t;
typedef void * keyspace_result_t;

/*
 * Get the result of the last operation
 *
 * Parameters:
 *	kc:		client object
 *	status: return the status of the operation
 *
 * Return value: the result object or KEYSPACE_INVALID_RESULT in case of error
 */ 
keyspace_result_t keyspace_client_result(keyspace_client_t kc, int *status);

/*
 * Close the result object and free allocated memory.
 */
void keyspace_result_close(keyspace_result_t kr);

/*
 * Get the next result.
 *
 * Parameters:
 *	kr:		result object
 *	status: return the status of the operation
 *
 * Return value: the result object or KEYSPACE_INVALID_RESULT in case of error
 */ 
keyspace_result_t keyspace_result_next(keyspace_result_t kr, int *status);

/*
 * Get the status of the last operation.
 *
 * Parameters:
 *	kr:		result object
 *
 * Return value: the status of the operation
 */
int	keyspace_result_status(keyspace_result_t kr);

/*
 * Get the key data of the result (if any)
 *
 * Parameters:
 *	kr:		result object
 *  keylen:	return the length of the key data
 *
 * Return value: pointer to the key data that is keylen long or 
 *	NULL if there is no data
 */
const void * keyspace_result_key(keyspace_result_t kr, unsigned *keylen);

/*
 * Get the value data of the result (if any)
 *
 * Parameters:
 *	kr:		result object
 *  vallen:	return the length of the value data
 *
 * Return value: pointer to the value data that is vallen long or 
 *	NULL if there is no data
 */
const void * keyspace_result_value(keyspace_result_t kr, unsigned *vallen);


/*
 * Create client object
 *
 * Return value: the new client object
 */
keyspace_client_t keyspace_client_create();

/*
 * Destroy client object
 *
 * Parameters:
 *	kc:		client object
 */
void keyspace_client_destroy(keyspace_client_t kc);

/*
 * Initialize client object. Set up nodes and timeout, connect to one of the
 * nodes.
 *
 * Parameters:
 *	kc:			client object
 *	nodec:		number of node names to use
 *	nodev:		array of node names
 *	timeout:	socket timeout
 *
 * Return value: the status of the operation
 */
int	keyspace_client_init(keyspace_client_t kc,
		int nodec, const char* nodev[], 
		uint64_t timeout);

/*
 * Get the master node.
 *
 * Parameters:
 *	kc:		client object
 *
 * Return value: the number of the master node or the error status of the 
 * operation when negative.
 */
int	keyspace_client_get_master(keyspace_client_t kc);

/*
 * GET operation with user-provided buffer.
 *
 * Return the value of a key, if it exists in the database. 
 *
 * Parameters:
 *	kc:		client object
 *	key:	buffer to the key data
 *	keylen:	length of the key
 *	val:	buffer to the value data
 *	vallen:	size of the buffer to the value
 *	dirty:	nonzero value denotes dirty operation
 *
 * Return value: the size of the key, or error when negative. If exceeds vallen,
 *	the value is not copied into val.
 */
int	keyspace_client_get_simple(keyspace_client_t kc, 
		const void *key, unsigned keylen, 
		void *val, unsigned vallen, 
		int dirty);

/*
 * GET operation.
 *
 * Return the value of a key, if it exists in the database. 
 * You get the result with keyspace_client_result().
 *
 * Parameters:
 *	kc:		client object
 *	key:	buffer to the key data
 *	keylen:	length of the key
 *	dirty:	nonzero value denotes dirty operation
 *
 * Return value: the status of the operation
 */
int	keyspace_client_get(keyspace_client_t kc, 
		const void *key, unsigned keylen, 
		int dirty);

/*
 * LISTKEYS operation.
 *
 * Return at most count 'keys' starting with 'prefix'.
 * You get the result with keyspace_client_result().
 *
 * Parameters:
 *	kc:			client object
 *	prefix:		buffer to the prefix data
 *	prefixlen:	length of the prefix
 *  start_key:	buffer to the starting key
 *  sklen:		length of the starting key
 *  count:		limit the number of results to this value
 *  skip:		skip that much items in the result
 *	dirty:		nonzero value denotes dirty operation
 *
 * Return value: the status of the operation
 */
int	keyspace_client_list_keys(keyspace_client_t kc, 
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int skip,
		int dirty);


/*
 * LISTKEYVALUES operation.
 *
 * Return at most 'count' keys and their values starting with 'prefix'.
 * You get the result with keyspace_client_result().
 *
 * Parameters:
 *	kc:			client object
 *	prefix:		buffer to the prefix data
 *	prefixlen:	length of the prefix
 *  start_key:	buffer to the starting key
 *  sklen:		length of the starting key
 *  count:		limit the number of results to this value
 *  skip:		skip that much items in the result
 *	dirty:		nonzero value denotes dirty operation
 *
 * Return value: the status of the operation
 */
int keyspace_client_list_keyvalues(keyspace_client_t kc, 
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int skip,
		int dirty);

/*
 * SET operation.
 *
 * Set the value of 'key', overwriting the previous value if it already existed
 * in the database.
 *
 * Parameters:
 *	kc:			client object
 *	key:		buffer to the key data
 *	keylen:		length of the key
 *	val:		buffer to the value data
 *	vallen:		length of the value
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int	keyspace_client_set(keyspace_client_t kc,
		const void *key, unsigned keylen,
		const void *val, unsigned vallen,
		int submit);


/*
 * TEST-AND-SET operation.
 *
 * Changes the value of 'key' to 'value' if its current value is 'test'.
 * 
 * Parameters:
 *	kc:			client object
 *	key:		buffer to the key data
 *	keylen:		length of the key
 *	test:		buffer to the test data
 *	testlen:	length of the test
 *	val:		buffer to the value data
 *	vallen:		length of the value
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int	keyspace_client_test_and_set(keyspace_client_t kc,
		const void *key, unsigned keylen,
		const void *test, unsigned testlen,
		const void *val, unsigned vallen,
		int submit);

/*
 * ADD operation.
 *
 * Treats the value of 'key' as a number and atomically increments it by 'num',
 * where 'num' may be a negative number.
 * 
 * Parameters:
 *	kc:			client object
 *	key:		buffer to the key data
 *	keylen:		length of the key
 *	num:		increment the value by this number
 *	result:		return the resulting value
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int	keyspace_client_add(keyspace_client_t kc,
		const void *key, unsigned keylen,
		int64_t num,
		int64_t *result,
		int submit);

/*
 * DELETE operation.
 *
 * Delete 'key' and its value from the database.
 * 
 * Parameters:
 *	kc:			client object
 *	key:		buffer to the key data
 *	keylen:		length of the key
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int	keyspace_client_delete(keyspace_client_t kc,
		const void *key, unsigned keylen,
		int submit);

/*
 * REMOVE operation.
 *
 * Delete 'key' and its value from the database, and return the value.
 * 
 * Parameters:
 *	kc:			client object
 *	key:		buffer to the key data
 *	keylen:		length of the key
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int keyspace_client_remove(keyspace_client_t kc,
		const void *key, unsigned keylen,
		int submit);

/*
 * RENAME operation.
 *
 * Rename key named 'from' to key named 'to'.
 * 
 * Parameters:
 *	kc:			client object
 *	from:		buffer to the name
 *	keylen:		length of the from
 *  to:			buffer to the name
 *  tolen:		length of the to
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int keyspace_client_rename(keyspace_client_t kc,
		const void *from, unsigned fromlen,
		const void *to, unsigned tolen,
		int submit);


/*
 * PRUNE operation
 *
 * Delete all key starting with 'prefix' from the database.
 *
 * Parameters:
 *  kc:			client object
 *  prefix:		buffer to the prefix
 *  prefixlen:	length of the prefix
 *	submit:		nonzero value submits the operation automatically
 *
 * Return value: the status of the operation
 */
int keyspace_client_prune(keyspace_client_t kc,
		const void *prefix, unsigned prefixlen,
		int submit);

/*
 * Begin grouping commands.
 *
 * You can group several write operations and submit them as one. Before
 * grouping commands you have to call this function, and then call write
 * operations with 'submit' parameter set to 0.
 * 
 * Parameters:
 *	kc:			client object
 *
 * Return value: the status of the operation
 */
int	keyspace_client_begin(keyspace_client_t kc);

/*
 * Submit grouped commands.
 *
 * This function waits for all the grouped operations to complete. If any of
 * the operations fail, the return value of this function will be negative.
 * 
 * Parameters:
 *	kc:			client object
 *
 * Return value: the status of the grouped operations.
 */
int	keyspace_client_submit(keyspace_client_t kc);


#ifdef __cplusplus
}
#endif

#endif
