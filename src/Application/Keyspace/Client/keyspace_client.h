#ifndef KEYSPACE_CLIENT_H
#define KEYSPACE_CLIENT_H

#ifdef _WIN32
typedef __int64				int64_t;
typedef unsigned __int64	uint64_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Status values. 
 * When using as a return value, negative values mean error.
 *
 * TODO:
 */
#define KEYSPACE_SUCCESS			0
#define KEYSPACE_API_ERROR			-1

#define KEYSPACE_PARTIAL			-101
#define KEYSPACE_FAILURE			-102

#define KEYSPACE_NOMASTER			-201
#define KEYSPACE_NOCONNECTION		-202

#define KEYSPACE_MASTER_TIMEOUT		-301
#define KEYSPACE_GLOBAL_TIMEOUT		-302

#define KEYSPACE_NOSERVICE			-401
#define KEYSPACE_FAILED				-402

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
keyspace_result_t keyspace_client_result(keyspace_client_t kc);

/*
 * Close the result object and free allocated memory.
 *
 * Parameters:
 *	kr:		result object
 *
 */
void keyspace_result_close(keyspace_result_t kr);

/*
 * Set the result cursor to the beginning of the resultset.
 *
 * Parameters:
 *	kr:		result object
 *
 */
void keyspace_result_begin(keyspace_result_t kr);

/*
 * Check if the result cursor is at the end of the resultset.
 *
 * Parameters:
 *  kr:		result object
 *
 * Return value: nonzero on end
 */
int keyspace_result_is_end(keyspace_result_t kr);

/*
 * Get the next result.
 *
 * Parameters:
 *	kr:		result object
 *
 */ 
void keyspace_result_next(keyspace_result_t kr);

/*
 * Get the transport status of the last operation.
 *
 * Parameters:
 *	kr:		result object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_PARTIAL
 *  - KEYSPACE_FAILURE
 */
int	keyspace_result_transport_status(keyspace_result_t kr);

/*
 * Get the connectivity status of the last operation.
 *
 * Parameters:
 *	kr:		result object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_NOMASTER
 *  - KEYSPACE_NOCONNECTION
 */
int keyspace_result_connectivity_status(keyspace_result_t kr);

/*
 * Get the timeout status of the last operation.
 *
 * Parameters:
 *	kr:		result object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_MASTER_TIMEOUT
 *  - KEYSPACE_GLOBAL_TIMEOUT
 */
int keyspace_result_timeout_status(keyspace_result_t kr);

/*
 * Get the status of the command at the cursor
 *
 * Parameters:
 *	kr:		result object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_NOSERVICE
 *  - KEYSPACE_FAILED
 */
int keyspace_result_command_status(keyspace_result_t kr);

/*
 * Get the key data of the result (if any)
 *
 * Parameters:
 *	kr:		result object
 *  key:	pointer to the key data
 *  keylen:	return the length of the key data
 *
 * Return value: command status
 *	
 */
int keyspace_result_key(keyspace_result_t kr, const void **key, unsigned *keylen);

/*
 * Get the value data of the result (if any)
 *
 * Parameters:
 *	kr:		result object
 *  val:	pointer to the value data
 *  vallen:	return the length of the value data
 *
 * Return value: command status
 *
 */
int keyspace_result_value(keyspace_result_t kr, const void **val, unsigned *vallen);


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
 *
 * Return value: can be any of
 *	- KEYSPACE_SUCCESS
 *  - KEYSPACE_API_ERROR
 *
 */
int	keyspace_client_init(keyspace_client_t kc,
		int nodec, const char* nodev[]);

/*
 * Set the global timeout
 *
 * Parameters:
 *  kc:			client object
 *  timeout:	timeout in millisec
 *
 * Return value: can be any of
 * - KEYSPACE_SUCCESS
 * - KEYSPACE_API_ERROR
 *
 */
int keyspace_client_set_global_timeout(keyspace_client_t kc, uint64_t timeout);

/*
 * Set the master timeout
 *
 * Parameters:
 *  kc:			client object
 *  timeout:	timeout in millisec
 *
 * Return value: can be any of
 * - KEYSPACE_SUCCESS
 * - KEYSPACE_API_ERROR
 *
 */
int keyspace_client_set_master_timeout(keyspace_client_t kc, uint64_t timeout);

/*
 * Get the global timeout
 *
 * Parameters:
 *  kc:			client object
 *
 * Return value: the timeout
 *
 */
uint64_t keyspace_client_get_global_timeout(keyspace_client_t kc);

/*
 * Get the master timeout
 *
 * Parameters:
 *  kc:			client object
 *
 * Return value: the timeout
 *
 */
uint64_t keyspace_client_get_master_timeout(keyspace_client_t kc);

/*
 * Get the transport status of the last operation.
 *
 * Parameters:
 *	kc:		client object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_PARTIAL
 *  - KEYSPACE_FAILURE
 */
int	keyspace_client_transport_status(keyspace_client_t kc);

/*
 * Get the connectivity status of the last operation.
 *
 * Parameters:
 *	kr:		client object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_NOMASTER
 *  - KEYSPACE_NOCONNECTION
 */
int keyspace_client_connectivity_status(keyspace_client_t kc);

/*
 * Get the timeout status of the last operation.
 *
 * Parameters:
 *	kc:		client object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_MASTER_TIMEOUT
 *  - KEYSPACE_GLOBAL_TIMEOUT
 */
int keyspace_client_timeout_status(keyspace_client_t kc);


/*
 * Get the status of the first command
 *
 * Parameters:
 *	kc:		client object
 *
 * Return value: can be any of:
 *  - KEYSPACE_SUCCESS
 *  - KEYSPACE_NOSERVICE
 *  - KEYSPACE_FAILED
 */
int keyspace_client_command_status(keyspace_client_t kc);

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
 *  submit:	nonzero value submits the operation automatically
 *
 * Return value: the command status of the operation
 */
int	keyspace_client_get(keyspace_client_t kc, 
		const void *key, unsigned keylen, 
		int dirty, int submit);

/*
 * COUNT operation
 *
 * Return the number of matching keys with the same parameters as
 * with list.
 * The count is returned in res variable.
 *
 * Parameters:
 *  kc:			client object
 *  res:		contains the return value
 *	prefix:		buffer to the prefix data
 *	prefixlen:	length of the prefix
 *  start_key:	buffer to the starting key
 *  sklen:		length of the starting key
 *  count:		limit the number of results to this value
 *  skip:		skip that much items in the result
 *  backward:	direction of listing, nonzero means backwards listing
 *	dirty:		nonzero value denotes dirty operation
 *
 * Return value: the command status of the operation
 */
int keyspace_client_count(keyspace_client_t kc,
		uint64_t *res,
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int skip,
		int backward,
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
 *  backward:	direction of listing, nonzero means backwards listing
 *	dirty:		nonzero value denotes dirty operation
 *
 * Return value: the command status of the operation
 */
int	keyspace_client_list_keys(keyspace_client_t kc, 
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int skip,
		int backward,
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
 *  backward:	direction of listing, nonzero means backwards listing
 *	dirty:		nonzero value denotes dirty operation
 *
 * Return value: the command status of the operation
 */
int keyspace_client_list_keyvalues(keyspace_client_t kc, 
		const void *prefix, unsigned prefixlen,
		const void *start_key, unsigned sklen,
		uint64_t count,
		int backward,
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
 * Return value: the command status of the operation
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
 * Return value: the command status of the operation
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
 * Return value: the command status of the operation
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
 * Return value: the command status of the operation
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
 * Return value: the command status of the operation
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
 * Return value: the command status of the operation
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
 * Return value: the command status of the operation
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
 * Return value: the transport status of the operation
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
 * Return value: the transport status of the grouped operations.
 */
int	keyspace_client_submit(keyspace_client_t kc);

/*
 * Cancel grouped commands.
 *
 * This function cancels all the grouped operations.
 * 
 * Parameters:
 *	kc:			client object
 *
 * Return value: the transport status of the grouped operations.
 */
int keyspace_client_cancel(keyspace_client_t kc);

#ifdef __cplusplus
}
#endif

#endif
