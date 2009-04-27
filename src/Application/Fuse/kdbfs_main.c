#include "kdbfs.h"

static struct fuse_operations kdbfs_oper = {
    .getattr	= kdbfs_getattr,
    .readdir	= kdbfs_readdir,
    .open		= kdbfs_open,
    .read		= kdbfs_read,
};

int main(int argc, char *argv[])
{
	// TODO fuse_main
	return 0;
}

