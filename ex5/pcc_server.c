#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

typedef struct pcc_total
{
	u_int16_t total[95];
} pcc_total;

pcc_total total;

// marking volatile, to not get optimized incorrectly by compiler
volatile int sigInted = 0;
int connfd = 0, listenfd = 0;

void sigint_handler(int sig)
{
	if (connfd > 0)
	{
		sigInted = 1;
	}
	else
	{
		for (size_t i = 0; i < 95; i++)
		{
			printf("char '%c' : %u times\n", (char)(i + 32), total.total[i]);
		}
		if (listenfd != 0)
		{
			close(listenfd);
		}
		
		exit(0);
	}
	
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: <server port>\n");
		exit(1);
	}

	signal(SIGINT, sigint_handler);

	char *endptr;
	u_int32_t data_len, nread, count = 0;
	struct sockaddr_in serv_addr, my_addr, peer_addr;
	socklen_t addrsize = sizeof(struct sockaddr_in);

	char data_buff[1024];

	// get fd of socket we're listening on
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	// set socket options to reuse address, the 1 is to turn it on apparently
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	memset(&serv_addr, 0, addrsize);
	serv_addr.sin_family = AF_INET;
	// INADDR_ANY = any local machine address
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// convert port number to network byte order
	serv_addr.sin_port = htons((uint16_t)strtol(argv[1], &endptr, 10));

	if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, addrsize))
	{
		fprintf(stderr, "\n Error : Bind Failed. %s \n", strerror(errno));
		exit(1);
	}

	if (0 != listen(listenfd, 10))
	{
		fprintf(stderr, "\n Error : Listen Failed. %s \n", strerror(errno));
		exit(1);
	}

	while (!sigInted)
	{
		connfd = accept(listenfd, (struct sockaddr *)&peer_addr, &addrsize);

		if (connfd < 0)
		{
			fprintf(stderr, "\n Error : Accept Failed. %s \n", strerror(errno));
			continue;
		}

		// read data length from client
		if(read(connfd, &data_len, sizeof(data_len)) != sizeof(data_len))
		{
			fprintf(stderr, "\n Error : Read Failed. %s \n", strerror(errno));
			connfd = 0;
			continue;
		}
		data_len = ntohl(data_len);

		// read data from client
		u_int32_t total_size = 0;
		u_int16_t currTot[95] = {0};
		while(total_size < data_len)
		{
			nread = read(connfd, data_buff, sizeof(data_buff));
			if (nread < 0)
			{
				fprintf(stderr, "\n Error : Read Failed. %s \n", strerror(errno));
				break;
			}
			total_size += nread;

			for (size_t i = 0; i < nread; i++)
			{
				if (data_buff[i] >= 32 && data_buff[i] <= 126)
				{
					currTot[data_buff[i] - 32]++;
					count++;
				}
			}
		}

		if (nread < 0)
		{
			count = 0;
			continue;
		}
		
		for (size_t i = 0; i < 95; i++)
		{
			total.total[i] += currTot[i];
		}

		// send data back to client
		count = htonl(count);
		if (write(connfd, &count, sizeof(count)) != sizeof(count))
		{
			fprintf(stderr, "\n Error : Write Failed. %s \n", strerror(errno));
			count = 0;
			continue;
		}
		count = 0;
		
		
		

		// close socket
		close(connfd);
		connfd = 0;
	}

	// after siginted while handling, print the total
	for (size_t i = 0; i < 95; i++)
	{
		printf("char '%c' : %u times\n", (char)(i + 32), total.total[i]);
	}
	if (listenfd != 0)
	{
		close(listenfd);
	}
	exit(0);
}
