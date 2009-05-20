// DO NOT USE! This code is experimental at the moment.
#ifdef KDBFS
#include "kdbfs.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "keyspace_client.h"

#define MAX_PATH	256

static const char *kdbfs_str = "KeyspaceDB File System\n";
static const char *kdbfs_path = "/kdb";
static keyspace_client_t kc;
static FILE *logfile;

void Log_SetOutputFile(FILE *logfile);

#define log_trace(fmt, ...) {fprintf(logfile, "%s:%d: " fmt "\n", __func__, __LINE__, __VA_ARGS__); fflush(logfile); }

static int
kdbfs_fgetattr(const char *path, struct stat *stbuf,
                  struct fuse_file_info *fi) {
  memset(stbuf, 0, sizeof(struct stat));
  
  if (strcmp(path, "/") == 0) { /* The root directory of our file system. */
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }
  return -ENOENT;
}

/*
static int
kdbfs_fgetattr(const char *path, struct stat *stbuf,
                  struct fuse_file_info *fi)
{
    int res = 0;
	unsigned pathlen;
	char dir[MAX_PATH + 1];
	keyspace_result_t kr;
	const char *pval;
	unsigned vallen;
	
	log_trace("path = %s", path);
	
	pathlen = strlen(path);

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
		log_trace("%s", "returning 0");
		return 0;
    }
    else if(strcmp(path, kdbfs_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(kdbfs_str);
		log_trace("%s", "returning 0");
		return 0;
    }
    else
	{
		log_trace("%s", "before get");
		res = keyspace_client_get(kc, path, pathlen, 1);
		if (res < 0)
			return -ENOENT;
		
		kr = keyspace_client_result(kc, &res);
		if (kr == KEYSPACE_INVALID_RESULT)
			return -ENOENT;
		
		pval = keyspace_result_value(kr, &vallen);
		keyspace_result_close(kr);
		
		dir[res] = 0;
		if (dir[res - 1] == '/') {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
		} else {
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = vallen;
		}
		return 0;
	}
    return -ENOENT;
}
*/

static int 
kdbfs_getattr(const char *path, struct stat *stbuf)
{
	log_trace("path = %s", path);
	return kdbfs_fgetattr(path, stbuf, NULL);
}


static int
kdbfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
	char		dir[MAX_PATH + 1];
	unsigned	dirlen;
	char		file[MAX_PATH + 1];
	unsigned	filelen;
	const char	*pfile;
	char		*slash;
	keyspace_result_t kr;
	int			status;

	log_trace("path = %s", path);

    (void) offset;
    (void) fi;

	slash = strrchr(path, '/');
	if (!slash)
		return -ENOENT;
	
	dirlen = slash - path;
	if (dirlen > MAX_PATH)
		return -ENOENT;

	strncpy(dir, path, dirlen);
	dir[dirlen] = 0;
	
	status = keyspace_client_list(kc, dir, dirlen, 0, 1);
	if (status < 0)
		return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
	kr = keyspace_client_result(kc, &status);
	while (kr != KEYSPACE_INVALID_RESULT)
	{
		pfile = keyspace_result_key(kr, &filelen);
		if (filelen > MAX_PATH)
			continue;
		
		snwritef(file, sizeof(file), "%B", filelen - dirlen, pfile + dirlen);
		filler(buf, file, NULL, 0);
		
		kr = keyspace_result_next(kr, &status);
	}
	
    return 0;
}

static int
kdbfs_open(const char *path, struct fuse_file_info *fi)
{
	unsigned pathlen;
	char val[MAX_PATH + 1];
	unsigned vallen;
	int res;

	log_trace("path = %s", path);
	
	pathlen = strlen(path);
	vallen = MAX_PATH;
	
	res = keyspace_client_get_simple(kc, path, pathlen, val, vallen, 1);
	if (res < 0)
		return -ENOENT;
	
    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int
kdbfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	unsigned pathlen;
	const char *pval;
	unsigned vallen;
	int res;
	keyspace_result_t kr;

    (void) fi;
	
	log_trace("path = %s", path);

	pathlen = strlen(path);
	vallen = MAX_PATH;
	
	res = keyspace_client_get(kc, path, pathlen, 1);
	if (res < 0)
		return -ENOENT;

	kr = keyspace_client_result(kc, &res);
	if (res < 0)
		return -ENOENT;
	
	pval = keyspace_result_value(kr, &vallen);

    if (offset < vallen) {
        if (offset + size > vallen)
            size = vallen - offset;
        memcpy(buf, pval + offset, size);
    } else
        size = 0;

    return size;
}

static void *
kdbfs_init(struct fuse_conn_info *conn)
{
	int status;
	uint64_t timeout = 10000;
	char *nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};

	logfile = fopen("/tmp/kdbfs.log", "a");
	Log_SetOutputFile(logfile);

//	FUSE_ENABLE_XTIMES(conn);
	
	kc = keyspace_client_create();

	status = keyspace_client_init(kc, sizeof(nodes) / sizeof(*nodes), nodes, timeout);
	log_trace("\nstatus = %d", status);
	
	return NULL;
}

static void
kdbfs_destroy(void *userdata)
{
	(void) userdata;
	
	log_trace("%s", "before fclose");
	
	fclose(logfile);
	keyspace_client_destroy(kc);
}

struct fuse_operations kdbfs_oper = {
	.init		= kdbfs_init,
	.destroy	= kdbfs_destroy,
	.fgetattr	= kdbfs_fgetattr,
    .getattr	= kdbfs_getattr,
    .readdir	= kdbfs_readdir,
    .open		= kdbfs_open,
    .read		= kdbfs_read,
};
#endif
/*
 * kfs.c
 * kfs
 *
 * Created by Attila Gazso on 4/29/09.
 * Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "keyspace_client.h"

static keyspace_client_t kc;

static int
kfs_fgetattr(const char *path, struct stat *stbuf,
                  struct fuse_file_info *fi) {
  memset(stbuf, 0, sizeof(struct stat));
  
  if (strcmp(path, "/") == 0) { /* The root directory of our file system. */
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }
  return -ENOENT;
}

static int
kfs_getattr(const char *path, struct stat *stbuf) {
  return kfs_fgetattr(path, stbuf, NULL);
}

static int
kfs_readlink(const char *path, char *buf, size_t size) {
  return -ENOENT;
}

static int
kfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
  if (strcmp(path, "/") != 0) /* We only recognize the root directory. */
    return -ENOENT;
  
  filler(buf, ".", NULL, 0);           /* Current directory (.)  */
  filler(buf, "..", NULL, 0);          /* Parent directory (..)  */
  
  return 0;
}

static int
kfs_mknod(const char *path, mode_t mode, dev_t rdev) {
  return -ENOSYS;
}

static int
kfs_mkdir(const char *path, mode_t mode) {
  return -ENOSYS;
}

static int
kfs_unlink(const char *path) {
  return -ENOSYS;
}

static int
kfs_rmdir(const char *path) {
  return -ENOSYS;
}

static int
kfs_symlink(const char *from, const char *to) {
  return -ENOSYS;
}

static int
kfs_rename(const char *from, const char *to) {
  return -ENOSYS;
}

static int
kfs_exchange(const char *path1, const char *path2, unsigned long options) {
  return -ENOSYS;
}

static int
kfs_link(const char *from, const char *to) {
  return -ENOSYS;
}

static int
kfs_fsetattr_x(const char *path, struct setattr_x *attr,
                    struct fuse_file_info *fi) {
  return -ENOENT;
}

static int
kfs_setattr_x(const char *path, struct setattr_x *attr) {
  return -ENOENT;
}

static int
kfs_getxtimes(const char *path, struct timespec *bkuptime,
                   struct timespec *crtime) {
  return -ENOENT;
}

static int
kfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -ENOSYS;
}

static int
kfs_open(const char *path, struct fuse_file_info *fi) {
  return -ENOENT;
}

static int
kfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  return -ENOSYS;
}

static int
kfs_write(const char *path, const char *buf, size_t size,
               off_t offset, struct fuse_file_info *fi) {
  return -ENOSYS;
}

static int
kfs_statfs(const char *path, struct statvfs *stbuf) {
  memset(stbuf, 0, sizeof(*stbuf));
  stbuf->f_files = 2;  /* For . and .. in the root */
  return 0;
}

static int
kfs_flush(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int
kfs_release(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int
kfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
  return 0;
}

static int
kfs_setxattr(const char *path, const char *name, const char *value,
                  size_t size, int flags, uint32_t position) {
  return -ENOTSUP;
 }

static int
kfs_getxattr(const char *path, const char *name, char *value, size_t size,
                  uint32_t position) {
  return -ENOATTR;
}

static int
kfs_listxattr(const char *path, char *list, size_t size) {
  return 0;
}

static int
kfs_removexattr(const char *path, const char *name) {
  return -ENOATTR;
}

void *
kfs_init(struct fuse_conn_info *conn) {
	int status;
	uint64_t timeout = 10000;
	char *nodes[] = {"127.0.0.1:7080", "127.0.0.1:7081", "127.0.01:7082"};
  FUSE_ENABLE_XTIMES(conn);
  
	kc = keyspace_client_create();
	status = keyspace_client_init(kc, sizeof(nodes) / sizeof(*nodes), nodes, timeout);
  
  return NULL;
}

void
kfs_destroy(void *userdata) {
  /* nothing */
  keyspace_client_destroy(kc);
}

struct fuse_operations kdbfs_oper = {
  .init        = kfs_init,
  .destroy     = kfs_destroy,
  .getattr     = kfs_getattr,
  .fgetattr    = kfs_fgetattr,
/*  .access      = kfs_access, */
  .readlink    = kfs_readlink,
/*  .opendir     = kfs_opendir, */
  .readdir     = kfs_readdir,
/*  .releasedir  = kfs_releasedir, */
  .mknod       = kfs_mknod,
  .mkdir       = kfs_mkdir,
  .symlink     = kfs_symlink,
  .unlink      = kfs_unlink,
  .rmdir       = kfs_rmdir,
  .rename      = kfs_rename,
  .link        = kfs_link,
  .create      = kfs_create,
  .open        = kfs_open,
  .read        = kfs_read,
  .write       = kfs_write,
  .statfs      = kfs_statfs,
  .flush       = kfs_flush,
  .release     = kfs_release,
  .fsync       = kfs_fsync,
  .setxattr    = kfs_setxattr,
  .getxattr    = kfs_getxattr,
  .listxattr   = kfs_listxattr,
  .removexattr = kfs_removexattr,
  .exchange    = kfs_exchange,
  .getxtimes   = kfs_getxtimes,
  .setattr_x   = kfs_setattr_x,
  .fsetattr_x  = kfs_fsetattr_x,
};

