#include "smtp.h"

/*=====Send a string to the socket=====*/
void Send_Socket(char *s) {
	write(sock,s,strlen(s));
	write(1,s,strlen(s));
}

/*=====Read a string from the socket=====*/
void Read_Socket(void) {
  char buf[BUFSIZ+1];
  int len;

	len = read(sock,buf,BUFSIZ);
	write(1,buf,len);
}

/*=====Sending warning mail=====*/
int Socket_SMTP(char *str) {

	printf("Enviando mail...\n\n");

	/*=====Create Socket=====*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock==-1) {
		perror("opening stream socket");
		return -1;
	}

	/*=====Verify host=====*/
	server.sin_family = AF_INET;
	hp = gethostbyname(HOST_ID);
	if (hp==(struct hostent *) 0) {
		fprintf(stderr, "%s: unknown host\n", HOST_ID);
		free(str);
		return -1;
	}

	/*=====Connect to port 25 on remote host=====*/
	memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
	server.sin_port=htons(25); 	/* SMTP PORT */

	if (connect(sock, (struct sockaddr *) &server, sizeof server)==-1) {
		perror("connecting stream socket");
		free(str);
		return -1;
	}

	/*=====Write some data then read some =====*/
	Read_Socket(); 				/* SMTP Server logon string */
	
	Send_Socket(HELO); 			/* introduce ourselves */
	Read_Socket(); 				/*Read reply */
	
	Send_Socket("MAIL FROM: "); /* Mail from us */
	Send_Socket(FROM_ID);
	Send_Socket("\n");
	Read_Socket(); 				/* Sender OK */

	Send_Socket("RCPT TO: "); 	/*Mail to*/
	Send_Socket(TO_ID);
	Send_Socket("\n");
	Read_Socket(); 				/*Recipient OK*/

	Send_Socket(DATA);			/*body to follow*/
	Read_Socket(); 				/*ok to send */

	Send_Socket(str);
	Send_Socket("\n.\n");		// End

	Read_Socket(); 				/* OK*/
	Send_Socket(QUIT); 			/* quit */
	Read_Socket(); 				/* log off */

	/*=====Close socket and finish=====*/
	close(sock);
	return 0;
}
 
