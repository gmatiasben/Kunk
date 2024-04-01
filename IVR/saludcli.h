/************************************************************************
 *   							SALUDCLI.H								*
 ************************************************************************/

// INCLUDE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <srllib.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <dxxxlib.h>
#include <sys/stat.h>
#include </usr/include/mysql/mysql.h>


// DEFINE
// NOTE: Revisar si estos archivos deben ser generados
#define AUSTRALOMISOCKETFILE "australomi.socket.cfg"
#define ARAUCARIASOCKETFILE "araucaria.socket.cfg"
#define AUSTRALOMIDBFILE "australomi.db.cfg"
#define ARAUCARIAHOST "201.251.126.61"
#define ARAUCARIAPORT "8787"
#define COMPROBANTEFILE "comprobante.sum"
#define LOGFILE "/tmp/log-ivrsalud.txt"
#define TERMINAL "888"
#define POTS_BUSYTONE 201

#define MAXDTMF         	4       // Number of Digits Expected            
#define MAXCHANS        	4      	// Numero Maximo de Canales
#define MAXRINGS			1     	// Numero de Rings antes de atender
#define MAXRETRIES 			3
#define MAXTIME 			250
#define MAXINTENTOS 		10000
#define MAXTERMS 			8
#define MAXERRORS 			6
#define MAXMSGS  			4
#define MAXLONG 			256
#define MAXTIMEOUT			30
#define MAXPRESTADORES		7
// Para MySQL
#define MAXINTENTOSHOSTS 	2
#define MAXHOSTS 			10
#define MAXORDEN 			2


// Limitaciones para los campos
#define MAXDIGCOMP		999999999

// Variables para encriptacion
#define UNO		0x01	// En binario 0000 0001 
#define MASK	0x80	// En binario 1000 0000
#define	BYTE	8
#define LIMPWD 	':'

//Se corresponden con tipo_operacion
#define TRX_AUTORIZACION 	1
#define TRX_ANULACION 		2

#define FALSE           0
#define TRUE            1
#define SIGEVTCHK       0x1010  // Check for type of signalling event   
#define SIGBITCHK       0x1111  // Check for signalling bit change      

// Longitudes de variables
#define LPRESTADOR 		11
#define LCODIGO			12
#define LAFILIADO 		11
#define LPRESTACION 	7
#define NPRESTACION 	5


// Estados
#define ST_WTRING       1       // Waiting for an Incoming Call    
#define ST_OFFHOOK      2       // Going Off Hook to Accept Call   
#define ST_GETDIGIT     3       // Get DTMF Digits (Access Code)   
#define ST_PLAY         4       // 
#define ST_INVALID      5       // Play invalid.vox (Invalid Code) 
#define ST_ONHOOK       6       // Go On Hook                      
#define ST_ERROR        7       // An Error has been Encountered   
#define ST_WAIT			8		// Espera un nuevo evento para no generar conflictos con BUSYTONE
#define ST_CONCURRENT	9		// Estado de procesos concurrentes entre el envio del msj y el play del prompt


// Prompts
#define PR_HOLA			 			0
#define PR_TIPO_PRESTADOR			1
#define PR_OPCION_INICIO 			2
#define PR_PRESTADOR 				3
#define PR_CODIGO					4
#define PR_AFILIADO					5
#define PR_PRESTACION				6
#define PR_LEER_PRESTACION         	7
#define PR_PRESTACION_OK			8
#define PR_OTRA_PRESTACION			9
#define PR_OPCION_PRESTADOR_ASOC	10
#define PR_PRESTADOR_ASOC			11
#define PR_PROCESO					12
#define PR_AUTORIZA 				13
#define PR_DENEGADA 				14
#define PR_ANULA					15
#define PR_NO_ANULA					16
#define PR_OTRAMAS 					17
#define PR_REPETIR 					18
#define PR_ADIOS         			19
#define PR_DESVIO	      			20
#define PR_CALL_CENTER     			21

#define MAXPROMPTS 					22  


// Lista .vox
#define PROMPTS 0
#define DENEGAR 1
#define ERRORES 2 
#define DIGITOS 3


//GLOBALES
char tmpbuff[256];					// Buffer temporal
static int end = 0;					// Las variables inicializadas se definen como static
static int d4xbdnum = 1;			// Default D/4x Board Number to Start use 
static int dtibdnum = 1;			// Default DTI Board Number to Start use 
static int frontend = CT_NTANALOG;	// Default network frontend is T1 
static int scbus    = TRUE;			// Default Bus mode is SCbus 
static int routeag  = FALSE;		// Route analog frontend to resource ??? 
// -- Antes en saludcli.c
int pcad;
int piott;
int logfile[4];
char *cadena[25];
FILE *fpv;


typedef struct{
  int pos;
  int nro;
} CONV;


//ATENCION CON ESTA VARIABLE....
typedef struct {
  int chdev;
  DX_IOTT iott_num[15];
} dx_infonum;

dx_infonum dx_nro[MAXCHANS+1];
//DX_IOTT iott_num[15];

typedef struct{
   char respu[MAXLONG];
} RESP;

RESP answer[MAXCHANS+1];


//Estructura que contiene el nombre de fd del vox para cada digito
typedef struct {
  char voxfile[20];
  int voxfd;
  long offset;
  long length;
} vox_struct ;


//Estructura que contiene el nombre y fd del vox para cada archivo de sonido
typedef struct {
  char voxfile[20];
  int fd;
} vox_msgs ;

int offset;
int length;


//Estados de cada canal
typedef struct dx_info {
   DV_DIGIT digbuf;				// Buffer for DTMF Digits       
   DX_IOTT  iott[1];			// I/O Transfer Table		   
   unsigned int nrocomp;		// nro de comprobante
   unsigned int nroorden;				// 
   int	    chdev;				// Channel Device Descriptor    
   int	    state;				// State of Channel             
   int	    msg_fd;				// File Descriptor para el prompt corriente
   int      cant_digs;      	// Cantidad Maxima de Digitos para cada GetDig
   int      tiempo_espera;  	// Tiempo Maximo de espera en GetDig
   int      estado;         	// Prompt en el que se encuentra el canal   
   int 		retry;    			// Numero de reintentos
   int 		indicerror;			// Indice del error devuelto
   int 		np;					// Número de prestaciones hasta el momento ingresadas
   int 		hola;				// si = 0 indica que aún no se reprodujo la bienvenida
   int 		resultado;			// Guardo el resultado de operacion()
   int 		code;				// Guardo el codigo de respuesta para analisis posterior
   char 	tipo_operacion; 				// 1-Venta 2-Anulación
   char 	mas_prestacion; 				// 1-Si 2-No
   char 	prestador_asoc; 				// 1-Si 2-No
   char 	prestacion_ok; 					// 1-Si 2-No
   // El +3 se debe al exceso de caracteres
   char	 	msg_tipo_prestador[3];									// Numero del prestador
   char 	msg_prestador[LPRESTADOR+1+3];						// Numero del prestador
   char 	msg_codigo[LCODIGO+1+3];							// Codigo para anulacion
   char 	msg_afiliado[LAFILIADO+1+3];						// Numero de afiliado
   char 	msg_prestacion[NPRESTACION][LPRESTACION+1+3];		// Matriz de prestaciones 
   char		msg_prestador_asoc[LPRESTADOR+1+3];					// Numero del prestador asociado
} DX_INFO;

DX_INFO canal[MAXCHANS+1];


static int long_campo[]={1,3,1,LPRESTADOR,LCODIGO,LAFILIADO,LPRESTACION,1,1,1,1,
				  LPRESTADOR,1,1,1,1,1,1,1,1,1};


static vox_msgs all_msgs[MAXMSGS]={{"prompts.vox",0},{"denegar.vox",0},{"error.vox",0},
                     		{"digitos.vox",0}};



#define PR_OFF0		0
#define PR_OFF1		11640
#define PR_OFF2		102291
#define PR_OFF3		126958
#define PR_OFF4		142684
#define PR_OFF5		160490
#define PR_OFF6		176678
#define PR_OFF7		193637
#define PR_OFF8		200266
#define PR_OFF9		220693
#define PR_OFF10	242817
#define PR_OFF11	265479
#define PR_OFF12	283748
#define PR_OFF13	300090
#define PR_OFF14	307490
#define PR_OFF15	316972
#define PR_OFF16	325991
#define PR_OFF17	334933
#define PR_OFF18	355977
#define PR_OFF19	380104
#define PR_OFF20	395598
#define PR_OFF21	416242

#define PR_LEN0		11640
#define PR_LEN1		90651
#define PR_LEN2		24667
#define PR_LEN3		15726
#define PR_LEN4		17807
#define PR_LEN5		16188
#define PR_LEN6		16959
#define PR_LEN7		6629
#define PR_LEN8		20428
#define PR_LEN9		22124
#define PR_LEN10	22663
#define PR_LEN11	18269
#define PR_LEN12	16342
#define PR_LEN13	7400
#define PR_LEN14	9482
#define PR_LEN15	9019
#define PR_LEN16	8942
#define PR_LEN17	21044
#define PR_LEN18	24128
#define PR_LEN19	15494
#define PR_LEN20	20644
#define PR_LEN21	37100

static vox_struct st_prompt[MAXPROMPTS] ={
	{"bienvenido.vox"		,0,PR_OFF0,PR_LEN0},
	{"tprestador.vox"		,0,PR_OFF1,PR_LEN1},
	{"opinicio.vox"			,0,PR_OFF2,PR_LEN2},
	{"prestador.vox	"		,0,PR_OFF3,PR_LEN3},
	{"codigo.vox"			,0,PR_OFF4,PR_LEN4},
	{"afiliado.vox"			,0,PR_OFF5,PR_LEN5},
	{"prestacion.vox"		,0,PR_OFF6,PR_LEN6},
	{"leerprestacion.vox"	,0,PR_OFF7,PR_LEN7},
	{"prestacionok.vox"		,0,PR_OFF8,PR_LEN8},
	{"otraprestacion.vox"	,0,PR_OFF9,PR_LEN9},
	{"opprestadorasoc.vox"	,0,PR_OFF10,PR_LEN10},
	{"prestadorasoc.vox"	,0,PR_OFF11,PR_LEN11},
	{"proceso.vox"			,0,PR_OFF12,PR_LEN12},
	{"autoriza.vox"			,0,PR_OFF13,PR_LEN13},
	{"denegada.vox"			,0,PR_OFF14,PR_LEN14},
	{"sianula.vox"			,0,PR_OFF15,PR_LEN15},
	{"noanula.vox"			,0,PR_OFF16,PR_LEN16},
	{"otramas.vox"			,0,PR_OFF17,PR_LEN17},
	{"repetir.vox"			,0,PR_OFF18,PR_LEN18},
	{"adios.vox"			,0,PR_OFF19,PR_LEN19},
	{"desvio.vox"			,0,PR_OFF20,PR_LEN20},
	{"callcenter.vox"		,0,PR_OFF21,PR_LEN21}
}; 


#define DN_OFF0		0
#define DN_OFF1		10251
#define DN_OFF2		21169
#define DN_OFF3		29415
#define DN_OFF4		39463
#define DN_OFF5		47575

#define DN_LEN0		10251
#define DN_LEN1		10918
#define DN_LEN2		8246
#define DN_LEN3		10048
#define DN_LEN4		8112
#define DN_LEN5		4289



static vox_struct denegar[]={
		{"prestadorden.vox"		,0,DN_OFF0,DN_LEN0},
		{"codigoden.vox"		,0,DN_OFF1,DN_LEN1},
		{"afiliadoden.vox"		,0,DN_OFF2,DN_LEN2},
		{"prestacionden.vox"	,0,DN_OFF3,DN_LEN3},
		{"convenioden.vox"		,0,DN_OFF4,DN_LEN4},
		{"trxden.vox"			,0,DN_OFF5,DN_LEN5}
}; 


#define ER_OFF0		0
#define ER_OFF1		9238
#define ER_OFF2		19977
#define ER_OFF3		29509
#define ER_OFF4		39285
#define ER_OFF5		46386


#define ER_LEN0		9238
#define ER_LEN1		10740
#define ER_LEN2		9532
#define ER_LEN3		9776
#define ER_LEN4		7102
#define ER_LEN5		3751


static vox_struct errores[MAXERRORS] ={
	{"malprestador.vox"		,0,ER_OFF0,ER_LEN0},
	{"malcodigo.vox	"		,0,ER_OFF1,ER_LEN1},
	{"malafiliado.vox"		,0,ER_OFF2,ER_LEN2},
	{"malprestacion.vox"	,0,ER_OFF3,ER_LEN3},
	{"malopcion.vox"		,0,ER_OFF4,ER_LEN4},
	{"malabsoluto.vox"		,0,ER_OFF5,ER_LEN5}
};
  

#define DG_OFF0	100
#define DG_OFF1	3274
#define DG_OFF2	6517
#define DG_OFF3	12173
#define DG_OFF4	14844
#define DG_OFF5	18500
#define DG_OFF6	22064
#define DG_OFF7	26234
#define DG_OFF8	30091
#define DG_OFF9	33365

#define DG_LEN0	3174
#define DG_LEN1	3243
#define DG_LEN2	4050 // 5656
#define DG_LEN3	2671
#define DG_LEN4	3656
#define DG_LEN5	3564
#define DG_LEN6	4171
#define DG_LEN7	3857
#define DG_LEN8	3274
#define DG_LEN9	3632


static vox_struct digitos[10]={
	{"cero.vox"		,0,DG_OFF0,DG_LEN0},
	{"uno.vox"		,0,DG_OFF1,DG_LEN1},
	{"dos.vox"		,0,DG_OFF2,DG_LEN2},
	{"tres.vox"		,0,DG_OFF3,DG_LEN3},
	{"cuatro.vox"	,0,DG_OFF4,DG_LEN4},
	{"cinco.vox"	,0,DG_OFF5,DG_LEN5},
	{"seis.vox"		,0,DG_OFF6,DG_LEN6},
	{"siete.vox"	,0,DG_OFF7,DG_LEN7},
	{"ocho.vox"		,0,DG_OFF8,DG_LEN8},
	{"nueve.vox"	,0,DG_OFF9,DG_LEN9}
};  


//**************************************************************
// 	Prototipos para operar placa IVR
//**************************************************************
void sysinit();
void intr_hdlr();
void init_voxes();
void end_app();
int set_hkstate(int, int);
int play(int, int);
int play_error(int, int);
int get_digits(int, DV_DIGIT *);

//**************************************************************
// 	Prototipos propios
//**************************************************************
void disp_status(int, char *);
int process(int, int);
int curr_state(int, int);
int next_state(int);
int operacion (int);
int leer_digitos(int, int, char *);
int resultado_autorizacion(char *, int);
int resultado_anulacion(char *, int);

//**************************************************************
// 	Prototipos de sockets 
//**************************************************************
int send_australomi(short, char *, char *, char *);
int connect_server(int);
int connect_socket_australomi(char *, char *,short);
int send_msg_australomi(int,char *,int ,short);
int get_msg_australomi(int,char *,short);
void close_socket_australomi(int); 

int send_araucaria(char *, int);
int connect_socket_araucaria(char *, char *, int);
int send_msg_araucaria(int, char *, int, int);
void close_socket_araucaria(int);

//**************************************************************
// 	Prototipos Auxiliares 
//**************************************************************
int pot(int ,int );
void clean_num(char *,int);
void clear_garbage(char *,int);
void reset_all(short);
void reset_cycle(short);
int find_field(int, int);
void contador(char *, char *, char*, char*, char *, char *);

//**************************************************************
// 	Prototipos de LOGs 
//**************************************************************
void toLog(FILE *, char *, char *, short, int, short);
int resguardo_txt(char *, char *, int);
int resguardo_sql(char *, char *, int);
int inderr(int,int);
char *sw_chstate(long);
char *sw_evttype(long);
char *sw_state(int);
char *sw_prompt(unsigned short);
char *sw_cstevent(unsigned short);
char *sw_hookev(unsigned short );

//**************************************************************
// 	Prototipos MySQL 
//**************************************************************
MYSQL *connect_db(char *);
int prompt_read(void);

//**************************************************************
// 	Prototipos de Encriptacion 
//**************************************************************
int read_sql_file(char *, char *, char *, char *, char *);
void unsecure_pwd(char *, int, char *);
void print_bin(char *, int);
