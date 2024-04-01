#include "saludcli.h"

/************************************************************************************
 * 									LOGS											*
 *************************************************************************************/

void toLog(FILE *fpr, char * msg, char *msgerror, short set, int chan, short mom) {
	struct tm z;
	time_t now;
	char momento[30];
	char tmp[50];
	long lasterr;

	if (mom == 1) {
		time(&now);
		z = *localtime(&now);
		if (msg != NULL || msgerror != NULL) {
			if (chan == 0)
				sprintf(momento,"MENSAJE!! --> %02d-%02d-%04d %02d:%02d:%02d ",
									z.tm_mday, z.tm_mon+1, z.tm_year+1900, z.tm_hour,
									z.tm_min, z.tm_sec);
			else
				sprintf(momento,"CANAL %d--> %02d-%02d-%04d %02d:%02d:%02d ",
					chan, z.tm_mday, z.tm_mon+1, z.tm_year+1900, z.tm_hour,
					z.tm_min, z.tm_sec);
			
			printf("%s", momento);
			if (fpr != NULL)
				fprintf(fpr, "%s", momento);
		}
	}

	if (msg != NULL)
		printf("%s", msg);

	if (fpr != NULL) {
		if (msg != NULL)
			fprintf(fpr, "%s", msg);
	}

	//MENSAJES DE ERROR PROPIOS DE LAS FUNCIONES DEL IVR
	if (msgerror != NULL) {
		lasterr = ATDV_LASTERR(canal[chan].chdev);
		if (lasterr == EDX_SYSTEM ) {
			sprintf(tmp,"ERROR: %s %d:%s",msgerror,errno,strerror(errno));
		} 
		else {
			sprintf(tmp, "ERROR: %s %s %d %s", msgerror,
					ATDV_NAMEP(canal[chan].chdev),
					ATDV_LASTERR(canal[chan].chdev),
					ATDV_ERRMSGP(canal[chan].chdev));
		}

		if (fpr != NULL)
		fprintf(fpr, "%s\n", tmp);
		printf("%s\n", tmp);
	}

	if (fpr != NULL && set==1)
		fflush(fpr);
	
	return;
}

int resguardo_txt(char *msg, char *msgresp, int chan) {
 FILE *fpin,*fpout;
 char in[50],out[50];
 time_t now;
 struct tm z;

	// Resguardo de los buffers en archivos
	time(&now);
	z = *localtime(&now);
	memset(in,0x00,sizeof(in));
	sprintf(in,"/ivr/txrx/msgreq%02d%02d-%02d%02d%02d",z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);
	memset(out,0x00,sizeof(out));
	sprintf(out,"/ivr/txrx/msgresp%02d%02d-%02d%02d%02d",z.tm_mday,z.tm_mon+1,z.tm_hour,z.tm_min,z.tm_sec);
	fpin=fopen(in,"w+");
	fpout=fopen(out,"w+");
	if (fpin == NULL || fpout == NULL) {
    	toLog(fpv,NULL,"fopen failed -> archivos de resguardo",1,chan,1);
    	return -1;
    } 
	fprintf(fpin,"%s\n",msg);
	fprintf(fpout,"%s\n",answer[chan].respu);
	fclose(fpin);
	fclose(fpout);

	return 1;
}


int resguardo_sql(char *msg, char *msgresp, int chan) {
 MYSQL *gConn;
 char lSql[512];
 char datetime[20];
 time_t now;
 struct tm z;

	time(&now);
	z = *localtime(&now);
	memset(datetime,0x00,sizeof(datetime));
	sprintf(datetime,"%04d-%02d-%02d %02d:%02d:%02d",z.tm_year+1900,z.tm_mon+1,z.tm_mday,z.tm_hour,z.tm_min,z.tm_sec);

	if ( (gConn=connect_db(AUSTRALOMIDBFILE)) == NULL  )
		return -1;

	memset(lSql,0x00,sizeof(lSql));
	sprintf(lSql,"INSERT INTO tx VALUES('%s','%s');",datetime,msg);

	if (mysql_query(gConn,lSql) == -1) {
		sprintf(tmpbuff,"Error en mysql_query %s \n",mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,chan,1);
		mysql_close(gConn);
		return -1;
	}

	memset(lSql,0x00,sizeof(lSql));
	sprintf(lSql,"INSERT INTO rx VALUES('%s','%s');",datetime,msgresp);

	if (mysql_query(gConn,lSql) == -1) {
		sprintf(tmpbuff,"Error en mysql_query %s \n",mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,chan,1);
		mysql_close(gConn);
		return -1;
	}

	mysql_close(gConn);
	
	return 1;
}

int inderr(int error, int chan) {
	int i;
	int den,indice;
	char tmp[30];
	
	switch (error) {
	/* case 2:
		sprintf(tmpbuff,"Den %d -> Codigo Desconocido",error);
		den=1;
		break; */
	case 3: // Checked
		sprintf(tmpbuff,"Den %d -> Prestador Desconocido o Suspendido ",error);
		den=0;
		break;
	case 4: // Checked
		indice=find_field(chan,15);
		memset(tmp,0x00,sizeof(tmp));
		i=0;
		while( answer[chan].respu[indice+i] != '%' && (i < sizeof(tmp)) ) {
			tmp[i]=answer[chan].respu[indice+i];
			++i;
		}
		if(strncmp(tmp,"[EMPRESA NO HABILITADA]",23)==0) {
			sprintf(tmpbuff,"Den %d -> Afiliado Desconocido o Suspendido ",error);
			den=2;
		}
		else if(strncmp(tmp,"PRACTICA",8)==0) {
			sprintf(tmpbuff,"Den %d -> Prestacion Desconocida",error);
			den=3;
		}
		// Cualquier cosa que no entre dentro de los parámetros error
		else
			den=5;
		break;
	case 5:
		indice=find_field(chan,15);
		memset(tmp,0x00,sizeof(tmp));
		i=0;
		while( answer[chan].respu[indice+i] != '%' && (i < sizeof(tmp)) ) {
			tmp[i]=answer[chan].respu[indice+i];
			++i;
		}
		if(strncmp(tmp,"AFILIADO INEXISTENTE",20)==0) {
			sprintf(tmpbuff,"Den %d -> Afiliado Desconocido o Suspendido ",error);
			den=2;
		}
		// Cualquier cosa que no entre dentro de los parámetros error
		else
			den=5;
		break;
	default:
		sprintf(tmpbuff,"Den %d -> Motivo Desconocido",error);
		den=5;
	}
	toLog(fpv,tmpbuff,NULL,0,0,1);
	toLog(fpv,"\n",NULL,1,0,0);

	return den;
}

char *sw_chstate(long std) {
	switch (std) {
	case CS_DIAL:
		return "CS_DIAL ";
	case CS_CALL:
		return "CS_CALL ";
	case CS_GTDIG:
		return "CS_GTDIG";
	case CS_HOOK:
		return "CS_HOOK ";
	case CS_IDLE:
		return "CS_IDLE ";
	case CS_PLAY:
		return "CS_PLAY ";
	case CS_RECD:
		return "CS_RECD ";
	case CS_RECVFAX:
		return "CS_RECVFAX";
	case CS_SENDFAX:
		return "CS_SENDFAX";
	case CS_STOPD:
		return "CS_STOPD";
	case CS_TONE:
		return "CS_TONE ";
	case CS_WINK:
		return "CS_WINK ";
	default:
		return "CS_???? ";
	}
}

char *sw_evttype(long evt) {
	switch (evt) {
	case TDX_PLAY:
		return "TDX_PLAY    ";
	case TDX_PLAYTONE:
		return "TDX_PLAYTONE";
	case TDX_RECORD:
		return "TDX_RECORD  ";
	case TDX_GETDIG:
		return "TDX_GETDIG  ";
	case TDX_DIAL:
		return "TDX_DIAL    ";
	case TDX_CALLP:
		return "TDX_CALLP   ";
	case TDX_CST:
		return "TDX_CST     ";
	case TDX_SETHOOK:
		return "TDX_SETHOOK ";
	case TDX_WINK:
		return "TDX_WINK    ";
	case TDX_ERROR:
		return "TDX_ERROR   ";
	default:
		return "TDX_????????";
	}
	return "";
}

char *sw_state(int st) {
	switch (st) {
	case ST_WTRING:
		return "ST_WTRING  ";
	case ST_OFFHOOK:
		return "ST_OFFHOOK ";
	case ST_GETDIGIT:
		return "ST_GETDIGIT";
	case ST_PLAY:
		return "ST_PLAY    ";
	case ST_INVALID:
		return "ST_INVALID ";
	case ST_ONHOOK:
		return "ST_ONHOOK  ";
	case ST_ERROR:
		return "ST_ERROR   ";
	case ST_WAIT:
		return "ST_WAIT   ";
	case ST_CONCURRENT:
		return "ST_WAIT   ";
	default:
		return "ST_?????   ";
	}
	return "";
}

char *sw_prompt(unsigned short pr) {
	switch (pr) {
	case PR_HOLA:
		return "PR_HOLA";
	case PR_TIPO_PRESTADOR:
		return "PR_TIPO_PRESTADOR ";
	case PR_OPCION_INICIO:
		return "PR_OPCION_INICIO ";
	case PR_PRESTADOR:
		return "PR_PRESTADOR";
	case PR_CODIGO:
		return "PR_CODIGO";
	case PR_AFILIADO:
		return "PR_AFILIADO";
	case PR_PRESTACION:
		return "PR_PRESTACION";
	case PR_LEER_PRESTACION:
		return "PR_LEER_PRESTACION";
	case PR_PRESTACION_OK:
		return "PR_PRESTACION_OK";
	case PR_OTRA_PRESTACION:
		return "PR_OTRA_PRESTACION";
	case PR_OPCION_PRESTADOR_ASOC:
		return "PR_OPCION_PRESTADOR_ASOC";
	case PR_PRESTADOR_ASOC:
		return "PR_PRESTADOR_ASOC";
	case PR_PROCESO:
		return "PR_PROCESO";
	case PR_AUTORIZA:
		return "PR_AUTORIZA";
	case PR_DENEGADA:
		return "PR_DENEGADA";
	case PR_ANULA:
		return "PR_ANULA";
	case PR_NO_ANULA:
		return "PR_NO_ANULA";
	case PR_OTRAMAS:
		return "PR_OTRAMAS";
	case PR_REPETIR:
		return "PR_REPETIR";
	case PR_DESVIO:
		return "PR_DESVIO";
	case PR_CALL_CENTER:
		return "PR_CALL_CENTER";
	case PR_ADIOS:
		return "PR_ADIOS";
	default:
		return "PR_????";
	}
	return "";
}

char *sw_cstevent(unsigned short cst_event) {
	switch (cst_event) {
	case DE_DIGITS:
		return "DE_DIGITS   ";
	case DE_LCOFF:
		return "DE_LCOFF    ";
	case DE_LCON:
		return "DE_LCON     ";
	case DE_LCREV:
		return "DE_LCREV    ";
	case DE_RINGS:
		return "DE_RINGS    ";
	case DE_RNGOFF:
		return "DE_RNGOFF   ";
	case DE_SILOFF:
		return "DE_SILOFF   ";
	case DE_SILON:
		return "DE_SILON    ";
	case DE_TONEOFF:
		return "DE_TONEOFF  ";
	case DE_TONEON:
		return "DE_TONEON := Busy Tone Detected  ";
	case DE_WINK:
		return "DE_WINK     ";
	default:
		return "DE_?????????";
	}
}

char *sw_hookev(unsigned short cst_event) {
	switch (cst_event) {
	case DX_OFFHOOK:
		return "DX_OFFHOOK ";
	case DX_ONHOOK:
		return "DX_ONHOOK  ";
	default:
		return "DX_????????";
	}
}


