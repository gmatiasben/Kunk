/**********************************************************************
* tcp_client.c
*
* This file contains the code of a TCP Client.
*
* The program accepts one command-line parameter which is
* a command to be executed on another computer.
*
* The Client sends a command to a server that executes the
* command (using the system() function) in the computer on which it
* runs and returns the command return code to the Client.
*
* The Server-Client communication is done through INTERNET STREAM Sockets.
* The Server's Socket address is port number 4000 in the computer "localhost".
***********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>				/* internet definitions */
#include <netdb.h>					/* computer information */
#include <string.h>

#define PORT_NUMBER			4000	/* socket address port number */
#define SERVER_HOST   "localhost"	/* server computer name */

int main(int argc, char **argv)
{
	int  sd;						/* socket descriptor */
	struct sockaddr_in socket_address; /* socket address structure */
	struct hostent *host_info;		/* computer information */
	int  len;						/* length of command */
	int  ret;						/* command return code */

	/* Check invocation */				  
	if (argc != 2)
	{
		fprintf(stderr, "Usage:%s <command>\n", argv[0]);
		exit(1);
	}

	/* Initialization of the socket */

	/* Get socket descriptor for INTERNET, STREAM protocol */
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
	{
		perror("tcp_client.c : socket ");	/* report error */
		exit(1);
	}

	/* Get the server computer information */
	host_info = gethostbyname(SERVER_HOST);
	if (host_info == NULL)
	{
		perror("tcp_client.c : gethostbyname ");	/* report error */
		exit(1);
	}

	/* Prepare socket address.
	* Pay attention to the htons() function *
	* Pay attention that the computer address is taken from
	  the computer information structure. */
	bzero(&socket_address, sizeof(socket_address)); /* fill with zeros */
	socket_address.sin_family = AF_INET; /* indicate INTERNET address */
	socket_address.sin_port = htons(PORT_NUMBER); /* port in server computer */
	bcopy(host_info->h_addr, &socket_address.sin_addr,
		host_info->h_length); /* copy computer address (should be 4 bytes) */

	/* Connect to the server (blocks until connection established) */
	if (connect(sd, (struct sockaddr *) &socket_address,
		sizeof(socket_address)) == -1)
	{
		perror("tcp_client.c : connect ");	/* report error */
		exit(1);
	}

	/* Communicate with the server */

	/* Send the command length first */
	len = strlen(argv[1]);
	if (write(sd, &len, 4) != 4)
	{
		perror("tcp_client.c : write length ");	/* report error */
		exit(-1);
	}

	/* Now send the command itself */
	if (write(sd, argv[1], len) != len)
	{
		perror("tcp_client.c : write command ");	/* report error */
		exit(-1);
	}

	/* Get the command return code */
	if (read(sd, &ret, 4) != 4)
	{
		perror("tcp_client.c : read return code ");	/* report error */
		exit(-1);
	}
	else
		printf("The command return code was: %d\n", ret);

	/* No errors so far: that's it */
	close(sd);
	return 0;
}
