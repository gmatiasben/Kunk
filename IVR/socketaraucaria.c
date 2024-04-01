#include "saludcli.h"

/************************************************************************************
 *								SOCKETS	ARAUCARIA									*
*************************************************************************************/

int send_araucaria(char *msg, int chan) {
    int sockfd, i=0;
    char host[32], port[8];
    FILE *fp;
 
    memset(host,0x00,strlen(host));
    memset(port,0x00,strlen(port));

    // Para que no tenga el archivo el cliente ... si se descomenta crear variables 
    fp=fopen(ARAUCARIASOCKETFILE,"r");
    if (fp == NULL){
       toLog(fpv,"Error al abrir archivo de araucaria",NULL,1,chan,1);
       return -1;
    }
    else {
       fscanf(fp,"%s %s",host,port);
       fclose(fp);
    }

    toLog(fpv,"** Inicio transaccion ARAUCARIA\n",NULL,1,chan,1);
    //sockfd = connect_socket_araucaria(ARAUCARIAHOST,ARAUCARIAPORT,chan);
    sockfd = connect_socket_araucaria(host,port,chan);
     
 	if (sockfd == -1) return -1;
 	
    memset(tmpbuff,0x00,sizeof(tmpbuff));

    /*sprintf(tmpbuff,"Cliente conectado por Socket: %d\n",sockfd);
    toLog(fpv,tmpbuff,NULL,0,chan,0);*/

    if (send_msg_araucaria(sockfd,msg,strlen(msg),chan)== -1){
    	close_socket_araucaria(sockfd);
       return -1;
    }

    memset(msg,0x00,sizeof(msg));
    if (get_msg_araucaria(sockfd,msg,chan)!= -1){
        if (strncmp(msg,"KO",2)==0) {
            toLog(fpv,NULL,"Recibio KO del server....\n",1,chan,1);
            close_socket_araucaria(sockfd);
            return -1;
        }
    }
    close_socket_araucaria(sockfd);

    toLog(fpv,"* Fin transaccion ARAUCARIA\n",NULL,1,chan,1);

    return 1;
}

/*
FUNCION: ConnectSocketAraucaria
Parámetros: char *host: Host al cual conectarse
            char *puerto: Puerto en el cual está escuchando el socket.
Retorna: -1 Error
          Nro de Socket
*/
int connect_socket_araucaria(char *host, char *puerto, int chan) {
 int portno;
 struct sockaddr_in serv_addr;
 struct hostent *server;
 int sockfd;
 long arg;
 fd_set fds;
 struct timeval timeout;
 int ret, retconn;

 	memset(tmpbuff,0x00,sizeof(tmpbuff));
    /*toLog(fpv,"------> ConnectSocket\n",NULL,1,chan,1);
    sprintf(tmpbuff,"host:%s puerto:%s\n",host,puerto);
    toLog(fpv,tmpbuff,NULL,1,chan,1);*/
        
    timeout.tv_sec=1;
    timeout.tv_usec=0;

    portno = atoi(puerto);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        toLog(fpv,NULL,"Error al abrir el Socket\n",1,chan,1);
        return -1;
    }

    server = (struct hostent *)gethostbyname((char *)host);

    if (server == NULL) {
        toLog(fpv,NULL,"Error en gethostbyname\n",1,chan,1);
        return -1;
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    
    // Set non-blocking 
    arg = fcntl(sockfd, F_GETFL, NULL); 
    arg |= O_NONBLOCK; 
    fcntl(sockfd, F_SETFL, arg); 
    //fcntl(sockfd,F_SETFL,O_NONBLOCK);

    retconn=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (retconn < 0) {
       switch( errno) {
          case EINPROGRESS:    
            FD_ZERO(&fds);
            FD_SET(sockfd, &fds);
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;

            ret = select(sockfd+1, NULL, &fds, NULL, &timeout);
            if (ret > 0)
            {   if (FD_ISSET(sockfd,&fds)){
                    FD_CLR(sockfd,&fds);
                    // Set to blocking mode again...
                    arg = fcntl(sockfd, F_GETFL, NULL);
		            arg &= (~O_NONBLOCK);
	                fcntl(sockfd, F_SETFL, arg);
                    return sockfd;
                }
		        toLog(fpv,NULL,"Error FD_ISSET",1,chan,1);
            }
	        else toLog(fpv,NULL,"Error select",1,chan,1);
	        break;
         default:
            toLog(fpv,NULL,"Error connect",1,chan,1);
       }
    } 
    else {
       /*sprintf(tmpbuff," CONNECT Socket: %d\n",sockfd);
       toLog(fpv,tmpbuff,NULL,0,chan,0);*/
       // Set to blocking mode again... 
       arg = fcntl(sockfd, F_GETFL, NULL); 
       arg &= (~O_NONBLOCK); 
       fcntl(sockfd, F_SETFL, arg); 
       return sockfd;
    }

    return -1;
}



/*****************************************************************************
FUNCION: SendMsgAraucaria
Parámetros: int sk: Socket
            char *msg: Mensaje a Enviar
            int lmsg: Long. del mensaje a enviar
Retorna: -1 Error
          0 OK
*****************************************************************************/
int send_msg_araucaria(int sk, char *msg, int lmsg, int chan) {
 int wr;

 	wr=send(sk,msg,lmsg,0);
 	if (wr<0)  {
 		toLog(fpv,NULL,"SendMsg Error!!!\n",1,chan,1);
 		return -1;
 	}

 	memset(tmpbuff,0x00,sizeof(tmpbuff));
 	sprintf(tmpbuff,"MESSAGE (%d bytes):\n%s\n",lmsg,msg);
 	toLog(fpv,tmpbuff,NULL,0,chan,0);

 	return wr;
}

/**************************************************
FUNCION: GetMsgAraucaria
Parámetros: int sk: Socket
            char *msg: Donde se recibirá el mensaje
Retorna: -1 Error
          0 OK
**************************************************/
int get_msg_araucaria(int sk,char *msg, int chan) { 
 int r;
 
 	memset(tmpbuff,0x00,sizeof(tmpbuff));
  	//toLog(fpv,"--> GetMsg\n",NULL,0,chan,0);

  	memset(msg,0x00,MAXLONG);
  	usleep(50000);
  	r = recv(sk,msg,MAXLONG,0);
  	if (r<0) {
  		switch(errno){
  		case EBADF: sprintf(tmpbuff,"The socket argument is not a valid file descriptor.\n");break;
  		case ENOTSOCK: sprintf(tmpbuff,"The descriptor socket is not a socket.\n");break;
  		case EWOULDBLOCK: sprintf(tmpbuff,"Nonblocking mode has been set on the socket, and the read operation would block. (Normally, recv blocks until there is input available to be read.)\n");break;
  		case EINTR: sprintf(tmpbuff,"The operation was interrupted by a signal before any data was read. See Interrupted Primitives.\n");break;
  		case ENOTCONN: sprintf(tmpbuff,"You never connected this socket.\n");break;
  		default: sprintf(tmpbuff,"Error Desconocido....\n");
  		}
  		toLog(fpv,tmpbuff,"GetMsg recv",1,chan,1);
  		return -1;
  	}

  	//Super PARCHE - Algunas veces lee solamente 1 byte!!!
  	if (r==1) {
  		r=read(sk,msg+1,MAXLONG - 1);
  		if (r<0) {
  			toLog(fpv,NULL,"Read Error\n",1,chan,1);
  			return -1;
  		}
  		msg[r+1]=0x00;
  	} 
  	else 
  		msg[r]=0x00;
  
  	sprintf(tmpbuff,"ANSWER (%d bytes):\n%s\n",r,msg);
  	toLog(fpv,tmpbuff,NULL,0,chan,0);
  	return r;
}

/**************************************************
FUNCION: CloseSocketAraucaria
Parámetros: int st: Socket

**************************************************/
void close_socket_araucaria(int st) {
	shutdown(st,SHUT_RDWR);
	usleep(100000);
	close(st);
}

