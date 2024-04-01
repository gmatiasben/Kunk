#include "saludcli.h"

/************************************************************************************
 *								SOCKETS	AUSTRALOMI									*
*************************************************************************************/

int send_australomi(short chan, char *msg, char *inicio, char *final) {
 int gm;
 int sckt,st;
 char msgresp[MAXLONG];
 char fecha[16], hora[16];
 time_t ton,toff,treq,tresp;
 struct tm z1, z2;
 char timetr[5];
 
 	toLog(fpv,"** Inicio transaccion AUSTRAL O.M.I.\n",NULL,1,chan,1);
 	memset(tmpbuff,0x00,sizeof(tmpbuff));
 	
    // Guardo la hora y fecha de inicio de transacción. Envio mensaje !
    time(&treq);
	z1 = *localtime(&treq);
	sprintf(inicio,"%04d%02d%02d%02d%02d%02d",z1.tm_year+1900,z1.tm_mon+1,z1.tm_mday,z1.tm_hour,z1.tm_min,z1.tm_sec);
	sprintf(fecha,"%02d/%02d/%04d",z1.tm_mday,z1.tm_mon+1,z1.tm_year+1900);
	sprintf(hora,"%02d:%02d:%02d",z1.tm_hour,z1.tm_min,z1.tm_sec);

	if ( (sckt=connect_server(chan)) == -1 ) {
 		contador(fecha,hora,canal[chan].msg_prestador,"Error en socket",msg,"0");
 		return -1;
 	}
	//if( (sckt=connect_socket_australomi("200.51.94.37","44444",chan)) == -1 ) return -1;

	if (send_msg_australomi(sckt,msg,strlen(msg),chan)==-1) {
 		contador(fecha,hora,canal[chan].msg_prestador,"Error en Requerimiento",msg,"0");
 		return -1;
	}
       
   	//Deberia esperar unos segundos que se genere la respuesta
   	time(&ton);
  	do {
   		st=0;
   		if (ioctl(sckt,FIONREAD,&st)==-1){
   			toLog(fpv,"Error en IOCTL\n",NULL,1,chan,1);
   			close(sckt);
   			return -1;
   		}
   		if (st>0) break;
   		time(&toff);
   		if (difftime(toff,ton)>MAXTIMEOUT) {
   			toLog(fpv,"TimeOut Esperando Respuesta!\n",NULL,1,chan,1);
   			close(sckt);
   			//ATENCION:
   			//Enviar la anulacion de la trx anterior, ya que no se obtuvo la respuesta.....
   	 		contador(fecha,hora,canal[chan].msg_prestador,"Error en Respuesta",msg,"0");
   			return -1;
   		}
   	} while(1);
    
  	// memset(msgcr,0x00,sizeof(msgcr));
      
   	if ((gm=get_msg_australomi(sckt,msgresp,chan))==-1) { 
   		close(sckt);
		toLog(fpv,"Recepción incorrecta!\n",NULL,1,chan,1);
	 	contador(fecha,hora,canal[chan].msg_prestador,"Error en Respuesta",msg,"0");
   		return -1;
   	}

   	// Guardo la hora y fecha de fin de transacción. Recibo respuesta !
    time(&tresp);
	z2 = *localtime(&tresp);
	sprintf(final,"%04d%02d%02d%02d%02d%02d",z2.tm_year+1900,z2.tm_mon+1,z2.tm_mday,z2.tm_hour,z2.tm_min,z2.tm_sec);
 
	memset(timetr,0x00,sizeof(timetr));
	sprintf(timetr,"%2d\0",(z2.tm_min*60+z2.tm_sec)-(z1.tm_min*60+z1.tm_sec));
	
	// IMchanANTISIMO.... QUEDA EL RESULTADO EN LA VARIABLE GLOBAL!!!!
   	memset(answer[chan].respu,0x00,sizeof(answer[chan].respu));
   	strcpy(answer[chan].respu,msgresp);

 	toLog(fpv,"* Fin transaccion AUSTRAL O.M.I.\n",NULL,1,chan,1);
	contador(fecha,hora,canal[chan].msg_prestador,"OK",msg,timetr);
    
	return 1;
}

int connect_server(int chan) {
 MYSQL *gConn;
 MYSQL_ROW lRow; 
 MYSQL_RES *lRes;
 //MYSQL_RES *lRes=(MYSQL_RES*)malloc(sizeof(MYSQL_RES));
 char lSql[64];
 int i,sckt;
 int totalHosts;
 int actualHost;
 int nroIntentos;
 char nbrehost [10];
 char priority[MAXHOSTS];
 char host[MAXHOSTS][30];
 char port[MAXHOSTS][6];

 	if ( (gConn=connect_db(AUSTRALOMIDBFILE)) == NULL )
 		return -1;
	for (i=0 ; i<MAXHOSTS ; i++) {
		memset(host[i],0x00,sizeof(host[i]));
		memset(port[i],0x00,sizeof(port[i]));
	}
	memset(priority,0x00,sizeof(priority));
	memset(tmpbuff,0x00,sizeof(tmpbuff));
 	//toLog(fpv,"Busqueda IP del servidor completa\n",NULL,1,chan,1);

	memset(lSql,0x00,sizeof(lSql));
	sprintf(lSql,"select * from serveraomi");

	if (mysql_query(gConn,lSql) == -1) {
		sprintf(tmpbuff,"Error en mysql_query %s \n",mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,chan,1);
		mysql_close(gConn);
		return -1;
	}

	if ( !(lRes= mysql_store_result(gConn)) ) {
		sprintf(tmpbuff,"Error en mysql_store_result %s \n",mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,chan,1);
		mysql_close(gConn);
		return -1;
	}

	memset(tmpbuff,0x00,sizeof(tmpbuff));
	totalHosts = mysql_num_rows(lRes); 
	if (totalHosts==0) {
		sprintf(tmpbuff,"No se encontraron los hosts de '%s' \n",nbrehost);
		toLog(fpv,tmpbuff,NULL,1,chan,1);
		mysql_free_result(lRes);
		mysql_close(gConn);
		return -1;
	}
	actualHost=1;        
	while (lRow = mysql_fetch_row(lRes)){
		// '+' = predeterminado => lo guardo en el primer lugar
		if (lRow[0][0]=='+') {
			priority[0]=lRow[0][0];
			strcpy(host[0],lRow[1]);
			strcpy(port[0],lRow[2]);               
		}
		// 'o' = offline => no lo guardo
		else if (lRow[0][0]=='o') {}
		// El resto lo guardo en caso de que el predeterminado no funcione
		else {
			priority[actualHost]=lRow[0][0];
			strcpy(host[actualHost],lRow[1]);
			strcpy(port[actualHost],lRow[2]);               
			actualHost++;
		}
		//sprintf(tmpbuff,"Host:%s Puerto:%s \n",host[actualHost],port[actualHost]);
		//toLog(fpv,tmpbuff,NULL,1,chan,1);
	}
	mysql_free_result(lRes);
	mysql_close(gConn);
	
	// Si el numero de intentos es mayor que el numero de hosts voy a intentar conectarme
	// con alguno de los host + de una vez ... para esto este cliclo.
	if (MAXINTENTOSHOSTS > actualHost+1)
		nroIntentos = actualHost+1;
	else
		nroIntentos = MAXINTENTOSHOSTS;

	/* printf("\n--------------------------\n");
	for (i=0 ; i <= actualHost+1 ; ++i)
		printf("%c %s %s\n",priority[i],host[i],port[i]);
	printf("--------------------------\n");*/
	
	// Implemento dos métodos ... secuencial con predeterminado o al azar
	// 1. SECUENCIAL
	sckt=connect_socket_australomi(host[0],port[0],chan);
	if( sckt == -1) {
		for ( i=1 ; (i <= nroIntentos) && (sckt == -1) ; i++ ){
			sckt=connect_socket_australomi(host[i],port[i],chan);
			if (sckt==-1) {
				--nroIntentos;
				if (nroIntentos == 0) {
					sprintf(tmpbuff,"Se alcanzó el máximo número de intentos\n");
					toLog(fpv,tmpbuff,NULL,1,0,1);
				} 
			}
		}
	}
	
	// 2. RANDOM
	/* int azar, buffazar[MAXHOSTS];
	  memset(buffazar,0x00,sizeof(buffazar));
	  for ( i=1 ; (i <= nrointentos) && (sckt == -1) ; i++ ){
		azar = -1;
		while (azar == -1) {
			srand(time(0));
			azar=rand()%(actualhost+1);
			if (buffazar[azar] == 0)
				buffazar[azar]=1;
			else
				azar = -1;
		}
		sckt=ConnectSocket(host[azar],port[azar]);
		if (sckt==-1) {
			--nroIntentos;
			if (nroIntentos == 0) {
				sprintf(tmpbuff,"Se alcanzó el máximo número de intentos\n");
				toLog(fpv,tmpbuff,NULL,1,0,1);
			} 
		}
	} */

   return sckt;
}


int connect_socket_australomi(char *h, char *p, short chan) {
 int channo, sockfd;
 struct sockaddr_in serv_addr;
 struct hostent *server;

    channo = atoi(p);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
         toLog(fpv,"Error 'socket'",NULL,1,chan,1);
         return -1;
    }

    server = (struct hostent *)gethostbyname((char *)h);
    if (server == NULL) {
        toLog(fpv,"Error en Gethostbyname\n",NULL,1,chan,1);
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(channo);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) == -1) {
        toLog(fpv,"Error en connect\n",NULL,1,chan,1);
        return -1;
    }
    /* sprintf(tmpbuff,"Cliente conectado por Socket: %d\n",sockfd);
    toLog(fpv,tmpbuff,NULL,1,chan,1); */
    
    return sockfd;

}


int send_msg_australomi(int sk,char *msg,int lmsg,short chan) {
  int wr,w;

   	for (wr=0;wr<lmsg;wr++) {  
   		w=write(sk,&msg[wr],1);
   		if (w < 0) {
        // sprintf(tmpbuff,"write: %s",strerror(errno));
   		toLog(fpv,NULL,"write",1,chan,1);
        return -1;
   		}
   	}
 	memset(tmpbuff,0x00,sizeof(tmpbuff));
    sprintf(tmpbuff,"MESSAGE (%d bytes):\n%s\n",lmsg,msg);
   	toLog(fpv,tmpbuff,NULL,0,chan,0);

   	return lmsg;
}

int get_msg_australomi(int sk,char *msg,short chan) {
  int r;
  // int i,st=0;

  	memset(msg,0x00,MAXLONG);
  	usleep(50000);
  	r = recv(sk,msg,MAXLONG-1,0);
  	if (r<0) {
  		//sprintf(tmpbuff,"GetMsg: read recv %s\n",strerror(errno));
  		toLog(fpv,NULL,"recv",1,chan,1);
  		return -1;
  	}
  	msg[r]=0x00;
  	//sprintf(tmpbuff,"GetMsg: %s (%d bytes)",msg,r);
    
  	memset(tmpbuff,0x00,sizeof(tmpbuff));
    sprintf(tmpbuff,"ANSWER (%d bytes):\n%s\n",r,msg);
   	toLog(fpv,tmpbuff,NULL,0,chan,0);

  	return r;
}


void close_socket_australomi(int st) {
  shutdown(st,SHUT_RDWR);
  usleep(100000);
  close(st);
}


