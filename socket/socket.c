
#define MAXLONG 		256
#define MAXTIMEOUT		30

int send_sckt_msg(char *, char *, char *);
int connect_socket(char *, char *);
int send_msg(int, char *, int);
int get_msg(int, char *); 
void close_socket(int);


int send_sckt_msg(char *host, char *port, char *msgreq) {
 int gm;
 int sckt,st;
 char msgresp[MAXLONG];
 time_t ton,toff;
 
 	memset(tmpbuff,0x00,sizeof(tmpbuff));
 	
	sckt=connect_socket(host,port);
	if (sckt==-1) return -1;
	
	if (send_msg(sckt,msgreq,strlen(msgreq))==-1) return -1;
       
   	//Deberia esperar unos segundos que se genere la respuesta
   	time(&ton);
  	do {
   		st=0;
   		if (ioctl(sckt,FIONREAD,&st)==-1){
   			printf("Error en IOCTL\n");
   			close_socket(sckt);
   			return -1;
   		}
   		if (st>0) break;
   		time(&toff);
   		if (difftime(toff,ton)>MAXTIMEOUT) {
   			printf("TimeOut Esperando Respuesta!\n");
   			close_socket(sckt);
   			return -1;
   		}
   	} while(1);
    
   	if ((gm=get_msg_australomi(sckt,msgresp,chan))==-1) { 
   		close_socket(sckt);
		printf("Recepción incorrecta!\n");
   		return -1; 
   	}
   	

    return 1;
}


int connect_socket(char *host, char *port) {
 int channo, sockfd;
 struct sockaddr_in serv_addr;
 struct hostent *server;

    channo = atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
         printf("Error 'socket'\n");
         return -1;
    }

    server = (struct hostent *)gethostbyname((char *)host);
    if (server == NULL) {
        printf("Error en Gethostbyname\n");
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(channo);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) == -1) {
        printf("Error en connect\n");
        return -1;
    }
    
    return sockfd;
}


int send_msg(int sk, char *msg, int lmsg) {
  int wr,w;

   	for (wr=0;wr<lmsg;wr++) {  
   		w=write(sk,&msg[wr],1);
   		if (w < 0) {
   			printf("Write error\n");
   			return -1;
   		}
   	}

   	return lmsg;
}

int get_msg(int sk, char *msg) {
  int r;

  	memset(msg,0x00,MAXLONG);
  	usleep(50000);
  	r = recv(sk,msg,MAXLONG-1,0);
  	if (r<0) {
  		printf("Receive error\n");
  		return -1;
  	}
  	msg[r]=0x00;

  	return r;
}


void close_socket(int st) {
  shutdown(st,SHUT_RDWR);
  usleep(100000);
  close(st);
}

