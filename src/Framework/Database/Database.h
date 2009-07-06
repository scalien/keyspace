#ifndef DATABASE_H
#define DATABASE_H

#ifdef DATABASE_BDB
#include "BDB/BDBDatabase.h"
#endif

// the global database object
extern Database database;

#endif
