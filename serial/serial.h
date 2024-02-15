/****************************************************************
 * Esta librería realiza la configuración del modem y escritura.*
 * La lectura se implementa fácilmente con la función:			*
 * nbytes=read(FD,buffer,sizeof(buffer));						*
 *																*
 * Autor: Matias Bendel							Fecha: 01/07/07	*
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// baudrate settings are defined in <asm/termbits.h>, which is included by <termios.h>
#define BAUDRATE B38400
// blocking read until READSIZE character/s arrives
#define READSIZE 10
// POSIX compliant source
#define _POSIX_SOURCE 1

int FD;				//File Descriptor

// Prototipos
int Conf_Device(char *);
int Write_Port(char *);
