/************************************************************************
 * 																		*
 * 								SALUDSRV.C								*
 * 						Programa servidor IVR SALUD						*
 * 																		*
 * 28/09/07	Espera conexión de clientes remotos, que envian el mensaje 	*
 * 			por socket. Espera en puerto expresado como parámetro. 		*
 * 																		*
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <term.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//#define MAXMSGREQ 60
//#define MAXMSGRESP 121
#define MAXLONG 250
#define ENCRYPT 0
#define DECRYPT 1
#define MODEFILE S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH

short logging;
FILE *fpv;
char tmpBuff[250];
char shortBuff[50];

/*
 * Quinto Parametro de toLog:
 * 0- Archivo
 * 1- Archivo y Pantalla
*/

void toLog(FILE *,char *,char *,short,short);

void salir(int);
int Escuchar(int);
int GetMsg(int,char []);
int SendMsg(int,char *,int);
void CloseSocket(int);
int find_field(int, char *);
void operacion(char *, char *);

int main(int argc, char *argv[]) {
 time_t now;
 struct tm z;
 int st;
 int sockfd, portno;
 long pid;
 int clilen, clisockfd;
 struct sockaddr_in cli_addr;
 struct sockaddr_in serv_addr;


 	system("clear");

 	if (argc < 2) {
 		printf ("Faltan Parametros --> ivrsrv<puerto>\n\n");
 		exit(0);
 	}

 	logging=TRUE;
 	fpv = fopen("salida-ivrsrv.txt","a+");
 	if (fpv == NULL) {
 		logging=FALSE;
 		toLog(NULL,NULL,"salida-ivrsrv.txt",0,0);
 	}
 	time(&now);
 	z = *localtime( &now );
 	sprintf(tmpBuff,"INICIA IVRSRV: %02d/%02d - %02d:%02d:%02d\n",
 			z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);
 	toLog(fpv,tmpBuff,NULL,1,1);
 	//Puerto de entrada
 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
 	if (sockfd < 0) {
 		toLog(fpv,"Error en Apertura de Socket\n","socket",1,1);
 		exit(1);
 	}

 	bzero((char *) &serv_addr, sizeof(serv_addr));
 	portno = atoi(argv[1]);
 	serv_addr.sin_family = AF_INET;
 	serv_addr.sin_addr.s_addr = INADDR_ANY;
 	serv_addr.sin_port = htons(portno);
 	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        toLog(fpv,"Error en Bind\n","Bind",1,1);
        exit(1);
 	}

    sprintf(tmpBuff,"Esperando Conexion de IVR Remotos en Puerto %s...\n",argv[1]);
    toLog(fpv,tmpBuff,NULL,1,1);
    
    // El 10 corresponde a la cantidad simultánea de conexiones
    if (listen(sockfd,10)<0) {
        toLog(fpv,"Error en Listen","Listen",1,1); 
	    exit(1);
    }
    while(1) {
    	clilen = sizeof(cli_addr);
        clisockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        if (clisockfd < 0) {
        	toLog(fpv,"Error en Accept\n","Accept",1,1);
        	break;
        }

        sprintf(tmpBuff,"-------- CONEXION ACEPTADA DESDE....%s Socket:%d\n",
        		inet_ntoa(cli_addr.sin_addr),clisockfd);
        toLog(fpv,tmpBuff,NULL,1,1);

        pid = fork();
        if (pid < 0) {
        	toLog(fpv,"Error FORK\n","fork",1,1);
	        continue;
	    }
        
        // Este es el proceso hijo 
	    if (pid==0) {      
	    	// El hijo no necesita el listener
		    if (close(sockfd)==-1) toLog(fpv,"Error Close\n","close",1,1);
            trxtdf(clisockfd);
        } 
	    else {
	    	close(clisockfd);  //El padre no necesita esto
            wait(NULL);
        }
    }
    salir(0);
}

void salir(int ex) {
	exit(0);
}

void FinChild(int sckt) {
	toLog(fpv,"+ + +\n",NULL,1,1);
	shutdown(sckt,SHUT_RDWR);
	close(sckt);
	exit(0);
}

void toLog(FILE * fpv, char *toFile, char *toErr,short flush, short destino) { 
 char tmp[120];

 	if (logging && toFile != NULL){
 		fprintf(fpv,"%s",toFile);
 		if (flush) fflush(fpv);
 	}
 	
 	if (toFile!=NULL && destino == 1)
 		printf("%s",toFile);

 	if (toErr!=NULL) {
 		// Si se envia un mensaje de error, mostrar y loguear el error del sistema.
 		sprintf(tmp,"%s: %s\n",toErr,strerror(errno));
 		printf("%s",tmp);
 		if (logging) {
 			fprintf(fpv,"%s",tmp);
 			if (flush) fflush(fpv);
 		}
 	}
}

int  trxtdf(int sckt) { 
 int k,wr,rd;
 char msg[MAXLONG];
 char msgresp[MAXLONG];
 
    memset(msg,0x00,sizeof(msg));
    memset(msgresp,0x00,sizeof(msgresp));
    
    k=GetMsg(sckt,msg);
    if (k<0) FinChild(sckt);
    memset(tmpBuff,0x00,sizeof(tmpBuff));
    sprintf(tmpBuff,"msg in: %s %d (k=%d)\n",msg,strlen(msg),k);
    toLog(fpv,tmpBuff,NULL,1,1);
    
    operacion(msg,msgresp);    
    
    wr=SendMsg(sckt,msgresp,k);   //k sería la longitud....
    if (wr<0) {
        FinChild(sckt);
    }
    
    if (wr<strlen(msgresp)) {
       sprintf(tmpBuff,"ERROR EN RESPUESTA. %d bytes enviados de %d\n",wr,strlen(msg));
       toLog(fpv,tmpBuff,NULL,1,1);
       FinChild(sckt);
    }

    memset(tmpBuff,0x00,sizeof(tmpBuff));
    sprintf(tmpBuff,"msg out: %s %d (k=%d)\n",msgresp,strlen(msgresp),wr);
    toLog(fpv,tmpBuff,NULL,1,1);
 
    FinChild(sckt);    //return 0;

}

void operacion(char *msg, char *msgresp) {
 int i,pos,pcode;
 char tmp[10];
 time_t currtime;
 struct tm *loctime;
 char aux[8];
 char horaminuto[10], mesdia[10];
	
 	memset(horaminuto,0x00,sizeof(horaminuto));    
 	memset(mesdia,0x00,sizeof(mesdia));
 	currtime = time(NULL);
	loctime = localtime (&currtime);
	// HHMMSS
	strftime(horaminuto,8,"%H%M%S",loctime);
	// MMDD
	strftime(mesdia,8,"%m%d",loctime);
    
	
	// Primero emulo un análisis de los campos, primero los campos comunes a los
    // procesos de autorizacion y anulacion
    // Existe la cabecera?
    if(msg[0]!='&') {
    	toLog(fpv,"Error en el delimitador de cabecera",NULL,1,1);
    	sprintf(msgresp,"&%s|%s||05||%%",horaminuto,mesdia);
    	return;
    }
    // Código de proceso
    memset(tmp,0x00,sizeof(tmp));
    pos=find_field(2,msg);
    tmp[0]=msg[pos];
    tmp[1]=msg[pos+1];
    pcode=atoi(tmp);
    printf("Posicion: %d	Codigo de proceso es: %d\n",pos,pcode);
    
    // Número de prestador
    memset(tmp,0x00,sizeof(tmp));
    pos=find_field(12,msg);
    for(i=0 ; msg[pos+i]!='|' ;++i)
    	tmp[i]=msg[pos+i];

    if (!strcmp(tmp,"20278427200")) {
    	toLog(fpv,"Numero de prestador inválido",NULL,1,1);
		toLog(fpv,tmp,NULL,1,1);
    	sprintf(msgresp,"&%s|%s||01||%%",horaminuto,mesdia);
    	return;
   }
    
   	switch(pcode) {
    case 0:
    	// Número de afiliado
    	memset(tmp,0x00,sizeof(tmp));
    	pos=find_field(4,msg);
	    for(i=0 ; msg[pos+i]!='|' ;++i)
	    	tmp[i]=msg[pos+i];
    	printf("Numero de afiliado recibido: %s\n",tmp);
    			
    	if (!strcmp(tmp,"30133258")) {
    		toLog(fpv,"Numero de afiliado inválido",NULL,1,1);
    		toLog(fpv,tmp,NULL,1,1);
    		sprintf(msgresp,"&%s|%s||02||%%",horaminuto,mesdia);
    		return;
    	}
    	// Código de prestacion
    	memset(tmp,0x00,sizeof(tmp));
    	pos=find_field(6,msg);
	    for(i=0 ; msg[pos+i]!='|' ;++i)
	    	tmp[i]=msg[pos+i];
    	printf("Codigo de prestacion recibido: %s\n",tmp);
    			    	
    	if (!strcmp(tmp,"0123456")) {
    		toLog(fpv,"Numero de afiliado inválido",NULL,1,1);
    		toLog(fpv,tmp,NULL,1,1);
    		sprintf(msgresp,"&%s|%s||03||%%",horaminuto,mesdia);
    		return;
    	}

    	toLog(fpv,"Autorizacion",NULL,1,1);
	    sprintf(msgresp,"&%s|%s|1234|00||%%",horaminuto,mesdia);
    	break;
    	
    case 2:
		// Código de autorización para anulación
		memset(tmp,0x00,sizeof(tmp));
		pos=find_field(3,msg);
	    for(i=0 ; msg[pos+i]!='|' ;++i)
	    	tmp[i]=msg[pos+i];

		printf("Codigo de autorizacion recibido: %s \n",tmp);
		if (strcmp(tmp,"1234") != 0) {
			toLog(fpv,"Código de anulación inválido",NULL,1,1);
			toLog(fpv,tmp,NULL,1,1);
			sprintf(msgresp,"&%s|%s||03||%%",horaminuto,mesdia);
			return;
		}

		toLog(fpv,"Anulacion",NULL,1,1);
	    sprintf(msgresp,"&%s|%s||00||%%",horaminuto,mesdia);
		break;
		
    default:
    	toLog(fpv,"Processing code desconocido",NULL,1,1);
    	sprintf(msgresp,"&%s|%s||05||%%",horaminuto,mesdia);
    }

	return;
}


int GetMsg(int sk,char *msg)
{
  int r,i,st=0;
  time_t toff,ton;

  memset(msg,0x00,MAXLONG);
  usleep(50000);
  r = recv(sk,msg,MAXLONG,0);
  if (r<0) {
     switch(errno){
     case EBADF: sprintf(tmpBuff,"The socket argument is not a valid file descriptor.\n");break;
     case ENOTSOCK: sprintf(tmpBuff,"The descriptor socket is not a socket.\n");break;
     case EWOULDBLOCK: sprintf(tmpBuff,"Nonblocking mode has been set on the socket, and the read operation would block. (Normally, recv blocks until there is input available to be read.)\n");break;
     case EINTR: sprintf(tmpBuff,"The operation was interrupted by a signal before any data was read. See Interrupted Primitives.\n");break;
     case ENOTCONN: sprintf(tmpBuff,"You never connected this socket.\n");break;
     default: sprintf(tmpBuff,"Error Desconocido....\n");
     }
     toLog(fpv,tmpBuff,"GetMsg recv",1,1);
     return -1;
  }
  //Super PARCHE - Algunas veces lee solamente 1 byte!!!
  if (r==1) {
      r=read(sk,msg+1,MAXLONG - 1);
      if (r<0) {
        toLog(fpv,"GetMsg read Error\n","GetMsg read",1,1);
        return -1;
      }
      msg[r+1]=0x00;
  } else msg[r]=0x00;

  sprintf(tmpBuff,"GetMsg:%s (%d bytes)\n",msg,r);
  toLog(fpv,tmpBuff,NULL,1,1);
  return r;
}


int SendMsg(int sk, char *msg,int lmsg)
{int wr;

   /*
   for (wr=0;wr<lmsg;wr++)
   {
     w=write(sk,&msg[wr],1);
     if (w<0) {
	   toLog(fpv,"SendMsg Error!!!\n","write",1,1);
       return -1;
     }   
   }
   sprintf(tmpBuff,"SendMsg:%s (%d bytes)\n",msg,lmsg);
   toLog(fpv,tmpBuff,NULL,1,1);
   */

   wr=send(sk,msg,lmsg,0);
   if (wr<0)  {
	   toLog(fpv,"SendMsg Error!!!\n","write",1,1);
       return -1;
   }
   sprintf(tmpBuff,"SendMsg:%s (%d bytes) (wr=%d)\n",msg,lmsg,wr);
   toLog(fpv,tmpBuff,NULL,1,1);
   return wr;

}

int find_field(int field, char *msg) {
 int i, countpipes=0;
 
     // Cuento hasta 30 que es la máxima logitud posible para los campos
 	for(i=0 ; i<MAXLONG ; ++i) {
		if(msg[i]=='|')
			++countpipes;
		// Se le restan 2 para que coincida con el número de pipes
		if(countpipes==(field-1))
			return ++i;
	}

 	toLog(fpv,"No se encontró código de autorización !!!\n",NULL,1,1);
 	return -1;
}


