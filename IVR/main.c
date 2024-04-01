/************************************************************************
 * 																		*
 * 								MAIN.C									*
 * 																		*
 * 28/09/07	Arma mensaje de solicitud de validacion y lo envia al 		*
 *		    host externo. Espera la respuesta y responde al que llama.	*
 * 25/04/08	Incluye totales.txt, el nuevo mensaje para discriminar 		*
 *		    prestadores y la búsqueda de los offset/longitudes de los	*
 * 			archivos vox en MySQL.
 * 																		*
 ************************************************************************/

#include "saludcli.h"

int main(void) {
 int	chan;	    /* Channel Number for Index */
 int	chtsdev;	/* Channel/Timeslot Device Descriptor */
 int	mode;      
 long 	evttype;
 struct sigaction handler;    /* Signal handler specification structure */

 	/* Set InterruptSignalHandler() as handler function */
 	handler.sa_handler =  intr_hdlr;
 	/* Create mask that mask all signals */
 	if (sigfillset(&handler.sa_mask) < 0) 
 		end_app();
 	/* No flags */
 	handler.sa_flags = 0;
 	/* Set signal handling for interrupt signals */
 	if (sigaction(SIGINT, &handler, 0) < 0)
 		end_app();
 	
 	fpv = fopen(LOGFILE,"w+");
 	if ( fpv == NULL) printf("No se abrió el archivo de LOG\n");

 	// Busco y copio los datos que estan en la base de datos por única vez
 	/*if ( prompt_read() == -1) {
 		toLog(fpv,NULL,"No se pudieron obtener los paarámetros de los archivos .vox\n",NULL,1,0,1);	 
 		exit(1);
 	}*/
 	
 	//Polled Mode
 	mode = SR_POLLMODE;
 	if ( sr_setparm( SRL_DEVICE, SR_MODEID, &mode ) == -1 ) {
 		toLog(fpv,"No se puede setear Polled Mode\n",NULL,1,0,1);
 		end_app();
 	}

 	sysinit();

 	while ( 1 ) {      
 		if (end) {   // if end = 1, exit app
 			end_app();
 			exit(1);
 		}

 		/*  Esperar que se complete el evento. El argumento le dice cuanto tiempo
 		 *  en milisegundos debe esperar, si es -1 espera indefinidamente. */      
 		if (sr_waitevt(-1) != -1) {       	 
 			/*  Obtener el número de canal que recibió el evento. En realidad esto lo hace
 			 *  la función ATDX_CHNUM, sr_getevtdev determina cuál placa (en gral solo tenemos 1)
 			 *  y sr_getevttype el tipo de evento (TDX_DIAL, TDX_CALLP, etc). */         
    	 
 			chtsdev = sr_getevtdev(); 
 			evttype = sr_getevttype();	 
 			chan = ATDX_CHNUM(chtsdev);

 			if ((chan < 1 ) || (chan > MAXCHANS)) {
 				sprintf(tmpbuff, "Evento desconocido para Disp. %d Ignorado\n",chtsdev );
 				toLog(fpv,tmpbuff,NULL,1,0,1);
 				continue;
 			}	    
	 
 			sprintf(tmpbuff, "EVENTO. Before Process state:%s estado=%s\n",sw_state(canal[chan].state),
    			 sw_prompt(canal[chan].estado)); 
 			toLog(fpv,tmpbuff,NULL,1,chan,1);
	 
 			// Procesar el evento y guardar el proximo estado
 			canal[chan].state = process(chan,evttype);
	 
 			// Comenzar el siguiente estado chequeando el codigo de error retornado
 			if ( next_state(chan) == -1 ) {
 				toLog(fpv,"Error en next_state\n","next_state",1,chan,1);
 				canal[chan].state = ST_ERROR;
 				if (play (chan,st_prompt[PR_OTRAMAS].voxfd) == -1 ) {
 					toLog(fpv,"No se pudo pedir otra mas!!\n",NULL,1,chan,1);
 					if (set_hkstate( chan, DX_ONHOOK ) == -1) {
 						toLog(fpv,"Programa Abortado - set_hlstate ==> -1\n",NULL,1,0,1);
 						end_app();
 					}    
 				}
 			}
 		} /* if sr_wait() */
 	} /* while (1) */
}

