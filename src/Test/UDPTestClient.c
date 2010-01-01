#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	struct sockaddr_in to, me;
	int s, slen = sizeof(to);

	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s <from_port> <to_port> <msg>\n", argv[0]);
		return 1;
	}
	
	int from_port = atoi(argv[1]);
	int to_port = atoi(argv[2]);

	char* msg = argv[3];
	int msglen = strlen(msg);
	
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		exit(1);

	memset((char *) &me, 0, sizeof(me));
	me.sin_family = AF_INET;
	me.sin_port = htons((uint16_t)from_port);
	me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (const struct sockaddr*) &me, sizeof(me)) < 0)
		exit(1);
	
	memset((char *) &to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = htons(to_port);
	if (inet_aton("127.0.0.1", &to.sin_addr) == 0)
		exit(1);

	if (sendto(s, msg, msglen, 0, (const struct sockaddr*) &to, slen) < 0)
		exit(1);
	
	return 0;
}
