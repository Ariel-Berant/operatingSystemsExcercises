#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int sockfd = -1, read_bytes = 0;
	char file_buff[1024], *endptr;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: <ip of server> <port of server> <path of file to send>\n");
		exit(1);
	}


	int fd = open(argv[3], O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "Error: Problem opening file. %s\n", strerror(errno));
		exit(1);
	}
	// get file size because we need to send it to server(why not with ordinary functions ffs?)
	struct stat st;
	stat(argv[3], &st);
	u_int32_t size = htonl(st.st_size), *printableChar = malloc(sizeof(u_int32_t));
	

	struct sockaddr_in serv_addr; // where we Want to get to
	socklen_t addrsize = sizeof(struct sockaddr_in);

	memset(&file_buff, 0, sizeof(file_buff));
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "\n Error : Could not create socket. %s\n", strerror(errno));
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	inet_pton(AF_INET, argv[1], &serv_addr.sin_addr); // Note: inet_pton for address conversion
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons((uint16_t)strtol(argv[2], &endptr, 10));

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "\n Error : Connect Failed. %s \n", strerror(errno));
		exit(1);
	}
	
	write(sockfd, &size, sizeof(u_int32_t));
	printf("File size sent: %lu\n", st.st_size);

	// send file
	while ((read_bytes = read(fd, file_buff, sizeof(file_buff))) > 0)
	{
		write(sockfd, file_buff, read_bytes);
		printf("Sent %d bytes\n", read_bytes);
	}
	printf("File sent\n");

	// read # of printable characters
	read(sockfd, printableChar, sizeof(u_int32_t));
	*printableChar = ntohl(*printableChar);
	printf("# of printable characters: %u\n", *printableChar);

	close(sockfd);
	return 0;
}
