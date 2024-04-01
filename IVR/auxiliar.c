#include "saludcli.h"

/************************************************************************************
 * 										AUX											*
*************************************************************************************/

int pot(int n, int e) {
 int z,j;
 
 	z=1;
 	if (e==0) return 1;
 	if (e==1) return n;
 	for (j=0;j<e;j++)
 		z=(z*n);  
  
 	return z;  
}


int power(int x, int y) {
	if(y==0) return 1;
	return x*power(x,y-1);
}


void clean_num(char *str, int chan) {
 int i,j,k;

 	if (str==NULL) return ; 
   
 	j=strlen(str);
 	i=0;
 	while(i<j && str[i]!='#' && str[i]!=0){
 		if (!isdigit(str[i])) {
 			k=i;
 			while (k<j) {
 				str[k]=str[k+1];
 				k++;    
 			}
 		}
 		if (isdigit(str[i])) i++;
 	}
 	str[i]='\0';   
 	sprintf(tmpbuff,"clean_num --> %s \n",str);
 	toLog(fpv,tmpbuff,NULL,1,chan,1);   

}

void clear_garbage(char *stri, int len) {
 int i,j;
   
 	stri[len]=0;    
    j=strlen(stri);
       
    i=0;
    while (stri[0]=='0') {
        while (i < j) {
          stri[i]=stri[i+1];
          i++;
        }
        i=0; j=strlen(stri);
        printf("%s %d\n",stri,j);
    }
    sprintf(tmpbuff,"clear_leading_ceros %s\n",stri);
    toLog(fpv,tmpbuff,NULL,1,0,0);
}

void reset_all(short chan) {
 int i;
 
	canal[chan].hola = 0;
	canal[chan].nrocomp = 0;
	canal[chan].nroorden = 1;
    canal[chan].estado = PR_HOLA;
    
    return;
}

void reset_cycle(short chan) {
 int i;
 	for( i=0 ; i<NPRESTACION ; i++)
 		memset(canal[chan].msg_prestacion[i],0x00,LPRESTACION+1);

 	memset(canal[chan].msg_prestador,0x00,LPRESTADOR+1);
 	memset(canal[chan].msg_codigo,0x00,LCODIGO+1);
 	memset(canal[chan].msg_afiliado,0x00,LAFILIADO+1);
 	memset(canal[chan].msg_prestador_asoc,0x00,LPRESTADOR+1);
 	canal[chan].retry = 0;
 	canal[chan].resultado= 0;
 	canal[chan].indicerror = -1;
 	
 	return;
}

int find_field(int chan, int field) {
 int i, countpipes=0;
 
 	for(i=0 ; i<MAXLONG ; ++i) {
		if(answer[chan].respu[i]=='|')
			++countpipes;
		// Se le resta 1 para que coincida con el número de pipes
		if(countpipes==(field-1))
			return ++i;
	}

 	toLog(fpv,"No se encontró el campo !!!\n",NULL,1,chan,1);
 	return -1;
}

void contador(char *fecha, char *hora, char *prestador, char *statsend, char *msg, char *timetr) {
 FILE *fpt;
 
	fpt = fopen("/ivr/totales.txt","a+");
	if (fpt == NULL){
		toLog(fpv,"fopen totales.txt error",NULL,1,0,0);
		return;
	}
	fprintf(fpt,"%s\t%s\t00000%s\t%s\t%s\t%s\t%s.000\r\n",fecha,hora,TERMINAL,prestador,statsend,msg,timetr);
	fflush(fpt);
	fclose(fpt);
	
}

