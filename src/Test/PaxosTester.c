#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
	struct sockaddr_in me, from, to;
	int s, i, nread, slen = sizeof(to);
	char buf[512];

	if (argc != 2)
		exit(1);

	if (( s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		exit(1);

	memset((char *) &me, 0, sizeof(me));
	me.sin_family = AF_INET;
	me.sin_port = htons((uint16_t)11000);
	me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (const sockaddr*) &me, sizeof(me)) < 0)
		exit(1);

	memset((char *) &to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = htons(10000);
	if (inet_aton("127.0.0.1", &to.sin_addr) == 0)
		exit(1);

	for (i = 0; i < 1; i++)
	{
		if (sendto(s, argv[1], strlen(argv[1]), 0, (const sockaddr*) &to, slen) < 0)
			exit(1);
		
		/*
		slen = sizeof(from);
		if ((nread = recvfrom(s, buf, sizeof(buf), 0, (sockaddr*) &from, (socklen_t*) &slen)) < 0)
			exit(1);
		buf[nread] = '\0';
		printf("Message from %s:%d: %s\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port),  buf);
		*/
	}
}
