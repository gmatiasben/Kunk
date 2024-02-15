#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define UNO		0x01	// En binario 0000 0001 
#define MASK	0x80	// En binario 1000 0000
#define	BYTE	8
#define LIMPWD 	':'

void secure_pwd(char *, int, char *, char *);
void unsecure_pwd(char *, int, char *);
void printbin(char *, int);
char inttohex(int);
int hextoint(char *);
int power(int, int);

int main(void) {
 FILE *fp;
 int i,j;
 char tmp[5];
 int resto,hex;
 char xresto,xhex;
 int kill;
 int lpwd;
 char seed[20];
 char pwd[20];
 char secpwd[40];
 char unsecpwd[20];
 char srv[15], usr[15], db[15];
 
	memset(seed,0x00,sizeof(seed));
 	memset(pwd,0x00,sizeof(pwd));

	printf("Escriba un password para encriptar: ");
	fgets(pwd,sizeof(pwd),stdin);
	
	i=0; while (pwd[i] != '\n') ++i; pwd[i]=0x00;
	lpwd=i;
	
	for ( i=0 ; i<lpwd ; i++)
		seed[i]=48+i;

	printf("Password: %s\n",pwd);
	printf("Largo Password: %d\n",lpwd);
	//printbin(pwd,lpwd);
	printf("Seed: %s\n",seed);
	//printbin(seed,lpwd);
	
	secure_pwd(pwd,lpwd,seed,secpwd);
	printf("Password encriptada: %s\n",secpwd);
	//printbin(tmp,2*lpwd);
	
	fp=fopen("test","w+");
	for ( i=0 ; i<2*lpwd ; i++) {
		resto=(unsigned int)secpwd[i]%16;
		hex=(int)(secpwd[i]-resto)/16;
		if (hex<0) {
			xhex=inttohex((-1)*hex);
			printf("-");
			fprintf(fp,"-");
		}
		else
			xhex=inttohex(hex);
		// El resto es siempre positivo
		xresto=inttohex(resto);
		printf("%c",xhex);
		printf("%c:",xresto);
		fprintf(fp,"%c",xhex);
		fprintf(fp,"%c:",xresto);
	}
	putchar(LIMPWD);
	putchar(LIMPWD);
	fputc(LIMPWD,fp);
	fputc(LIMPWD,fp);
	fclose(fp);

	//printf("\n---------------------------\n"); 
	
	memset(secpwd,0x00,sizeof(secpwd));
	fp=fopen("test","rb");
	kill=0;
	for ( i=0 ; kill!=2 ; i++) {
		memset(tmp,0x00,sizeof(tmp));
		j=0;
		while( (tmp[j]=fgetc(fp)) != LIMPWD) ++j; //{ ++j; printf("j1: %d ",j); }
		tmp[j]=0x00;
		/*memset(tmp,0x00,sizeof(tmp));
		j=0;
		while( (tmp[j]=fgetc(fp)) != LIMPWD) ++j;
		tmp[j]=0x00;
		resto=hextoint(tmp,j);
		printf("resto: %d ",resto);*/
		if( j==0 )
			++kill;
		// De esta manera afirmo que los delimitadores deben ser consecutivos
		else {
			kill=0;
			secpwd[i]=hextoint(tmp);
			//printf("hex int: %d ",atoi(tmp));
			//printf("%d ",secpwd[i]);
		}
	}

	/*fscanf(fp,"%s",db);	db[strlen(db)]=0x00;
	printf("\nDatabase: %s\n",db);
	fscanf(fp,"%s",srv);	srv[strlen(srv)]=0x00;
	printf("Servidor: %s\n",srv);
	fscanf(fp,"%s",usr);	usr[strlen(usr)]=0x00;
	printf("User: %s\n",usr);*/
	
	fclose(fp);
	
	secpwd[i]!=0x00;
	lpwd=i/2;

	//printf("Password encriptada 2: %s\n",secpwd);
	//printf("Largo Password 2: %d\n",lpwd);
	// printbin(secpwd,2*lpwd);

	unsecure_pwd(secpwd,lpwd,unsecpwd);
	printf("\nPassword desencriptada: %s\n",unsecpwd);
	//printbin(unsecpwd,lpwd);
	
	printf("\n");
	for (j=0;j<strlen(pwd);j++) {
		pwd[j] += 2;
		printf("%d ",pwd[j]);
	}
	printf("\nPassword encriptada con shift 2: %s\n",pwd);
		   
	  
	return 0;
}


/* La función encripta un string de la siguiente forma ... primero
 * hace un XOR con una semilla (que tiene la misma longitud que el pwd)
 * y despues arma un nuevo paquete intercalando bit a bit el string 
 * XOReado y la semilla, con lo cual el string encriptado queda de 
 * longitud doble.
 */
void secure_pwd(char *pwd, int lpwd, char *seed, char *secpwd) {
 int i,j,k;
 char pwdx[lpwd];
 
	memset(secpwd,0x00,2*lpwd);
	memset(pwdx,0x00,lpwd);
	
	for (i=0 ; i < lpwd ; ++i)
		pwdx[i]=pwd[i]^seed[i];
	// printf("pwdx: %s\n",pwdx);
	// printbin(pwdx,lpwd);
		
	i=-1;
	for (k=0 ; k < 2*lpwd ; ++k) {
		if (!(k%2)) ++i;
		for (j=0 ; j < (BYTE/2) ; j++) {
			if ( (pwdx[i]&MASK) == MASK ) secpwd[k]=secpwd[k]|UNO;
			pwdx[i]=pwdx[i]<<1;
			secpwd[k]=secpwd[k]<<1;
			if ( (seed[i]&MASK) == MASK ) secpwd[k]=secpwd[k]|UNO;
			seed[i]=seed[i]<<1;
			if ( j!=((BYTE/2)-1) ) secpwd[k]=secpwd[k]<<1;
		}
	}
}


void unsecure_pwd(char *secpwd, int lpwd, char *unsecpwd) {
 int i,j,k;
 char xpwd[lpwd];
 char seed[lpwd];
 
 	memset(unsecpwd,0x00,lpwd);
 	memset(xpwd,0x00,lpwd);
 	memset(seed,0x00,lpwd);
	// printf("Size of password@unsecure: %d\n",lpwd);
	
	i=-1;
	for (k=0 ; k < 2*lpwd ; ++k) {
		if (!(k%2)) ++i;
		//p rintf("Paso numero i: %d\n",i);
		for (j=0 ; j < BYTE/2 ; j++) {
			if ( (secpwd[k]&MASK) == MASK ) xpwd[i]=xpwd[i]|UNO;
			if ( j!=((BYTE/2)-1) || !(k%2)) xpwd[i]=xpwd[i]<<1;
			secpwd[k]=secpwd[k]<<1;
			if ( (secpwd[k]&MASK) == MASK ) seed[i]=seed[i]|UNO;
			if ( j!=((BYTE/2)-1) || !(k%2)) seed[i]=seed[i]<<1;
			secpwd[k]=secpwd[k]<<1;
			/*printf("xpwd\n");
			printbin(xpwd,lpwd);
			printf("seed\n");
			printbin(seed,lpwd);*/
		}
	}
	
	for (i=0 ; i < lpwd ; ++i)
		unsecpwd[i]=xpwd[i]^seed[i];
	unsecpwd[i]=0x00;
	//printf("Seed: %s\n",seed);


}

void printbin(char *string, int lstr) {
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

char inttohex(int hex) {
	if (hex < 10)
		return (hex+48);
	else {
		switch(hex) {
		case 10:
			return 'A';
		case 11:
			return 'B';
		case 12:
			return 'C';
		case 13:
			return 'D';
		case 14:
			return 'E';
		case 15:
			return 'F';
		}
	}
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

int power(int x, int y) {
	if(y==0) return 1;
	return x*power(x,y-1);
}
