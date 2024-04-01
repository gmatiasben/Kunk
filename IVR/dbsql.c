#include "saludcli.h"

/************************************************************************************
 *										MySQL										*
*************************************************************************************/

MYSQL *connect_db(char *parfile) {
 MYSQL *gConn; 
 FILE *fpar;
 char srv[15], usr[15], pwd[20], db[15];
 int j=0;

	memset(srv,0x00,sizeof(srv));
	memset(usr,0x00,sizeof(usr));
	memset(pwd,0x00,sizeof(pwd));
	memset(db,0x00,sizeof(db));

 	read_sql_file(parfile,srv,usr,pwd,db);
 	
 	gConn = mysql_init(NULL);

 	// Connect to database
	if (!mysql_real_connect(gConn,srv,usr,pwd,db,0,NULL,0)) {
		sprintf(tmpbuff,"%s\n", mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,0,1);
		mysql_close(gConn);
		return NULL;
	}

	// sprintf(tmpbuff,"Conectado a %s\n",db);
	// toLog(fpv,tmpbuff,NULL,1,0,1);
	return gConn;
}


int prompt_read(void) {
 MYSQL *gConn;
 MYSQL_ROW lRow; 
 MYSQL_RES *lRes;
 int i;
 char lSql[512];
 char lsvoxfile[MAXPROMPTS][20]={	
		 "bienvenido.vox",
		 "tprestador.vox",
		 "opinicio.vox",
		 "prestador.vox	",
		 "codigo.vox",
		 "afiliado.vox",
		 "prestacion.vox",
		 "leerprestacion.vox",
		 "prestacionok.vox",
		 "otraprestacion.vox",
		 "opprestadorasoc.vox",
		 "prestadorasoc.vox",
		 "proceso.vox",
		 "autoriza.vox",
		 "denegada.vox",
		 "sianula.vox",
		 "noanula.vox",
		 "otramas.vox",
		 "repetir.vox",
		 "adios.vox"
 };

	if ( (gConn=connect_db(AUSTRALOMIDBFILE)) == NULL  )
		return -1;

	memset(lSql,0x00,sizeof(lSql));
	sprintf(lSql,"select * from voxprompt");

	if (mysql_query(gConn,lSql) == -1) {
		sprintf(tmpbuff,"Error en mysql_query %s \n",mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,0,1);
		mysql_close(gConn);
		return -1;
	}

	if ( !(lRes= mysql_store_result(gConn)) ) {
		sprintf(tmpbuff,"Error en mysql_store_result %s \n",mysql_error(gConn));
		toLog(fpv,tmpbuff,NULL,1,0,1);
		mysql_close(gConn);
		return -1;
	}

	memset(tmpbuff,0x00,sizeof(tmpbuff));
	
	if (mysql_num_rows(lRes) ==0 ) {
		toLog(fpv,"No se encontraron datos para los vox files\n",NULL,1,0,1);
		mysql_free_result(lRes);
		mysql_close(gConn);
		return -1;
	}

	for (i=0 ; i<MAXPROMPTS ; i++) {   
		lRow = mysql_fetch_row(lRes);
		strcpy(st_prompt[i].voxfile,lsvoxfile[i]);
		st_prompt[i].voxfd=0;
		st_prompt[i].offset=atol(lRow[0]);
		st_prompt[i].length=atol(lRow[1]);
	}
	
	mysql_free_result(lRes);
	mysql_close(gConn);
	
	return 1;
}


