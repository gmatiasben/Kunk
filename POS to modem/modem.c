/************************************************************************
 * mAC IP	(mini Access Controller)									*
 * Autor: Matias Bendel				Fecha: 08/11/07						*
 * 																		*
 * 																		*
 * Descripción: Aplicación que lee y escribe el puerto serie en modo	*
 * asíncrono (protocolo RS-232), a fin de procesar una transacción 		*
 * entre un terminal ingenico(c) y un modem externo US Robotics 56k(c).	*
 * Hubiera sido conveniente realizar el software en modo síncrono,		*
 * ya que la aplicación del terminal está configurado para comunicarse	*
 * vía HDLC con el NAC. Sin embargo, el puerto de la PC no está 		*
 * preparado para dicha función, porque aunque cumple con el standard 	*
 * RS-232C, el formato debería ser del tipo DB-25 para emular el clock 	*
 * de sincronismo.														*
 * 																		*
 ************************************************************************/

// NOTE: Constantes que pueden modificarse
//#define CIUDAD 			"Localidad: Gdor. Gregores"
//#define IPHOST			"192.168.1.3"
//#define IPPORT			"4446"#define MLOGFILE 			"mac"
#define PRESTADOR 			"australomi"


#define FALSE 	0
#define TRUE 	1

#define FLAG1 		"com:"
#define TTYS 		"/dev/ttyS"

#define STX 		0x02
#define ETX 		0x03
#define LF  		0x0A
#define DLE 		0x10
#define ITOA 		0x30
#define MAXMODEMS 	6

#define FALSE 0
#define TRUE 1

#define UNO		0x01	// En binario 0000 0001 
#define MASK	0x80	// En binario 1000 0000
#define	BYTE	8
#define LIMPWD 	':'

#define MAXTIMEOUT			30
#define MAXINTENTOSHOSTS 		2
#define MAXHOSTS 			10
#define MAXLONG 			1024

#define BAUDRATE B19200
#define BAUDINT	38400
#define READSIZE 10
#define _POSIX_SOURCE 1

int FD;				// File Descriptor
int iport;			// interface port -> en este caso Serial
FILE *flog;
char *ptrbuf;		// Pointer to buffer
char m_buffer[512];
char m_tmpBuff[512];
int idxbuf;
int lanswer;

// Prototipos
extern int analisisMAC(char *);
int Start_Module(void); 
int Conf_Device(char *);
int Write_Port(char *);
int Call_Set(int);
int Init_Modem(void);
int Read_Port_for_Handshake(char *, int);
int Read_Port_for_Msg(int *, int *);
void Analyze_Msg(void);
int Str_in_Buffer(char *, int);
int Search_Token(char *, int, int);
int Manage_Debug(int *);
void Reset_All(void);
void Print_End(void);

void mLog(FILE *, char *, int, int);

void intr_hdlr(int);
void end_app(void);


/***************************************************************************
 *        NAME: void intr_hdlr()
 * DESCRIPTION: Handler called when one of the following signals is
 *				received: SIGHUP, SIGINT, SIGQUIT, SIGTERM.
 *				This function stops I/O activity on all channels and
 *				closes all the channels.
 *       INPUT: int signalType
 ***************************************************************************/

void intr_hdlr(int signalType) {
	printf("Finalizando....\n");   
	exit(1);
}

/***************************************************************************
 *        NAME: void end_app()
 * DESCRIPTION: This function stops I/O activity on the modem.
 ***************************************************************************/

void end_app(void) { 
   	if (FD >= 0) close(FD);
   	if (flog != NULL) fclose(flog);
   	exit(1);
}


/****************************************************************************
 *        NAME: int Init(void)
 * DESCRIPTION: Invoca la función para configurar el puerto serie y copia
 * 				una serie de strings útiles para log.
 *       INPUT: void
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/
int Start_Module(void) {
 char *str=(char*)malloc(15*sizeof(char));		// contiene el string del puerto a montar
 char logging[32];
 char welcome[32];
 char city[32];
 FILE *fc;
 
 	system ("clear");

 	// De esta manera creo un archivo de log por modem
 	memset(str,0x00,sizeof(str));
 	memset(logging,0x00,sizeof(logging));
 	strcpy(logging,MLOGFILE);
 	// +1 para que coincida con el numero del modem
 	logging[sizeof(MLOGFILE)-1]=(iport-1)+ITOA+1;
 	logging[sizeof(MLOGFILE)]=0x00;
 	strcat(logging,".log");
	
 	flog = fopen(logging,"a+");
 	if (flog == NULL){ 
 		sprintf(m_tmpBuff,"No se abrió el archivo de log %s\n",MLOGFILE);
 		return -1;	 
 	}	 

 	memset(str,0x00,sizeof(str));
 	strcpy(str,TTYS);
 	str[sizeof(TTYS)-1]=(iport-1)+ITOA;
 	str[sizeof(TTYS)]=0x00;

 	fc = fopen("/usr/local/bin/parenvios","r");
  	if (fc == NULL){ 
 		sprintf(m_tmpBuff,"No se abrió el archivo de nombre de localidad %s\n",MLOGFILE);
 		return -1;	 
 	}	 
 	memset(city,0x00,sizeof(city));
  	fscanf(fc,"%s",city);	city[strlen(city)]=0x00;
  	fscanf(fc,"%s",city);	city[strlen(city)]=0x00;
  	fscanf(fc,"%s",city);	city[strlen(city)]=0x00;
 	
 	memset(welcome,0x00,sizeof(welcome));
 	sprintf(welcome,"COM%d: %s - %s",iport,str,city);
 	mLog(flog,welcome,1,0);
 	
 	if(Conf_Device(str) < 0) {
		mLog(flog,"Error en Conf_device()",1,0);
 		return -1;
 	}
 	
 	free(str);
	return 0;
}


/****************************************************************************
 *        NAME: int Conf_Device(char *modem_mount)
 * DESCRIPTION: Configura el modem segun ciertos requisitos como el hecho
 * 				de leer asicronicamente.
 *       INPUT: char *modem_mount = path del dispositivo, i.e. /dev/ttyS0
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: Cualquier cambio en esta configuración puede determinar
 * 				que el modem deje de funcionar adecuadamente.
 ***************************************************************************/
int Conf_Device(char *modem_mount) {
  struct termios newtio;

	/*  Abro el dispositivo para lectura y escritura.
	 *  O_NOCTTY: le dice a UNIX que este programa no desea ser un "termina de control"
	 *  para este puerto. Si no se especificase, entonces cualquier entrada (como puede
	 *  ser una señal de escape CTRL-C) podría afectar el proceso.
	 *  O_NDELAY: le dice a UNIX que a este programa no le interesa el estado de la 
	 *  señal DCD. 
	 */

	FD = open(modem_mount, O_RDWR | O_NOCTTY | O_NDELAY);
	if (FD < 0) { 
		mLog(flog,"Modo directo. Falló Open().",0,0); 
		return -1; 
	}
	
	//bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
	memset(&newtio,0x00,sizeof(newtio)); /* clear struct for new port settings */

	/*  Control Options (c_cflag)
	 *  BAUDRATE: Setea la tasa en bps
	 *  CS8     : 8 bits de datos
	 *  CLOCAL  : conexión local, no hay control del modem
	 *  CREAD   : activo la recepción de caracteres 
	 */
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD ;
	
	/*  Input Options (newtio.c_iflag)
	 *  IGNPAR: Ignorar errores de paridad
	 *  ICRNL: Mapea CR a NL  
	 */
	newtio.c_iflag = IGNPAR | ICRNL;

	/*  Output Options (newtio.c_oflag) 
	 *  ~OPOST: Salida cruda  
	 */
	newtio.c_oflag &= ~OPOST;
 
	/* Local options (c_lflag)
	 *  ~ICANON: entrada no-canónica, la lectura no se hace línea por línea
	 *  ISIG: Activación de las señales SIGINTR, SIGSUSP, SIGDSUSP, y SIGQUIT 
	 */
	newtio.c_lflag &= ~(ICANON | ISIG);

	newtio.c_cc[VTIME]    = 0;     			// tiempo inter caracteres no utilizado
	newtio.c_cc[VMIN]     = READSIZE;			// bloque de lectura

	/* se limpia la línea del modem y se activa la configuración en el puerto */
	tcflush(FD,TCIFLUSH);

	/*  La constante TCSANOW especifica que todos los cambios ocurrirán inmediatamente 
	 *  sin esperar a que los datos en la salida sean transmitidos o que los datos en 
	 *  la entrada sean recibidos. 
	 */
	tcsetattr(FD,TCSANOW,&newtio);

	return 0;
}


/****************************************************************************
 *        NAME: int Write_Port(char *string)
 * DESCRIPTION: Escribe el string en el puerto serie.
 *       INPUT: char *string = string a escribir
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/
int Write_Port(char *string) {

	if (write(FD,string,strlen(string)) < 0) {
		mLog(flog,"Error: Escribiendo Init String",0,0); 
		return -1;
	}
	/*  Comentario. I suspect that tcdrain() does not do what the manpage says:
	 *  tcdrain() waits until all output written to the object
	 *  referred to by FD has been transmitted.  
	 */
	if (tcdrain(FD) < 0) {
		mLog(flog,"Error: Drenando FD",0,0); 
		return -1;
	}
	tcflush(FD,TCIFLUSH);

	return 0;
}


/****************************************************************************
 *        NAME: int Call_Set(int first)
 * DESCRIPTION: Envia al modem un string de inicio y chequea que el mismo
 * 				responda correctamente. Aguarda la transacción.
 *       INPUT: int first: para la primera vez o en caso de no recibir respuesta
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: No modificar el string de inicialización.
 ***************************************************************************/
int Call_Set(int first) {
  int modemison=FALSE;
  int unplugged=0;
  char auxiliar[15];
  
	/*  Entro a este while la primera vez que se ejecuta el programa
	 *  y cada vez que se está en idle esperando una nueva llamada 
	 */
	while(!modemison) {
		Write_Port("AT\r\0");
		if(Read_Port_for_Handshake("OK\0",1) != -1) {
			modemison=TRUE;
			if(unplugged > 20)  Print_End();
			if(unplugged > 1) {
				while(Init_Modem() == -1) {}
				if(unplugged > 20) system("killall alarm_mac.sh");
			}
		}
		else	{
			// Espera 20 segundos y muestra el msj solo una vez
			if(unplugged==20) {
				mLog(flog,"Error: Verificar conexión y estado de portadora (CD)",1,0);
				sprintf(auxiliar,"./alarm_mac.sh %d &",iport);
				system(auxiliar);
			}
			++unplugged;
		}
	}
	mLog(flog,"Modem responde correctamente",1,0);
	if(first) Init_Modem();
	mLog(flog,"Aguardando transacción...",1,0);
	if(Read_Port_for_Handshake("RING\0",0) < 0) return -1;
	mLog(flog,"Ringing...",1,0);
	if(Read_Port_for_Handshake("CONN\0",20) < 0) return -1;
	mLog(flog,"conexión establecida!",1,0);
	return 0;
}


/****************************************************************************
 *        NAME: int Init_Modem(void)
 * DESCRIPTION: Inicializa el modem
 *       INPUT: void
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/
int Init_Modem(void) {
	/*  String de inicialiación: AT S0=1 V1 X4 &C1 E1 Q0 &M &B1
	 *  S0: Rings to Answer		X4: Advanced Result Codes 
	 *  &C1: Carrier detection	E1: Echo on
	 *  Q0: Result Codes Sent	&M: Asynchronous Mode
	 *  &B1: Fixed DTE Speed  
	 *  &M y &B1 son imprescindibles para el funcionamiento asíncrono
	 */
	if((Write_Port("AT &F0 S0=1 S28=0 B1 E1 Q0 X4 &A0 &B1 &C1 &M\r\0") < 0)) return -1;
	if(Read_Port_for_Handshake("OK\0",5) < 0) return -1;
	mLog(flog,"Inicialización exitosa!",1,0);
	return 0;
}

/****************************************************************************
 *        NAME: int Read_Port_for_Handshake(char *string, int timer)
 * DESCRIPTION: Lee los datos del puerto serie y los escribe en un buffer 
 * 				hasta que se encuentra el string. Función que solo se aplica
 * 				sólo para el handshake de la comunicación.
 *       INPUT: char *string = secuencia de caracteres a detectar
 * 				int timer = establece el tiempo máximo de espera para
 * 							detectar el string
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/
int Read_Port_for_Handshake(char *string, int timer) {
  int nbytes;
  int status;
  int end=FALSE;
  time_t ton,toff,tpoll1,tpoll2;	

	time(&ton);
	time(&tpoll1);
	while(!end)	{
		while ((nbytes=read(FD,ptrbuf,m_buffer+sizeof(m_buffer)-ptrbuf-1)) <= 0) {
			if(timer!=0)	
				{time(&toff); if (difftime(toff,ton)>=timer) return -1;}
			/*  Sólo para waiting eXchange ... revisa el status de la portadora.
			 *  Debería estar DOWN, nunca puede estar UP porque para eso tendría que
			 *  pasar esta etapa. Recordar que si el modem está apagado el status es UP.
			 */
			else	{
				/*  A fin de que no lea el estado de la portadora permanentemente 
				 *  y sólo lo haga cada 1 segundo */
				usleep(1000000/BAUDINT);
				time(&tpoll2); 
				if (difftime(tpoll2,tpoll1)>=1) {
				    time(&tpoll1);
				    ioctl(FD,TIOCMGET,&status);
				    if ((status & TIOCM_CD) == TIOCM_CD) {
				    	printf("ENTRO EN TIOM_CD");
				    	return -1;
				    }
				}
			}
		}
		if(Str_in_Buffer(string,sizeof(string))!=-1) 
			{ptrbuf+=nbytes; end=TRUE;}
	}
	return 0;
}

/****************************************************************************
 *        NAME: int Read_Port_for_Msg(int *posstx, int *posetx)
 * DESCRIPTION: Lee los datos del puerto serie y los escribe en un buffer 
 * 				desde que encuentra posstx hasta posetx. A diferencia de la
 * 				funcion Read_Port_for_Handshake, esta función no realiza
 * 				el análisis sobre el establecimiento de la llamada sino
 * 				sobre los mismos datos que el POS envía.
 *       INPUT: int *posstx = primer flag de secuencia
 * 				int *posetx = último flag de secuencia
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/
int Read_Port_for_Msg(int *posstx, int *posetx) {
  int i;
  int findstx=FALSE,findetx=FALSE;
  int status;			// estado de la línea
  int nbytes=0;			// nro de bytes leidos por ciclo
  int end=FALSE;
  int dle_stat=FALSE;
  int stx=STX,dle=DLE,etx=ETX;
  char aux[5];
  time_t toni,tonc,toff;
  // tiempos para limitar el poleo => toni: ton inicial, tonc: ton ciclico
 
	while(!end)	{
		time(&toni);
		time(&tonc);
		while ((nbytes=read(FD,ptrbuf,m_buffer+sizeof(m_buffer)-ptrbuf-1)) <= 0) {
			time(&toff);
			/*  Cada 1 segundo se fija el estado de la portadora y si luego de
			 *  10 segundos no pasa nada de nada se cierra la transacción 
			 */
			if (difftime(toff,tonc)>=1) {
				time(&tonc);
				/*  Se consulta el estado de la portadora, pero ojo que en caso 
				 *  de que el modem esté apagado se reconoce en estado activo.
				 */
				ioctl(FD, TIOCMGET, &status);
			    // Espero 1 segundo para que el cierre sea limpio
				if ((status & TIOCM_CD) != TIOCM_CD) {
				    while(difftime(toff,tonc)<2) time(&toff);
				    return -1;
				}
				// NOTE: Este tiempo puede modificarse segun corresponda
				if (difftime(toff,toni)>=45)
					return -1;
			}
		}
		// DEBUG
		memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
		for(i=0;i<nbytes;i++) {
			memset(aux,0x00,sizeof(aux));
			sprintf(aux,"%d ",ptrbuf[i]);
			strcat(m_tmpBuff,aux);
		}
		mLog(flog,m_tmpBuff,0,0);

		if(!findstx)
			if ((*posstx=Search_Token(m_buffer,sizeof(m_buffer),stx)) != -1)  {
				findstx=TRUE;
				memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
				sprintf(m_tmpBuff,"STX Encontrado. Posicion: %d",*posstx);
				mLog(flog,m_tmpBuff,0,0);
			}
		if(findstx) {
			if(!findetx) {
				/* Hay que analizar otra posibilidad. Que sucede si dentro de un mismo bloque
				 * existe un ETX con un DLE y mas adelante esta el verdadero ETX? 
				 */
				dle_stat=FALSE;
				while(!dle_stat) {
					if((*posetx=Search_Token(ptrbuf,m_buffer+sizeof(m_buffer)-ptrbuf-1,etx)) != -1) {
						if((int)m_buffer[*posetx+(ptrbuf-m_buffer)-1] != dle)  {
							*posetx+=(ptrbuf-m_buffer);
							dle_stat=TRUE;
							findetx=TRUE;
							memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
							sprintf(m_tmpBuff,"ETX Encontrado. Posicion: %d",*posetx);
							mLog(flog,m_tmpBuff,0,0);
							end=TRUE;
						}
						else {
							mLog(flog,"DLE found",0,0);
							// +1 para evitar el loop infinito
							// ejemplo: X X X 16 ; 3 X X X
							ptrbuf+=(*posetx+1);
							nbytes-=(*posetx+1);
						}
					}
					else
						dle_stat=TRUE;
				}
			}
			// Activar solo para uso de CRC		
			/* if(findetx)
				if((int)lm_tmpBuff[*posetx+1]!=0) end=TRUE; */
		}

		ptrbuf+=nbytes;
	}
	return 0;
}

/****************************************************************************
 *        NAME: void Analyze_Msg(void)
 * DESCRIPTION: Dentro del buffer de datos se encuentran bytes repetidos
 * 				para diferenciar un byte STX/ETX de otro byte real de datos
 * 				que contenga ese mismo valor. Esta función tambien comprueba
 * 				si el largo del mensaje coincide con el largo enviado.
 *       INPUT: void
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/
void Analyze_Msg(void)	{
  int lmensaje;
  char mensaje[512];
  char a,b;
  int i,j,ndle;
  int initmsg;		// variable que contiene la posicion en la que comienza el mensaje
  int posstx;		// posicion absoluta hasta STX
  int posetx;		// posicion absoluta hasta ETX
  int res;
  //time_t t1,t2,t3;
  
	memset(mensaje,0x00,sizeof(mensaje));
  	//time(&t1);
  	while(1)	{
		// Es != -1 solo después de leer el mensaje entero
		if((Read_Port_for_Msg(&posstx,&posetx)) != -1) {
			/* Guardo la longitud que se pasa como dato y lo casteo porque en mensajes como
		 	 * los 500 que superan los 128 bytes el valor resultante en int sería negativo
		 	 */
			b=(unsigned int)m_buffer[posstx+1];
			a=(unsigned int)m_buffer[posstx+2];
			i=(256*a)+b;
			lmensaje=(unsigned char)i;

			/*  Este if lo agrego en caso de que si llegara a entrar ruido luego del 210 y
	 	 	 *  uno de los datos fuera un STX y luego un ETX limito el procesamiento.
	 	 	 */
			if(lmensaje < sizeof(m_buffer)) {
				ndle=0;
				initmsg=posstx+3;	// +3 para evitar STX,a,b

				memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
			
				// Emulando el NAC debo agregar los bytes de longitud
				mensaje[0]=m_buffer[posstx+2];
				mensaje[1]=m_buffer[posstx+1];
				memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
				m_tmpBuff[0]=m_buffer[posstx+2];
				m_tmpBuff[1]=m_buffer[posstx+1];
				
				j=2;
				for(i=0; i<lmensaje+ndle ;i++)	{
					mensaje[j++]= m_buffer[i+initmsg];
					// Salteo los DLE, pero ojo que si vienen dos, uno lo dejo
					if((int) m_buffer[i+initmsg+1]==DLE) 
						{++i;++ndle;}
				}
				mLog(flog,"** Mensaje para ser enviado al validador **",0,iport);
				mLog(flog,mensaje,2,lmensaje+ndle+2);
			}
			// Fuera por ruido... me dice que la longitud es mayor que los bytes que recibi ?!?!
			else {
				mLog(flog,"Error: Longitud mayor que bytes recibidos",1,0);
				return;
			}

			/* DEBUG */
			memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
			sprintf(m_tmpBuff,"Longmens+ndle (DATA): %d",lmensaje+ndle);
			mLog(flog,m_tmpBuff,1,0);
			memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
			sprintf(m_tmpBuff,"PosETX-InitMSG (B RECV): %d",((unsigned char)posetx)-initmsg);
			mLog(flog,m_tmpBuff,1,0);
			
			//time(&t2);
			/*  Será que ab != longitud recibida? Existen 2 posibilidades si es así:
			 *  1. Realmente hubo un error en la recepción del mensaje
			 *  2. Casualmente al entrar ruido se recibió un STX y un ETX con 
			 *     longitudes admisibles. Se puede discriminar? Propongo que si
			 *     recibo más de 6 bytes luego del ETX lo considero ruido. El
			 *     problema es que paro de leer con ETX asi que sigo sin estar 
			 *     seguro de que no sea ruido.
			 */
			if((lmensaje+ndle)!=((unsigned char)posetx)-initmsg) {
				if(m_buffer[(unsigned char)posetx+6]==0)
					mLog(flog,"Ruido? Las longitudes no coinciden",1,0); 
				else
					mLog(flog,"Ruido? Las longitudes no coinciden + continuan llegando datos",1,0); 
				return;
			}
			// Si no hay error... ANALIZO transacción !!!
			else {
				mLog(flog,"Mensaje consistente",1,0); 
				lanswer=0;
				
				analisisMAC(mensaje);	

				mLog(flog,"** Mensaje recibido por validador (TDF/Salud) **",0,0);
				mLog(flog,mensaje,2,lanswer);

				// Debo incorporar el ETX y el STX a fin de emular el NAC. Tambien incluir los DLE
				j=0;
				memset(m_tmpBuff,0x00,sizeof(m_tmpBuff));
				if (lanswer != 0) {
					m_tmpBuff[j++]=2;										// STX
					// Parece que la inversión en el orden de los campos de longitud no es necesaria
					//m_tmpBuff[j++]=mensaje[1];
					//m_tmpBuff[j++]=mensaje[0];
					for(i=0; i<lanswer ;i++)	{							// DLE
						if( (int)mensaje[i]==DLE || (int)mensaje[i]==STX || (int)mensaje[i]==ETX) 
							m_tmpBuff[j++]=DLE;	
						m_tmpBuff[j++]=mensaje[i];
					}

					m_tmpBuff[j++]=3;										// ETX
					m_tmpBuff[j++]=1;										// CRC
					lanswer=j;
					
					mLog(flog,"** Mensaje para enviar al modem **",0,0);
					mLog(flog,m_tmpBuff,2,lanswer);
					
					// Le paso el mensaje al modem
					res = write(FD,m_tmpBuff,lanswer);
					if (res <= 0 || res != lanswer) {
				        sprintf(m_tmpBuff,"Write Dif! Sent=%d Written=%d",lanswer,res);
				        mLog(flog,m_tmpBuff,0,0);
					}
					else {
						sprintf(m_tmpBuff,"Write OK! Sent=%d Written=%d",lanswer,res);
						mLog(flog,m_tmpBuff,0,0);
					}
				}
				// time(&t3);
			}
			
			/*  NOTE: El problema es que los CRC's no coinciden...   
			 *  En la web dice que la función generadora es CRC CCITT x16+x12+x5+1,
			 *  pero no tiene demasiado sentido si el resultado (o sea el resto) es
			 *  de tan sólo 1 byte. Me parece que la función generadora que 
			 *  implementa el terminal es distinto.
			 */
			// if(Check_crc(a,b,initmsg+lmens+ndle+1,ndle,lmens) < 0) end=TRUE;
			Reset_All();
		}
		else return;	// Cuando termina la transacción debería salir AQUI !!
	}
	return;
}


/****************************************************************************
 *        NAME: int Str_in_Buffer(char *string, int sizestr)
 * DESCRIPTION: Compara directamente si el string está dentro del buffer.
 *       INPUT: char *string = string a buscar
 * 				int sizestr = tamaño del string
 *     RETURNS: -1 = Error
 *		 		 ORIG = Posicion de inicio del string dentro de buffer
 *    CAUTIONS: None
 ***************************************************************************/
int Str_in_Buffer(char *string, int sizestr) {
	int i,j=0,orig=0,pts=0;

	// Number of characters inside *string different from 0x00
	for(i=0; i<sizestr; ++i)
		if(string[i]!=0x00)
			++pts;

	// Checking string inside buffer
	for(i=0; i<sizeof(m_buffer); ++i)	{
		if((int)m_buffer[i]==(int)string[j])	{
			if(!j) 
				{orig=i; ++j;}
			else	{
				if((i-orig)!=j) j=0;
				else	
					{++j; if(j==pts) return orig;}
			}
		}
		else j=0;
	}
	return -1;
}


/****************************************************************************
 *        NAME: int Search_Token(char *buftmp, int sizeofbuf, int token)
 * DESCRIPTION: Busca la posicion del token dentro de buftmp y devuelve ese valor.
 *       INPUT: char *buftmp = buffer temporal
 * 				int sizeofbuf = tamaño del buffer temporal
 * 				int token = caracter que se busca
 *     RETURNS: -1 = Error
 *		 		 i = Posicion
 *    CAUTIONS: None
 ***************************************************************************/
int Search_Token(char *buftmp, int sizeofbuf, int token)	{
  int i;
	
	for(i=0; i<sizeofbuf; ++i)
		if((int)buftmp[i]==token)
			return i;
	return -1;
}


void Reset_All(void)	{
	ptrbuf=m_buffer;
	memset(m_buffer,0x00,sizeof(m_buffer));
}


void Print_End(void)	{
	mLog(flog,"-------- END --------",1,0);
}


void mLog(FILE *fpv, char *msg, int screen, int var) {
 int i,j;
 struct tm z;
 time_t now;
 char momento[32];
 char digit[8],strdig[32];

	time(&now);
	z = *localtime(&now);
	sprintf(momento,"%02d-%02d-%04d %02d:%02d:%02d ",z.tm_mday,z.tm_mon+1,z.tm_year+1900,z.tm_hour,
			z.tm_min,z.tm_sec);

	if (msg != NULL)
		// Screen => 0 -> archivo, 1 -> archivo + pantalla, 2 -> archivo formato trx
		switch(screen) {
		case 1:
			printf("%s",momento);
			printf("%s\n",msg);
		case 0:
			fprintf(fpv,"%s",momento);
			fprintf(fpv,"%s\n",msg);
			break;
		case 2:
			j=0;
			while(j<var) {
				fprintf(fpv,"%s",momento);
				memset(strdig,0x00,sizeof(strdig));
				for (i=0; i<8 ; ++i) {
					memset(digit,0x00,sizeof(digit));
					sprintf(digit,"%d ",msg[j+i]);
					strcat(strdig,digit);
				}
				fprintf(fpv,"%s\n",strdig);
				j=j+i;
			}
			break;
		default:
			break;
		}

	fflush(fpv);
	return;
}

