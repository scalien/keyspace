#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[])
{
	struct sockaddr_in to;
	int s, i, nread, slen = sizeof(to);
	char buf[100*1024];
	char* err;

	if (argc < 4)
	{
		printf("Usage: %s ip port s:3:hol:4:peru g:3:hol\n", argv[0]);
		return 1;
	}
	
	if (( s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		exit(1);
	
	memset((char *) &to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = htons(atoi(argv[2]));
	if (inet_aton(argv[1], &to.sin_addr) == 0)
	{
		err = strerror(errno);
		printf("%s\n", err);
		exit(1);
	}
	
	if (connect(s, (const struct sockaddr*)&to, slen) < 0)
	{
		err = strerror(errno);
		printf("%s\n", err);
		exit(1);
	}

	printf("Sending...\n");
	fflush(stdout);

	for (i = 3; i < argc; i++)
	{
		sprintf(buf, "%d:", strlen(argv[i]));
		if (send(s, buf, strlen(buf), 0) != strlen(buf))
		{
			err = strerror(errno);
			printf("%s\n", err);
			exit(1);
		}
		if (send(s, argv[i], strlen(argv[i]), 0) != strlen(argv[i]))
		{
			err = strerror(errno);
			printf("%s\n", err);
			exit(1);
		}
		
		printf("Sent: %s%s\n", buf, argv[i]);
		fflush(stdout);
	}
	
	sprintf(buf, "1:*");
	if (send(s, buf, strlen(buf), 0) < 0)
	{
		err = strerror(errno);
		printf("%s\n", err);
		exit(1);
	}
	printf("Sent: %s\n", buf);
	fflush(stdout);
	
	printf("Waiting for answer...\n");
	fflush(stdout);
		
	while(true)
	{
		if ((nread = recv(s, buf, sizeof(buf), 0)) < 0)
		{
			err = strerror(errno);
			printf("%s\n", err);
			exit(1);
		}
		if (nread > 0)
		{
			buf[nread] = '\0';
			printf("Received: %s\n", buf);
			fflush(stdout);
		}
		if (nread == 0)
			break;
	}
	
	printf("Server closed connection...\n");
	fflush(stdout);
}
