#include "saludcli.h"

/************************************************************************************
 *									Security										*
*************************************************************************************/

/* La función encripta un string de la siguiente forma ... primero
 * hace un XOR con una semilla (que tiene la misma longitud que el pwd)
 * y despues arma un nuevo paquete intercalando bit a bit el string 
 * XOReado y la semilla, con lo cual el string encriptado queda de 
 * longitud doble.
 */
int read_sql_file(char *parfile, char *srv, char *usr, char *pwd, char *db) {
 FILE *fp;
 int i,j;
 char tmp[5];
 int resto,hex;
 int kill;
 int lpwd;
 char seed[20];
 char secpwd[40];
 
	fp=fopen(parfile,"rb");
	if (fp == NULL) {
		sprintf(tmpbuff,"Error al abrir %s \n",parfile);
		toLog(fpv,tmpbuff,NULL,1,0,1);
		return -1;
	}

	memset(secpwd,0x00,sizeof(secpwd));
	memset(seed,0x00,sizeof(seed));

	kill=0;
	memset(tmp,0x00,sizeof(tmp));
	for ( i=0 ; kill!=2 ; i++) {
		j=0;
		while( (tmp[j]=fgetc(fp)) != LIMPWD) ++j;
		tmp[j]=0x00;
		if( j==0 )
			++kill;
		// De esta manera afirmo que los delimitadores deben ser consecutivos
		else {
			kill=0;
			secpwd[i]=hextoint(tmp);
		}
	}

	fscanf(fp,"%s",db);	
	db[strlen(db)]=0x00;
	fscanf(fp,"%s",srv);	
	srv[strlen(srv)]=0x00;
	fscanf(fp,"%s",usr);	
	usr[strlen(usr)]=0x00;
	
	fclose(fp);
	//secpwd[i]=0x00;
	lpwd=i/2;

	unsecure_pwd(secpwd,lpwd,pwd);
	
	/* memset(tmpbuff,0x00,sizeof(tmpbuff));
	sprintf(tmpbuff,"DB: %s, Server: %s, User: %s, password: %s\n",db,srv,usr,pwd);
	toLog(fpv,tmpbuff,NULL,1,0,1); */
	
	return 0;

}


void unsecure_pwd(char *secpwd, int lpwd, char *unsecpwd) {
 int i,j,k;
 char xpwd[lpwd];
 char seed[lpwd];
 
 	memset(unsecpwd,0x00,lpwd);
 	memset(xpwd,0x00,lpwd);
 	memset(seed,0x00,lpwd);
	
	i=-1;
	for (k=0 ; k < 2*lpwd ; ++k) {
		if (!(k%2)) ++i;
		for (j=0 ; j < BYTE/2 ; j++) {
			if ( (secpwd[k]&MASK) == MASK ) xpwd[i]=xpwd[i]|UNO;
			if ( j!=((BYTE/2)-1) || !(k%2)) xpwd[i]=xpwd[i]<<1;
			secpwd[k]=secpwd[k]<<1;
			if ( (secpwd[k]&MASK) == MASK ) seed[i]=seed[i]|UNO;
			if ( j!=((BYTE/2)-1) || !(k%2)) seed[i]=seed[i]<<1;
			secpwd[k]=secpwd[k]<<1;
		}
	}
	
	for (i=0 ; i < lpwd ; ++i)
		unsecpwd[i]=xpwd[i]^seed[i];
	unsecpwd[i]=0x00;
}


int hextoint(char *tmp) {
 int signo=1,valor=0,aux=0;
 int i,neg=0;
 
 	if (tmp[0]== '-') {
 		signo=(-1)*signo;
 		neg=1;
 	}
	for(i=neg ; i<neg+2 ; ++i) {
		if ( (int)(tmp[i]-48)<10 ) {
			aux=(int)(tmp[i]-48)*power(16,1+neg-i);
			valor=( ((1+neg-i)==0) && (signo==-1) )?valor-aux:valor+aux;
		}
		// Averiguo si esta en el rango posible .. de la A a la F en ascii
		else if ( (int)(tmp[i]-65)>-1 && (int)(tmp[i]-65)<6 ){
			aux=(int)(tmp[i]-65+10)*power(16,1+neg-i);
			valor=( ((1+neg-i)==0) && (signo==-1) )?valor-aux:valor+aux;
		}
		// Hubo una alteración en el password
		else return -1;
	}
 	
 	return (signo*valor);
}

void print_bin(char *string, int lstr) {
 int i,j;
 char tmp[40];
 
 	memset(tmp,0x00,sizeof(tmp));
 	strcpy(tmp,string);
  	for ( i=0 ; i<lstr ; ++i) {
 	 	for ( j=0 ; j<BYTE ; ++j) {
 	 		if ( (tmp[i]&MASK) == MASK )
 	 			printf("1");
 	 		else
 	 			printf("0");
 	 		tmp[i]=tmp[i]<<1;
	 	}
 	 	printf("\n");
 	}
}
