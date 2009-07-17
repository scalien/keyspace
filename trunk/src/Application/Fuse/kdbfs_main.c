#include <stdlib.h>
#include <fuse.h>

extern struct fuse_operations kdbfs_oper;

int main(int argc, char *argv[])
{
	int ret;

	umask(0);

	if (argc > 1) {
		ret = mkdir(argv[1], 0777);
	}
		
	return fuse_main(argc, argv, &kdbfs_oper, NULL);
}

