#include "kdbfs.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "Channel.h"


static const char *kdbfs_str = "KeyspaceDB File System\n";
static const char *kdbfs_path = "/kdb";
static Channel channel;

int kdbfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, kdbfs_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(kdbfs_str);
    }
    else
        res = -ENOENT;

    return res;
}

int kdbfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
	size_t	pathlen;

    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)
        return -ENOENT;

	pathlen = strlen(path);

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, kdbfs_path + 1, NULL, 0);

    return 0;
}

int kdbfs_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, kdbfs_path) != 0)
        return -ENOENT;

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

int kdbfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    if(strcmp(path, kdbfs_path) != 0)
        return -ENOENT;

    len = strlen(kdbfs_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, kdbfs_str + offset, size);
    } else
        size = 0;

    return size;
}
