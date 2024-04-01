 /************************************************************************
 * 																		*
 * 								SALUDCLI.C								*
 * 						Programa cliente IVR SALUD						*
 * 																		*
 ************************************************************************/

#include "saludcli.h"

/***************************************************************************
 *        NAME: void intr_hdlr()
 * DESCRIPTION: Handler called when one of the following signals is
 *				received: SIGHUP, SIGINT, SIGQUIT, SIGTERM.
 *				This function stops I/O activity on all channels and
 *				closes all the channels.
 *       INPUT: None
 *      OUTPUT: None.
 *     RETURNS: None.
 *    CAUTIONS: None.
 ***************************************************************************/

void intr_hdlr(int signalType) {
	toLog(fpv,"Finalizando....\n",NULL,1,0,1);   
	end = 1;
}


/***************************************************************************
 *        NAME: init_voxes()
 * DESCRIPTION: Open audio (vox) files specifying permissions.
 *       INPUT: None.
 *      OUTPUT: None.
 *     RETURNS: None.
 *    CAUTIONS: None.
 ***************************************************************************/

void init_voxes() {
 int j;
  
  	for (j=0;j<MAXMSGS;j++){
  		if ((all_msgs[j].fd=open(all_msgs[j].voxfile,O_RDONLY),0666)==-1){
  			sprintf(tmpbuff,"No se abrió %s\n",all_msgs[j].voxfile );
  			toLog(fpv,tmpbuff,"open voxfile",1,0,1);
  		} 
    	else { 
    		sprintf(tmpbuff, "voice file: %s .. Ready\n",all_msgs[j].voxfile); //,all_msgs[j].fd);
    		toLog(fpv,tmpbuff,NULL,1,0,1);
    	}
  	}
   	toLog(fpv,"\n",NULL,1,0,0);
}

/***************************************************************************
 *        NAME: void sysinit()
 * DESCRIPTION: Start D/4x System, Enable CST events, put line ON-Hook
 *				and open VOX files.
 *       INPUT: None.
 *      OUTPUT: None.
 *     RETURNS: None.
 *    CAUTIONS: None.
 ***************************************************************************/

void sysinit() {
 int board,chan;
 char d4xbname[32],d4xname[32];
 int parmval;

   	toLog(fpv,"Inicializando ... \n",NULL,0,0,1);
     
   	init_voxes();  

   	memset( canal, 0, (sizeof( DX_INFO ) * (MAXCHANS+1)) );         

    sprintf(d4xbname, "dxxxB1");
    if ( (board = dx_open(d4xbname,0) ) == -1 ) {
       toLog(fpv,"No se abrió la placa\n","dx_open",1,chan,1);
       end_app();
    }

    parmval=60;		// default = 50
    if (dx_setparm(board,DXBD_FLASHTM,(void *)&parmval) == -1) {
        toLog(fpv,"Error en parametro: Tiempo de flash (&)\n",NULL,0,chan,0);
    	end_app();
    }
    
    parmval=125;	// default = 200
    if (dx_setparm(board,DXBD_PAUSETM,(void *)&parmval) == -1) {
        toLog(fpv,"Error en parametro: Tiempo de pausa (,)\n",NULL,0,chan,0);
		end_app();
	}

    parmval=15;		// default = 5
    if (dx_setparm(board,DXBD_T_IDD,(void *)&parmval) == -1) {
        toLog(fpv,"Error en parametro: Tiempo entre DTMF\n",NULL,0,chan,0);
		end_app();
	}

    parmval=30;		// default = 10
    if (dx_setparm(board,DXBD_TTDATA,(void *)&parmval) == -1)  {
        toLog(fpv,"Error en parametro: Tiempo digito DTMF\n",NULL,0,chan,0);
  		end_app();
  	}
         
   	for ( chan = 1 ; chan <= MAXCHANS ; chan++ ) {
   		sprintf( d4xname, "dxxxB1C%d", chan);
   		if ( (canal[chan].chdev = dx_open(d4xname,0)) == -1 ) {
   			sprintf(tmpbuff, "No se abrió el canal %s,  %d\n",d4xname,errno);
   			toLog(fpv,tmpbuff,"dx_open",1,chan,1);
   			end_app();
		}
   	}
 
   	// Inicializo todos los puertos
   	for ( chan = 1 ; chan <= MAXCHANS ; chan++ ) {
   		// Esto sirve para aumentar el volumen ... por ejemplo en 4dB (SV_ADD4DB)
   		if ( dx_adjsv(canal[chan].chdev,SV_VOLUMETBL,SV_ABSPOS,SV_ADD4DB) == -1 ) {
   			printf("Imposible incrementar el volumen en 2dB\n");
   			printf("Lasterror = %d  Err Msg = %s\n",ATDV_LASTERR(canal[chan].chdev),ATDV_ERRMSGP(canal[chan].chdev));
   			dx_close(canal[chan].chdev);
   			end_app();
   		} 

   		if (set_hkstate(chan,DX_ONHOOK)==-1){
   			toLog(fpv,"SYSINIT - set_hkstate ==> -1\n","set_hkstate",1,chan,1); 
   			end_app();
   		}       
                  
   		// Seteo la velocidad de muestreo de la señal de audio
   		parmval = 8000;
   		if (dx_setparm(canal[chan].chdev,DXCH_PLAYDRATE,(void *)&parmval) == -1)
   			toLog(fpv,NULL,"dx_setparm",1,0,1); 
      
   		if (dx_setevtmsk(canal[chan].chdev,DM_RINGS|DM_WINK) == -1) {  
   			toLog(fpv,NULL,"dx_setevtmsk",1,chan,1);		
   			end_app();
   		}                
     
   		// Setear responder luego de MAXRINGS timbres       
   		if (dx_setrings(canal[chan].chdev,MAXRINGS) == -1 ) {
   			toLog(fpv,NULL,"dx_setrings",1,chan,1);			 
   			dx_close(canal[chan].chdev);  
   			end_app();
   		}    
     
   		/* 	D_LPD 	enable loop pulse detection
   		 * 	D_APD 	enable audio pulse digits detection
   		 * 	D_MF 	enable MF digit detection
   		 * 	D_DPD 	enable dial pulse digit (DPD) detection
   		 * 	D_DPDZ 	zero train DPD detection
   		 */ 

   		if (dx_setdigtyp(canal[chan].chdev,D_DTMF)==-1) {
   			toLog(fpv,NULL,"dx_setdigtyp",1,chan,1);			 
			end_app();
		}    

        if (dx_deltones(canal[chan].chdev) == -1) {
        	toLog(fpv,NULL,"dx_deltones",1,chan,1);
			end_app();
   		}   
   	}

	if (dx_bldstcad(POTS_BUSYTONE,425,30,40,6,30,6,2) == -1)
			end_app();

	for ( chan = 1 ; chan <= MAXCHANS ; chan++ ) {
   		if (dx_addtone(canal[chan].chdev,(int)NULL,0) == -1)
   			end_app();

   		//if (dx_distone( canal[chan].chdev, TONEALL, DM_TONEON|DM_TONEOFF) == -1 )  
   		if (dx_distone(canal[chan].chdev,POTS_BUSYTONE,DM_TONEOFF) == -1) { 
   			toLog(fpv,"Deteccion de Tonos - Error\n","dx_distone",1,chan,1);  
   			end_app();
   		}
 
  		// Iniciar la aplicacion en estado ONHOOK            
   		canal[chan].state = ST_ONHOOK;
   		canal[chan].estado = PR_HOLA;      

   		canal[chan].hola = 0;
   		canal[chan].retry = 0;       
   		canal[chan].indicerror = -1;
   		canal[chan].tipo_operacion = '1';
   
   		toLog(fpv,"Listo para recibir una llamada\n",NULL,1,chan,1);
 
	}


}   


/***************************************************************************
 *        NAME: void end_app()
 * DESCRIPTION: This function stops I/O activity on all channels and
 *              closes all the channels.
 *       INPUT: None
 *      OUTPUT: None.
 *     RETURNS: None.
 *    CAUTIONS: None.
 ***************************************************************************/

void end_app() { 
 int j;
 int chan;      

 	// Reemplazo los archivos de LOG por un unico archivo LOGFILE
   	for (chan = 1; chan <= MAXCHANS; ++chan ) {
   		if (dx_sethook(canal[chan].chdev,DX_ONHOOK,EV_ASYNC)==-1) 
   			toLog(fpv,"END_APP - set_hkstate >> -1\n",NULL,1,chan,1);
        if (dx_close(canal[chan].chdev)==-1) {
   			toLog(fpv,"END_APP - dx_close >> -1\n",NULL,1,chan,1);
   			/* switch(errno) {
   			case EBADF:
   				printf("1\n");
   				break;
   			case EINTR:
   				printf("2\n");
   				break;
   			case EINVAL:
   				printf("3\n");
   				break;
   			} */
        }
   	}
    
 	for (j=0;j<MAXMSGS;j++)
    	close(all_msgs[j].fd);

   	if (fpv != NULL) fclose(fpv);
   	
   	exit(1);

}


/***************************************************************************
 *        NAME: int play( chan, filedesc )
 * DESCRIPTION: Set up IOTT and TPT's and Initiate the Play-Back
 *       INPUT: int chan;	- Index into canal structure
 *				int filedesc;	- File Descriptor of VOX file to Play-Back
 *      OUTPUT: Starts the play-back
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/

int play(int chan, int filedesc) {
 int pl;
 DV_TPT tpt[3];
 DX_CST cst;
   
   	// Rebobinar el archivo   
   	if (lseek( filedesc, 0, SEEK_SET ) == -1 ) {
   		// sprintf( tmpbuff, "PLAY: No se pudo 'rebobinar' el archivo en %s\n",
   		// ATDV_NAMEP( canal[chan].chdev ) );
   		toLog(fpv,"No se pudo 'rebobinar' el archivo\n","lseek",1,chan,1);			 
   		return -1;
   	}

   	// Limpiar y setear IOTT    
   	memset( canal[chan].iott, 0, sizeof(DX_IOTT) );

   	canal[chan].iott[0].io_type = IO_DEV | IO_EOT;
   	canal[chan].iott[0].io_fhandle = filedesc;
   	canal[chan].iott[0].io_length = st_prompt[canal[chan].estado].length; //-1;
   	canal[chan].iott[0].io_offset = st_prompt[canal[chan].estado].offset; // -1;
    
   	// Clear and then Set the DV_TPT structures   
   	memset( tpt, 0, (sizeof( DV_TPT ) * 3) );    

  	// Terminar Play al recibir cualquier tono DTMF 
   	tpt[0].tp_type = IO_CONT;
   	tpt[0].tp_termno = DX_MAXDTMF;
   	tpt[0].tp_length = 1;
   	tpt[0].tp_flags = TF_MAXDTMF;
   	
   	// Terminar Play on MAXSIL
   	tpt[1].tp_type = IO_CONT;
   	tpt[1].tp_termno = DX_MAXSIL;
   	tpt[1].tp_length = 200;
   	tpt[1].tp_flags = TF_MAXSIL;      
   	
   	tpt[2].tp_type   = IO_EOT;  
   	tpt[2].tp_termno = DX_TONE;
   	tpt[2].tp_length = POTS_BUSYTONE;
   	tpt[2].tp_flags = TF_TONE;
   	tpt[2].tp_data = DX_TONEON;

   	// Play VOX file
   	pl=dx_play(canal[chan].chdev,canal[chan].iott,tpt,EV_ASYNC); 
   	if (pl==-1) {
   		//sprintf(tmpbuff,"Play Error: %s\n", ATDV_ERRMSGP(canal[chan].chdev));
   		toLog(fpv,NULL,"dx_play",1,chan,1);			 
   	}
   
   	//*****************************************************************
   	// 	Funcion generadora de eventos     !!!!!!!!!!!!!!!!!
   	//*****************************************************************
   	if(canal[chan].estado == PR_PROCESO) {
   		cst.cst_event = DE_WINK;
   		cst.cst_data  = 0;
   		// printf("Entro en funcion generadora !\n");
   		if (dx_sendevt(canal[chan].chdev,TDX_CST,&cst,sizeof(DX_CST),EVFL_SENDSELF) == -1) {
   			printf("Error message = %s", ATDV_ERRMSGP(canal[chan].chdev));
   			return -1;
   		}
   	}
   	
   	return pl;   
}


/***************************************************************************
 *        NAME: int play_error( chan, err )
 * DESCRIPTION: Reproduce los errores correspondientes a cada estado
 *       INPUT: int chan;	- Index into canal structure
 *				int err;	- Tipo de error
 *      OUTPUT: Starts the play-back
 *     RETURNS: -1 = Error
 *		 		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/

int play_error(int chan, int err) {
 DV_TPT ltpt[4];  
 DX_IOTT lprompt;
 int resplay=0; 
 int chdev;
 int den;
 int aux;
 
   chdev = canal[chan].chdev;
   
   sprintf(tmpbuff,"Playing %s\n",all_msgs[ERRORES].voxfile);        
   toLog(fpv,tmpbuff,NULL,1,chan,1);
  
   // Rebobinar el archivo   
   if ( lseek(all_msgs[ERRORES].fd, 0, SEEK_SET ) == -1 ) {
	   	sprintf( tmpbuff, "PLAY_ERROR: No se pudo 'rebobinar' el archivo errores en %s\n",
		ATDV_NAMEP(chdev));
        toLog(fpv,tmpbuff,"lseek",1,chan,1);
        return -1;
   }
   
   // Rebobinar el archivo   
    if ( lseek(all_msgs[DENEGAR].fd, 0, SEEK_SET ) == -1 ) {
 	   	sprintf( tmpbuff, "PLAY_ERROR: No se pudo 'rebobinar' el archivo denegar en %s\n",
 		ATDV_NAMEP(chdev));
        toLog(fpv,tmpbuff,"lseek",1,chan,1);
        return -1;
    }

    if (dx_clrdigbuf(chdev)==-1) {
        sprintf(tmpbuff, "No se hizo dx_clrdigbuf para %s\n",ATDV_NAMEP(chdev));
        toLog(fpv,tmpbuff,"dx_clrdigbuf",1,chan,1);
   } 
   
   memset( &lprompt, 0, sizeof(DX_IOTT) );   
   memset( ltpt, 0, (sizeof( DV_TPT ) * 4) );    
       
   dx_clrtpt(ltpt,4);   
  
   // Terminar Play al recibir cualquier tono DTMF 
   ltpt[0].tp_type = IO_CONT;
   ltpt[0].tp_termno = DX_MAXDTMF;
   ltpt[0].tp_length = 1;
   ltpt[0].tp_flags = TF_MAXDTMF;
   
   ltpt[1].tp_type = IO_CONT;
   ltpt[1].tp_termno = DX_MAXTIME;
   ltpt[1].tp_length = 250;
   ltpt[1].tp_flags = TF_MAXTIME;
   
   // Terminar Play on MAXSIL
   ltpt[2].tp_type = IO_CONT;
   ltpt[2].tp_termno = DX_MAXSIL;
   ltpt[2].tp_length = 200;
   ltpt[2].tp_flags = TF_MAXSIL; 
   
   ltpt[3].tp_type   = IO_EOT;  
   ltpt[3].tp_termno = DX_TONE;
   ltpt[3].tp_length = POTS_BUSYTONE;
   ltpt[3].tp_flags = TF_TONE;
   ltpt[3].tp_data = DX_TONEON;

   if (canal[chan].indicerror==-1) {
	   // Tabla de conversión
	   switch(err) {
	   case PR_PRESTADOR: case PR_PRESTADOR_ASOC:
		   aux=0;
		   break;
	   case PR_CODIGO:
		   aux=1;
		   break;
	   case PR_AFILIADO:
	   	   aux=2;
	   	   break;
	   case PR_PRESTACION:
		   aux=3;
		   break;
	   case PR_TIPO_PRESTADOR: case PR_OPCION_INICIO: case PR_PRESTACION_OK: case PR_OTRA_PRESTACION:
	   case PR_OPCION_PRESTADOR_ASOC: case PR_OTRAMAS: case PR_REPETIR:
		   aux=4;
		   break;
	   default:
		   aux=5;
	   }

	   lprompt.io_type = IO_DEV|IO_EOT; 			// Desde archivo
	   lprompt.io_bufp = 0;
	   lprompt.io_offset = errores[aux].offset;    	// 0;
	   lprompt.io_length = errores[aux].length;     // -1;
	   lprompt.io_nextp = NULL;    
	   lprompt.io_fhandle = all_msgs[ERRORES].fd; 	// errores[err].voxfd;                   
   
	   if ((resplay=dx_play(chdev,&lprompt,ltpt,EV_ASYNC)) == -1) {
		   toLog(fpv,NULL,"dx_play",1,chan,1);
	   }
   }
   else {

	   toLog(fpv,"Entro en la parte de la denegada\n",NULL,1,chan,1);
      
	   den=canal[chan].indicerror; 
	   lprompt.io_type = IO_DEV|IO_EOT; 			// Desde archivo
	   lprompt.io_bufp = 0;
	   lprompt.io_offset = denegar[err].offset;		// errores[err].offset;    // 0;
	   lprompt.io_length = denegar[err].length;		// errores[err].length;    // -1;
	   lprompt.io_nextp = NULL;    
	   lprompt.io_fhandle = all_msgs[DENEGAR].fd;	//all_msgs[ERRORES].fd; //errores[err].voxfd;                   
   
	   if ((resplay=dx_play(chdev,&lprompt,ltpt,EV_ASYNC)) == -1)
		   toLog(fpv,NULL,"dx_play",1,chan,1);
      
	   canal[chan].indicerror=-1;
   }              

   toLog(fpv,"Final de play_error\n",NULL,1,chan,1);
    
   return(resplay);    
}


/***************************************************************************
 *        NAME: int get_digits( chan, digbufp )
 * DESCRIPTION: Set up TPT's and Initiate get-digits function
 *       INPUT: int chan;		- Index into canal structure
 *		DV_DIGIT *digbufp;	- Pointer to Digit Buffer
 *      OUTPUT: Starts to get the DTMF Digits
 *     RETURNS: -1 = Error
 *		 0 = Success
 *    CAUTIONS: None
 ***************************************************************************/

int get_digits( int chan,DV_DIGIT * digbufp) {
 DV_TPT tpt[5];
 int gd;
   
   memset( tpt, 0, (sizeof( DV_TPT ) * 5) );     
   
   // Terminate GetDigits on Receiving MAXDTMF Digits
   tpt[0].tp_type = IO_CONT;
   tpt[0].tp_termno = DX_MAXDTMF;
   tpt[0].tp_length = canal[chan].cant_digs;
   tpt[0].tp_flags = TF_MAXDTMF;
      
   tpt[1].tp_type = IO_CONT;
   tpt[1].tp_termno = DX_MAXTIME;
   tpt[1].tp_length = 250;   
   tpt[1].tp_flags = TF_MAXTIME;
         
   tpt[2].tp_type = IO_CONT;
   tpt[2].tp_termno = DX_DIGMASK;
   tpt[2].tp_length = DM_P;
   tpt[2].tp_flags = TF_DIGMASK;        
   
   tpt[3].tp_type = IO_CONT;
   tpt[3].tp_termno = DX_MAXSIL;
   tpt[3].tp_length = 100;   
   tpt[3].tp_flags = TF_MAXSIL;

   tpt[4].tp_type   = IO_EOT;  
   tpt[4].tp_termno = DX_TONE;
   tpt[4].tp_length = POTS_BUSYTONE;
   tpt[4].tp_flags = TF_TONE;
   tpt[4].tp_data = DX_TONEON;

   gd=dx_getdig(canal[chan].chdev,tpt,digbufp,EV_ASYNC);
   if (gd==-1)
       toLog(fpv,NULL,"dx_getdig",1,chan,1);			 
  
   return gd;
}


/***************************************************************************
 *        NAME: int set_hkstate( chan, state )
 * DESCRIPTION: Set the channel to the appropriate hook status
 *       INPUT: int chan;		- Index into canal structure
 *				int state;		- State to set channel to
 *      OUTPUT: None.
 *     RETURNS: -1 = Error
 *				 0 = Success
 *    CAUTIONS: None.
 ***************************************************************************/

int set_hkstate(int chan,int state) {
 int chdev;   
 time_t ton,toff;
   
   // Make sure you are in CS_IDLE state before setting the hook status
   
   chdev = canal[chan].chdev;
   
   sprintf(tmpbuff,"State before hkstate: %s (chan=%d,chdev=%d)\n",
		   (ATDX_STATE(chdev)==AT_FAILURE?"FALLO ATDX_STATE":sw_chstate(ATDX_STATE(chdev))),chan,chdev);  
   toLog(fpv,tmpbuff,NULL,1,chan,1);			 
   
   
   if (ATDX_STATE(chdev) != CS_IDLE) {
      if (dx_stopch( chdev, EV_ASYNC )==-1){
    	  sprintf(tmpbuff,"Error en dx_stopch, %d %s\n", ATDV_LASTERR(chdev), ATDV_ERRMSGP(chdev));
    	  toLog(fpv,NULL,"dx_stopch",1,chan,1);			 
    	  return -1;	           
      }
      time(&ton);
      toLog(fpv,"Waiting for idle state\n",NULL,1,chan,1);			 
      while ( ATDX_STATE(chdev) != CS_IDLE ) {
           time(&toff);
           if (difftime(toff,ton) >= 35) {
        	   toLog(fpv,"ERROR: NO VOLVIO A ESTADO IDLE...",NULL,1,chan,1);
        	   break;
           }  
      }
      if (ATDX_STATE(chdev) != CS_IDLE) { 
    	  toLog(fpv,"Programa Abortado!!\n",NULL,1,chan,1);
    	  return -1;	           
      }
      toLog(fpv,"Got Idle State!\n",NULL,1,chan,1);
   }
         
   if (state==DX_OFFHOOK) {       
       if ( dx_sethook( chdev, DX_OFFHOOK, EV_ASYNC ) == -1 ) {
          //sprintf( tmpbuff, "No se pudo setear el canal de %s a Off-Hook (%s)\n",
    	  //ATDV_NAMEP(chdev), ATDV_ERRMSGP( chdev ) );
          toLog(fpv,NULL,"dx_set-OFF-hook",1,chan,1);
          return( -1 );
       }  
   } 
   else {
       if ( dx_sethook( chdev, DX_ONHOOK,EV_ASYNC ) == -1 ) {
    	  //sprintf( tmpbuff, "No se pudo setear el canal de %s a On-Hook (%s)\n",
    	  //ATDV_NAMEP( chdev ), ATDV_ERRMSGP( chdev ) );
    	  toLog(fpv,NULL,"dx_set-ON-hook",1,chan,1);
          return( -1 );        
       }
       if ( dx_clrdigbuf( chdev ) == -1 ) {
         //sprintf( tmpbuff, "No se hizo dx_clrdigbuf para %s\n",ATDV_NAMEP( chdev ) );
          toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
       }      
   }   
            
   sprintf(tmpbuff,"State after hkstate: %s\n",sw_chstate(ATDX_STATE(chdev)));  
   toLog(fpv,tmpbuff,NULL,1,chan,1);
   return(0);
}


/***************************************************************************
 *        NAME: int process( chan, event )
 * DESCRIPCION: Comenzar procesamiento inicial del evento recibido
 *       INPUT: int chan;	- Indice de canal
 *				int event;	- Evento siendo procesado
 *      OUTPUT: Nada
 *     RETURNS: Nuevo estado del canal.
  ***************************************************************************/
    
int process( int chan, int event ) {
 DX_CST *cstp;
 DX_EBLK *eblk;
 long term=0;  
 
/*
 *  TDX_PLAY 		129
 *  TDX_RECORD 		130
 *  TDX_GETDIG 		131
 *  TDX_DIAL 		132
 *  TDX_CALLP 		133
 *  TDX_CST 		134
 *  TDX_SETHOOK 	135
 *  TDX_WINK 		136
 *  TDX_ERROR 		137
 *  TDX_PLAYTONE 	138
 *  TDX_NOSTOP 		141
 *  TDX_TXDATA 		144
 *  TDX_RXDATA 		145
 */
   
   // De acuerdo al evento recibido      

	sprintf(tmpbuff," L %s\n",sw_evttype(event));
	toLog(fpv,tmpbuff,NULL,1,chan,1);
	
	switch (event) {
 	// CST: Call Status Transition
 	case TDX_CST:   
	   /*  Get pointer to event data in call status transition struct
       *
       *		typedef struct DX_CST {
       *			unsigned short cst_event;
       *			unsigned short cst_data;
       *        } DX_CST;
	   */
	  	
 	  	cstp = (DX_CST *) sr_getevtdatap();
 	  	sprintf(tmpbuff,"    L %s\n",sw_cstevent(cstp->cst_event));
 	  	toLog(fpv,tmpbuff,NULL,1,chan,1);

 	  	switch ( cstp->cst_event ) {
 	  	case DE_RINGS:   
 	  		// disp_status(chan,"Listo para Aceptar una Llamada" );
 	  		// Llamada entrante, se recibieron RINGS	 
 	  		if ( canal[chan].state == ST_WTRING ) 
 	  			return(ST_OFFHOOK);
 	  		else  
 	  			toLog(fpv,"Se recibio DE_RINGS en estado <> ST_WTRING\n",NULL,1,chan,1);
 	  		break;
 	  		/* Para el caso de BusyTone ! */
 	  	case DE_TONEON:
 	  		if (canal[chan].hola!=0)
 	  			return(ST_WAIT);  
 	  		/* Esto sucede porque existen casos en los que luego de recibir el ring al realizar
 	  		 * el hook off ya hay tono de ocupado (generalmente esto pasa con las PBX), entonces
 	  		 * la aplicación queda esperando el próximo evento que nunca sucede ... por alguna razon
 	  		 * se queda en IDLE total.
 	  		 */
 	  		else
 	  			return(ST_WTRING);
 	  	/* Este evento nunca sucede en la realidad (sinonimo de FLASH), pero lo emulo para realizar
 	  	 * los procesos de envio del mensaje y reproduccion del prompt "Aguarde unos instantes ..." */
 	  	case DE_WINK:
 	  		canal[chan].resultado=operacion(chan);
 	  		return(ST_CONCURRENT);
 	  	// case DE_LCOFF: return(ST_ONHOOK);	           	     
 	  	// case DE_DIGITS:              
 	  	// case DE_DIGOFF: 
 	  	// case DE_LCREV:
 	  	// case DE_TONEOFF:
 	  	default:
 	  		sprintf(tmpbuff,"Algun otro CST event %s\n",ATDV_NAMEP(sr_getevtdev()));                            	 
 	  		toLog(fpv,tmpbuff,NULL,1,chan,1);
 	  	}
    break;
      
    // Get Digits Completed
 	case TDX_GETDIG:		       
       if (strlen(canal[chan].digbuf.dg_value)==0) {
    	   toLog(fpv,"Ningun otro digito ingresado\n",NULL,1,chan,1);
    	   return(ST_ONHOOK);  
       }  
   
    // Se completó PLAY                      
 	case TDX_PLAY:
 		/*  The ATDX_TERMMSK() function returns a bitmap containing the reason(s) for the 
 		 *  last termination on the channel chdev 
 		 */
       term = ATDX_TERMMSK(canal[chan].chdev);     
       
       if ( term & TM_TONE ) return (ST_ONHOOK);
       if ( (term & TM_MAXSIL) && (strlen(canal[chan].digbuf.dg_value)==0) ) return (ST_ONHOOK);        	   
       if ( (term & TM_MAXNOSIL) && (strlen(canal[chan].digbuf.dg_value)==0) ) return (ST_ONHOOK); 
       if ( (term & TM_MAXTIME) && (strlen(canal[chan].digbuf.dg_value)==0) ) return (ST_ONHOOK); 
       // if ( term & TM_LCOFF ) return (ST_ONHOOK);
       // Esto quiere decir que espera cuando sólo recibió un #
       if ( (term & TM_DIGIT) && strlen(canal[chan].digbuf.dg_value)==1 ) {
    	   toLog(fpv,"Se recibio #!!\n",NULL,1,chan,1);
	  	   canal[chan].retry++;
	  	   if (canal[chan].retry==MAXRETRIES) { 
	  		   canal[chan].estado=PR_OTRAMAS;
	  		   return (ST_ERROR);
	  	   } 
	  	   return (ST_PLAY);            
       }  

       return( curr_state(chan,event) );
    	   
    // Set-Hook Complete	
 	case TDX_SETHOOK:
	    // Se consulta porqué se produjo el cambio de estado
        term = ATDX_TERMMSK(canal[chan].chdev);  
        // The structure indicates which call status transition event occurred
        eblk = (DX_EBLK *) sr_getevtdatap();
     
        return( curr_state(chan,event) );

    case TDX_ERROR:
	    memset(tmpbuff,0x00,sizeof(tmpbuff));
        sprintf(tmpbuff,"TDX_ERROR .... %s\n",ATDV_ERRMSGP(canal[chan].chdev));              
        toLog(fpv,NULL,tmpbuff,1,chan,1);
        return(ST_ONHOOK);         
   
    default:
	    memset(tmpbuff,0x00,sizeof(tmpbuff));
	   	sprintf(tmpbuff,"Evento Desconocido %d, State = %d\n",event,canal[chan].state);
        toLog(fpv,tmpbuff,NULL,1,chan,1);
        return( curr_state(chan,event) );
   }
  
   return( canal[chan].state );       
}


/***************************************************************************
 *        NAME: int curr_state( chan, event )
 * DESCRIPTION: Completar el procesamiento del Estado Corriente.
 *				Identificar el proximo estado
 *       INPUT: int chan;	- Indice en canal
 *				int event;	- Evento siendo procesado
 *      OUTPUT: Ninguna
 *     RETURNS: Proximo Estado del Canal
 *    CAUTIONS: None
 ***************************************************************************/

int curr_state(int chan, int event) { 
 int cantdigs;
  
   switch (canal[chan].state) {
	   
   case ST_ONHOOK:
	  reset_all(chan);
	  return(ST_WTRING);
   
   case ST_WTRING:      
      return(ST_OFFHOOK);

   case ST_OFFHOOK:
      return(ST_PLAY);
      
   case ST_GETDIGIT:         
   // Validar Digitos para el canal
      sprintf(tmpbuff,"Se ingresaron dígitos: %s\n",canal[chan].digbuf.dg_value);
      toLog(fpv,tmpbuff,NULL,1,chan,1);
      
      memset(tmpbuff,0x00,sizeof(tmpbuff));
      sprintf(tmpbuff,"Estado %s \n",sw_prompt(canal[chan].estado));
      toLog(fpv,tmpbuff,NULL,1,chan,1);

      //De acuerdo al estado guardo los digitos en el lugar donde van            
      switch (canal[chan].estado) {
			case PR_TIPO_PRESTADOR:
			    cantdigs = strlen(canal[chan].digbuf.dg_value);
	            memset(canal[chan].msg_tipo_prestador,0,cantdigs);
	            strncpy(canal[chan].msg_tipo_prestador,canal[chan].digbuf.dg_value,cantdigs);
	            // Lo que se hace es limpiar el string de todos los carcateres no decimales
	            clean_num(canal[chan].msg_tipo_prestador,chan);
	            cantdigs=strlen(canal[chan].msg_tipo_prestador);

				if ((cantdigs == 0) || (canal[chan].digbuf.dg_value[0]=='#')
						|| (cantdigs > long_campo[canal[chan].estado])) {
					toLog(fpv,"No ingreso Tipo de prestador\n",NULL,1,chan,1);
					canal[chan].retry++;
					if (canal[chan].retry==MAXRETRIES) { 
						canal[chan].estado=PR_OTRAMAS;
						return(ST_ERROR);
					}	    
					return (ST_INVALID); 
				}   
	            if (dx_clrdigbuf(canal[chan].chdev)==-1) 
	            	toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
		        sprintf(tmpbuff,"Tipo prestador: %d \n",atoi(canal[chan].msg_tipo_prestador));
	    	    toLog(fpv,tmpbuff,NULL,1,chan,1);

	            canal[chan].indicerror=-1;
		        canal[chan].retry=0;    
		        
				if( atoi(canal[chan].msg_tipo_prestador)>MAXPRESTADORES || 
						atoi(canal[chan].msg_tipo_prestador)<1 ){
					toLog(fpv,"Tipo de Operacion Desconocida!\n",NULL,1,chan,1);
					return (ST_INVALID);
				}

				canal[chan].estado=PR_OPCION_INICIO;
    
				return (ST_PLAY);

    	case PR_OPCION_INICIO:
			reset_cycle(chan);
            cantdigs = strlen(canal[chan].digbuf.dg_value);  	    
            if ((cantdigs == 0) || (canal[chan].digbuf.dg_value[0]=='#')
            || (cantdigs > long_campo[canal[chan].estado])) {
		           toLog(fpv,"No ingreso Tipo de Operacion\n",NULL,1,chan,1);
		           canal[chan].retry++;
		           if (canal[chan].retry==MAXRETRIES) { 
		        	   canal[chan].estado=PR_OTRAMAS;
		        	   return(ST_ERROR);
		           }	    
		           return (ST_INVALID); 
            }   
            canal[chan].tipo_operacion=canal[chan].digbuf.dg_value[0];            
            sprintf(tmpbuff,"Tipo de Operación: %c\n",canal[chan].tipo_operacion);
            toLog(fpv,tmpbuff,NULL,1,chan,1);
	    
            if (dx_clrdigbuf(canal[chan].chdev)==-1)
            	toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
            
            canal[chan].retry=0; 	    
            
            // Cualquier operación requiere luego el nro de prestador.
            // La bifurcación sucederá en el siguiente estado.
            switch(canal[chan].tipo_operacion){
            case '1':
            case '2':
        		canal[chan].estado=PR_PRESTADOR;
            	break;
            /* LIBRE para transferencia de llamadas
             * case '3':
             * 		break; 
             *	Definir ... hay que reproducir algun mensaje? Como se va a implementar?
             *	En caso afirmativo habrá que hacerlo con el flash.
             */
            default:
            	sprintf(tmpbuff,"Tipo de Operacion Desconocida! %c\n",canal[chan].tipo_operacion);
    		    toLog(fpv,tmpbuff,NULL,1,chan,1);
    		    return (ST_INVALID);
            }
            
            return (ST_PLAY);

      	case PR_PRESTADOR:
			cantdigs = strlen(canal[chan].digbuf.dg_value);
            memset(canal[chan].msg_prestador,0,cantdigs);
            strncpy(canal[chan].msg_prestador,canal[chan].digbuf.dg_value,cantdigs);
            // Lo que se hace es limpiar el string de todos los carcateres no decimales
            clean_num(canal[chan].msg_prestador,chan);
            cantdigs=strlen(canal[chan].msg_prestador);
	         
            // Esto es porque la longitud del CUIT es fijo -> 11 dígitos
            if ((cantdigs == 0) || cantdigs > long_campo[canal[chan].estado] ||
            cantdigs < long_campo[canal[chan].estado] ) {
            	toLog(fpv,"Numero de prestador vacio o erroneo\n",NULL,1,chan,1);
            	canal[chan].retry++;
            	if (canal[chan].retry==MAXRETRIES) {
            		canal[chan].estado=PR_OTRAMAS;
            		return (ST_ERROR);
            	}
            	return (ST_INVALID);
            }
            
            // TODO: Funcion a implementar
            /* if(check_prestador(canal[chan].msg_prestador)==-1) {
            	toLog(fpv,"Numero de prestador ingresado no valido\n",NULL,1,chan,1);
            	canal[chan].retry++;
            	if (canal[chan].retry==MAXRETRIES) {
            		canal[chan].estado=PR_OTRAMAS;
            		return (ST_ERROR);
            	}
            	return (ST_INVALID);
            } */
             	
            if (dx_clrdigbuf(canal[chan].chdev)==-1) 
            	toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
	        sprintf(tmpbuff,"Prestador: %s \n",canal[chan].msg_prestador);
    	    toLog(fpv,tmpbuff,NULL,1,chan,1);

            canal[chan].indicerror=-1;
	        canal[chan].retry=0;

	        switch(canal[chan].tipo_operacion){
            case '1':
    	        canal[chan].estado=PR_AFILIADO;
    	        break;
            case '2':
        		canal[chan].estado=PR_CODIGO;
            	break;
            }

	        return (ST_PLAY);
	    
      	case PR_CODIGO:    
           	cantdigs = strlen(canal[chan].digbuf.dg_value);
            memset(canal[chan].msg_codigo,0,cantdigs);
            strncpy(canal[chan].msg_codigo,canal[chan].digbuf.dg_value,cantdigs);
    	    clean_num(canal[chan].msg_codigo,chan);
            cantdigs=strlen(canal[chan].msg_codigo);

            if ( (cantdigs == 0) || cantdigs > long_campo[canal[chan].estado] ) {
            	toLog(fpv,"Numero de afiliado -> cantidad incorrecta de digitos\n",NULL,1,chan,1);
       	        canal[chan].retry++;
    	        if (canal[chan].retry==MAXRETRIES) {
    	           canal[chan].estado=PR_OTRAMAS;
    	           return (ST_ERROR);
    	        }
                return (ST_INVALID);
            }

      		if (dx_clrdigbuf(canal[chan].chdev)==-1)
      			toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
   	        
      		sprintf(tmpbuff,"Codigo para anulacion: %s\n",canal[chan].msg_codigo);
    	        toLog(fpv,tmpbuff,NULL,1,chan,1);

    	    canal[chan].indicerror=-1;
     	    canal[chan].retry=0;
    	    canal[chan].estado=PR_PROCESO;
    	    return (ST_PLAY);
    		        
        case PR_AFILIADO:
        	cantdigs = strlen(canal[chan].digbuf.dg_value);
            memset(canal[chan].msg_afiliado,0,cantdigs);
            strncpy(canal[chan].msg_afiliado,canal[chan].digbuf.dg_value,cantdigs);
	        clean_num(canal[chan].msg_afiliado,chan);
            cantdigs=strlen(canal[chan].msg_afiliado);

            if ( (cantdigs == 0) || cantdigs > long_campo[canal[chan].estado] ||
            		cantdigs < 6) {
    	       toLog(fpv,"Numero de afiliado -> cantidad incorrecta de digitos\n",NULL,1,chan,1);
   	           canal[chan].retry++;
	           if (canal[chan].retry==MAXRETRIES) {
	        	   canal[chan].estado=PR_OTRAMAS;
	        	   return (ST_ERROR);
	           }
               return (ST_INVALID);
            }

            // TODO: Funcion a implementar
            /* if(check_afiliado(canal[chan].msg_afiliado)==-1) {
            	toLog(fpv,"Numero de afiliado ingresado no valido\n",NULL,1,chan,1);
            	canal[chan].retry++;
            	if (canal[chan].retry==MAXRETRIES) {
            		canal[chan].estado=PR_OTRAMAS;
            		return (ST_ERROR);
            	}
            	return (ST_INVALID);
        	} */

      		if (dx_clrdigbuf(canal[chan].chdev)==-1)
		        toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
	        sprintf(tmpbuff,"Afiliado: %s\n",canal[chan].msg_afiliado);
	        toLog(fpv,tmpbuff,NULL,1,chan,1);

	        canal[chan].indicerror=-1;
 	        canal[chan].retry=0;
	        canal[chan].np=0;
	        canal[chan].estado=PR_PRESTACION;
	        return (ST_PLAY);
	 
	    case PR_PRESTACION:
        	cantdigs = strlen(canal[chan].digbuf.dg_value);
            memset(canal[chan].msg_prestacion[canal[chan].np],0,cantdigs);
            strncpy(canal[chan].msg_prestacion[canal[chan].np],canal[chan].digbuf.dg_value,cantdigs);

	        clean_num(canal[chan].msg_prestacion[canal[chan].np],chan);
            cantdigs=strlen(canal[chan].msg_prestacion[canal[chan].np]);

            if ((cantdigs == 0) || cantdigs > long_campo[canal[chan].estado] ) {
    	       toLog(fpv,"Numero de prestacion -> cantidad incorrecta de digitos\n",NULL,1,chan,1);
   	           canal[chan].retry++;
	           if (canal[chan].retry==MAXRETRIES) {
	        	   canal[chan].estado=PR_OTRAMAS;
	        	   return (ST_ERROR);
	           }
               return (ST_INVALID);
            }

            // TODO: Funcion a implementar
            /* if(check_prestacion(canal[chan].msg_prestacion)==-1) {
            	toLog(fpv,"Numero de prestacion ingresada no valido\n",NULL,1,chan,1);
            	canal[chan].retry++;
            	if (canal[chan].retry==MAXRETRIES) {
            		canal[chan].estado=PR_OTRAMAS;
            		return (ST_ERROR);
            	}
            	return (ST_INVALID);
        	} */

	        if (dx_clrdigbuf(canal[chan].chdev)==-1)
		        toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
	        sprintf(tmpbuff,"Prestacion: %s\n",canal[chan].msg_prestacion[canal[chan].np]);
	        toLog(fpv,tmpbuff,NULL,1,chan,1);

	        leer_digitos(chan,cantdigs,canal[chan].msg_prestacion[canal[chan].np]);

	        canal[chan].indicerror=-1;
	        canal[chan].retry=0;
	        canal[chan].estado=PR_PRESTACION_OK;
	        return (ST_PLAY);
	    
        case PR_PRESTACION_OK:
   			cantdigs = strlen(canal[chan].digbuf.dg_value);

	    	if ((cantdigs == 0) || (canal[chan].digbuf.dg_value[0]=='#')
	    	|| (cantdigs > long_campo[canal[chan].estado])) {
	    		toLog(fpv,"Opcion invalida\n",NULL,1,chan,1);
	    		canal[chan].retry++;
	    		if (canal[chan].retry==MAXRETRIES) { 
	    			canal[chan].estado=PR_OTRAMAS;
	    			return(ST_ERROR);
	    		}	    
	    		return (ST_INVALID); 
	    	}   

	    	canal[chan].prestacion_ok=canal[chan].digbuf.dg_value[0];            

	    	if (dx_clrdigbuf(canal[chan].chdev)==-1)
	    		toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
	    	
	        canal[chan].indicerror=-1;
	    	canal[chan].retry=0;
		 
	    	switch(canal[chan].prestacion_ok){
            case '1':
            	if (canal[chan].np < (NPRESTACION-1)) {
            		sprintf(tmpbuff,"El codigo de prestacion es correcta? Si\n");
            		toLog(fpv,tmpbuff,NULL,1,chan,1);
            		canal[chan].estado=PR_OTRA_PRESTACION;
           	}	
            	else
            		canal[chan].estado=PR_PROCESO;
            	break;
            case '2': 
            	sprintf(tmpbuff,"El codigo de prestacion es correcta? No\n");
            	toLog(fpv,tmpbuff,NULL,1,chan,1);
            	canal[chan].estado=PR_PRESTACION;
            	break;
            default:
            	sprintf(tmpbuff,"Tipo de Operacion Desconocida! %c\n",canal[chan].prestacion_ok);
            	toLog(fpv,tmpbuff,NULL,1,chan,1);
            	// canal[chan].estado=PR_PRESTACION_OK;
            	return (ST_INVALID);
	    	}

	        return (ST_PLAY);
        	
        	
   	    case PR_OTRA_PRESTACION:
           	cantdigs = strlen(canal[chan].digbuf.dg_value);
  
            if ((cantdigs == 0) || (canal[chan].digbuf.dg_value[0]=='#')
            || (cantdigs > long_campo[canal[chan].estado])) {
            	toLog(fpv,"Opción invalida\n",NULL,1,chan,1);
            	canal[chan].retry++;
            	if (canal[chan].retry==MAXRETRIES) { 
            		canal[chan].estado=PR_OTRAMAS;
            		return(ST_ERROR);
            	}	    
            	return (ST_INVALID); 
            }   

            canal[chan].mas_prestacion=canal[chan].digbuf.dg_value[0];            

	        if (dx_clrdigbuf(canal[chan].chdev)==-1)
	        	toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
                        
	        canal[chan].indicerror=-1;
	        canal[chan].retry=0;
            
            switch(canal[chan].mas_prestacion){
            case '1':
            	sprintf(tmpbuff,"Otra prestación? Si\n");
            	toLog(fpv,tmpbuff,NULL,1,chan,1);
            	canal[chan].np++;
            	canal[chan].estado=PR_PRESTACION;
            	break;
            case '2': 
            	sprintf(tmpbuff,"Otra prestación? No\n");
            	toLog(fpv,tmpbuff,NULL,1,chan,1);
				//canal[chan].estado=PR_OPCION_PRESTADOR_ASOC;
				canal[chan].estado=PR_PROCESO;
            	break;
            default:
            	sprintf(tmpbuff,"Tipo de Operacion Desconocida! %c\n",canal[chan].tipo_operacion);
            	toLog(fpv,tmpbuff,NULL,1,chan,1);
            	// canal[chan].estado=PR_OTRA_PRESTACION;
            	return (ST_INVALID);
            }
                       
   	        return (ST_PLAY);
        
   	    case PR_OPCION_PRESTADOR_ASOC:
       		cantdigs = strlen(canal[chan].digbuf.dg_value);

   	    	if ((cantdigs == 0) || (canal[chan].digbuf.dg_value[0]=='#')
   	    	|| (cantdigs > long_campo[canal[chan].estado])) {
   	    		toLog(fpv,"Opción invalida\n",NULL,1,chan,1);
   	    		canal[chan].retry++;
   	    		if (canal[chan].retry==MAXRETRIES) { 
   	    			canal[chan].estado=PR_OTRAMAS;
   	    			return(ST_ERROR);
   	    		}	    
   	    		return (ST_INVALID); 
   	    	}   

   	    	canal[chan].prestador_asoc=canal[chan].digbuf.dg_value[0];            

   	    	if (dx_clrdigbuf(canal[chan].chdev)==-1)
   	    		toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
                   
	        canal[chan].indicerror=-1;
   	    	canal[chan].retry=0;
   		 
	    	switch(canal[chan].prestador_asoc) {
            case '1':
   	    		sprintf(tmpbuff,"Desea ingresar prestador asociado? Si\n");
   	    		toLog(fpv,tmpbuff,NULL,1,chan,1);
             	canal[chan].estado=PR_PRESTADOR_ASOC;
            	break;
            case '2': 
   	    		sprintf(tmpbuff,"Desea ingresar prestador asociado? No\n");
   	    		toLog(fpv,tmpbuff,NULL,1,chan,1);
             	canal[chan].estado=PR_PROCESO;
            	break;
            default:
            	sprintf(tmpbuff,"Tipo de Operacion Desconocida! %c\n",canal[chan].prestador_asoc);
            	toLog(fpv,tmpbuff,NULL,1,chan,1);
             	return (ST_INVALID);
	    	}
 
   	        return (ST_PLAY);

   	    case PR_PRESTADOR_ASOC:
            cantdigs = strlen(canal[chan].digbuf.dg_value);
            memset(canal[chan].msg_prestador_asoc,0,cantdigs);
            strncpy(canal[chan].msg_prestador_asoc,canal[chan].digbuf.dg_value,cantdigs);
            // Lo que se hace es limpiar el string de todos los carcateres no decimales
            clean_num(canal[chan].msg_prestador_asoc,chan);
            cantdigs=strlen(canal[chan].msg_prestador_asoc);
	         
            if ((cantdigs == 0) || cantdigs > (long_campo[canal[chan].estado]) ||
            	cantdigs < (long_campo[canal[chan].estado])) {
            	toLog(fpv,"Numero de prestador asociado vacio o erroneo\n",NULL,1,chan,1);
            	canal[chan].retry++;
            	if (canal[chan].retry==MAXRETRIES) {
            		canal[chan].estado=PR_OTRAMAS;
            		return (ST_ERROR);
            	}
            	return (ST_INVALID);
            }

            if (dx_clrdigbuf(canal[chan].chdev)==-1) 
  		           toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);

            sprintf(tmpbuff,"Prestador asociado: %s\n",canal[chan].msg_prestador_asoc);
    	    toLog(fpv,tmpbuff,NULL,1,chan,1);
	       
            canal[chan].indicerror=-1;
	        canal[chan].retry=0;
	        canal[chan].estado = PR_PROCESO;

            return (ST_PLAY);
	        
	    // Para codigo de autorización
   	    case PR_REPETIR:    		    	
	    	canal[chan].retry = 0;            
	    	cantdigs = strlen(canal[chan].digbuf.dg_value);  	    
	    	if (canal[chan].digbuf.dg_value[0] == '1') {
		        switch (canal[chan].tipo_operacion) {
			    case '1': 
			    	resultado_autorizacion(NULL,chan);
			    	break;
			    case '2': 
			    	resultado_anulacion(NULL,chan);
			    	break;
		        }
	    		canal[chan].estado = PR_REPETIR;
	    	} 
	    	else {
	       		if(canal[chan].nroorden >= MAXORDEN)
		    		canal[chan].estado = PR_ADIOS;
	       		else
	       			canal[chan].estado = PR_OTRAMAS;            	
	    	}
	    	
	    	return (ST_PLAY);    	    
	
	    case PR_OTRAMAS:	    
	    	canal[chan].retry = 0;
	    	cantdigs = strlen(canal[chan].digbuf.dg_value);  	    
	    	if (canal[chan].digbuf.dg_value[0] == '1') {
		        canal[chan].nroorden++;
    			canal[chan].estado = PR_OPCION_INICIO;            
	    		return (ST_PLAY);
	    	}
	    	canal[chan].estado = PR_ADIOS;
	    	return (ST_PLAY);
	   
	    default: 
		    //Solo entra aqui cuando hubo algún error.... 
		    sprintf(tmpbuff,"Estado Desconocido!! %d\n",canal[chan].estado);
		    toLog(fpv,tmpbuff,NULL,1,chan,1);
		    play_error(chan,ST_ERROR);
		    
	    } //end switch (estado)       
	    break;

	    
   case ST_PLAY:       
	 	switch (canal[chan].estado) {
	   	case PR_HOLA:
	        canal[chan].estado=PR_TIPO_PRESTADOR;
	   		return(ST_PLAY);
	   		
	   	case PR_PROCESO:
	   		// si es -10 es porque fallo el envio por socket
	   		// si es mayor que 0 es porque no le gusto algun campo
   			//printf("El canal es %d\n",chan);
   			//printf("El resultado es %d\n",canal[chan].resultado);
	   		if ((canal[chan].resultado == -2) || (canal[chan].code > 0)) {
   				if(chan==1 || chan==3) {
   					canal[chan].estado=PR_DESVIO;
   	   	   			return (ST_PLAY); 
   				}
   				else if(chan==2 || chan==4) {
   					canal[chan].estado=PR_CALL_CENTER;
   	   	   			return (ST_PLAY); 
   				}
   				else {
   	   				canal[chan].estado=PR_OTRAMAS;    	                 
   	   				return (ST_ERROR); 
   				}
   			}
	 		// si es -1 es porque fallo en guardar la transaccion o algun punto intermedio 
	 		else if (canal[chan].resultado == -1) {
   				canal[chan].estado=PR_OTRAMAS;    	                 
   				return (ST_ERROR); 
	 		}
	   	    else {
	   	    	canal[chan].estado=PR_REPETIR;
	   	    	return(ST_PLAY);
	   	    }
   			
	    case PR_DESVIO:	    
		   toLog(fpv,"Desviando...\n",NULL,1,chan,1);
           if(dx_dial(canal[chan].chdev,"T&,102,",(DX_CAP *)NULL,EV_SYNC) == -1) {
                 toLog(fpv,"dx_dial error\n",NULL,1,chan,1);
                 return -1;
            }
            sleep(3);
 	        canal[chan].retry = 0;
 	        canal[chan].estado= PR_HOLA;
 	        return(ST_ONHOOK);

	    case PR_CALL_CENTER:
	        canal[chan].retry = 0;
	        canal[chan].estado= PR_ADIOS;
	        return(ST_PLAY);

	    case PR_ADIOS:
	        canal[chan].retry = 0;
	        canal[chan].estado= PR_HOLA;
	        return(ST_ONHOOK);
	        
	   	// Incluyo el resto de los casos para no dejar cabos sueltos
	   	case PR_TIPO_PRESTADOR:
	   	case PR_OPCION_INICIO:
	   	case PR_PRESTADOR:
	   	case PR_CODIGO:
	   	case PR_AFILIADO:
	   	case PR_PRESTACION:
	   	case PR_PRESTACION_OK:
	   	case PR_OTRA_PRESTACION:
	   	case PR_OPCION_PRESTADOR_ASOC:
	   	case PR_PRESTADOR_ASOC:
	   	default:
	   		return(ST_GETDIGIT);
	   	}
      
   case ST_INVALID:                      
      return(ST_PLAY);

   case ST_ERROR:                         
      canal[chan].retry = 0;
   	  canal[chan].estado= PR_OTRAMAS;
      return(ST_PLAY);

   case ST_WAIT:
	   return(ST_WAIT);
	   
   default:       
      sprintf(tmpbuff,"Evento Desconocido %x, State=%d Estado=%d\n",event, canal[chan].state, 
    		  canal[chan].estado);
      toLog(fpv,tmpbuff,NULL,1,chan,1);
      return(ST_ONHOOK);	 
   }   
   
   return -1;
}

/**************************************************************************
 *        NAME: int next_state( chan )
 * DESCRIPTION: Comenzar el siguiente estado - Iniciar las funciones
 *       INPUT: int chan;	- Index into canal structure
 *      OUTPUT: Ninguna
 *     RETURNS: Estado Pasa/Falló
 *    CAUTIONS: Ninguna 
***************************************************************************/
int next_state(int chan)   
{
   int errcode = 0;
   
   switch ( canal[chan].state ) {
   case ST_ONHOOK:
	   errcode = set_hkstate(chan,DX_ONHOOK);       	 
       break;

   case ST_WTRING:                  
	   toLog(fpv,"Esperando llamada\n",NULL,0,chan,1);          
       break;

   case ST_OFFHOOK:
       toLog(fpv,"Llamada entrante\n",NULL,0,chan,1);          
       errcode = set_hkstate( chan, DX_OFFHOOK );      
       break;
   
   case ST_PLAY:           
	   // st_prompt mantiene los nombres y fds de cada archivo para play. Se abren todos al ppio
	   if ( dx_clrdigbuf( canal[chan].chdev ) == -1 ) {
		   // sprintf( tmpbuff, "No se pudo limpiar el Buffer DTMF para %s\n",
		   // ATDV_NAMEP( canal[chan].chdev ) );
		   toLog(fpv,NULL,"dx_clrdigbuf",1,chan,1);
	   }        
       if (canal[chan].hola==0) {
    	   canal[chan].msg_fd = all_msgs[PROMPTS].fd;     
    	   if (canal[chan].msg_fd == -1) {
    		   sprintf(tmpbuff, "No está abierto el archivo st_prompt[%d].voxfile = %d para play\n",
    				   canal[chan].estado,st_prompt[canal[chan].estado].voxfd);
    		   toLog(fpv,tmpbuff,NULL,1,chan,1);
    		   errcode = -1;
    	   }
    	   canal[chan].hola=1;
       }

       if (errcode == 0)               	 
    	   errcode = play(chan,all_msgs[PROMPTS].fd);
       
       break;
      
   case ST_GETDIGIT:
   	 	switch (canal[chan].estado) {
   	 	case PR_TIPO_PRESTADOR:
   	 		canal[chan].cant_digs = 1;          	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_OPCION_INICIO:
   	 		canal[chan].cant_digs = 1;          	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_PRESTADOR:
   	 		canal[chan].cant_digs = 0; 			//LPRESTADOR - 1;           	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_CODIGO:
   	 		canal[chan].cant_digs = 0; 			//LCODIGO - 1;           	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_AFILIADO:
   	 		canal[chan].cant_digs = 0; 			//LAFILIADO - 1;           	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_PRESTACION:
   	 		canal[chan].cant_digs = 0; 			//LPRESTACION - 1;           	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_PRESTACION_OK:
   	 		canal[chan].cant_digs = 1;         	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_OTRA_PRESTACION:
   	 		canal[chan].cant_digs = 1;            	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_OPCION_PRESTADOR_ASOC:
   	 		canal[chan].cant_digs = 1;         	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	case PR_PRESTADOR_ASOC:
   	 		canal[chan].cant_digs = 0; 			//LPRESTADOR - 1;           	    
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
 	 	case PR_REPETIR:
   	 		canal[chan].cant_digs = 1;
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;	
   	 	case PR_OTRAMAS:
   	 		canal[chan].cant_digs = 1;          
   	 		canal[chan].tiempo_espera = 0; 		//MAXTIME;   //0;
   	 		break;
   	 	// default:
   	 	} 
    
   	 	errcode = get_digits(chan,&(canal[chan].digbuf));
   	 	break;
      
   case ST_INVALID:   
	   	errcode = play_error(chan,canal[chan].estado);         
	   	break;
  
   case ST_ERROR:
	   	/*  Si llega a este estado es porque discó MAXRETRIES veces de forma incorrecta
	     *  dentro de alguno de los menúes o porque la autorización fue inválida.
	     *  Contemplo que siempre está en la última posición del array.
	   	 *  Paso como parámetro MAXPROMPTS porque estoy seguro de que es un valor que
	   	 *  excede cualquier PROMPT y en el switch del playerror entrará por default
	   	 */
	   	errcode = play_error(chan,MAXPROMPTS);            
	   	break;

   case ST_WAIT:
	   break;

   case ST_CONCURRENT:
	   canal[chan].state = ST_PLAY;
	   break;
   }

   return(errcode);
}


/***************************************************************************
 *        NAME: int operacion( chan )
 * DESCRIPTION: Operacion que invoca la función para enviar via socket
 * 				el mensaje y luego reproduce el resultado.
 *       INPUT: short tipo_op;	- Tipo de operación.
 *				int chan;		- Indice en canal.
 *      OUTPUT: None.
 *     RETURNS: Estado Pasa/Falló.
 *    CAUTIONS: None.
 ***************************************************************************/

int operacion (int chan){
 FILE *fpcomp;
 char msg[MAXLONG];
 char inicio[25],final[25];
 char msg_araucaria[1024];
 char tpres[32];

	memset(tpres,0x00,sizeof(tpres));
	switch (atoi(canal[chan].msg_tipo_prestador)) {
 	case 1:
 		sprintf(tpres,"50452100");	// UPCN
 		break;
 	case 2:
 		sprintf(tpres,"10880300");	// OSUTHGRA
 		break;
 	case 3:
 		sprintf(tpres,"22201900");	// OSTEL
 		break;
 	case 4:
 		sprintf(tpres,"89982822");	// OMIDIR
 		break;
 	case 5:
 		sprintf(tpres,"89982324");	// POLICIA
 		break;
 	case 6:
 		sprintf(tpres,"89982112");	// IPAUSS
 		break;
 	case 7:
 		sprintf(tpres,"11070100");	// Luz y Fuerza
 		break;
 	default:
    	toLog(fpv,"Error en el tipo de operacion........\n",NULL,1,chan,1);
    	return -1;
 	}

 	memset(msg,0x00,sizeof(msg));
	memset(msg_araucaria,0x00,sizeof(msg_araucaria));
 	switch(canal[chan].tipo_operacion) {
	case '1':
	    // Hay que crear el archivo comprobante.cfg previamente
	    fpcomp = fopen(COMPROBANTEFILE,"r");
	    if (fpcomp == NULL) {
	    	toLog(fpv,NULL,"fopen comprobante failed",1,chan,1);
	    	return -1;
	    } 
	    fscanf(fpcomp,"%u",&canal[chan].nrocomp);
	    // Si llegué al máximo empiezo de nuevo
	    if (canal[chan].nrocomp > MAXDIGCOMP)
	    	canal[chan].nrocomp=1;
	    else if (canal[chan].nrocomp == 0) {
	    	toLog(fpv,NULL,"Numero de comprobante erroneo",1,chan,1);
	    	return -1;
	    }
	    	
	    fclose(fpcomp);

	    sprintf(msg,"&0100|00|%u|%s%s||%s|%s|%s|%s|%s|%s|%s%%",canal[chan].nrocomp,
    			tpres,canal[chan].msg_afiliado,canal[chan].msg_prestacion[0],
    			canal[chan].msg_prestacion[1],canal[chan].msg_prestacion[2],
    			canal[chan].msg_prestacion[3],canal[chan].msg_prestacion[4],TERMINAL,
    			canal[chan].msg_prestador) ;

	    memset(inicio,0x00,sizeof(inicio));
	    memset(final,0x00,sizeof(final));
	    
	    // Envio por SOCKETS a Australomi!
    	if (send_australomi(chan,msg,inicio,final)==-1) return -2;

    	if (resguardo_txt(msg,answer[chan].respu,chan)==-1) return -1;
    	if (resguardo_sql(msg,answer[chan].respu,chan)==-1) return -1;
   	
 	    fpcomp = fopen(COMPROBANTEFILE,"w+");
	    if (fpcomp == NULL) {
	    	toLog(fpv,NULL,"fopen comprobante failed",1,chan,1);
	    	return -1;
	    } 
	    fprintf(fpcomp,"%u",++canal[chan].nrocomp);
	    fclose(fpcomp);

        // Armo mensaje para enviar por socket, para que quede registrado en la base de Araucaria...
        // "Msgreq+MsgResp-fechahoraini+fechahorafin"
        sprintf(msg_araucaria,"%s+%s+%s+%s2\0",msg,answer[chan].respu,inicio,final);

	    return(resultado_autorizacion(msg_araucaria,chan));
	
	case '2':

		sprintf(msg,"&0100|02|%s%s|%s||%s|%s|%s|%s|%s|%s|%s%%",canal[chan].msg_codigo,
				tpres,canal[chan].msg_afiliado,canal[chan].msg_prestacion[0],
    			canal[chan].msg_prestacion[1],canal[chan].msg_prestacion[2],
    			canal[chan].msg_prestacion[3],canal[chan].msg_prestacion[4],TERMINAL,
    			canal[chan].msg_prestador) ;
		
	    memset(inicio,0x00,sizeof(inicio));
	    memset(final,0x00,sizeof(final));

	    // Envio por SOCKETS a Australomi!
    	if (send_australomi(chan,msg,inicio,final)==-1) return -2;

    	if (resguardo_txt(msg,answer[chan].respu,chan)==-1) return -1;
    	if (resguardo_sql(msg,answer[chan].respu,chan)==-1) return -1;

    	// Armo mensaje para enviar por socket, para que quede registrado en la base de Araucaria...
        // "Msgreq+MsgResp-fechahoraini+fechahorafin"
        sprintf(msg_araucaria,"%s+%s+%s+%s2\0",msg,answer[chan].respu,inicio,final);
   	
    	return(resultado_anulacion(msg_araucaria,chan));
    
	default:
    	toLog(fpv,"Error en la operacion........\n",NULL,1,chan,1);
    	return -1;
	}
}


/***************************************************************************
 *        NAME: int resultado_$( chan )
 * DESCRIPTION: Reproduce vocalmente el resultado de la operación
 *       INPUT: int chan;	- Indice en canal
 *      OUTPUT: Ninguna
 *     RETURNS: Estado Pasa/Falló
 *    CAUTIONS: None
 ***************************************************************************/

int resultado_autorizacion(char *msg_araucaria, int chan) {
 int den,i;
 int p=0;
 int indice;
 int codetmp;
 char tmp[5];
 // char codaut[6];
 DX_IOTT lprompt[15]; 
 DV_TPT tpt_play[MAXTERMS] = {
	{IO_CONT,DX_MAXDTMF,1,TF_MAXDTMF,0,(unsigned short)NULL},
	{IO_CONT,DX_MAXTIME,0,TF_MAXTIME,0,(unsigned short)NULL},        
	{IO_CONT,DX_MAXSIL,100,TF_MAXSIL,0,(unsigned short)NULL},
	{IO_CONT,DX_MAXNOSIL,150,TF_MAXNOSIL,0,(unsigned short)NULL},
	// {IO_CONT,DX_LCOFF,1,TF_LCOFF,0,(unsigned short)NULL},        
	{IO_CONT,DX_DIGMASK,0,TF_DIGMASK,0,(unsigned short)NULL},
	{IO_CONT,DX_PMOFF,4,2,3,(unsigned short)NULL},
	{IO_EOT,DX_PMON,4,2,3,(unsigned short)NULL}};
	

 	dx_clrdigbuf(canal[chan].chdev);

 	if ((indice=find_field(chan,4))==-1)
    	return -1;	   

 	memset(tmp,0x00,sizeof(tmp));
	tmp[0]=answer[chan].respu[indice];
	tmp[1]=answer[chan].respu[indice+1];
	canal[chan].code=atoi(tmp);

	if (canal[chan].code==0) {    //OK!
    	//"El codigo de la autorizacion es..."			
        p=0;
        lprompt[p].io_fhandle = all_msgs[PROMPTS].fd;
        lprompt[p].io_type = IO_DEV;
   	    lprompt[p].io_bufp = 0;
   	    lprompt[p].io_offset = st_prompt[PR_AUTORIZA].offset;
   	    lprompt[p].io_length = st_prompt[PR_AUTORIZA].length;
        p++;
        
        indice=find_field(chan,3);
        while(answer[chan].respu[indice]!='|') {
        	codetmp=answer[chan].respu[indice]-48;
        	// sprintf(tmpbuff," %d ",code);
        	// toLog(fpv,tmpbuff,NULL,1,chan,0);
        	if (all_msgs[DIGITOS].fd<=0) {
        		sprintf(tmpbuff,"digitos[%d].voxfile=%s\n",codetmp,all_msgs[DIGITOS].voxfile);
        		toLog(fpv,tmpbuff,NULL,1,chan,1);
        		return -1;
        	}    
        	lprompt[p].io_fhandle = all_msgs[DIGITOS].fd ; 	//digitos[ch].voxfd ;   
        	lprompt[p].io_type = IO_DEV; 
        	lprompt[p].io_bufp = 0;
        	lprompt[p].io_offset = digitos[codetmp].offset;   //0;
        	lprompt[p].io_length = digitos[codetmp].length;   //-1;          
        	++indice; 
        	p++;
        }
	
        //toLog(fpv,"\n",NULL,1,chan,0);
        lprompt[p-1].io_type = IO_DEV|IO_EOT;   //Ultima entrada en la lista	
    } 
    else {    
    	// "La autorizacion ha sido denegada..."
        p=0;
        if (all_msgs[DENEGAR].fd<=0) {
        	toLog(fpv,"No esta abierto archivo de MENSAJE de DENEGADA!!\n",NULL,1,chan,1);
        	return -1;	   
        }
        lprompt[p].io_fhandle = all_msgs[PROMPTS].fd;     
        lprompt[p].io_type = IO_DEV;
        lprompt[p].io_bufp = 0;
        lprompt[p].io_offset = st_prompt[PR_DENEGADA].offset;
        lprompt[p].io_length = st_prompt[PR_DENEGADA].length;
        p++;

        den=inderr(canal[chan].code,chan);	   
        /*sprintf(tmpbuff,"Indice Denegada: %d\n",den);
        toLog(fpv,tmpbuff,NULL,1,chan,1);*/
	
        lprompt[p].io_fhandle = all_msgs[DENEGAR].fd; 	   
        lprompt[p].io_type = IO_DEV|IO_EOT; 
        lprompt[p].io_bufp = 0;
        lprompt[p].io_offset = denegar[den].offset; //0;
        lprompt[p].io_length = denegar[den].length; //-1;          	
    }
	
    // Que sea distinto de NULL quiere decir que se invocó por primera vez
	if (msg_araucaria != NULL) {
	    // Envio por SOCKETS a Araucaria!
		send_araucaria(msg_araucaria,chan);
		// Tengo que esperar que termine la reproducción anterior !
		if (sr_waitevt(-1) != -1) {       	 
			if (dx_play(canal[chan].chdev,lprompt,tpt_play,EV_ASYNC)==-1) {	         
				toLog(fpv,NULL,"dx_play",1,chan,1);
				return -1;
			}
		}
	}
	else {
		if (dx_play(canal[chan].chdev,lprompt,tpt_play,EV_SYNC)==-1) {	         
			toLog(fpv,NULL,"dx_play",1,chan,1);
			return -1;
		}
	}
	
	return 0;
}


int resultado_anulacion(char *msg_araucaria, int chan) {
 int den;
 int code;
 int indice;
 int p=0,i=0;
 char tmp[5];
 DX_IOTT lprompt[15]; 
 DV_TPT tpt_play[MAXTERMS] = {
    {IO_CONT,DX_MAXDTMF,1,TF_MAXDTMF,0,(unsigned short)NULL},
    {IO_CONT,DX_MAXTIME,0,TF_MAXTIME,0,(unsigned short)NULL},        
	{IO_CONT,DX_MAXSIL,100,TF_MAXSIL,0,(unsigned short)NULL},
    {IO_CONT,DX_MAXNOSIL,150,TF_MAXNOSIL,0,(unsigned short)NULL},
    // {IO_CONT,DX_LCOFF,1,TF_LCOFF,0,(unsigned short)NULL},        
    {IO_CONT,DX_DIGMASK,0,TF_DIGMASK,0,(unsigned short)NULL},
    {IO_CONT,DX_PMOFF,4,2,3,(unsigned short)NULL},
    {IO_EOT,DX_PMON,4,2,3,(unsigned short)NULL}};
	
	
    dx_clrdigbuf(canal[chan].chdev);

 	if ((indice=find_field(chan,4))==-1)
    	return -1;	   

 	memset(tmp,0x00,sizeof(tmp));
	tmp[0]=answer[chan].respu[indice];
	tmp[1]=answer[chan].respu[indice+1];
	code=atoi(tmp);

    if (code==0) {    //OK!
        // "La anulación se realizó exitosamente"			
    	p=0;
    	lprompt[p].io_fhandle = all_msgs[PROMPTS].fd;
    	lprompt[p].io_type = IO_DEV|IO_EOT;
    	lprompt[p].io_bufp = 0;
    	lprompt[p].io_offset = st_prompt[PR_ANULA].offset;
    	lprompt[p].io_length = st_prompt[PR_ANULA].length;
    } 
    else {    
        // "La anulacion no ha podido realizarse"
        p=0;
        lprompt[p].io_fhandle = all_msgs[PROMPTS].fd;    
        lprompt[p].io_type = IO_DEV;
        lprompt[p].io_bufp = 0;
        lprompt[p].io_offset = st_prompt[PR_NO_ANULA].offset;
        lprompt[p].io_length = st_prompt[PR_NO_ANULA].length;
        p++;	
        
    	memset(tmp,0x00,sizeof(tmp)); i=0;
        indice=find_field(chan,3);
        while(answer[chan].respu[indice]!='|')
        	tmp[i++]=answer[chan].respu[indice++];
 	   
        den=inderr(code,chan);	   
        sprintf(tmpbuff,"Indice Anulacion Denegada:%d\n",den);
        toLog(fpv,tmpbuff,NULL,1,chan,1);

        lprompt[p].io_fhandle = all_msgs[DENEGAR].fd;	   
        lprompt[p].io_type = IO_DEV|IO_EOT; 
        lprompt[p].io_bufp = 0;
        lprompt[p].io_offset = denegar[den].offset;
        lprompt[p].io_length = denegar[den].length;          	
	}    

    // Que sea distinto de NULL quiere decir que se invocó por primera vez
	if (msg_araucaria != NULL) {
	    // Envio por SOCKETS a Araucaria!
		send_araucaria(msg_araucaria,chan);
		// Tengo que esperar que termine la reproducción anterior !
		if (sr_waitevt(-1) != -1) {       	 
			if (dx_play(canal[chan].chdev,lprompt,tpt_play,EV_ASYNC)==-1) {	         
				toLog(fpv,NULL,"dx_play",1,chan,1);
				return -1;
			}
		}
	}
	else {
		if (dx_play(canal[chan].chdev,lprompt,tpt_play,EV_SYNC)==-1) {	         
			toLog(fpv,NULL,"dx_play",1,chan,1);
			return -1;
		}
	}
    
    return 1;
}

/***************************************************************************
 *        NAME: int resultado_$( chan )
 * DESCRIPTION: Reproduce vocalmente el resultado de la operación
 *       INPUT: int chan;	- Indice en canal
 *      OUTPUT: Ninguna
 *     RETURNS: Estado Pasa/Falló
 *    CAUTIONS: None
 ***************************************************************************/
int leer_digitos(int chan, int cantdigs, char *n)
{
int i;
int p=0;
char ch;

 
 DX_IOTT lprompt[10]; 
 DV_TPT tpt_play[MAXTERMS] = {
        {IO_CONT,DX_MAXDTMF,1,TF_MAXDTMF,0,(unsigned short)NULL},
        {IO_CONT,DX_MAXTIME,0,TF_MAXTIME,0,(unsigned short)NULL},        
        {IO_CONT,DX_MAXSIL,100,TF_MAXSIL,0,(unsigned short)NULL},
        {IO_CONT,DX_MAXNOSIL,150,TF_MAXNOSIL,0,(unsigned short)NULL},
        // {IO_CONT,DX_LCOFF,1,TF_LCOFF,0,(unsigned short)NULL},        
        {IO_CONT,DX_DIGMASK,0,TF_DIGMASK,0,(unsigned short)NULL},
        {IO_CONT,DX_PMOFF,4,2,3,(unsigned short)NULL},
        {IO_EOT,DX_PMON,4,2,3,(unsigned short)NULL}};
	
   	// {IO_CONT,DX_TONE,TN_DISC,TF_TONE,DX_TONEON,(unsigned short)NULL},
   	dx_clrdigbuf(canal[chan].chdev);
    
   	// El numero de prestacion ingresado es...
   	lprompt[p].io_fhandle = all_msgs[PROMPTS].fd;      
   	lprompt[p].io_bufp = 0;
   	lprompt[p].io_offset = st_prompt[PR_LEER_PRESTACION].offset;
   	lprompt[p].io_length = st_prompt[PR_LEER_PRESTACION].length;
   	lprompt[p].io_type = IO_DEV;   //Primer entrada en la lista
   	p++;

    for (i=0 ; i < cantdigs ; i++) {
         ch=n[i]-48;	// Convierto de int a ascii
         if (all_msgs[DIGITOS].fd<=0) {
        	 sprintf(tmpbuff,"digitos[%d].voxfile=%s\n",ch,all_msgs[DIGITOS].voxfile);
        	 toLog(fpv,tmpbuff,NULL,1,chan,1);
        	 return -1;
         }    
         lprompt[p].io_fhandle = all_msgs[DIGITOS].fd ; 	  
         lprompt[p].io_type = IO_DEV; 
         lprompt[p].io_bufp = 0;
         lprompt[p].io_offset = digitos[(int)ch].offset;
         lprompt[p].io_length = digitos[(int)ch].length;      
         p++;		   
    }
    lprompt[p-1].io_type = IO_DEV|IO_EOT;   //Ultima entrada en la lista	
    //toLog(fpv,tmp,NULL,1,chan,1);
    
    if (dx_play(canal[chan].chdev,lprompt,tpt_play,EV_SYNC)==-1) {	         
        sprintf(tmpbuff,"Error en dx_setparm %s\n",ATDV_ERRMSGP(canal[chan].chdev));	     
    	printf("RESPUESTA: %s\n",tmpbuff);
        toLog(fpv,NULL,"dx_play",1,chan,1);
        return -1;
    }
    
    return 0;
}


/*
int check_prestador(char *buf) {
	if (EnvioTrxHostRemoto(chan,msg)==-1) return -1;
		    
} */


