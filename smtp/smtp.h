/****************************************************************
 * Esta librería abre un socket en el puerto 25 para el envío	*
 * de un mail vía host SMTP (RFC 821).							*
 * Podría adaptarse para enviar datos por otro puerto...		*
 *																*
 * Autor: Matias Bendel							Fecha: 01/07/07	*
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
//#include <sys/types.h>	
// Al incluir esta libreria existen fallas porque colisiona con <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Identificaciones para source & destination
#define HOST_ID "mailtdfcard.ath.cx"
#define FROM_ID "matiasbendel@tdfcard.com"
#define TO_ID "matiasbendel@tdfcard.com"

#define HELO "HELO\n"
#define DATA "DATA\n"
#define QUIT "QUIT\n"

int sock;
struct sockaddr_in server;
struct hostent *hp, *gethostbyname();

void Send_Socket(char *); 
void Read_Socket(void);
int Socket_SMTP(char *);
