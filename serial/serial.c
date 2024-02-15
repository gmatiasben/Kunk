#include "serial.h"

int Conf_Device(char *modem_mount) {
  struct termios newtio;

	/*  Abro el dispositivo para lectura y escritura.
	 *  O_NOCTTY: le dice a UNIX que este programa no desea ser un "termina de control"
	 *  para este puerto. Si no se especificase, entonces cualquier entrada (como puede
	 *  ser una se�al de escape CTRL-C) podr�a afectar el proceso.
	 *  O_NDELAY: le dice a UNIX que a este programa no le interesa el estado de la 
	 *  se�al DCD. 
	 */
	//printf("Device to mount: %s\n",modem_mount);
	FD = open(modem_mount, O_RDWR | O_NOCTTY | O_NDELAY);
	if (FD < 0) 
		{ perror("Modo directo. Fall� Open().\n"); return -1; }
	
	//bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
	memset(&newtio,0x00,sizeof(newtio)); /* clear struct for new port settings */

	/*  Control Options (c_cflag)
	 *  BAUDRATE: Setea la tasa en bps
	 *  CS8     : 8 bits de datos
	 *  CLOCAL  : conexi�n local, no hay control del modem
	 *  CREAD   : activo la recepci�n de caracteres 
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
	 *  ~ICANON: entrada no-can�nica, la lectura no se hace l�nea por l�nea
	 *  ISIG: Activaci�n de las se�ales SIGINTR, SIGSUSP, SIGDSUSP, y SIGQUIT 
	 */
	newtio.c_lflag &= ~(ICANON | ISIG);

	newtio.c_cc[VTIME] = 0;     			// tiempo inter caracteres no utilizado
	newtio.c_cc[VMIN] = READSIZE;			// bloque de lectura

	/* se limpia la l�nea del modem y se activa la configuraci�n en el puerto */
	tcflush(FD,TCIFLUSH);

	/*  La constante TCSANOW especifica que todos los cambios ocurrir�n inmediatamente 
	 *  sin esperar a que los datos en la salida sean transmitidos o que los datos en 
	 *  la entrada sean recibidos. 
	 */
	tcsetattr(FD,TCSANOW,&newtio);

	return 0;
}


int Write_Port(char *string)	
{
	// DEBUG: printf("%s\n",string);
	if (write(FD,string,strlen(string)) < 0) 
		{perror("Escribiendo Init String"); return -1;}
	/*  !!! I suspect that tcdrain() does not do what the manpage says:
	 *  tcdrain() waits until all output written to the object
	 *  referred to by FD has been transmitted.  
	 */
	if (tcdrain(FD) < 0)
		{perror("Drenando FD"); return -1;}
	tcflush(FD,TCIFLUSH);

	return 0;
}

