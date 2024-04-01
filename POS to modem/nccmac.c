/*******************************************************************************

PROGRAMA: nccmac.c
EJECUTABLE: nccmac
Utiliza Archivo de Parámetros "params.cfg" para la activación de Cliente.

- Modificacion que envia Vacio el Campo del TRACK1 a Servidor Externo, por ingreso manual.
- Modificacion que envia Vacio el CUIT a Serv. Externo, en caso de pasar dos veces la tarjeta del afiliado, 
- Modificacion que muestra el tiempo transcurrido de cada trx, en pantalla.

26/09/06 - Ultima Modificacion PSEUDO-UNIFICACION DE LOS DISTINTOS NAKS.
26/09/06 - Modificacion para que agregue tiempo de cada transaccion en totales.txt (SECTOR SALUD)
18/10/06 - Modificacion que envía Vacío el CUIT a Serv. Externo, ingresar manualmente dos veces la tarjeta del afiliado
           o que tenga errores de grabación el track1.

19/07/07 - Modificacion de los nombres de los archivos MsgReq y MsgResp con la
           fecha y hora.
           Registro del momento en que se llama a ext y cuando vuelve.
           No registra el tiempo de transaccion en /salud/totales.txt (No compartir más el
           archivo entre los dos...)           
           
20/11/07 - Eliminación de las variables globales...

27/11/08 - Se quitan los accesos a la base de datos. 

02/12/08 - Se reemplaza la generacion del archivo 'entra' y la espera del archivo 'sale' por 
           llamado a hypmodem.int con los nombres de los archivos entra/sale como parametros. 
	   Estos nombres tienen la forma:
	   	   entra-[numero de terminal] y se espera el archivo sale con el mismo numero de terminal.
           
*******************************************************************************/

#include "/usr/lib/comunes/mystrings.c"
#include "/usr/lib/comunes/strext.h"
#include "/usr/lib/comunes/funext.c"
#include "/usr/lib/comunes/strnaks.h"

#include "/usr/lib/comunes/mysocks.c"
#include "/usr/lib/comunes/funnaks.c"

#include "modem.c"

//Prototipos

int readFromMAC(char *,MSG *msg);
int writeToMAC(char *disp,MSG *msg);

int analisisMAC(char *);
int analisis(MSG *);
int r110(MSG *);
int r110Err(MSG *);
int r210(MSG *);
int r210Err(MSG *);
int r410(MSG *);
int r410Err(MSG *);
int r510(MSG *);
int r810(MSG *);
void readbuf(char *, char *, int );

void showResp(MSG *);
void showGenTr(MSG *);
void show500tr(MSG *);
void writeLog(MSG *);
void RespuestaError(MSG *);

void fillBuf();
void quitOK(int);

//Funciones para Host externo
int r110A(MSG *);
int r410A(MSG *);
int r110AErr(MSG *);
int r410AErr(MSG *);

/*int uploadOutxSock(MSG *);
int downloadxSock(MSG *msg);
int downloadOutxSock(MSG *);
int trxRemota(MSG *);
char *armaListaPrestaciones(MSG *);
*/

int main (int argc, char *argv[]) 

{ int first=1;					// Solo se usa para configurar el modem cuando la aplicacion se inicia
  struct sigaction handler;		// Signal handler specification structure
  
     //Agregado para evitar los defunct de los hijos.
     signal(SIGCHLD, SIG_IGN);

	if(argc == 1 || argc > 2 || (!strcmp(argv[1],"-?")) || (!strcmp(argv[1],"/?")) ||
	(strncmp(argv[1],FLAG1,sizeof(FLAG1)-1))) {
		printf("Version 1.1 - Para mayor información abrir README\n"); 
		exit(0);
	}

	iport=(int)(argv[1][sizeof(FLAG1)-1])-ITOA;
	// Los puertos serie se mapean en Linux desde ttyS0 al ttyS5  
	if(iport < 1 || iport > 6) {
		printf("El puerto especificado no existe\n");
		exit(-1);
	}

  	handler.sa_handler =  intr_hdlr;
  	if (sigfillset(&handler.sa_mask) < 0) end_app();
  	handler.sa_flags = 0;
  	if (sigaction(SIGINT, &handler, 0) < 0)	end_app();
	
  	if (Start_Module() < 0)
  		exit(1);
  	
	while(1)	{
		// Limpio los buffers
		Reset_All();
		// Comienzo un ciclo para una nueva transacción	
		if(Call_Set(first) < 0) mLog(flog,"Finalizando lectura de transacción...",1,0);
		else Analyze_Msg();
		Print_End();
		if(first) first=0;
	}

	exit(0);
} 


/*
 * FUNCION: analisisMAC
 * 
 * 
 * */ 

int analisisMAC(char *mensaje)
{
  MSG msg; 	
	
    readFromMAC(mensaje,&msg);
  
    if (analisis(&msg)!=-1) {
      writeToMAC(mensaje,&msg);
    }
    
  return 0;  
}


int readFromMAC(char *disp,MSG *msg)
{
	
  int crc,lfd;
  char a,b,ax;
  char *strcrc = (char *) malloc (sizeof(char)*8);
  char *stra = (char *) malloc (sizeof(char)*3);
  char *strb = (char *) malloc (sizeof(char)*3);
  struct tm z;
  char lstmpBuff[250];
  char lsshortBuff[25];
 
  idxbuf=0;
 		 
	    //Indico en un campo del mismo registro, el modo de ingreso NAC, ING, IP
	 	msg->tipoEntrada = MAC;
	 	//Limpio campos que se setearán de acuerdo a la banda que se deslizó.
	 	memset(msg->request,0x00,sizeof(msg->request));
	 	memset(msg->answer,0x00,sizeof(msg->answer));
	 	memset(msg->msgext.hostDestino,0x00,sizeof(msg->msgext.hostDestino));
	 		 	
	 	msg->config.PideCantidad = FALSE;
	 	msg->config.PideImportes = FALSE;
	 	msg->config.PideNomencla = FALSE;
	   
	 	msg->tipoMensaje = -1;
	 	
	    //Comienza el analisis de lo leido
	 	msg->indiso=0;
	    time(&msg->timeon);

	    //Comienza el analisis de lo leido
	    z = *localtime( &msg->timeon );
	    sprintf(lstmpBuff,"\nNUEVO ANALISIS: %02d/%02d - %02d:%02d:%02d\n",z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);
	    toLog(lstmpBuff,NULL,0,0);

	    gettimeofday(&msg->tim1, NULL);

	    readbuf(disp,&a,1);
	    msg->iso[msg->indiso++]=a;
	    
	    readbuf(disp,&b,1);
	    msg->iso[msg->indiso++]=b;

	    crc = toascii(a) ^ toascii(b);
	    stra = hexar(a,2);
	    strb = hexar(b,2);
	    strcpy(strcrc,stra);
	    strcat(strcrc,strb);
	    msg->longmens = 0;
	    msg->longmens = decimar(strcrc);
	    free(stra);free(strb);free(strcrc);

	    sprintf(lstmpBuff,"Longmens: %d\n",msg->longmens);
	    toLog(lstmpBuff,NULL,1,0);

	    if (msg->longmens > 512) {
	         toLog("Long. de Mensaje Exagerada (> 512)",NULL,0,0);
	         return -1;
	    }
	    //Llena mensnac

	   readbuf(disp,msg->mensnac,msg->longmens);

	   for (msg->pmens=0;msg->pmens<msg->longmens;msg->pmens++){
	       ax=msg->mensnac[msg->pmens];
	       msg->iso[msg->indiso++]=ax;
	       sprintf(lsshortBuff,"%c",ax);
	       toLog(lsshortBuff,NULL,0,0);
	   }
	   msg->iso[msg->indiso]=0x00;
	   toLog("\n",NULL,0,0);

	   lfd=open("mensaje-mac.txt",PARAMSFILE,MODEFILE);
	   if (lfd!=-1) {
	      write(lfd,"MSGIN: ",7);
	      write(lfd,msg->iso,msg->indiso);
	      write(lfd,"\n",1);
	      close(lfd);
	   }

	   toLog(" - Fin Lectura de Mensaje -\n",NULL,0,0);
      
	   return 0;

}  
  
/*
 * FUNCION: analisis
 * DESCRIPCION: Se encarga del analisis del mensaje ingresado
 * 
 * */ 

int analisis(MSG *msg)
{
 int cantbytes;
 struct tm z;
 char lstmpBuff[250];
 char lsshortBuff[25];
 int actr; 
 int resuresp;
 time_t now;
 long pid,pid1;

   msg->pmens = 0;
   if (readTPDU(msg)==-1) return -1;

   if (inputnac(2,1,msg->mtype,msg)==-1) {
      toLog("Error Inputnac - mtype ",NULL,0,0);
      return -1;
   }
   
   //Control del tipo de mensaje que viene
   if (strcmp(msg->mtype,"0100") != 0 &&
		   strcmp(msg->mtype,"0200") != 0 &&
		   strcmp(msg->mtype,"0400") != 0 &&
		   strcmp(msg->mtype,"0500") != 0 &&
		   strcmp(msg->mtype,"0800") != 0 )
   {
	   sprintf(lsshortBuff,"TIPO DE MENSAJE INVALIDO!! --> mtype=%s \n",msg->mtype);
	   toLog(lsshortBuff,NULL,1,1);
	   return -1;
   }

   sprintf(lsshortBuff,"mtype=%s ",msg->mtype);
   toLog(lsshortBuff,NULL,0,0);

   //OJO, controla que sea numero...
   if (esNumero(msg->mtype)==-1){
      toLog("Error mtype - No es número\n",NULL,0,0);
      return -1;
   }

   if (readBitmap(msg)==-1) {
       toLog("Error readBitmap\n",NULL,0,0);
       return -1;
   }

   memset(msg->prac,0x00,sizeof(msg->prac));
   if (msg->bitmap[2]=='1') {
        if (readPrAN(msg)==-1) {
           toLog("Error readPrAN ",NULL,0,0);
	       return -1;
        }
        sprintf(lsshortBuff,"prac=%s ",msg->prac);
        toLog(lsshortBuff,NULL,0,0);
        if (esNumero(msg->prac)==-1) {
           toLog("Error prac - No es numérico\n",NULL,0,0);
           return -1;
        }
        if (strcmp(msg->prac,"")!=0){
     	   //if (DeterminarHostDestino(msg->prac,msg->msgext.hostDestino)==-1){
     		  if (DeterminarHostDestinoFijo(msg->prac,msg->msgext.hostDestino)==-1) {
     			  toLog("ERROR. No se pudo determinar el Host Destino\n",NULL,1,1);
     			  return -1;
     		  }
     	   //}   
        }
        
    }
    memset(msg->prcode,0x00,sizeof(msg->prcode));
    if (msg->bitmap[3]=='1') {
       if (inputnac(3,1,msg->prcode,msg)==-1) {
         toLog("Error Inputnac - prcode\n",NULL,0,0);
         return -1;
       }
       sprintf(lsshortBuff,"prcode=%s ",msg->prcode);
       toLog(lsshortBuff,NULL,0,0);

       if (esNumero(msg->prcode)==-1) {
           toLog("Error prcode - No es numérico\n",NULL,0,0);
           return -1;
       }
    }

    //Primer MONTO (Monto de la venta O A cargo del Afiliado)
    memset(msg->amtr,0x00,sizeof(msg->amtr));
    if (msg->bitmap[4]=='1') {

       if (inputnac(6,1,msg->amtr,msg)==-1) {
         toLog("Error Inputnac - amtr\n",NULL,1,1);
         return -1;
       }
       sprintf(lsshortBuff,"amtr=%s ", msg->amtr);
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->amtr)==-1) {
          toLog("Error 'amtr' - No es numérico\n",NULL,1,1);
          return -1;
       }
    }

    // -------------------------------------------------------
    // Segundo MONTO (A cargo de la Obra Social) Viaja en el Campo5
    memset(msg->amtr2,0x00,sizeof(msg->amtr2));
    if (msg->bitmap[5]=='1') {
    	msg->config.PideImportes = TRUE;
 	    msg->tipoMensaje = LISTA_IMPORTES;    
    	if (inputnac(6,1,msg->amtr2,msg)==-1) {
         toLog("Error Inputnac - amtr2\n",NULL,1,1);
         return -1;
       }
       sprintf(lsshortBuff,"amtr2=%s ", msg->amtr2);
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->amtr2)==-1) {
          toLog("Error 'amtr2' - No es numérico\n",NULL,1,1);
          return -1;
       }
    }
    
    //Tercer MONTO (Monto Total de la Operacion) Viaja en el Campo6
    memset(msg->amtr3,0x00,sizeof(msg->amtr3));
    if (msg->bitmap[6]=='1') {
       if (inputnac(6,1,msg->amtr3,msg)==-1) {
         toLog("Error Inputnac - amtr3\n",NULL,1,1);
         return -1;
       }
       sprintf(lsshortBuff,"amtr3=%s ", msg->amtr3);
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->amtr3)==-1) {
          toLog("Error 'amtr3' - No es numérico\n",NULL,1,1);
          return -1;
       }
    }
    // -------------------------------------------------------

    memset(msg->sytr,0x00,sizeof(msg->sytr));
    if (msg->bitmap[11]=='1') {
       if (inputnac(3,1,msg->sytr,msg)==-1)
       {
	  toLog("Error Inputnac - sytr\n",NULL,0,0);
          return -1;
       }
       sprintf(lsshortBuff,"sytr=%s ", msg->sytr);
       toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->loti,0x00,sizeof(msg->loti));
    if (msg->bitmap[12]=='1') {
        if (inputnac(3,1,msg->loti,msg)==-1)
       {
           toLog("Error Inputnac - loti\n",NULL,0,0);
           return -1;
       }
        sprintf(lsshortBuff,"loti=%s ", msg->loti);
        toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->loda,0x00,sizeof(msg->loda));
    if (msg->bitmap[13]=='1') {
       if (inputnac(2,1,msg->loda,msg)==-1)
       {
           toLog("Error Inputnac - loda \n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"loda=%s ", msg->loda);
       toLog(lsshortBuff,NULL,0,0);
    }

    memset(msg->daex,0x00,sizeof(msg->daex));
    if (msg->bitmap[14]=='1') {
       if (inputnac(2,1,msg->daex,msg)==-1)
       {
           toLog("Error Inputnac - daex\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"daex=%s ", msg->daex);
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->daex)==-1) {
          toLog("Error 'daex' - No es numérico\n",NULL,0,0);
          return -1;
       }
    }
    memset(msg->dase,0x00,sizeof(msg->dase));
    if (msg->bitmap[15]=='1') {
       if (inputnac(2,1,msg->dase,msg)==-1)
       {
           toLog("Error Inputnac - dase\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"dase=%s ", msg->dase);
       toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->poen,0x00,sizeof(msg->poen));
    if (msg->bitmap[22]=='1') {
       if (inputnac(2,1,msg->poen,msg)==-1)
       {
           toLog("Error Inputnac - poen\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"poen=%s ", msg->poen);
       toLog(lsshortBuff,NULL,0,0);
    }

    memset(msg->netid,0x00,sizeof(msg->netid));
    if (msg->bitmap[24]=='1') {
       if (inputnac(2,1,msg->netid,msg)==-1)
       {
           toLog("Error Inputnac - netid\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"netid=%s ", msg->netid);
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->netid)==-1) {
          toLog("Error 'netid' - No es numérico\n",NULL,0,0);
          return -1;
       }
    }

 
    memset(msg->poco,0x00,sizeof(msg->poco));
    if (msg->bitmap[25]=='1') {
       if (inputnac(1,1,msg->poco,msg)==-1)
       {
           toLog("Error Inputnac - poco\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"poco=%s \n", msg->poco);
       toLog(lsshortBuff,NULL,0,0);
    }

    memset(msg->tr2,0x00,sizeof(msg->tr2));
    if (msg->bitmap[35]=='1') {
       if (readTR2(msg)==-1){
           toLog("Error readTR2\n",NULL,0,0);
           return -1;
       }
       if (strcmp(msg->tr2,"")!=0 && strcmp(msg->msgext.hostDestino,"")==0){
     	   //if ( DeterminarHostDestino(msg->tr2,msg->msgext.hostDestino)==-1){
     		  if (DeterminarHostDestinoFijo(msg->tr2,msg->msgext.hostDestino)==-1) {
     			  toLog("ERROR. No se pudo determinar el Host Destino\n",NULL,1,1);
     			  return -1;
     		  }
     	   //}   
       }

       sprintf(lstmpBuff,"tr2=%s \n", msg->tr2);
       toLog(lstmpBuff,NULL,0,0);
    }

    memset(msg->retrefnu,0x00,sizeof(msg->retrefnu));
    if (msg->bitmap[37]=='1') {
       if (inputnac(12,0,msg->retrefnu,msg)==-1)
       {
           toLog("Error Inputnac - retrefnu\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"retrefnu=%s ",msg->retrefnu );
       toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->sauthid,0x00,sizeof(msg->sauthid));
    if (msg->bitmap[38]=='1') {
       if (inputnac(6,0,msg->sauthid,msg)==-1)
       {
           toLog("Error Inputnac - sauthid\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"sauthid=%s ",msg->sauthid );
       toLog(lsshortBuff,NULL,0,0);
    }
    
    //NUEVO IPAUSS - Nomencla
    //if (msg->nomencla!=0x00)    //IPAUSS 
    memset(msg->nomencla,0x00,sizeof(msg->nomencla));
    if (msg->bitmap[40]=='1') {
       msg->config.PideNomencla = TRUE;
       if (inputnac(3,0,msg->nomencla,msg)==-1)
       {
           toLog("Error Inputnac - nomencla\n",NULL,1,1);
           return -1;
       }
       //strcpy(msg->nomencla,"001");
       //strncpy(msg->nomencla,right(msg->nomencla,3),3);
       //msg->nomencla[sizeof(msg->nomencla)-1]=0x00;
       sprintf(lsshortBuff,"nomencla=%s ", msg->nomencla);
       toLog(lsshortBuff,NULL,0,0);
      
       //Si llegó el nomenclador, es IPAUSS
       msg->config.PideNomencla=TRUE;      
    }
    memset(msg->teid,0x00,sizeof(msg->teid));
    if (msg->bitmap[41]=='1') {
       if (inputnac(8,0,msg->teid,msg)==-1)
       {
           toLog("Error Inputnac - teid\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"teid=%s ",msg->teid );
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->teid)==-1) {
          toLog("Error 'teid' - No es numérico\n",NULL,0,0);
          return -1;
       }

    }
    memset(msg->crdac,0x00,sizeof(msg->crdac));
    if (msg->bitmap[42]=='1') {
       if (inputnac(15,0,msg->crdac,msg)==-1)
       {
           toLog("Error Inputnac - crdac\n",NULL,0,0);
           return -1;
       }
       RemoveBlancos(msg->crdac,sizeof(msg->crdac)-1);
       sprintf(lsshortBuff,"crdac=%s ",msg->crdac );
       toLog(lsshortBuff,NULL,0,0);
       if (esNumero(msg->crdac)==-1) {
          toLog("Error 'msg->crdac' - No es numérico\n",NULL,0,0);
          return -1;
       }

    }

    memset(msg->tr1,0x00,sizeof(msg->tr1));
    if (msg->bitmap[45]=='1') {
       cantbytes = readLong(msg);
	   if (inputnac(cantbytes,0,msg->tr1,msg)==-1)
       {
           toLog("Error Inputnac - tr2\n",NULL,0,0);
           return -1;
       }
	   sprintf(lstmpBuff,"bitmap[45]-cantbytes=%d tr1=%s\n",cantbytes,msg->tr1);
       toLog(lstmpBuff,NULL,0,0);
       if (strcmp(msg->tr1,"")!=0 && strcmp(msg->msgext.hostDestino,"")==0){
     	   //if ( DeterminarHostDestino(msg->tr1,msg->msgext.hostDestino)==-1){
     		  if (DeterminarHostDestinoFijo(msg->tr1,msg->msgext.hostDestino)==-1) {
     			  toLog("ERROR. No se pudo determinar el Host Destino\n",NULL,1,1);
     			  return -1;
     		  }
     	   //}   
       }
    }
    memset(msg->cucot,0x00,sizeof(msg->cucot));
    if (msg->bitmap[49]=='1') {
       if (inputnac(3,0,msg->cucot,msg)==-1)
       {
           toLog("Error Inputnac - cucot\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"cucot=%s ",msg->cucot );
       toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->cucos,0x00,sizeof(msg->cucos));
    if (msg->bitmap[50]=='1') {
       if (inputnac(3,0,msg->cucos,msg)==-1)
       {
           toLog("Error Inputnac - cucos\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"cucos=%s ",msg->cucos );
       toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->pin,0x00,sizeof(msg->pin));
    if (msg->bitmap[52]=='1') {
       if (inputnac(8,0, msg->pin,msg)==-1)
       {
           toLog("Error Inputnac - pin\n",NULL,0,0);
           return -1;
       }
       sprintf(lsshortBuff,"pin=%s ",msg->pin );
       toLog(lsshortBuff,NULL,0,0);
    }
    memset(msg->banu,0x00,sizeof(msg->banu));
    if (msg->bitmap[60]=='1') {
       if (inputnac(2,1,msg->talen,msg)==-1)
       {
           toLog("Error Inputnac - talen\n",NULL,0,0);
           return -1;
       }
       //cantbytes=atoi(talen);
       cantbytes = getTalen(msg->talen);
       if (inputnac(cantbytes,0,msg->banu,msg)==-1)
       {
           toLog("Error Inputnac - banu\n",NULL,0,0);
           return -1;
       }
       sprintf(lstmpBuff,"bm60 talen=%s cantbytes=%d banu=%s\n",
    		   msg->talen,cantbytes,msg->banu);
       toLog(lstmpBuff,NULL,0,0);
    }

    memset(msg->campo61,0x00,sizeof(msg->campo61));
    if (msg->bitmap[61]=='1') {
       if (inputnac(2,1,msg->talen,msg)==-1)   //1 o 0???
       {
          toLog("Error Inputnac - talen\n",NULL,0,0);
	      return -1;
       }
       //cantbytes=atoi(talen);
       cantbytes = getTalen(msg->talen);
       msg->longcampo61=cantbytes;
       
       switch(msg->longcampo61){
          case LONG_LISTA_SIMPLE_7:  
    	     msg->config.PideCantidad = FALSE;
    	     if (msg->tipoMensaje == -1) msg->tipoMensaje = LISTA_SIMPLE_7;
    	     break;
          case LONG_LISTA_SIMPLE:  
    	     msg->config.PideCantidad = FALSE;
    	     if (msg->tipoMensaje == -1) msg->tipoMensaje = LISTA_SIMPLE;
    	     break;
          default:
    	     msg->config.PideCantidad = TRUE;
    	     if (msg->tipoMensaje == -1) msg->tipoMensaje = LISTA_COMPUESTA;    
       }

       if (inputnac(cantbytes,0,(char *)msg->campo61,msg)==-1) {
             toLog("Error Inputnac - campo61\n",NULL,0,0);
	     return -1;
       }
       sprintf(lstmpBuff,"bm61 talen=%s cantbytes=%d campo61=%s longcampo61=%d\n",
    		   msg->talen,cantbytes,msg->campo61,msg->longcampo61);
       toLog(lstmpBuff,NULL,0,0);
    }

    memset(msg->innu,0x00,sizeof(msg->innu));
     if (msg->bitmap[62]=='1') {
	   if (inputnac(2,1,msg->talen,msg)==-1) {
         toLog("Error Inputnac - talen\n",NULL,0,0);
	     return -1;
	   }
	   if (inputnac(6,0,msg->innu,msg)==-1) {
         toLog("Error Inputnac - innu\n",NULL,0,0);
	     return -1;
	   }
       if (esNumero(msg->innu)==-1) {
          toLog("Error 'innnu' - No es numérico\n",NULL,0,0);
          return -1;
       }

	   sprintf(lsshortBuff,"innu=%s\n", msg->innu);
       toLog(lsshortBuff,NULL,0,0);
    }

    memset(msg->cove,0x00,sizeof(msg->cove)); memset(msg->imve,0x00,sizeof(msg->imve));
    memset(msg->cocr,0x00,sizeof(msg->cocr)); memset(msg->imcr,0x00,sizeof(msg->imcr));
    memset(msg->r,0x00,sizeof(msg->r));
    memset(msg->numpa,0x00,sizeof(msg->numpa));memset(msg->paypla,0x00,sizeof(msg->paypla));
    if (msg->bitmap[63]=='1') {
	    if (inputnac(2,1,msg->talen,msg)==-1) {
        	 toLog("Error Inputnac - talen-b\n",NULL,0,0);
	         return -1;
	    }
        if (strcmp(msg->mtype,"0500")==0){
             if (inputnac(3,0, msg->cove,msg)==-1) {
        	toLog("Error Inputnac - cove\n",NULL,0,0);
	        return -1;
	     }
	     if (inputnac(12,0, msg->imve,msg)==-1) {
	         toLog("Error Inputnac - imve\n",NULL,0,0);
	         return -1;
	      }
             if (inputnac(3,0, msg->cocr,msg)==-1) {
	         toLog("Error Inputnac - cocr\n",NULL,0,0);
	         return -1;
	      }
             if (inputnac(12,0, msg->imcr,msg)==-1) {
	         toLog("Error Inputnac - imcr\n",NULL,0,0);
	         return -1;
	      }
	     if (inputnac(60,0,msg->r,msg)==-1) {
	         toLog("Error Inputnac - r\n",NULL,0,0);
	         return -1;
	      }
             sprintf(lstmpBuff,"cove=%s imve=%s cocr=%s imcr=%s r=%s \n",
            		 msg->cove,msg->imve,msg->cocr,msg->imcr,msg->r);
             toLog(lstmpBuff,NULL,0,0);
	     } else {
	         cantbytes=atoi(msg->talen);
	        //sprintf(lstmpBuff,"BM63 talen=%s c/atoi cantbytes=%d ",talen, cantbytes);
            //toLog(lstmpBuff,NULL,0,0);

	        //cantbytes = getTalen(talen);
	        //sprintf(lstmpBuff," c/getTalen cantbytes=%d \n",cantbytes);
            //toLog(lstmpBuff,NULL,0,0);
            if (proc63(cantbytes,msg)==-1) {
	        toLog("Error proc63 \n",NULL,0,0);
	        return -1;
	    }	
      }
    }
  toLog(" Check Point\n",NULL,0,0);

  //Si no tiene seteado el hostDestino, por defecto es TDF....
  if (strlen(msg->msgext.hostDestino)==0){
	  toLog("Host destino no seteado... Por Defecto: TDF\n",NULL,1,0);
	  strcpy(msg->msgext.hostDestino,"TDF");
  } 
  
  sprintf(lstmpBuff,"Host Destino: %s\n",msg->msgext.hostDestino);
  toLog(lstmpBuff,NULL,1,1);
  
  //Si no es de TDF y no tiene mensaje, se lo seteo a mano.
  if (strcmp(msg->msgext.hostDestino,"TDF")!=0 && msg->tipoMensaje == -1)   {
	  toLog("Tipo de Mensaje no seteado.. Por defecto: LISTA_SIMPLE_7\n",NULL,1,1);
	  msg->tipoMensaje=LISTA_SIMPLE_7;
  } else {
	  if (strcmp(msg->msgext.hostDestino,"TDF")!=0)
		  sprintf(lstmpBuff,"Tipo de Mensaje: %d\n",msg->tipoMensaje);
  	  else 
  		  sprintf(lstmpBuff,"Mensaje de TDF\n");
	  
	  toLog(lstmpBuff,NULL,1,1);
	  		  
  }	 
  // Definir el tipo de transacción
  if (strcmp(msg->mtype,"0100")==0) {
      if (strcmp(msg->prcode,"310000")==0) {
    	  msg->tipotrx=TRX_CONSULTA;
      } else {
         if (strcmp(msg->prcode,"000000")==0) msg->tipotrx=TRX_AUTORIZACION;
         else msg->tipotrx=TRX_ANULACION_SALUD;
      }
  }
  if (strcmp(msg->mtype,"0200")==0) { 
      msg->tipotrx=TRX_COMPRA; 
   }
  //Atencion
  //Si es un 0400 externo, devolver string fijo solo para eliminar el string del terminal
  if (strcmp(msg->mtype,"0400")==0) {
      if (strcmp(msg->msgext.hostDestino,"TDF")==0) {
    	  msg->tipotrx=TRX_REVERSO;
      } else {
          msg->tipotrx=TRX_REVERSO_SALUD;
      }
  }
  if (strcmp(msg->mtype,"0500")==0) {
      msg->tipotrx=TRX_CIERRE; 
  }
  if (strcmp(msg->mtype,"0800")==0) { 
	  msg->tipotrx=TRX_PRUEBA; 
  }
  //Dar la respuesta según el tipo de transacción
  //Marco inicio de la respuesta
  time(&now);
  z = *localtime( &now );
  sprintf(msg->msgext.fechaini,"%04d%02d%02d",z.tm_year+1900,z.tm_mon+1,z.tm_mday);
  sprintf(msg->msgext.horaini,"%02d%02d%02d",z.tm_hour,z.tm_min,z.tm_sec);
  
  //Nombres de los archivos entra-sale
  memset(msg->entra,0x00,sizeof(msg->entra));
  sprintf(msg->entra,"/tdf/prog/nac/entra-%8s",msg->teid);
  memset(msg->sale,0x00,sizeof(msg->sale));
  sprintf(msg->sale,"/tdf/prog/nac/sale-%8s",msg->teid); 

  switch (msg->tipotrx) {  
  	case TRX_CONSULTA:  		
  		resuresp=r110(msg); break;
  	case TRX_COMPRA:
  		resuresp=r210(msg); break;
  	case TRX_REVERSO:
  		resuresp=r410(msg); break;
  	case TRX_CIERRE:
  		resuresp=r510(msg); break;
  	case TRX_PRUEBA:
  		resuresp=r810(msg); break;
  	case TRX_AUTORIZACION:
  		resuresp=r110A(msg); break;
  	case TRX_ANULACION_SALUD:
  		resuresp=r110A(msg); break;
  	case TRX_REVERSO_SALUD:
  		resuresp=r410A(msg); break;
  	default:
  		toLog("FALTA 'tipotrx'!!\n",NULL,1,1);
  		resuresp=-1;
  }
  
  //Tiempo final de la transaccion....
  time(&now);
  z = *localtime( &now );
  sprintf(msg->msgext.fechafin,"%04d%02d%02d",z.tm_year+1900,z.tm_mon+1,z.tm_mday);
  sprintf(msg->msgext.horafin,"%02d%02d%02d",z.tm_hour,z.tm_min,z.tm_sec);

  //Fin analisis
   gettimeofday(&msg->tim2, NULL);
   msg->lapso = (float)(msg->tim2.tv_sec - msg->tim1.tv_sec) + (float)abs(msg->tim2.tv_usec - msg->tim1.tv_usec) / 1000000;

   sprintf(lstmpBuff,"Tiempo Transcurrido: %.3f\n",msg->lapso);
   toLog(lstmpBuff,NULL,1,1);
   actr += msg->lapso;

   time(&msg->timeoff);
   z = *localtime( &msg->timeoff );
   sprintf(lstmpBuff,"FIN ANALISIS: %02d/%02d - %02d:%02d:%02d\n",z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);
   toLog(lstmpBuff,NULL,0,0);
   
   if (resuresp!=-1){
      switch (msg->tipotrx) {
	  	case TRX_CONSULTA:  		
	  	case TRX_COMPRA:
	  	case TRX_REVERSO:
	  	case TRX_CIERRE:
	  	case TRX_PRUEBA:
		
		           pid1 = fork();
			   if (pid1==-1) {
			      toLog("Fork Error!\n","fork",1,1);
			   }
			   if (!pid1) { // Este es el proceso hijo
			     sprintf(lstmpBuff,"Pid SendDB: %d \n",getpid());
		             toLog(lstmpBuff,NULL,1,1);
		    	     if (SendDB(msg) == -1)
		     	       toLog("Transaccion Financiera NO enviada a Araucaria...\n",NULL,1,1);
		             exit(0);
			   }   	
   			   break;
			   
   	    case TRX_AUTORIZACION:
   	    case TRX_ANULACION_SALUD:
 /*  	      if (SendAraucaria(msg) == -1)
   	          toLog("Transaccion NO enviada a Araucaria...\n",NULL,1,1);
*/
//--------------------------------
          pid = fork();
	      if (pid==-1) {
	          toLog("Fork Error!\n","fork",1,1);
	      }
	      if (!pid) { // Este es el proceso hijo
	        sprintf(lstmpBuff,"Pid: %d \n",getpid());
            toLog(lstmpBuff,NULL,1,1);
    	    if (SendAraucaria(msg) == -1)
     	       toLog("Transaccion NO enviada a Araucaria...\n",NULL,1,1);
            exit(0);
	      }   	      
//---------------------------------   	      
      }
   }
   
/*   if (msg->tipotrx==TRX_REVERSO || msg->tipotrx==TRX_REVERSO_SALUD) continue;
   else break;
   
 } while (1);
 */
 return resuresp;

}  // end analisis

/* FUNCION: r110 
 * Respuesta a las Consultas de Saldo
 * 
 */
int r110(MSG *msg)
 {
  int i, pmant;
  char laux[15];
  char lstempnac[MAXLONG];
  char lstmpBuff[250];
  
  
   toLog("------> r110\n",NULL,1,1);
  
   if (uploadTr(msg)==-1) return -1;
   showGenTr(msg);
   if (downloadTr(msg)==-1) return -1;
   showResp(msg);
   
   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));
   writeTPDU(msg);
   writeBCD("0110",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[4]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]= msg->bitmap[54]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->samtr,12,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR(msg->srespco,2,msg);
   writeSTR(msg->teid,8,msg);
   writeBCD("0012",4,msg);
   sprintf(laux,"00%s",msg->ssahoy);
   writeSTR(laux,12,msg);

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                 //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);          //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t100++;
   writeLog(msg);
   return 0;

 }   //Function R110

/* FUNCION: r110A 
 * Respuesta a las Autorizaciones Externas
 * 
 */
int r110A( MSG *msg)
{
 int i, pmant, desp, p=0;
 char cantPrest[SIZE_CANTIDAD+1];
 char codPrest[SIZE_PRESTACION+1];
 char descrPrest[SIZE_DESCR_PRESTACION+1];
 char laux[15];
 char lstempnac[MAXLONG];
 char lstmpBuff[120];
 int longi;
 char strlong[4+1];
 

   toLog("------> r110A\n",NULL,1,1);
 
   if (uploadOut(msg)==-1) {
        RespuestaError(msg);
        return -1;
   }
   showGenTr(msg);

   //OJO..... ext siempre devuelve un mensaje de respuesta supuestamente!!!!!!
   //Entonces no debería dar ooootra respuesta mas...
   if (downloadOut(msg)==-1){
        RespuestaError(msg);
        return -1;
   }
   showResp(msg);

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   writeTPDU(msg);
   writeBCD("0110",4,msg);

   clearBitmap(msg);
   msg->bitmap[PRCODE]=msg->bitmap[AMTR]=msg->bitmap[SYTR]=msg->bitmap[NETID]='1';
   msg->bitmap[RETREFNU]=msg->bitmap[RESPCO]=msg->bitmap[TEID]='1';
   msg->bitmap[CAMPO61]='1';

   RemoveBlancos(msg->dismsg,sizeof(msg->dismsg));
   if (strcmp(msg->dismsg,"")!=0) {
      msg->bitmap[CAMPO63]='1';
	  rightpad(msg->dismsg,' ',40);
   } 
   //
   if (msg->config.PideImportes) { msg->bitmap[AMTR2]='1';msg->bitmap[AMTR3]='1';}      
   if (msg->config.PideNomencla) msg->bitmap[NOMEN]='1';
   
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   if (msg->bitmap[AMTR]=='1') writeBCD(msg->amtr,12,msg);
   if (msg->bitmap[AMTR2]=='1') writeBCD(msg->samtr2,12,msg);
   if (msg->bitmap[AMTR3]=='1') writeBCD(msg->samtr3,12,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->srespco,2,msg);
   if (msg->bitmap[NOMEN]=='1') writeSTR(msg->nomencla,3,msg);
   writeSTR(msg->teid,8,msg);
   
   if (msg->bitmap[CAMPO61]=='1') {
	  switch (msg->tipoMensaje) {
	    case LISTA_SIMPLE_7:
	       longi = (SIZE_PRESTACION_7 + SIZE_DESCR_PRESTACION) * MAX_PRESTACIONES; 
	  	   sprintf(strlong,"%04d",longi);
	  	   writeBCD(strlong,4,msg);
           for (p=0; p<MAX_PRESTACIONES; p++)
           {
            memset(codPrest,0x00,sizeof(codPrest));
            memset(descrPrest,0x00,sizeof(descrPrest));
            RemoveBlancos(msg->msgext.prestacs[p].Prestacion,SIZE_PRESTACION_7);
            RemoveBlancos(msg->msgext.prestacs[p].Descripcion,SIZE_DESCR_PRESTACION);
                          
            memcpy(codPrest,leftpad(msg->msgext.prestacs[p].Prestacion,'0',SIZE_PRESTACION_7),SIZE_PRESTACION_7);
            if (strcmp(msg->msgext.prestacs[p].Descripcion,"")==0) 
          	  memcpy(descrPrest,blancos(SIZE_DESCR_PRESTACION),SIZE_DESCR_PRESTACION);
            else 
          	  sprintf(descrPrest,"%-*s",SIZE_DESCR_PRESTACION,msg->msgext.prestacs[p].Descripcion);

            sprintf(lstmpBuff,"Cod:%s Val:%s\n",codPrest,descrPrest);
            toLog(lstmpBuff,NULL,1,1);

            writeSTR(codPrest,SIZE_PRESTACION_7,msg);
            writeSTR(descrPrest,SIZE_DESCR_PRESTACION,msg);
            desp+=(SIZE_PRESTACION_7+SIZE_DESCR_PRESTACION);
           }
           break;
	    case LISTA_SIMPLE:
  	       longi = (SIZE_PRESTACION + SIZE_DESCR_PRESTACION) * MAX_PRESTACIONES; 
	  	   sprintf(strlong,"%04d",longi);
	  	   writeBCD(strlong,4,msg);
           for (p=0; p<MAX_PRESTACIONES; p++)
           {
              memset(codPrest,0x00,sizeof(codPrest));
              memset(descrPrest,0x00,sizeof(descrPrest));
              RemoveBlancos(msg->msgext.prestacs[p].Prestacion,SIZE_PRESTACION);
              RemoveBlancos(msg->msgext.prestacs[p].Descripcion,SIZE_DESCR_PRESTACION);
                            
              memcpy(codPrest,leftpad(msg->msgext.prestacs[p].Prestacion,'0',SIZE_PRESTACION),SIZE_PRESTACION);
              if (strcmp(msg->msgext.prestacs[p].Descripcion,"")==0) 
            	  memcpy(descrPrest,blancos(SIZE_DESCR_PRESTACION),SIZE_DESCR_PRESTACION);
              else 
            	  sprintf(descrPrest,"%-*s",SIZE_DESCR_PRESTACION,msg->msgext.prestacs[p].Descripcion);

              sprintf(lstmpBuff,"Cod:%s Val:%s\n",codPrest,descrPrest);
              toLog(lstmpBuff,NULL,1,1);

              writeSTR(codPrest,SIZE_PRESTACION,msg);
              writeSTR(descrPrest,SIZE_DESCR_PRESTACION,msg);
              desp+=(SIZE_PRESTACION+SIZE_DESCR_PRESTACION);
         }
         break;
	     case LISTA_IMPORTES:
	     case LISTA_COMPUESTA:
	    	 
	    	  toLog("Lista Compuesta\n",NULL,1,1);
	    	 
              //Son 245 bytes (CANTIDAD x PRESTACION)
	  	      longi = (SIZE_CANTIDAD + SIZE_PRESTACION + SIZE_DESCR_PRESTACION) * MAX_PRESTACIONES; 
	  		  sprintf(strlong,"%04d",longi);
	  	      writeBCD(strlong,4,msg);
	  	      desp=0;
              for (p=0; p<MAX_PRESTACIONES; p++)
              {
                  memset(cantPrest,0x00,sizeof(cantPrest));
                  memset(codPrest,0x00,sizeof(codPrest));
                  memset(descrPrest,0x00,sizeof(descrPrest));
                  
                  RemoveBlancos(msg->msgext.prestacs[p].Cantidad,SIZE_CANTIDAD);
                  RemoveBlancos(msg->msgext.prestacs[p].Prestacion,SIZE_PRESTACION);
                  RemoveBlancos(msg->msgext.prestacs[p].Descripcion,SIZE_DESCR_PRESTACION);		  
                  
                  strcpy(cantPrest,leftpad(msg->msgext.prestacs[p].Cantidad,'0',SIZE_CANTIDAD));
                  strcpy(codPrest,leftpad(msg->msgext.prestacs[p].Prestacion,'0',SIZE_PRESTACION));
                                    
                  if (strcmp(msg->msgext.prestacs[p].Descripcion,"")==0) memcpy(descrPrest,blancos(SIZE_DESCR_PRESTACION),SIZE_DESCR_PRESTACION);                 
                  else sprintf(descrPrest,"%-*s",SIZE_DESCR_PRESTACION,msg->msgext.prestacs[p].Descripcion);
                  
                  writeSTR(cantPrest,SIZE_CANTIDAD,msg);
                  writeSTR(codPrest,SIZE_PRESTACION,msg);
                  writeSTR(descrPrest,SIZE_DESCR_PRESTACION,msg);

	  			  sprintf(lstmpBuff,"%d Cant:%s Prest:%s Val:%s Prestacion:'%s'\n",
	  					  p,cantPrest,codPrest,descrPrest,msg->msgext.prestacs[p].Prestacion);
                  toLog(lstmpBuff,NULL,1,1);
                  desp+=(SIZE_CANTIDAD+SIZE_PRESTACION+SIZE_DESCR_PRESTACION);
              }
              break;
	     default:
	    	 toLog("Sin Tipo de Mensaje...\n",NULL,1,1);
	  }
      toLog("\n",NULL,0,0);
   }

   if (msg->bitmap[CAMPO63]=='1') {
      writeBCD("0040",4,msg);
      writeSTR(msg->dismsg,40,msg);
   }

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);
   sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                    //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);              //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t100A++;
   writeLog(msg);

   return 0;

 }   //Function R110A


/* FUNCION: r210 
 * Respuesta a las Autorizaciones de Venta
 * 
 */
int r210(MSG *msg)
{
int i, pmant;
char laux[15];
char lstempnac[MAXLONG];
char lstmpBuff[120];


   toLog("------> r210\n",NULL,1,1);
   
   if (uploadTr(msg)==-1) return -1;
   showGenTr(msg);
   if (downloadTr(msg)==-1) return -1;
   showResp(msg);

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   writeTPDU(msg);
   writeBCD("0210",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]='1';

   // ** Venta en 2,3 cuotas, Plan Especial. 
   // ** Envio como respuesta al terminal, el importe que retorno el host, en el campo 04 del mensaje.
   msg->bitmap[4]='1';

   if (strcmp(msg->dismsg,blancos(39))!=0) {
      msg->bitmap[63]='1';
   }
   writeBitmap(msg);
   writeBCD(msg->prcode,6,msg);
 // ******************************  VENTA EN 2 y 3 Cuotas Plan Especial - Importe de la venta.
   writeBCD(msg->samtr,12,msg); 
 // ******************************  
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR(msg->srespco,2,msg);
   writeSTR(msg->teid,8,msg);
   if (strcmp(msg->dismsg,blancos(39))!=0) {
      writeBCD("0040",4,msg);
      writeSTR(msg->dismsg,40,msg);
   }

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                    //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);             //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t200++;
   writeLog(msg);

   return 0;
 }   //Function R210


/* FUNCION: r410A
 * Respuesta a 0400 de terminales Externas.
 * Devuelve un mensaje fijo 
 * ya que los 0400 no son resueltos por el servidor de Salud.
 * 
 */

int r410A(MSG *msg)
{
 int i, pmant;
 char laux[15];
 time_t now;
 struct tm z;
 char lstempnac[MAXLONG];
 
 char lstmpBuff[120];

   toLog("------> r410A\n",NULL,1,1);

   //Simulo uploadtrout y downloadtrout ya que se trata de un 0400
   time(&now);
   z = *localtime( &now );
   sprintf(msg->shora,"%02d%02d%02d",z.tm_hour,z.tm_min,z.tm_sec);
   sprintf(msg->sdia,"%02d%02d",z.tm_mon+1,z.tm_mday);
   strcpy(msg->sauthid,msg->shora);
   strcpy(msg->srespco,"00");

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));
   writeTPDU(msg);
   writeBCD("0410",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR(msg->srespco,2,msg);
   writeSTR(msg->teid,8,msg);

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++) lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long:%s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                    //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);             //LONG
   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   sprintf(lstmpBuff,"prcode=%s sytr=%s shora=%s sdia=%s netid=%s retrefnu=%s sauthid=%s srespco=%s teid=%s\n",
                      msg->prcode,msg->sytr,msg->shora,msg->sdia,msg->netid,msg->retrefnu,msg->sauthid,msg->srespco,msg->teid);
   toLog(lstmpBuff,NULL,0,0);

   //t400A++;
   writeLog(msg);
   return 0;

 }   //Function R410A

/* FUNCION: r410
 * Respuesta a 0400
 * 
 */
int r410(MSG *msg)
{
 int i, pmant;
 char laux[15];
 char lstempnac[MAXLONG];
 char lstmpBuff[250];
 
 
   toLog("------> r410\n",NULL,1,1);

   if (uploadTr(msg)==-1) {    //Arma "entra"
       return -1;
   }
   showGenTr(msg);    //Muestra lo que se envia a Hyper
   if (downloadTr(msg)==-1) {   //Recibe lo que envia Hyper (a traves de "sale")
       return -1;
   }
   showResp(msg);     //Muestra la respuesta

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));
   writeTPDU(msg);
   writeBCD("0410",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR(msg->srespco,2,msg);
   writeSTR(msg->teid,8,msg);

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++) lstempnac[i]=msg->mensnac[i];

   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long:%s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                    //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);             //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   sprintf(lstmpBuff,"prcode=%s sytr=%s shora=%s sdia=%s netid=%s retrefnu=%s sauthid=%s srespco=%s teid=%s\n",
		   msg->prcode,msg->sytr,msg->shora,msg->sdia,msg->netid,msg->retrefnu,msg->sauthid,msg->srespco,msg->teid);
   toLog(lstmpBuff,NULL,0,0);

   //t400++;
   writeLog(msg);
   return 0;
 }   //Function R410

/* FUNCION: r510
 * Respuesta a Cierres de Lote
 * 
 */
int r510(MSG *msg)
{
int i, pmant, respco;
char laux[15];
char lstempnac[MAXLONG];
char lstmpBuff[250];


   toLog("------> r510\n",NULL,1,1);

   if (upload500(msg) == -1) return -1;

   show500tr(msg);

   if (downloadTr(msg) == -1) return -1;

   showResp(msg);

   //Comienza a armar la cadena para enviar al NCC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   writeTPDU(msg);
   writeBCD("0510",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[39]=msg->bitmap[41]=msg->bitmap[63]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   respco = atoi(msg->srespco);
   sprintf(lstmpBuff,"respco=%s-%d\n",msg->srespco,respco);
   switch (respco) {
     case 0:
        { writeSTR(msg->srespco,2,msg);
          writeSTR(msg->teid,8,msg);
    	  writeBCD("0040",4,msg);
    	  if (strcmp(msg->dismsg,blancos(39))!=0) 
          {
        	  sprintf(laux,"%s",msg->dismsg);
              writeSTR(laux,40,msg);
          } else {
        	  sprintf(laux,"L:%s- AP:%s ", msg->banu, msg->sauthid);
        	  writeSTR(laux,20,msg);
        	  sprintf(laux,"CIERRE OK %8s",left(msg->sfecpre,8));
        	  writeSTR(laux,20,msg);
          }
	      break;
	    }
     case 77:
        { writeSTR(msg->srespco,2,msg);
          writeSTR(msg->teid,8,msg);
	  writeBCD("0040",4,msg);
	  writeSTR("ERROR EN CIERRE     ",20,msg);
	  writeSTR("LLAME A TDF         ",20,msg);
	  break;
	 }
     case 95:
        { writeSTR("00",2,msg);
          writeSTR(msg->teid,8,msg);
	  writeBCD("0040",4,msg);
	  writeSTR("-CIERRE CONDICIONAL-",20,msg);
	  writeSTR("--- LLAME A TDF --- ",20,msg);
	  break;
	 }
     default:
        { writeSTR(msg->srespco,2,msg);
          writeSTR(msg->teid,8,msg);
	  writeBCD("0040",4,msg);
	  writeSTR("ERROR EN CIERRE     ",20,msg);
	  sprintf(laux,"CODIGO %s       ",msg->srespco);
	  writeSTR(laux,20,msg);
	  break;
	 }
    }  //End switch

   //Longitud del Mensaje
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);     //msg->pmens tiene la longitud del mensaje...
   sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;
   writeBCD(laux,4,msg);

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   sprintf(lstmpBuff,"prcode=%s sytr=%s shora=%s sdia=%s netid=%s retrefnu=%s\n",
		   msg->prcode,msg->sytr,msg->shora,msg->sdia,msg->netid,msg->retrefnu);
   toLog(lstmpBuff,NULL,0,0);

   //t500++;
   writeLog(msg);

   return 0;
 }   //Function R510


int r810(MSG *msg)
{
int i, pmant;
char laux[25];
char dia[5], hora[7];
time_t now;
struct tm z;
char lstempnac[MAXLONG];
char lstmpBuff[120];


   toLog("------> r810\n",NULL,1,1);

   time(&now);

   z = *localtime( &now );
   sprintf(dia,"%02d%02d",z.tm_mday,z.tm_mon+1);
   sprintf(hora,"%02d%02d%02d",z.tm_hour,z.tm_min,z.tm_sec);
   sprintf(lstmpBuff,"dia=%s hora=%s ",dia,hora);
   toLog(lstmpBuff,NULL,0,0);

   toLog("PRUEBA DE COMUNICACION\n",NULL,1,1);

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   writeTPDU(msg);
   writeBCD("0810",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[41]=msg->bitmap[42]='1';  //Segun el manual msg->bitmap[42] no va como parte de la respuesta...
   writeBitmap(msg);
   
   writeBCD(msg->prcode,6,msg);
   writeBCD(hora,6,msg);
   writeBCD(dia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->teid,8,msg);
   writeSTR(msg->crdac,15,msg);  //...por lo tanto tampoco va crdac

   //LONG
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);
   sprintf(lstmpBuff,"Long:%s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;
   writeBCD(laux,4,msg);

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

    msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   return 0;


 }   //R810


/* FUNCION: uploadxSock
 * Arma mensaje correspondiente al 
 * 
 */

int uploadxSock(MSG *msg)
{
  int logfd, w;
  char laux[50];
  char lstmpBuff[250];
  
 	 toLog("------> uploadxSock\n",NULL,1,1);  

	  //VERIFICAR
	  sprintf(lstmpBuff,"mtype=%s prcode=%s innu=%s prac=%s len(prac)=15 ==> tr2 %s (%d) \n",
			  msg->mtype,msg->prcode,msg->innu,msg->prac,msg->tr2,sizeof(msg->tr2));
	  toLog(lstmpBuff,NULL,0,0);

	  sprintf(lstmpBuff,"tr2=%s amtr=%s daex=%s teid=%s numpa=%s paypla=%s crdac=%s \n",
			  msg->tr2,msg->amtr,msg->daex,msg->teid,msg->numpa,msg->paypla,msg->crdac);
	  toLog(lstmpBuff,NULL,0,0);

	  //ARMO MENSAJE REAL
 	 
      memset(laux,0x00,sizeof(laux));
      if (strlen(msg->prac) == 15){
    	  sprintf(laux,"%6s%12s",left(msg->prac,6),blancos(12));
    	  strcpy(msg->tr2,laux);
      }
     	   	  
	  //Junto el rompecabezas armando el mensaje real
	  sprintf(msg->request,"%4s%2s%12s%9s%18s%12s%4s%8s%2s%s%-15s",
 			 msg->mtype,			//Tipo de Mensaje
 			 left(msg->prcode,2),	//Cod. Procesamiento (0.Venta, 1.Anulacion)
 			 leftpad(msg->innu,'0',12),				//Numero de Comprobante
 			 right(msg->prac,9),	//Numero de Banda
 			 leftpad(msg->tr2,' ',18),			//Track2			 
 			 leftpad(msg->amtr,' ',12),			//Monto de la Operacion
 			 leftpad(msg->daex,' ',4),   		//Fecha de Vencimiento de la tarjeta
 			 msg->teid,							//Nro de Terminal
 			 msg->numpa,						//Nro de Pagos
 			 msg->paypla,						//Tipo de Plan
 			 msg->crdac							//Codigo de Comercio
 	  );
	  
	  msg->lrequest = strlen(msg->request);

	  logfd = open("loginout",PARAMSFILE,MODEFILE);
	  if (logfd != -1) {
 	     w=write(logfd,"E: ",3);
 	     w=write(logfd,msg->request,msg->lrequest);
 	     w=write(logfd,"\n",1);
 	     close(logfd);
 	  }

	  if (trxRemota(msg) == -1)
	  {
		toLog("Hubo algún error en el envío de la transacción a Host Remoto\n",NULL,1,1);
		return -1;          
	  }
	   
     return 0;
}

/* FUNCION: trxRemota
 * Defino host y puerto de acuerdo al numero de banda
 * Envio el mensaje por socket
 * Espero respuesta del Host
 * La respuesta queda en el campo "answer" de MSG
 */
/*
 * int trxRemota(MSG *msg)
{
 time_t now;
 struct tm z;
 int clisock;
 char lstmpBuff[250];
 char puerto[5+1];
 char host[15+1];

   toLog("------> trxRemota\n",NULL,0,0);
  
   if (GetSockDestino(msg,host,puerto)==-1){
	   toLog("No se pudo obtener el Host/Puerto Destino\n",NULL,1,1);
	   return -1;
   }
  
   //ENVIO DEL MENSAJE!!
   time(&now);
   z = *localtime( &now );
   clisock=ConfigCliSocket(host,puerto);
   if (clisock == -1) {
      toLog("No hubo conexión con el server!\n",NULL,0,0);
      return -1;
   }
      
   if (SendMsgSock(clisock,msg->request,msg->lrequest)==-1) {
      toLog("Cliente: Error en el envio del mensaje\n","SendMsgSock",0,0);
      CloseSocket(clisock);
      return -1;
   }

   toLog("Envío el Mensaje... Esperando respuesta\n",NULL,0,0);
   memset(msg->answer,0x00,sizeof(msg->answer));
     
   if ((msg->lanswer=GetMsgSock(clisock,msg->answer,MAXLONG,32))==-1) {
      toLog("Cliente: Error en la recepción del mensaje\n",NULL,1,1);
   } else {
      sprintf(lstmpBuff,"Mensaje Recibido: %s (%d bytes)\n",msg->answer,msg->lanswer);
      toLog(lstmpBuff,NULL,1,1);
   }
     
   CloseSocket(clisock);

   return msg->lanswer;

}
*/
/* FUNCION: uploadOutxSock
 * 
 * 
 * 
 */
/*
 int uploadOutxSock(MSG *msg)
{
 int lfd, logfd, w;
 char lstr1[55+1];
 char lsnomen[5];
 char lsmontos[(12*3)+8];
 char lstmpBuff[250];

 FILE *fp,*fpar;
 char ch;
 int i,sysres;
 time_t now;
 struct tm z;
 char externo[30];
 int confin=FALSE;
 
  toLog("------> uploadOutxSock\n",NULL,0,0);

  sprintf(lstmpBuff,"mtype=%s prcode=%s innu=%s tr1=%s tr2=%s codigos=%s teid=%s crdac=%s \n",
		  msg->mtype,msg->prcode,msg->innu,msg->tr1,msg->tr2,msg->campo61,msg->teid,msg->crdac);
  toLog(lstmpBuff,NULL,0,0);

  //PRAC - Primary Account Number - Este viaja cuando el ingreso es MANUAL
  memset(lstr1,0x00,sizeof(lstr1));
  RemoveBlancos(msg->prac,19);
  if (strcmp(msg->prac,"")!=0) {
     strncpy(lstr1,msg->prac,strlen(msg->prac));
  } else {
     //TRACK1
     RemoveBlancos(msg->tr1,55);
     if (strcmp(msg->tr1,"")!=0) strncpy(lstr1,msg->tr1,strlen(msg->tr1));
  }     

  //CODIGO PRESTADOR (CRDAC) (15)
  //Comparo con el tr2 de la tarjeta, o con el tr1 si es ingreso manual
  if ( (strcmp(left(msg->crdac,8),left(msg->tr2,8))!=0) &&     //No se volvio a pasar la tarjeta
       (strcmp(left(msg->crdac,8),left(msg->prac,8))!=0) &&    //No se ingreso la tarjeta a mano de nuevo
       (strstr(msg->tr1,left(msg->crdac,8))==NULL) )           
  {
      RemoveBlancos(msg->crdac,15);
      //w=write(lfd,msg->crdac,strlen(msg->crdac));
  } else {
	 memset(msg->crdac,0x00,sizeof(msg->crdac));
  }
  // IPAUSS -------------------------------------------------    
    memset(lsnomen,0x00,sizeof(lsnomen));
    if (msg->config.PideNomencla==TRUE)
    {
      //TIPO DE NOMENCLADOR (1)
      sprintf(lsnomen,"%s|",msg->nomencla);
    }    
  //--------------------------------------------------
    memset(lsmontos,0x00,sizeof(lsmontos));
    if (msg->config.PideImportes==TRUE)    //IPAUSS 
    {
      //MONTO A CARGO DEL AFILIADO
      sprintf(lsmontos,"%s|%s|%s|",
    		  ltrim(msg->amtr,'0'),ltrim(msg->amtr2,'0'),ltrim(msg->amtr3,'0'));
    } 
    RemoveBlancos(msg->tr2,strlen(msg->tr2));
    //--------------------------------------------------
    memset(msg->request,0x00,sizeof(msg->request));
    sprintf(msg->request,"&%4s|%2s|%s|%s|%s|%s%s%s%s|%s%%%d\n",
		  msg->mtype,
		  left(msg->prcode,2),
		  ltrim(msg->innu,'0'),
		  lstr1,
		  msg->tr2,
		  lsnomen,
		  armaListaPrestaciones(msg),
		  lsmontos,
		  ltrim(msg->teid,'0'),
		  msg->crdac,
		  msg->tipoMensaje);
  
  msg->lrequest = strlen(msg->request);
  toLog(msg->request,NULL,1,1);
  toLog("\n",NULL,1,1);
    
  logfd = open("loginout",PARAMSFILE,MODEFILE);
  if (logfd != -1) {
	 w=write(logfd,"********** TRANSACCION EXTERNA ********* \n",42);
     w=write(logfd,"E: ",3);
     w=write(logfd,msg->request,msg->lrequest);
     w=write(logfd,"\n",1);
     close(logfd);
  }

// OPCION A - Mando la transaccion directamente al Servidor
 // if (trxRemota(msg) == -1)
 // {
  //	toLog("Hubo algún error en el envío de la transacción a Host Remoto\n",NULL,1,1);
//	return -1;          
  }
//
  //--------------------------- FIN OPCION A

  // OPCION B - Llamo directamente a ext con los parametros
   fpar=fopen("/salud/params.cfg","r");
   if (fpar == NULL) {
        toLog("Error en params.cfg - Usando Default: 'ext'\n","fopen",1,1);
        //Si hay error... el cliente por defecto:
        strcpy(externo,"/salud/ext");
   } else {
       fscanf(fpar,"%s",externo);externo[strlen(externo)]=0x00;
       fclose(fpar);
   }
 
  //Armo los nombres de los archivos de entrada y salida.
  time(&now);
  z = *localtime( &now );
  sprintf(msg->msgext.ReqFile,"/salud/txrx/MsgReq%02d%02d-%02d%02d%02d",z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);
  sprintf(msg->msgext.RespFile,"/salud/txrx/MsgResp%02d%02d-%02d%02d%02d",z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);

  lfd = open(msg->msgext.ReqFile,O_CREAT|O_TRUNC|O_RDWR,MODEFILE);
  if (lfd == -1) {
      toLog("No se abrió MsgReq\n","Open MsgReq",1,1);
      return -1;
  }
  
  write(lfd,msg->request,msg->lrequest);
  close(lfd);

  sprintf(lstmpBuff,"%s %s %s 35 2>>/salud/Cli.txt",
		    externo,msg->msgext.ReqFile,msg->msgext.RespFile);   
  toLog(lstmpBuff,NULL,1,1);
  sysres=-1;
  sysres = system(lstmpBuff);
  if (sysres == -1) {
     sprintf(lstmpBuff,"Error en %s\n",externo);
     toLog(lstmpBuff,NULL,1,1);
     return -1;
  }
  
  toLog("\n",NULL,0,0);
  
  fp=waitingFile(msg->msgext.RespFile,43,52);
  if (fp == NULL) {
     toLog("No existe MsgResp\n",msg->msgext.RespFile,1,1);
     return -1;
  }
  memset(msg->answer,0x00,sizeof(msg->answer));
  confin=FALSE;
  i=0;
  ch = fgetc(fp);
  if (ch != '&') {
     toLog("MsgResp no tiene Inicio de Mensaje\n",NULL,1,1);
     return -1;
  } 
  msg->answer[i++]=ch;
  fprintf(stderr,"%c",ch);    //Primer '&'
  while (!feof(fp)){
     ch = fgetc(fp);
     fprintf(stderr,"%c",ch);
     msg->answer[i++]=ch;
     if (ch == '%') { confin=TRUE; break; }     
   }
   fclose(fp);
   fprintf(stderr,"\n");
   //El ultimo caracter leido deberia ser '%' 
   if (confin == FALSE) {
     toLog("MsgResp no tiene fin de Mensaje!\n",NULL,1,1);
     return -1;
   }  

   msg->answer[i]=0;
   msg->lanswer=i;
  
  //--------------------------- FIN OPCION B
  return 0;
  
} */ 
 
/* FUNCION: armaListaPrestaciones
 * Arma la lista de prestaciones, separadas por | para enviar al host externo.
 * 
 */
/*
char *armaListaPrestaciones(MSG *msg)
{  
 int desp = 0, p=0;
 char *lista;
 int plis=0, j=0;
 char presta[SIZE_PRESTACION+1];
 char canti[SIZE_CANTIDAD+1];  
 char lstmpBuff[120];
 
 toLog("------> armaListaPrestaciones\n",NULL,1,0);
 
 lista = (char *) malloc (sizeof(char) * 300);
 //NUEVA VERSION

 switch( msg->longcampo61) {
	case LONG_LISTA_SIMPLE_7:   //(SIZE_PRESTACION( de 7) *MAX_PRESTACIONES)     
	  //Separo los CODIGOS del campo61
	  desp=0; p=0;
	  while (p<MAX_PRESTACIONES)
	  {   memset(msg->msgext.prestacs[p].Prestacion,0x00,sizeof(msg->msgext.prestacs[p].Prestacion));
	  	  memcpy(msg->msgext.prestacs[p].Prestacion,msg->campo61+desp,SIZE_PRESTACION_7);
		  p++;
	      desp+=SIZE_PRESTACION_7;
	  }
	  for (p=0;p<MAX_PRESTACIONES;p++)
	  {
	      RemoveBlancos(msg->msgext.prestacs[p].Prestacion,SIZE_PRESTACION_7);
	      if (atoi(msg->msgext.prestacs[p].Prestacion) > 0) {
		  	strcpy(presta,ltrim(msg->msgext.prestacs[p].Prestacion,'0'));
		  	//strncat(lista,presta,strlen(presta));
	    		//plis+=strlen(presta);
			for (j=0;j<strlen(presta);j++) {
				lista[plis++]=presta[j];
			}
	      }   
	      lista[plis++]='|';
	  }
	  break;
 	case LONG_LISTA_SIMPLE:     //(SIZE_PRESTACION*MAX_PRESTACIONES)     
 	  desp=0; p=0;
      while (p<MAX_PRESTACIONES)
      {   memset(msg->msgext.prestacs[p].Prestacion,0x00,sizeof(msg->msgext.prestacs[p].Prestacion));
	      memcpy(msg->msgext.prestacs[p].Prestacion,msg->campo61+desp,SIZE_PRESTACION);
	      p++;
          desp+=SIZE_PRESTACION;
      }
      for (p=0;p<MAX_PRESTACIONES;p++)
      {
          RemoveBlancos(msg->msgext.prestacs[p].Prestacion,SIZE_PRESTACION);
          if (atoi(msg->msgext.prestacs[p].Prestacion) > 0) {
		    	strcpy(presta,ltrim(msg->msgext.prestacs[p].Prestacion,'0'));
			for (j=0;j<strlen(presta);j++) {
				lista[plis++]=presta[j];
			}
		    	//strncat(lista,presta,strlen(presta));
            		//plis+=strlen(presta);
			
          }   
      	   lista[plis++]='|';
      }
      break;
 	case LONG_LISTA_COMPUESTA:   //(SIZE_PRESTACION+SIZE_CANTIDAD)*MAX_PRESTACIONES)     
	   //Separo los pares CANTIDAD/CODIGO del campo61
	   desp=0; p=0;
	   while (p<MAX_PRESTACIONES) 
	   {  memset(msg->msgext.prestacs[p].Cantidad,0x00,sizeof(msg->msgext.prestacs[p].Cantidad));
   		  memset(msg->msgext.prestacs[p].Prestacion,0x00,sizeof(msg->msgext.prestacs[p].Prestacion));
   		  memcpy(msg->msgext.prestacs[p].Cantidad,msg->campo61+desp,SIZE_CANTIDAD);
   		  memcpy(msg->msgext.prestacs[p].Prestacion,msg->campo61+desp+SIZE_CANTIDAD,SIZE_PRESTACION);
   		  p++;
   		  desp+=(SIZE_CANTIDAD+SIZE_PRESTACION);
	   }
	   for (p=0;p<MAX_PRESTACIONES;p++)
	   {
		   RemoveBlancos(msg->msgext.prestacs[p].Cantidad,SIZE_CANTIDAD);
		   RemoveBlancos(msg->msgext.prestacs[p].Prestacion,SIZE_PRESTACION);
		   if (atoi(msg->msgext.prestacs[p].Prestacion) != 0){
   	     		memset(canti,0x00,sizeof(canti));memset(presta,0x00,sizeof(presta));
   	     		strcpy(canti,ltrim(msg->msgext.prestacs[p].Cantidad,'0'));
   	     		strcpy(presta,ltrim(msg->msgext.prestacs[p].Prestacion,'0'));
   	     		strncat(lista,canti,strlen(canti));
   	     		plis+=strlen(canti);
   	     		lista[plis++]='|';
   	     		strncat(lista,presta,strlen(presta));
   	     		plis+=strlen(presta);
   	     		lista[plis++]='|';
		   } else {            
			   	lista[plis++]='|';
			   	lista[plis++]='|';
		   }
	   }
	   break;
   default:  
   	    toLog("FALTA longcampo61....\n",NULL,1,1);

 	}    
	
	lista[plis]=0x00;
	sprintf(lstmpBuff,"Lista Final: %s %d\n",lista,strlen(lista)); 
	toLog(lstmpBuff,NULL,1,1);
    return lista;
}
*/

/*

int downloadOutxSock(MSG *msg)
{
 int i, j, sep, p=0;
 FILE *logfp;
 char logsg[120];
 char lstmpBuff[120];
	 
  toLog("------> downloadOutxSock\n",NULL,1,1);
	  	  
  //LOGINOUT
  logfp=fopen("loginout","a+");
  if (logfp==NULL) toLog("No se abrió LOGINOUT\n","Loginout",0,0);
  else {
      fprintf(logfp,"S: %s\n",msg->answer);
      fclose(logfp);
  }

   toLog(msg->answer,NULL,1,1);
   toLog("\n",NULL,1,1);
     
  //Limpieza de variables
   memset(logsg,0x00,sizeof(logsg));
   memset(msg->shora,0x00,sizeof(msg->shora));
   memset(msg->sdia,0x00,sizeof(msg->sdia));
   memset(msg->srespco,0x00,sizeof(msg->srespco));
   memset(msg->ssahoy,0x00,sizeof(msg->ssahoy));
   memset(msg->dismsg,0x00,sizeof(msg->dismsg));
   memset(msg->nomencla,0x00,sizeof(msg->nomencla));
   memset(msg->samtr,0x00,sizeof(msg->samtr));
   memset(msg->samtr2,0x00,sizeof(msg->samtr2));
   memset(msg->samtr3,0x00,sizeof(msg->samtr3));
   memset(msg->msgext.nombre_afiliado,0x00,sizeof(msg->msgext.nombre_afiliado));
   for (p=0;p<MAX_PRESTACIONES;p++){
	  memset(msg->msgext.prestacs[p].Cantidad,0x00,sizeof(msg->msgext.prestacs[p].Cantidad));
	  memset(msg->msgext.prestacs[p].Prestacion,0x00,sizeof(msg->msgext.prestacs[p].Prestacion));
	  memset(msg->msgext.prestacs[p].Descripcion,0x00,sizeof(msg->msgext.prestacs[p].Descripcion));
	  memset(msg->msgext.prestacs[p].Valor,0x00,sizeof(msg->msgext.prestacs[p].Valor));
   }
   switch(msg->tipoMensaje) {
	   
      case LISTA_SIMPLE_7:
      case LISTA_SIMPLE:
      	j=0;
      	sep = 0; p=0;
      	for (i=1;i<msg->lanswer; i++)
      	{   if (msg->answer[i]=='|') {
      			switch (sep) {
                  case 0: msg->shora[j]=0x00;break;
                  case 1: msg->sdia[j]=0x00;break;
                  case 2: msg->retrefnu[j]=0x00;break;
                  case 3: msg->srespco[j]=0x00;break;
                  case 4: msg->msgext.nombre_afiliado[j]=0x00;break;
                  case 5: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 6: msg->msgext.prestacs[p].Descripcion[j]=0x00;break;
                  case 7: msg->msgext.prestacs[p++].Valor[j]=0x00;break;
                  case 8: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 9: msg->msgext.prestacs[p].Descripcion[j]=0x00;break;
                  case 10: msg->msgext.prestacs[p++].Valor[j]=0x00;break;
                  case 11: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 12: msg->msgext.prestacs[p].Descripcion[j]=0x00;break;
                  case 13: msg->msgext.prestacs[p++].Valor[j]=0x00;break;
                  case 14: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 15: msg->msgext.prestacs[p].Descripcion[j]=0x00;break;
                  case 16: msg->msgext.prestacs[p++].Valor[j]=0x00;break;
                  case 17: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 18: msg->msgext.prestacs[p].Descripcion[j]=0x00;break;
                  case 19: msg->msgext.prestacs[p++].Valor[j]=0x00;break;
      			}
      			j=0;
      			sep ++;
      		} else {
      			if (msg->answer[i]== '%') { msg->dismsg[j]=0x00; break;}
      			switch (sep) {
                  case 0: msg->shora[j++]=msg->answer[i];break;
                  case 1: msg->sdia[j++]=msg->answer[i];break;
                  case 2: msg->retrefnu[j++]=msg->answer[i];break;
                  case 3: msg->srespco[j++]=msg->answer[i];break;
		  case 4: msg->msgext.nombre_afiliado[j++]=msg->answer[i];break;
                  case 5: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                  case 6: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                  case 7: msg->msgext.prestacs[p].Valor[j++]=msg->answer[i];break;
                  case 8: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                  case 9: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                  case 10: msg->msgext.prestacs[p].Valor[j++]=msg->answer[i];break;
                  case 11: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                  case 12: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                  case 13: msg->msgext.prestacs[p].Valor[j++]=msg->answer[i];break;
                  case 14: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                  case 15: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                  case 16: msg->msgext.prestacs[p].Valor[j++]=msg->answer[i];break;
                  case 17: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                  case 18: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                  case 19: msg->msgext.prestacs[p].Valor[j++]=msg->answer[i];break;
                  case 20: msg->dismsg[j++]=msg->answer[i];break;
               }
           }
      	}	
        sprintf(lstmpBuff,"\nshora=%s sdia=%s retrefnu=%s srespco=%s\n dismsg=%s\n",
                           msg->shora,msg->sdia,msg->retrefnu,msg->srespco,
                           msg->dismsg);
        
        toLog(lstmpBuff,NULL,0,0);
        break;
        
   case LISTA_COMPUESTA:
	  j=0;
      sep = 0; p=0;
      for (i=1;i<strlen(msg->answer); i++)
          { if (msg->answer[i]=='|') {
           switch (sep) {
                  case 0: msg->shora[j]=0x00;break;
                  case 1: msg->sdia[j]=0x00;break;
                  case 2: msg->retrefnu[j]=0x00;break;
                  case 3: msg->srespco[j]=0x00;break;
                  case 4: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                  case 5: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 6: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                  case 7: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                  case 8: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 9: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                  case 10: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                  case 11: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 12: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                  case 13: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                  case 14: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 15: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                  case 16: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                  case 17: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                  case 18: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
           }
           j=0;
           sep ++;
        } else {
            if (msg->answer[i]== '%') { msg->dismsg[j]=0x00; break;}
            switch (sep) {
                 case 0: msg->shora[j++]=msg->answer[i];break;
                 case 1: msg->sdia[j++]=msg->answer[i];break;
                 case 2: msg->retrefnu[j++]=msg->answer[i];break;
                 case 3: msg->srespco[j++]=msg->answer[i];break;
                 case 4: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                 case 5: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                 case 6: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                 case 7: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                 case 8: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                 case 9: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                 case 10: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                 case 11: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                 case 12: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                 case 13: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                 case 14: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                 case 15: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                 case 16: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                 case 17: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                 case 18: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                 case 19: msg->dismsg[j++]=msg->answer[i];break;
            }
        }
      }
      sprintf(lstmpBuff,"\nshora=%s sdia=%s retrefnu=%s srespco=%s\n dismsg=%s\n",
                         msg->shora,msg->sdia,msg->retrefnu,msg->srespco,
                         msg->dismsg);
      
      toLog(lstmpBuff,NULL,0,0);
	        break;
     
      case LISTA_IMPORTES:
   	     j=0;
         sep = 0; p=0;
         for (i=1;i<strlen(msg->answer); i++)
         { 
           //sprintf(lstmpBuff,"i=%d j=%d sep=%d p=%d %c \n",i,j,sep,p,msg->answer[i]);
           //toLog(lstmpBuff,NULL,1,1);
            
           if (msg->answer[i]=='|') {
              switch (sep) {
                     case 0: msg->shora[j]=0x00;break;
                     case 1: msg->sdia[j]=0x00;break;
                     case 2: msg->retrefnu[j]=0x00;break;
                     case 3: msg->srespco[j]=0x00;break;
                     case 4: msg->nomencla[j]=0x00;break;                                          
                     case 5: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                     case 6: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                     case 7: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                     case 8: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                     case 9: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                     case 10: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                     case 11: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                     case 12: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                     case 13: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                     case 14: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                     case 15: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                     case 16: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                     case 17: msg->msgext.prestacs[p].Cantidad[j]=0x00;break;
                     case 18: msg->msgext.prestacs[p].Prestacion[j]=0x00;break;
                     case 19: msg->msgext.prestacs[p++].Descripcion[j]=0x00;break;
                     case 20: msg->dismsg[j]=0x00;break;
                     case 21: msg->samtr[j]=0x00;break;
                     case 22: msg->samtr2[j]=0x00;break;
              }
              j=0;
              sep ++;
           } else {
               if (msg->answer[i]== '%') { msg->samtr3[j]=0x00; break; }
               switch (sep) {
                    case 0: msg->shora[j++]=msg->answer[i];break;
                    case 1: msg->sdia[j++]=msg->answer[i];break;
                    case 2: msg->retrefnu[j++]=msg->answer[i];break;
                    case 3: msg->srespco[j++]=msg->answer[i];break;
                    case 4: msg->nomencla[j++]=msg->answer[i];break;                                          
                    case 5: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                    case 6: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                    case 7: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                    case 8: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                    case 9: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                    case 10: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                    case 11: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                    case 12: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                    case 13: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                    case 14: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                    case 15: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                    case 16: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                    case 17: msg->msgext.prestacs[p].Cantidad[j++]=msg->answer[i];break;
                    case 18: msg->msgext.prestacs[p].Prestacion[j++]=msg->answer[i];break;
                    case 19: msg->msgext.prestacs[p].Descripcion[j++]=msg->answer[i];break;
                    case 20: msg->dismsg[j++]=msg->answer[i];break;
                    case 21: msg->samtr[j++]=msg->answer[i];break;
                    case 22: msg->samtr2[j++]=msg->answer[i];break;
                    case 23: msg->samtr3[j++]=msg->answer[i];break;
               }
           }
         }
         sprintf(lstmpBuff,"\nshora=%s sdia=%s retrefnu=%s srespco=%s\n importe1=%s importe2=%s importe3=%s \ndismsg=%s\n",
                            msg->shora,msg->sdia,msg->retrefnu,msg->srespco,
                            msg->samtr,msg->samtr2,msg->samtr3,msg->dismsg);
         toLog(lstmpBuff,NULL,1,1);
	     break;
	     
      default:
    	  toLog("FALTA Tipo de Mensaje......!!\n",NULL,1,1);
    	  
   }
   return 0;

}
*/
int upload500xSock(MSG *msg)
{
 int logfd, w;
 char lstmpBuff[250];
  
  toLog("------> upload500xSock\n",NULL,0,0);

  sprintf(lstmpBuff,"mtype=%s prcode=%s banu=%s cove=%s imve=%s cocr=%s imcr=%s teid=%s dase=%s\n",
		  msg->mtype,left(msg->prcode,2),msg->banu,msg->cove,msg->imve,msg->cocr,msg->imcr,msg->teid,msg->dase);
  toLog(lstmpBuff,NULL,1,1);

  sprintf(msg->request,"%4s%2s%6s%3s%12s%3s%12s%19s%8s%14s%4s",
		  msg->mtype,
		  left(msg->prcode,2),
		  msg->banu,
		  msg->cove,
		  msg->imve,
		  msg->cocr,
		  msg->imcr,
		  "0000000000000000000",
  		  msg->teid,
		  "00000000000000",
          msg->dase);
  
  msg->lrequest = strlen(msg->request);
  
  
  logfd = open("loginout",PARAMSFILE,MODEFILE);
  if (logfd>0) {
      w=write(logfd,"E: ",3);
      w=write(logfd,msg->request, msg->lrequest);
      w=write(logfd,"\n",1);
      close(logfd);
  }

  trxRemota(msg);
  
  return 0;
}


int downloadxSock(MSG *msg)
{
int i, j;
int fp; 
char logsg[120];
char lstmpBuff[250];

   

   toLog("------> downloadxSock\n",NULL,1,1);
   
   //LOGINOUT
   fp=open("loginout",O_APPEND|O_RDWR);
   if (fp!=-1) {
      write(fp,"S: ",3);
      write(fp,msg->answer,msg->lanswer);
      write(fp,"\n",1);
      close(fp);
   }
   //Limpieza de variables
   memset(logsg,0x00,sizeof(logsg));
   memset(msg->samtr,0x00,sizeof(msg->samtr));
   memset(msg->shora,0x00,sizeof(msg->shora));
   memset(msg->sdia,0x00,sizeof(msg->sauthid));
   memset(msg->srespco,0x00,sizeof(msg->srespco));
   memset(msg->ssahoy,0x00,sizeof(msg->ssahoy));
   memset(msg->sauthid,0x00,sizeof(msg->sauthid));
   i=0;
   for (j=0;j<12;j++) msg->samtr[j]=msg->answer[i++]; msg->samtr[j]=0;
   for (j=0;j<6;j++) msg->shora[j]=msg->answer[i++]; msg->shora[j]=0;
   for (j=0;j<4;j++) msg->sdia[j]=msg->answer[i++]; msg->sdia[j]=0;
   for (j=0;j<6;j++) msg->sauthid[j]=msg->answer[i++]; msg->sauthid[j]=0;
   for (j=0;j<2;j++) msg->srespco[j]=msg->answer[i++]; msg->srespco[j]=0;
   for (j=0;j<10;j++) msg->ssahoy[j]=msg->answer[i++]; msg->ssahoy[j]=0;
   for (j=0;j<120;j++) logsg[j]=msg->answer[i++]; logsg[j]=0;

   strcpy(msg->sfecpre,msg->ssahoy);
   
   // ************************************************
   // * Venta en 2 y 3 Cuotas, Plan Especial. 
   // * Recupero el importe retornado por el host, para enviarlo nuevamente al terminal.       
   if (atol(msg->samtr) == 0 ) strcpy(msg->samtr,msg->amtr);   
   // *************************************************

   memset(msg->dismsg,0x00,sizeof(msg->dismsg));

   if (strcmp(msg->mtype,"0200")==0) {
      for (j=0;j<39;j++) msg->dismsg[j]=msg->answer[i++];
   }

   sprintf(lstmpBuff,"\nsamtr=%s amtr=%s shora=%s sdia=%s sauthid=%s srespco=%s ssahoy=%s\n bufi=%s \n dismsg=%s\n ",
		   msg->samtr,msg->amtr,msg->shora,msg->sdia,msg->sauthid,msg->srespco,msg->ssahoy,logsg, msg->dismsg);
   toLog(lstmpBuff,NULL,1,1);

   if (msg->ssahoy[9]=='S') {
       appResgu(logsg);
       msg->ssahoy[9]=' ';
   }
   
   return 0;
}


void showResp(MSG *msg)
{
  char lstmpBuff[120];

   sprintf(lstmpBuff,"\nRH> %s %s %s %s %s %s ", 
		   msg->samtr,msg->ssahoy,msg->shora,msg->sdia,msg->sauthid,msg->srespco);
   //show(upwin,lstmpBuff,-1,-1);
   toLog(lstmpBuff,NULL,1,1);
   if(strcmp(msg->srespco,"00")!=0) toLog("<<<\n",NULL,1,1);
   else toLog("\n",NULL,1,1);
/*
   if(strcmp(msg->srespco,"00")!=0) show(upwin,"<<<\n",-1,-1);
   else show(upwin,"\n",-1,-1);
*/
}

void showGenTr(MSG *msg)
{
char lstmpBuff[250];

  switch (msg->tipotrx)
  {  case TRX_AUTORIZACION:
     case TRX_ANULACION_SALUD:
        sprintf(lstmpBuff,"RN> %s %s %s ", msg->teid,msg->mtype,left(msg->prcode,2));
        toLog(lstmpBuff,NULL,1,1);
        if (strcmp(msg->prac,"")!=0) {
           if (strlen(msg->prac) == 15) sprintf(lstmpBuff,"T%15s",msg->prac);
           else sprintf(lstmpBuff,"M%s ",right(msg->prac,9));
        } else {
           if (strcmp(msg->tr2,"")!=0) sprintf(lstmpBuff,"B%s ", right(msg->tr2,14));
           else sprintf(lstmpBuff," %14s "," ");
        }
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp((char *)msg->campo61,"")!=0) sprintf(lstmpBuff,"%s ",msg->campo61);
        else sprintf(lstmpBuff,"%40s "," ");
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->daex,"")!=0) sprintf(lstmpBuff,"%s ",msg->daex);
        else sprintf(lstmpBuff,"%4s "," ");
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->crdac,"")!=0) sprintf(lstmpBuff,"%8s\n",msg->crdac);
        else sprintf(lstmpBuff,"%15s\n"," ");
        toLog(lstmpBuff,NULL,1,1);
        break;
     default:

        sprintf(lstmpBuff,"RN> %s %s %s ", msg->teid,msg->mtype,left(msg->prcode,2));
        toLog(lstmpBuff,NULL,1,1);
        if (strcmp(msg->prac,"")!=0) {
            if (strlen(msg->prac) == 15) sprintf(lstmpBuff,"T%15s",msg->prac);
            else sprintf(lstmpBuff,"M%s ",right(msg->prac,9));
        } else {
            if (strcmp(msg->tr2,"")!=0) sprintf(lstmpBuff,"B%s ", right(msg->tr2,14));
            else sprintf(lstmpBuff," %14s "," ");
        }
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->amtr,"")!=0) sprintf(lstmpBuff,"%s ",right(msg->amtr,9));
        else sprintf(lstmpBuff,"%9s "," ");
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->daex,"")!=0) sprintf(lstmpBuff,"%s ",msg->daex);
        else sprintf(lstmpBuff,"%4s "," ");
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->numpa,"")!=0) sprintf(lstmpBuff,"%s ",msg->numpa);
        else sprintf(lstmpBuff,"%2s "," ");
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->paypla,"")!=0) sprintf(lstmpBuff,"%s ",msg->paypla);
        else sprintf(lstmpBuff,"  ");
        toLog(lstmpBuff,NULL,1,1);

        if (strcmp(msg->crdac,"")!=0) sprintf(lstmpBuff,"%8s\n",left(msg->crdac,8));
        else sprintf(lstmpBuff,"%8s\n"," ");
        toLog(lstmpBuff,NULL,1,1);
    }

}

void show500tr(MSG *msg)
{char lstmpBuff[120];

   sprintf(lstmpBuff,"RN> %s %s %s %s %s %s %s %s %s\n", 
		   msg->teid,msg->mtype,left(msg->prcode,2),msg->banu,msg->cove,msg->imve,msg->cocr,msg->imcr,msg->dase);
   toLog(lstmpBuff,NULL,1,1);
}

void writeLog(MSG *msg)
{
FILE *lfp;
char arch[20];
char lstmpBuff[120];

/*
   if (strcmp(mtype,"0500")==0) strcpy(arch,"trans500.dat");
   else strcpy(arch,"transac.dat");
*/

   strcpy(arch,"loginout");

   lfp = fopen(arch,"a");
   if (lfp == NULL) { sprintf(lstmpBuff,"No se pudo abrir log"); return;}
   fprintf(lfp,"%s,%s,%s,",msg->teid,msg->mtype,left(msg->prcode,2));

   if (strcmp(msg->mtype,"0500")==0)
       fprintf(lfp,"%s,%s,%s,%s,%s,%s", msg->banu, msg->cove, msg->imve, msg->cocr, msg->imcr, msg->dase);
   else {
      if (strcmp(msg->prac,"")!=0) fprintf(lfp,"%s,",right(msg->prac,9)); 
      else fprintf(lfp,"%9s,"," ");
      if (strcmp(msg->tr2,"")!=0) fprintf(lfp,"%s,",right(msg->tr2,14)); 
      else fprintf(lfp,"%14s,"," ");
      if (strcmp(msg->amtr,"")!=0) fprintf(lfp,"%s,",right(msg->amtr,9)); 
      else fprintf(lfp,"%9s,"," ");
      if (strcmp(msg->daex,"")!=0) fprintf(lfp,"%s,",msg->daex); 
      else fprintf(lfp,"    ,");   //4
      if (strcmp(msg->numpa,"")!=0) fprintf(lfp,"%s,",msg->numpa); 
      else fprintf(lfp,"  ,");   //2
      if (strcmp(msg->paypla,"")!=0) fprintf(lfp,"%s,",msg->paypla); 
      else fprintf(lfp," ,");  //1
      if (strcmp(msg->crdac,"")!=0) fprintf(lfp,"%s,",left(msg->crdac,8)); 
      else fprintf(lfp,"%8s,"," ");
  }

  fprintf(lfp,"%s,%s,%s,%s,%s,%s\n",
		  msg->samtr,msg->ssahoy,msg->shora,msg->sdia,msg->sauthid,msg->srespco);
  fclose(lfp);
  
  /*
   * if (strcmp(msg->mtype,"0500")==0) strcpy(arch,"trans500.ind");
  else strcpy(arch,"transac.ind");
  lfp = fopen(arch,"r");
  if (lfp == NULL) {
       sprintf(lstmpBuff,"writeLog - Error al abrir %s\n",arch);
       toLog(lstmpBuff,"writeLog-fopen",0,0);
       //salir(0);
       return ;
  }
  fscanf(lfp,"%d",&n);
  fclose(lfp);

  lfp = fopen(arch,"w");
  if (lfp == NULL) {
       sprintf(lstmpBuff,"writeLog - Error al abrir %s\n",arch);
       toLog(lstmpBuff,"writeLog-fopen",0,0);
       //salir(0);
       return;
  }
  fprintf(lfp,"%d",n+1);
  fclose(lfp);
  */
  
}   //End writeLog

//Cierra el disp. y desmonta '/mnt/resgu' al presionar Ctrl-C
 void quitOK(int s)
{
 char lstmpBuff[120];

  printf("Programa terminado\n");
  close(s);

  //Elimino archivos de intercambio
  if (remove("/tdf/prog/nac/entra") == -1) toLog("En quitOK, Error al eliminar entra\n",NULL,0,0);
  remove("/tdf/prog/nac/entraTMP");
  remove("/tdf/prog/nac/sale"); 

  sprintf(lstmpBuff,"Programa Terminado - Signal: %d\n",s);
  toLog(lstmpBuff,NULL,0,0);

  exit(0);
}


void RespuestaError(MSG *msg)
{
   toLog("------> RespuestaError\n",NULL,1,1);
   
   switch (msg->tipotrx) {
      case TRX_AUTORIZACION:
           r110AErr(msg);
           break;
      case TRX_REVERSO_SALUD:
           r410AErr(msg);
           break;
/*      case TRX_COMPRA:
           r210Err(msg);
           break;
      case TRX_CONSULTA:
           r110Err(msg);
           break;
      case TRX_REVERSO:
           r410Err(msg);
           break;
*/      //default:

     }
}


//Respuesta 110 error
int r110Err(MSG *msg)
 {
  int i, pmant;
  char laux[15];
  char lstempnac[MAXLONG];
  char lstmpBuff[250];
  
  
   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   writeTPDU(msg);
   writeBCD("0110",4,msg);
   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[4]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]= msg->bitmap[54]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->samtr,12,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR("91",2,msg);             //Codigo de SUCURSAL FUERA DE LINEA.........
   writeSTR(msg->teid,8,msg);
   writeBCD("0012",4,msg);
   sprintf(laux,"00%s",msg->ssahoy);
   writeSTR(laux,12,msg);

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                 //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);          //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t100++;
   writeLog(msg);

   return 0;
 }   //Function R110Err



int r110AErr( MSG *msg)
{
 int i, pmant, desp, p=0;
 char cantPrest[2+1];
 char codPrest[SIZE_PRESTACION+1];
 char descrPrest[SIZE_DESCR_PRESTACION+1];
 char laux[15];
 char lstempnac[MAXLONG];
 char lstmpBuff[250];
 int longi=0;
 char strlong[4+1];

 //Comienza a armar la cadena para enviar al NAC
 msg->pmens=0;
 memset(msg->mensnac,0x00,sizeof(msg->mensnac));
 writeTPDU(msg);
 writeBCD("0110",4,msg);
 clearBitmap(msg);
 msg->bitmap[PRCODE]=msg->bitmap[AMTR]=msg->bitmap[SYTR]=msg->bitmap[NETID]='1';
 msg->bitmap[RETREFNU]=msg->bitmap[RESPCO]=msg->bitmap[TEID]='1';
 msg->bitmap[CAMPO61]='1';
 //Tiene que avisar que hubo error en la comunicacion.
 memset(msg->dismsg,0x00,sizeof(msg->dismsg));
 sprintf(msg->dismsg," SIN RESPUESTA - REINTENTE POR FAVOR ");

 if (msg->config.PideNomencla) msg->bitmap[NOMEN]='1';
 if (msg->config.PideImportes) { msg->bitmap[AMTR2]='1';msg->bitmap[AMTR3]='1';}      
 
 writeBitmap(msg);

 writeBCD(msg->prcode,6,msg);
 writeBCD(msg->amtr,12,msg);
 if (msg->bitmap[AMTR2]=='1') writeBCD(msg->samtr2,12,msg);
 if (msg->bitmap[AMTR3]=='1') writeBCD(msg->samtr3,12,msg);
 writeBCD(msg->sytr,6,msg);
 writeBCD(msg->netid,4,msg);
 writeSTR(msg->retrefnu,12,msg);
 writeSTR("91",2,msg);
 if (msg->bitmap[NOMEN]=='1') writeSTR(msg->nomencla,3,msg);
 writeSTR(msg->teid,8,msg);
 
 if (msg->bitmap[CAMPO61]=='1') {
	  switch (msg->tipoMensaje) {
	     case LISTA_SIMPLE_7:
	       longi = (SIZE_PRESTACION_7 + SIZE_DESCR_PRESTACION) * MAX_PRESTACIONES; 
	  	   sprintf(strlong,"%04d",longi);
	  	   writeBCD(strlong,4,msg);
	  	   for (p=0; p<MAX_PRESTACIONES; p++)
	  	   {
         	   memset(codPrest,0x00,sizeof(codPrest));
	  		   memset(descrPrest,0x00,sizeof(descrPrest));            
	  		   memcpy(codPrest,blancos(SIZE_PRESTACION_7),SIZE_PRESTACION_7);
	  		   memcpy(descrPrest,blancos(SIZE_DESCR_PRESTACION),SIZE_DESCR_PRESTACION);
         
	  		   writeSTR(codPrest,SIZE_PRESTACION_7,msg);
	  		   writeSTR(descrPrest,SIZE_DESCR_PRESTACION,msg);
	  		   desp+=(SIZE_PRESTACION_7+SIZE_DESCR_PRESTACION);
	  	   }
	  	   break;
	     case LISTA_SIMPLE:
	       longi = (SIZE_PRESTACION + SIZE_DESCR_PRESTACION) * MAX_PRESTACIONES; 
	  	   sprintf(strlong,"%04d",longi);
	  	   writeBCD(strlong,4,msg);
           for (p=0; p<MAX_PRESTACIONES; p++)
           {
            
            memset(codPrest,0x00,sizeof(codPrest));
            memset(descrPrest,0x00,sizeof(descrPrest));            
            memcpy(codPrest,blancos(SIZE_PRESTACION),SIZE_PRESTACION);
            memcpy(descrPrest,blancos(SIZE_DESCR_PRESTACION),SIZE_DESCR_PRESTACION);
            
            writeSTR(codPrest,SIZE_PRESTACION,msg);
            writeSTR(descrPrest,SIZE_DESCR_PRESTACION,msg);
            desp+=(SIZE_PRESTACION+SIZE_DESCR_PRESTACION);
       }
       break;
	     case LISTA_IMPORTES:
	     case LISTA_COMPUESTA:
            //Son 245 bytes (CANTIDAD x PRESTACION)
	  	      longi = (SIZE_CANTIDAD + SIZE_PRESTACION + SIZE_DESCR_PRESTACION) * MAX_PRESTACIONES; 
	  		  sprintf(strlong,"%04d",longi);
	  	      writeBCD(strlong,4,msg);
	  	      desp=0;
            for (p=0; p<MAX_PRESTACIONES; p++)
            {
                memset(cantPrest,0x00,sizeof(cantPrest));
                memset(codPrest,0x00,sizeof(codPrest));
                memset(descrPrest,0x00,sizeof(descrPrest));
                
                memcpy(cantPrest,blancos(SIZE_CANTIDAD),SIZE_CANTIDAD);
                memcpy(codPrest,blancos(SIZE_PRESTACION),SIZE_PRESTACION);
                memcpy(descrPrest,blancos(SIZE_DESCR_PRESTACION),SIZE_DESCR_PRESTACION);

                writeSTR(cantPrest,SIZE_CANTIDAD,msg);
                writeSTR(codPrest,SIZE_PRESTACION,msg);
                writeSTR(descrPrest,SIZE_DESCR_PRESTACION,msg);
                desp+=(SIZE_CANTIDAD+SIZE_PRESTACION+SIZE_DESCR_PRESTACION);
            }
            break;
	     default:
	    	 toLog("Falta TipoMensaje!!!\n",NULL,1,1);
	  }
    toLog("\n",NULL,0,0);
 }

 if (msg->bitmap[CAMPO63]=='1') {
    writeBCD("0040",4,msg);
    writeSTR(msg->dismsg,40,msg);
 }

 //Copio en temporal para obtener la longitud del mensaje
 for (i=0;i<msg->pmens;i++)
    lstempnac[i]=msg->mensnac[i];
 memset(msg->mensnac,0x00,sizeof(msg->mensnac));

 memset(laux,0x00,sizeof(laux));
 sprintf(laux,"%02x",msg->pmens);
 sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
 toLog(lstmpBuff,NULL,0,0);

 pmant=msg->pmens;
 msg->pmens = 0;                    //Agrega longitud al comienzo del msg->mensnac
 writeBCD(laux,4,msg);              //LONG

 //Vuelvo a copiar el mensaje en msg->mensnac
 for (i=0;i<pmant;i++)
    msg->mensnac[msg->pmens++]=lstempnac[i];

 msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

 //t100A++;
 writeLog(msg);

 return 0;
}   //Function R110AErr


int r210Err(MSG *msg)
{
int i, pmant;
char laux[15];
char lstempnac[MAXLONG];
char lstmpBuff[250];


   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   writeTPDU(msg);
   writeBCD("0210",4,msg);
   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]='1';
   // ** Venta en 2,3 cuotas, Plan Especial. 
   // ** Envio como respuesta al terminal, el importe que retorno el host, en el campo 04 del mensaje.
   //msg->bitmap[4]='1';
   if (strcmp(msg->dismsg,blancos(39))!=0) {
      msg->bitmap[63]='1';
   }
   writeBitmap(msg);
   writeBCD(msg->prcode,6,msg);
 // ******************************  VENTA EN 2 y 3 Cuotas Plan Especial - Importe de la venta.
   //writeBCD(msg->amtr,12,msg);
 // ******************************  
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR("91",2,msg);       //Sucursal Fuera de Línea
   writeSTR(msg->teid,8,msg);
   if (strcmp(msg->dismsg,blancos(39))!=0) {
      writeBCD("0040",4,msg);
      writeSTR(msg->dismsg,40,msg);
   }

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++)
      lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long: %s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                 //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);          //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t200++;
   writeLog(msg);

   return 0;

 }   //Function R210Err


int r410AErr(MSG *msg)
{
 int i, pmant;
 char laux[15];
 time_t now;
 struct tm z;
 char lstempnac[MAXLONG];
 char lstmpBuff[250];
 
  toLog("------> R410AErr\n",NULL,0,0);

  //Simulo uploadtrout y downloadtrout ya que se trata de un 0400
   time(&now);
   z = *localtime( &now );
   sprintf(msg->shora,"%02d%02d%02d",z.tm_hour,z.tm_min,z.tm_sec);
   sprintf(msg->sdia,"%02d%02d",z.tm_mon+1,z.tm_mday);
   strcpy(msg->sauthid,msg->shora);
   strcpy(msg->srespco,"91");

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));
   writeTPDU(msg);
   writeBCD("0410",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR(msg->srespco,2,msg);
   writeSTR(msg->teid,8,msg);

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++) lstempnac[i]=msg->mensnac[i];
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long:%s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,1,1);

   pmant=msg->pmens;
   msg->pmens = 0;                 //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);          //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t400A++;
   writeLog(msg);
   return 0;
 }   //Function R410A


int r410Err(MSG *msg)
{
 int i, pmant;
 char laux[15];
 time_t now;
 struct tm z;
 char lstempnac[MAXLONG];
 char lstmpBuff[125];

   toLog("------> r410Err\n",NULL,0,0);

  //Simulo uploadtrout y downloadtrout ya que se trata de un 0400
   time(&now);
   z = *localtime( &now );
   sprintf(msg->shora,"%02d%02d%02d",z.tm_hour,z.tm_min,z.tm_sec);
   sprintf(msg->sdia,"%02d%02d",z.tm_mon+1,z.tm_mday);
   strcpy(msg->sauthid,msg->shora);
   strcpy(msg->srespco,"91");

   //Comienza a armar la cadena para enviar al NAC
   msg->pmens=0;
   memset(msg->mensnac,0x00,sizeof(msg->mensnac));
   writeTPDU(msg);
   writeBCD("0410",4,msg);

   clearBitmap(msg);
   msg->bitmap[3]=msg->bitmap[11]=msg->bitmap[12]=msg->bitmap[13]='1';
   msg->bitmap[24]=msg->bitmap[37]=msg->bitmap[38]=msg->bitmap[39]=msg->bitmap[41]='1';
   writeBitmap(msg);

   writeBCD(msg->prcode,6,msg);
   writeBCD(msg->sytr,6,msg);
   writeBCD(msg->shora,6,msg);
   writeBCD(msg->sdia,4,msg);
   writeBCD(msg->netid,4,msg);
   writeSTR(msg->retrefnu,12,msg);
   writeSTR(msg->sauthid,6,msg);
   writeSTR(msg->srespco,2,msg);
   writeSTR(msg->teid,8,msg);

   //Copio en temporal para obtener la longitud del mensaje
   for (i=0;i<msg->pmens;i++) lstempnac[i]=msg->mensnac[i];

   memset(msg->mensnac,0x00,sizeof(msg->mensnac));

   memset(laux,0x00,sizeof(laux));
   sprintf(laux,"%02x",msg->pmens);

   sprintf(lstmpBuff,"Long:%s %d\n",laux,msg->pmens);
   toLog(lstmpBuff,NULL,0,0);

   pmant=msg->pmens;
   msg->pmens = 0;                 //Agrega longitud al comienzo del msg->mensnac
   writeBCD(laux,4,msg);          //LONG

   //Vuelvo a copiar el mensaje en msg->mensnac
   for (i=0;i<pmant;i++)
      msg->mensnac[msg->pmens++]=lstempnac[i];

   msg->mensnac[msg->pmens]=0;                    //Fin de msg->mensnac

   //t400++;
   writeLog(msg);
   return 0;

 }   //Function R410Err


// Emula la función read para un string
void readbuf(char *buffer, char *tmp, int nrochar) {
 int i;
 
 	for (i=0; i<nrochar ; ++i) {
 		tmp[i]=buffer[idxbuf];
 		++idxbuf;
 	}
 	tmp[i]=0x00;
 	
 	return;
}

int writeToMAC(char *disp,MSG *msg)
{
  int lfd,i,res=0;
  
	lfd=open("mensaje-mac.txt",PARAMSFILE,MODEFILE);
	if (lfd !=-1 ) {
		res = write(lfd,"RESP : ",7);
		res = write(lfd,msg->mensnac,msg->pmens);
		res = write(lfd,"\n",1);
		close(lfd);
	}

	memset(disp,0x00,sizeof(disp));
	for (i=0 ; i<msg->pmens ; ++i)
        disp[i]=msg->mensnac[i];
	disp[i]=0x00;
	lanswer=msg->pmens;
	
    return 0;	     
}

