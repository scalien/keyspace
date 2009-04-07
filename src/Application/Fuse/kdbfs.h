#ifndef KDBFS_H
#define KDBFS_H

#include <fuse.h>

#ifdef __cplusplus
extern "C" {
#endif

int kdbfs_getattr(const char *path, struct stat *stbuf);
int kdbfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi);
int kdbfs_open(const char *path, struct fuse_file_info *fi);
int kdbfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi);

void kdbfs_init();
void kdbfs_close();

#ifdef __cplusplus
}
#endif

#endif
