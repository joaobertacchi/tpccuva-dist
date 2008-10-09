/********************************************************************\
|*               BENCHMARK TPCC-UVA HEADER FILE                     *|
|* -----------------------------------------------------------------*|
|* En esta librería se definen algunas de las constantes necesarias *|
|* para le ejecución del benchmark y las estructuras de datos que   *|
|* utilizan el MT y los ETR para la comunicación.                   *|
|* ---------------------------------------------------------------- *|
|* Programmed by:                                                   *|
|*	Julio Alberto Hernández Gonzalo.                            *|
|*	Eduardo Hernández Perdiguero.                               *|
\********************************************************************/

/* filenameBuffer is used to build the path to the auxiliary 
 * file being opened */
char filenameBuffer[100];

/********************************************************************\
|* Ficheros de Cabecera.                                            *|
\* ---------------------------------------------------------------- */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/timeb.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

/********************************************************************\
|* Declaración de constantes                                        *|
\* ---------------------------------------------------------------- */

/***************** DATABASE SCALE *******************/
#define NUM_MAX_ALM 100     /*Max mumber of warehouses in the database */
#define NUM_ART 100000      /*Number of items for each warehouse            */
#define NUM_CLIENT 3000     /*Number of clients for each district           */
#define NUM_ORDER 3000      /*Number of orders for each district            */
#define N_OR_UNDELIVER 2101 /*Determinará las 900 NewOrder sin repartit por cada distrito */
#define NEXT_O_ID NUM_ORDER + 1 /*Número de la siguiente orden de los distritos 3001      */
#define ART_UNUSED 150000   /*Núlero de articulo inváklido*/
#define MAX_C_LAST 999      /*Rango de valores para la generación de c_last               */
#define SEMILLACARGA 1      /*Semilla inicial para calcular los vectores de estado para la*/
                            /*generación de datos en la población inicial de la base de datos*/

/*TIEMPOS DE TECLADO DE LOS ETR PARA CADA TRANSACCIÓN*****************/
#define TT_NEW_ORDER 18   /* NEW-ORDER transaction */
#define TT_PAYMENT 3      /* PAYMENT transaction */ 
#define TT_OSTATUS 2      /* ORDER STATUS transaction */
#define TT_DELIVERY  2    /* DELIVERY transaction */
#define TT_STOCK_LEVEL 2  /* STOCK_LEVEL transaction */

/****TIEMPOS MEDIOS DE PENSAR DE LOS ETR PARA CADA TRANSACCIÖN*****************/
#define TMP_NEW_ORDER 12   /* NEW-ORDER transaction */
#define TMP_PAYMENT 12     /* PAYMENT transaction */ 
#define TMP_OSTATUS 10     /* OSTATUS transaction */
#define TMP_DELIVERY 5     /* DELIVERY transaction */
#define TMP_STOCK_LEVEL 5  /* STOCK_LEVEL transaction */

/*TIEMPOS DE RESPUESTA PARA EL 90% DE LAS TRANSACCIONES MÁXIMO*/
#define TR90thNO 5   /* NEW-ORDER transaction */
#define TR90thP 5    /* PAYMENT transaction */ 
#define TR90thOS 5   /* ORDER STATUS transaction */
#define TR90thD 5    /* DELIVERY transaction */
#define TR90thSL 20  /* STOCK_LEVEL transaction */


/******** Checkpoint interval in seconds ********/
#define INT_CHECKPOINT 30*60 

/********************************************************************************/
/*	DEFINICIÓN DE ESTRUCTURAS DE MENSAJES                                   */
/********************************************************************************/
/******* TERMINAL MESSAGE *********/

struct tmsgtrm { /*MENSAJE DEL TERMINAL PARA EL MT*/ 
	int codctl; /*código de control: identifica si el mensaje es de conexión o de desconexión*/
	key_t sem_llave; /* Key for the semaphore set */
	key_t sem_ident; /* Semaphore identifier inside each key */
	key_t shm_llave; /*llave de la memoria del ETR*/
} msgtrm;   

/***********MENSAJE DE TRANSACCION NEW_ORDER:*************/
struct tnew_order_men {
    int w_id;
    int d_id;
    int c_id;
    struct articulo { /*vector de líneas de artículo de que se compone la transacción*/
	int ol_i_id; 
	int ol_supply_w_id;
	int ol_quantity;
	char flag;	/*este flag indica la presencia de articulo */
    } item[15];			
};

/***********MENSAJE DE TRANSACCION PAYMENT:******************/
struct tpayment_men {
    int w_id;
    int d_id;
    int c_id;
    int c_w_id;
    int c_d_id;
    char c_last[16];
    double h_amount;
};

/**********MENSAJE DE TRANSACCION ORDER_STATUS:************/
struct torder_status_men {
    int w_id;
    int d_id;
    int c_id;
    char c_last[16];
};


/***********MENSAJE DE TRANSACCION STOCK_LEVEL:*************/
struct tstock_level_men {
    int w_id;
    int d_id;
    int threshold;
};

/***********MENSAJE DE TRANSACCION DELIVERY:****************/
struct tdelivery_men {
    time_t seg;  /*PRIMER SELLO DE HORA QUE EL TERMINAL MANDA AL MT*/
    unsigned short mseg;	
    int w_id;
    int d_id;	
    int o_carrier_id;
};

/*********ESTRUCTURA DE MENSAJE QUE EL ETR ENVIA AL TERMINAL**********/
struct mensaje{ 
		int tipo;  /*TIPO DE MENSAJE */
  	        int id;     /*IDENTIFICADOR DE CLIENTE*/

                /*TIPOS DE MENSAJES:*/
		union ttran{	

		/***********Mensaje de Terminal ***************/
					struct tmsgtrm msgtrm;   

		/***********TRANSACCION NEW_ORDER:*************/
					struct tnew_order_men new_order; 

		/***********TRANSACCION ORDER_STATUS:************/
					struct torder_status_men ostatus;

		/***********TRANSACCION PAYMENT:******************/
					struct tpayment_men payment;  

		/***********TRANSACCION DELIVERY:****************/
					struct tdelivery_men delivery;  

		/***********TRANSACCION STOCK_LEVEL:*************/
					struct tstock_level_men stock_level;  
		} tran;
}; /* struct mensage */
/********************************************************************************/

/********************************************************************************/
/*	DEFINICIÓN DE ESTRUCTURAS MENSAJES DE RESPUESTA	DEL MT					*/
/********************************************************************************/
/***********TRANSACCION NEW_ORDER:*************/
struct tnew_order {
    char c_last[17];
    char c_credit[3];
    char o_entry_d[21];
    float c_discount;
    int o_id;
    int o_ol_cnt;
    float w_tax;
    float d_tax;
    char ctl;
    float total_amount;
    struct res_artic { /*vector de líneas de artículo de que se compone la orden*/
	char i_name[25];
	int s_quantity;
	char b_g;
	float i_price;
	float ol_amount;
    } item[15];
};

/***********TRANSACCION ORDER_STATUS:************/
struct torder_status {
    int c_id;
    char c_first[17];
    char c_middle[3];
    char c_last[17];
    float c_balance;
    int o_id;
    char o_entry_d[21];
    int o_carrier_id;
    int num_art;
    struct res_artic_o_s {  /*vector de artículos de que se*/
					/*componía la New-Order consultada en la transacción*/
	int ol_supply_w_id;
	int ol_i_id;
	int ol_quantity;
	float ol_amount;
	char ol_delivery_d[21];
    } item[15];
};

/***********TRANSACCION PAYMENT:******************/
struct tpayment {
    char w_street_1[21];
    char w_street_2[21];
    char w_city[21];
    char w_state[3];
    char w_zip[10];
    char d_street_1[21];
    char d_street_2[21];
    char d_city[21];
    char d_state[3];
    char d_zip[10];
    int c_id;
    char c_first[17];
    char c_middle[3];
    char c_last[17];
    char c_street_1[21];
    char c_street_2[21];
    char c_city[21];
    char c_state[3];
    char c_zip[10];
    char c_phone[17];
    char c_credit[3];
    char c_since[21];
    float c_credit_lim;
    float c_discount;
    float c_balance;
    char c_data[501];
    char h_date[21];
};

/***********TRANSACCION STOCK_LEVEL:*************/
struct tstock_level {
    int lowstock;
};
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/
/* ESTRUCTURA DEMEMORIA COMPARTIDA POR LA QUE EL MT CONTESTA A LOS ETR          */
/********************************************************************************/
union tshm{
 int id;

/***********TRANSACCION NEW_ORDER:*************/
    struct tnew_order new_order;

/***********TRANSACCION ORDER_STATUS:************/
    struct torder_status  ostatus;

/***********TRANSACCION PAYMENT:******************/
    struct tpayment payment;

/***********TRANSACCION STOCK_LEVEL:*************/
    struct tstock_level stock_level;
};

/********************************************************************************/
/********************************************************************************/
/* Function prototypes */

int double2int(double);
int decimal(double, int);
double aleat_dbl(double, double);
int aleat_int(int, int);
char *getfechahora(char *);
char *ctime_sin_salto(time_t *);
int cad_alfa_num(int, int, char *, char *, char *);
int cad_num(int, char *, char *);
char *crea_clast(int, char *);
int nurand(int, int, int, int, char *, char *);
void permutacion_int(int *, int, char *);
void posicion_cartas(int *, int,char *);
int buscar_n_vector(int *, int, int);
void insert_ord(int *, int, int);
double resta_tiempos(struct timeb *, struct timeb *);
int es_entero(char *);
int es_real(char *);
int es_alfa(char *);

