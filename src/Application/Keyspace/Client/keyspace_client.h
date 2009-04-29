#ifndef KEYSPACE_CLIENT_H
#define KEYSPACE_CLIENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYSPACE_OK					0
#define KEYSPACE_NOTMASTER			-1
#define KEYSPACE_FAILED				-2
#define KEYSPACE_ERROR				-3

#define KEYSPACE_INVALID_RESULT		NULL

typedef void * keyspace_client_t;
typedef void * keyspace_result_t;

keyspace_result_t	keyspace_client_result(keyspace_client_t kc, int *status);
void				keyspace_result_close(keyspace_result_t kr);
keyspace_result_t	keyspace_result_next(keyspace_result_t kr, int *status);
int					keyspace_result_status(keyspace_result_t kr);
const void *		keyspace_result_key(keyspace_result_t kr, unsigned *keylen);
const void *		keyspace_result_value(keyspace_result_t kr, unsigned *vallen);

keyspace_client_t	keyspace_client_create();
void				keyspace_client_destroy();

int					keyspace_client_init(keyspace_client_t kc,
						int nodec, char* nodev[], 
						uint64_t timeout);

int					keyspace_client_connect_master(keyspace_client_t kc);
int					keyspace_client_get_master(keyspace_client_t kc);

int					keyspace_client_get_simple(keyspace_client_t kc, 
						const void *key, unsigned keylen, 
						void *val, unsigned vallen, 
						int dirty);

int					keyspace_client_get(keyspace_client_t kc, 
						const void *key, unsigned keylen, 
						int dirty);

int					keyspace_client_list(keyspace_client_t kc, 
						const void *key, unsigned keylen,
						uint64_t count,
						int dirty);

int					keyspace_client_listp(keyspace_client_t kc, 
						const void *key, unsigned keylen,
						uint64_t count,
						int dirty);

int					keyspace_client_set(keyspace_client_t kc,
						const void *key, unsigned keylen,
						const void *val, unsigned vallen,
						int submit);

int					keyspace_client_test_and_set(keyspace_client_t kc,
						const void *key, unsigned keylen,
						const void *test, unsigned testlen,
						const void *val, unsigned vallen,
						int submit);

int					keyspace_client_add(keyspace_client_t kc,
						const void *key, unsigned keylen,
						int64_t num,
						int64_t *result,
						int submit);

int					keyspace_client_delete(keyspace_client_t kc,
						const void *key, unsigned keylen,
						int submit);

int					keyspace_client_begin(keyspace_client_t kc);
int					keyspace_client_submit(keyspace_client_t kc);


#ifdef __cplusplus
}
#endif

#endif
