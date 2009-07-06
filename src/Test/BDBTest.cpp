#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <db_cxx.h>


long int
elapsed(struct timeval start, struct timeval end)
{
    return ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) / 1000;
}

void
bdb_test_loop_get_set_param(Db *db, int len, int nloop)
{
	u_int64_t id;
	char *value;
	struct timeval start, end;

	id = (u_int64_t) rand();
	value = new char[len];
	
	Dbt key(&id, sizeof(id));
	Dbt data(value, len);

	db->put(NULL, &key, &data, 0);
	
	fprintf(stderr, "size of value = %d, number of loops = %d\n", len, nloop);
	
	gettimeofday(&start, NULL);

	for (int i = 0; i < nloop; i++)
	{
		int ret = db->get(NULL, &key, &data, 0);
	}
	
	gettimeofday(&end, NULL);
	
	long int e = elapsed(start, end);
	fprintf(stderr, "elapsed: %ld ms, time(get) = %f usec\n", e, (float) e / nloop * 1000);
	
	delete[] value;
}

void
bdb_test_loop_set_param(Db *db, int len, int nloop)
{
	u_int64_t id;
	char *value;
	struct timeval start, end;
	
	id = (u_int64_t) rand();
	value = new char[len];
		
	Dbt data(value, len);
//	Dbt key(&id, sizeof(id));
	char buffer[nloop * sizeof(Dbt)];
	Dbt *keys[nloop];
	char rndbuf[nloop * sizeof(Dbt)];
	for (int i = 0; i < nloop; i++)
	{		
		Dbt *key = new (buffer + i * sizeof(Dbt)) Dbt(buffer + i * sizeof(Dbt), sizeof(Dbt));
		keys[i] = key;
	}
	
	fprintf(stderr, "size of value = %d, number of loops = %d\n", len, nloop);
	
	gettimeofday(&start, NULL);

	DbEnv *env = db->get_env();
	for (int i = 0; i < nloop; i++)
	{	
		DbTxn *txn = NULL;
		env->txn_begin(NULL, &txn, 0);
		int ret = db->put(txn, keys[i], &data, 0);
		txn->commit(DB_TXN_SYNC);
//		db->sync(0);
	}

	gettimeofday(&end, NULL);
	
	long int e = elapsed(start, end);
	fprintf(stderr, "elapsed: %ld ms, time(get) = %f usec\n", e, (float) e / nloop * 1000);
	
	delete[] value;
}

void
bdb_test_loop_get_set(Db *db)
{
	int len;
	const int NTEST = 100000;
	
//	len = 1 + (int) (16.0 * 1024 * 1024 * (rand() / (RAND_MAX + 1.0)));
	/*
	 * env_flags = DB_INIT_TXN | DB_RECOVER_FATAL | DB_CREATE | DB_INIT_MPOOL;
	 * db_type = DB_BTREE
	 * NTEST = 100000
	 *
	 * len = 16, 1.56 usec
	 * len = 128, 1.72 usec
	 * len = 1024, 1.76 usec
	 * len = 16384, 6.55 usec
	 * len = 131072, 37.48 usec
	 * len = 1048576, 3623.19 usec
	 *
	 * len = 1, 1.59 usec
	 * len = 10, 1.54 usec
	 * len = 100, 1.58 usec
	 * len = 1000, 1.64 usec
	 * len = 10000, 4.08 usec
	 * len = 100000, 30.93 usec
	 * len = 1000000, 3549.21 usec
	 *
	 * env_flags = DB_CREATE | DB_INIT_MPOOL
	 * db_type = DB_BTREE
	 * NTEST = 100000
	 *
	 * len = 1, 1.45 usec
	 * len = 10, 1.60 usec
	 * len = 100, 1.42 usec
	 * len = 1000, 1.60 usec
	 * len = 10000, 3.99 usec
	 * len = 100000, 36.41 usec
	 * len = 1000000, 3497.36 usec
	 */
	len = 1000000;

	bdb_test_loop_get_set_param(db, len, NTEST);
}

void
bdb_test_big_value(Db *db)
{
	char buffer[1024 * 1024];
	char key[20];
	int keylen;
	
	db->set_pagesize(sizeof(buffer));
	
	for (int i = 0; i < 10; i++)
	{
		keylen = snprintf(key, sizeof(key), "%i", i);
		Dbt dbtKey(key, keylen);
		Dbt dbtValue(buffer, sizeof(buffer));
		int ret = db->put(&dbtKey, &dbtValue, 0);
	}
}

void
bdb_test(Db *db)
{
	const char *filename = "test.db";
	const char *dbname = NULL;
	int filemode = 0;
	
	db->open(NULL, filename, dbname, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT, filemode);
	
//	bdb_test_loop_get_set(db);
//	bdb_test_loop_set_param(db, 20, 10);
	bdb_test_big_values(db);

	db->close(0);
}

Db*
bdb_open()
{
	return new Db(NULL, 0);
}

void
bdb_simple_test()
{
	bdb_test(bdb_open());
}

Db*
bdb_txn_open()
{
	DbEnv *env(0);
	u_int32_t flags = DB_INIT_TXN | DB_RECOVER_FATAL | DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_LOCK;
//	u_int32_t flags = DB_CREATE | DB_INIT_MPOOL;
	int mode = 0;
	
	env = new DbEnv(0);
	env->open(".", flags, mode);
	
	return new Db(env, 0);
}

void
bdb_txn_test()
{
	bdb_test(bdb_txn_open());
}

int
bdbtest()
{
	try
	{
//		bdb_txn_test();
		bdb_simple_test();
	}
	catch (DbException &e)
	{
		fprintf(stderr, "DbException\n");
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "std::exception\n");
	}
	
	return 0;
}
