#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "test.h"

char *filename = "test.txt";

int
open_direct_sync_io()
{
	int fd;
	int ret;
	char buf[] = "open_direct_sync_io";
	
	fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if (fd < 0)
		return TEST_FAILURE;
		
	ret = write(fd, buf, sizeof(buf) - 1);
	if (ret < 0)
		return TEST_FAILURE;
	
	close(fd);
	
	return TEST_SUCCESS;
}


int
main(int argc, char *argv[])
{
	int ret = 0;
	
	if (argc >= 2) {
		filename = argv[1];
	}
	
	ret += TEST(open_direct_sync_io);
	
	return test_eval(__FILE__, ret);
}
