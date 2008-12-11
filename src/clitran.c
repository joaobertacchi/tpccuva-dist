
/*
 * TPCC-UVa Project: Open-source implementation of TPCC-v5
 *
 * clitrans.c: Remote Terminal Emulator
 *
 * Developed by 
 *      Eduardo Hernandez Perdiguero                                
 *      Julio Alberto Hernandez Gonzalo                             
 * under the advise of Diego R. Llanos.
 *
 * Information on how the software works and implementation details in:
 * "TPCC-UVa: An Open-Source implementation of the TPC-C Benchmark",
 * available at http://www.infor.uva.es/~diego/tpcc.html
 *
 * Copyright (C) 2000-2006 Diego R. Llanos <diego@infor.uva.es>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/********************************************************************\
|*         PROGRAMA EMULADOR DE TERMINAL REMOTO (ETR)	            *|
|* -----------------------------------------------------------------*|
|* Simulas las funciones de un usuario emulado. Realiza las funcnes *| 
|* de terminal remoto. Genera transacciones, con sus datos          *|
|* correspondientes, y las envía al Monitor de Transacciones.       *|
|* Registra los tiempos de respuesta en un fichero de bitácora.     *|
|* ---------------------------------------------------------------- *|
|* El ETR se conecta con el MT mediante un mensaje de conexión      *|
|* pasándole la llave del semáforo y de la memoria compartida. A    *|
|* A continuación el ETR envía transacciones. El ETR se desconecta  *|
|* del MT mediante un mensaje de desconexión.                       *|
\********************************************************************/

/********************************************************************\
|* Header files.                                                    *|
\* ---------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/tpcc.h"
/********************************************************************/

/********************************************************************\
|* Constants declaration                                            *|
\* ---------------------------------------------------------------- */
/*                      Messages                                    */
//#define MSGTRM 2		/*identificador de mensaje de trminal       */
//#define CONECTAR 0		/*Connection terminal message identificator */
//#define DESCONECTAR 1		/*Disconnection terminal message            */
			/*Identificator                             */
//#define NEW_ORDER 3		/*New-Order Transaction indentificator      */
//#define PAYMENT 4		/*Payment Transaction identificator         */
//#define ORDER_STATUS 5		/*Order-Status Transaction identificator    */
//#define DELIVERY 6		/*Delivery transaction identificator        */
//#define STOCK_LEVEL 7		/*Stock-Level transaction identificator     */
//#define LLAVE_COLA 5		/*Key for the message queue                 */
/********************************************************************/

/*               Constants for the  vectores de estado                   */
	/*Vector de estado para la selección de transacciones */
#define E_TRAN 0

	/*Vectores de estado para la transacción New-Order */
#define E_NO_D_ID 1
#define E_NO_C_ID 2
#define E_NO_A_C_ID 3
#define E_NO_OL_CNT 4
#define E_NO_RBK 5
#define E_NO_OL_I_ID 6
#define E_NO_A_OL_I_ID 7
#define E_NO_LOCAL  8
#define E_NO_R_W 9
#define E_NO_OL_QUAN 10

	/*Vectores de estado para la transacción Payment */
#define E_P_D_ID 11
#define E_P_SELEC 12
#define E_P_C_LAST 13
#define E_P_A_C_LAST 14
#define E_P_C_ID 15
#define E_P_A_C_ID 16
#define E_P_LOCAL 17
#define E_P_R_W 18
#define E_P_C_D_ID 19
#define E_P_H_AMOUNT 20

	/*Vectores de estado para la transacción Order-Status */
#define E_OS_D_ID 21
#define E_OS_SELEC 22
#define E_OS_C_LAST 23
#define E_OS_A_C_LAST 24
#define E_OS_C_ID 25
#define E_OS_A_C_ID 26

	/*Vectores de estado para la transacción Delivery */
#define E_D_O_CARR 27

	/*Vectores de estado para la transacción Stock-Level */
#define E_SL_THR 28

	/*Vectores de estado para los Tiempos de Pensar */
#define E_TP_NEW_ORDER 29	/*New-Order                           */
#define E_TP_PAYMENT 30		/*Payment                             */
#define E_TP_STOCK_LEVEL 31	/*Stock-Level                         */
#define E_TP_OSTATUS 32		/*Order-Status                        */
#define E_TP_DELIVERY 33	/*Delivery                            */

/********************************************************************\
|* Global variables.                                                *|
\* ---------------------------------------------------------------- */
char estado[50][32];		/*Vectores de apuntadores a vectores de estado */
char e_global[32];		/*Vector de estado genérico                    */

int C_C_LAST;			/*Variable para la generación de c_last        */
int C_C_ID;			/*Variable para la generación de c_id          */
int C_OL_I_ID;			/*Variable para la generación de c_ol_i_id     */
char entrada[20];		/*Cadena de caracteres para los datos generados */


struct mensaje men;		/*Estructura de mensaje que el ETR envía el MT  */

FILE *out;			/*Puntero para el fichero de salidas por pantalla */
FILE *flog;			/*Puntero al fichero de bitácora                */
FILE *fcons;			/*Puntero al fichero de constantes              */
int i;
int semid, shmid, colid;	/*Identificadores de memoria, semáforo y cola */
int cod_term;			/*Identificadoe de terminal                */
int tipo;			/*Tipo de mensaje                          */
int semilla;			/*Smilla para la generación de números     */
			 /*aleatorios                               */
int ol_cnt;			/*Número de artículos por orden                 */
int w_id, d_id;			/*Identificadores de almacén y de distrito      */
int art_remoto;			/*Número de artículos remotos                   */
int paym_remota;		/*Indicador de transacción payment remota       */
int clien_paym;			/*Indicador de modo de seleccion de cliente en  */
		    /* la transacción payment                       */
int clien_os;			/*Indicador de modo de seleccion de cliente en  */
		    /*la transacción order_status                   */
key_t llave;			/*Variable para almacenar llaves                */
int flag = 0;			/*Determina la permanencia en el bucle de       */
		    /*eleccion de transacciones                     */


union tshm *shm;		/*estructura de mensaje de respuesta             */

int
aleat_tpensar (int media, char *estado)
{
/* ---------------------------------------------- *\
|* Función de cálculo del tiempo de pensar        *|
|* ---------------------------------------------- *|
|* Genera el tiempo de pensar del ETR según las   *|
|* especificaciones de la cláusula 5.2.5.4 del    *|
|* TPC-C                                          *|
|* ---------------------------------------------- *|
|* Parámetro media: media de la distribución      *|
|* aleatória                                      *|
|* Parámetro estado: vector de estado asignado a  *|
|* la variable aleatória                          *|
\* ---------------------------------------------- */


  double t;			/* tiempo de pensar                     */
  int result;

  setstate (estado);
  t = -log (((double) random ()) / RAND_MAX) * media;
  if (errno == ERANGE)
     {
       t = 10 * media;
     }
  if (t > 10 * media)
     {
       t = 10 * media;
     }
  result = (int) lrint (t);
  //  printf("----- Este cliente dormira %lf, esto, %i, segundos\n",
  //  t, lrint(t));

  return (result);
}

void
genera_datos_new_order (int w, struct tnew_order_men *new_order)
{
/* ---------------------------------------------- *\
|* Función de generación de datos de transacción  *|
|* New-Order                                      *|
|* ---------------------------------------------- *|
|* Genera los datos de transacción según la       *|
|* cláusula 2.4 del TPC-C                         *|
|* ---------------------------------------------- *|
|* Parámetro w: numero de almacenes.              *|
|* Parámetro new_order: puntero a la estructura   *|
|* de mensaje donde se almacenan los datos        *|
|* generados.                                     *|
\* ---------------------------------------------- */

  int i, x, y, rbk;		/*variables auxiliares            */

  new_order->w_id = w_id;	/*almecén del terminal    */

  setstate (estado[E_NO_D_ID]);	/*se fija el estado para la generación de no_d_id */

  art_remoto = 0;		/*se inicializa el número de artículos remotos */

/* Seleccion del distrito */
  do
     {
       sprintf (entrada, "%d\n", aleat_int (1, 10));
     }
  while (!es_entero (entrada));	/*Validación de dato generado */

  sscanf (entrada, "%d\n", &new_order->d_id);	/*se escribe el dato generado en la estructura de mensaje */

/* Seleccion del cliente */
  do
     {
       sprintf (entrada, "%d\n",
		nurand (1023, 1, NUM_CLIENT, C_C_ID, estado[E_NO_C_ID],
			estado[E_NO_A_C_ID]));
     }
  while (!es_entero (entrada));	/*Validación de dato generado */
  sscanf (entrada, "%d\n", &new_order->c_id);

/*Generación del número de artículos para la orden*/
  setstate (estado[E_NO_OL_CNT]);
  ol_cnt = (int) aleat_int (5, 15);

/*Se ponen a cero los flag que indican le presencia de artículos en el vector*/
  for (i = 0; i < 15; i++)
    new_order->item[i].flag = 0;

/*Generación de datos de cada artículo*/
  for (i = 0; i < ol_cnt; i++)
     {

       /*Se decide si el artículo ha de inválido en el 1% de los casos */
       setstate (estado[E_NO_RBK]);
       rbk = (int) aleat_int (1, 100);
       new_order->item[i].flag = 1;	/*presencia de artículo */
       if (i == ol_cnt - 1)
	  {			/*si es el último artículo se mira si es inválido */
	    if (rbk == 20)
	       {
		 /*artículo no usado */
		 do
		    {
		      sprintf (entrada, "%d\n", ART_UNUSED);
		    }
		 while (!es_entero (entrada));	/*se comprueba el dato */
		 sscanf (entrada, "%d\n", &new_order->item[i].ol_i_id);
	       }
	    else
	       {		/*artículo válido */
		 do
		    {
		      /*se genera el número de artículo */
		      sprintf (entrada, "%d\n",
			       nurand (8191, 1, NUM_ART, C_OL_I_ID,
				       estado[E_NO_OL_I_ID],
				       estado[E_NO_A_OL_I_ID]));
		    }
		 while (!es_entero (entrada));	/*se comprueba el dato */
		 sscanf (entrada, "%d\n", &new_order->item[i].ol_i_id);
	       }
	  }
       else
	  {			/*no es el último artículo de la orden */
	    do
	       {
		 /*Se genera el número del artículo */
		 sprintf (entrada, "%d\n",
			  nurand (8191, 1, NUM_ART, C_OL_I_ID,
				  estado[E_NO_OL_I_ID],
				  estado[E_NO_A_OL_I_ID]));
	       }
	    while (!es_entero (entrada));	/*Se valida el número generado */
	    sscanf (entrada, "%d\n", &new_order->item[i].ol_i_id);	/*Se escribe el dato */
	  }

       /*Selección de ol_supply_w_id: 99% local; 1% remoto */
       if (w != 1)
	  {			/*si hay mas de un almacén */
	    setstate (estado[E_NO_LOCAL]);
	    x = (int) aleat_int (1, 100);
	    if (x > 1)
	       {		/*almacén local */
		 do
		    {
		      sprintf (entrada, "%d\n", w_id);	/*se genera el dato */
		    }
		 while (!es_entero (entrada));	/*se valida el dato */
		 /*Se escribe el dato */
		 sscanf (entrada, "%d\n", &new_order->item[i].ol_supply_w_id);
	       }
	    else
	       {		/*almacén remoto */
		 /*Se selecciona uno de los almacenes restantes */
		 setstate (estado[E_NO_R_W]);
		 y = (int) aleat_int (1, w - 1);
		 if (y == w_id)
		   y = w;
		 do
		    {
		      sprintf (entrada, "%d\n", y);
		    }
		 while (!es_entero (entrada));
		 sscanf (entrada, "%d\n", &new_order->item[i].ol_supply_w_id);
		 art_remoto++;	/*incrementamos el recuento de artículos remotos */
	       }
	  }
       else
	  {			/*sólo hay un almacén */
	    do
	       {
		 sprintf (entrada, "%d\n", 1);
	       }
	    while (!es_entero (entrada));
	    sscanf (entrada, "%d\n", &new_order->item[i].ol_supply_w_id);	/*Se escribe el dato */
	  }

       /*Generación de ol_quantity */
       setstate (estado[E_NO_OL_QUAN]);
       do
	  {
	    sprintf (entrada, "%d\n", aleat_int (1, 10));	/*Se genera el dato */
	  }
       while (!es_entero (entrada));	/*Se comprueba el dato */
       sscanf (entrada, "%d\n", &new_order->item[i].ol_quantity);	/*Se escribe el dato */
     }				/*de for */
}				/*de genera_datos_new_order */

void
genera_datos_payment (int w, struct tpayment_men *payment)
{
/* ---------------------------------------------- *\
|* Función de generación de datos de transacción  *|
|* Payment                                        *|
|* ---------------------------------------------- *|
|* Genera los datos de transacción según la       *|
|* cláusula 2.5 del TPC-C                         *|
|* ---------------------------------------------- *|
|* Parámetro w: numero de almacenes.              *|
|* Parámetro payment: puntero a la estructura     *|
|* de mensaje donde se almacenan los datos        *|
|* generados.                                     *|
\* ---------------------------------------------- */

  int i, x, y;			/*variables auxiliares                 */
  char cad[20];


  payment->w_id = w_id;
  paym_remota = 0;
  clien_paym = 0;

/* Seleccion del distrito */
  setstate (estado[E_P_D_ID]);
  do
     {
       sprintf (entrada, "%d\n", aleat_int (1, 10));
     }
  while (!es_entero (entrada));
  sscanf (entrada, "%d\n", &payment->d_id);

/*Se determina si el cliente se selecciona por c_last o por c_id*/
  setstate (estado[E_P_SELEC]);
  x = aleat_int (1, 100);
  if (x <= 60)
     {
       /*se selecciona por C_LAST */
       do
	  {			/* se genera c_last */
	    crea_clast (nurand
			(255, 0, MAX_C_LAST, C_C_LAST, estado[E_P_C_LAST],
			 estado[E_P_A_C_LAST]), cad);
	    sprintf (entrada, "%s\n", cad);
	  }
       while (!es_alfa (entrada));
       sscanf (entrada, "%s\n", payment->c_last);
       payment->c_id = 0;	/*se indica que se ha seleccionado por c_last */
     }				/*de if */
  else
     {
       /*se selecciona por C_ID */
       do
	  {			/*Se selecciona por c_id */
	    sprintf (entrada, "%d\n",
		     nurand (255, 0, NUM_CLIENT, C_C_ID, estado[E_P_C_ID],
			     estado[E_P_A_C_ID]));
	  }
       while (!es_entero (entrada));
       sscanf (entrada, "%d\n", &payment->c_id);
       payment->c_last[0] = '\0';	/*Se indica que se ha seleccionado por c_id */
       clien_paym = 1;		/*Se indica que la transacción se ha seleccionado por c_id */
     }				/*de else */

/*Se determina si la transacción es local o remota*/
  setstate (estado[E_P_LOCAL]);
  x = aleat_int (1, 100);
  if (x <= 85)
     {				/*almacén local */
       do
	  {
	    sprintf (entrada, "%d\n", payment->w_id);
	  }
       while (!es_entero (entrada));
       sscanf (entrada, "%d\n", &payment->c_w_id);
       do
	  {
	    sprintf (entrada, "%d\n", payment->d_id);
	  }
       while (!es_entero (entrada));
       sscanf (entrada, "%d\n", &payment->c_d_id);
     }
  else
     {				/*almacén remoto */
       if (w > 1)
	  {			/*Si hay mas de un almacén se selecciona uno de los restantes */
	    setstate (estado[E_P_R_W]);
	    x = (int) aleat_int (1, w - 1);
	    if (x == w_id)
	      x = w;
	    paym_remota = 1;
	  }
       else
	 x = 1;			/*Si sólo hay un almacén se asigna el primero */

       /*almacén del cliente */
       do
	  {
	    sprintf (entrada, "%d\n", x);
	  }
       while (!es_entero (entrada));
       sscanf (entrada, "%d\n", &payment->c_w_id);

       /*distrito del cliente */
       setstate (estado[E_P_C_D_ID]);
       do
	  {
	    sprintf (entrada, "%d\n", aleat_int (1, 10));
	  }
       while (!es_entero (entrada));
       sscanf (entrada, "%d\n", &payment->c_d_id);
     }				/*de else */

/*Generación de h_amount*/
  setstate (estado[E_P_H_AMOUNT]);
  do
     {
       sprintf (entrada, "%4.2lf\n", aleat_dbl (1, 5000));	/*Se genera el dato */
     }
  while (!es_real (entrada));	/*se valida el dato */
  sscanf (entrada, "%lf\n", &payment->h_amount);	/*se escribe el dato */
}				/*de genera_datos_payment */

void
genera_datos_order_status (int w, struct torder_status_men *ostatus)
{

/* ---------------------------------------------- *\
|* Función de generación de datos de transacción  *|
|* Order-Status                                   *|
|* ---------------------------------------------- *|
|* Genera los datos de transacción según la       *|
|* cláusula 2.6 del TPC-C                         *|
|* ---------------------------------------------- *|
|* Parámetro w: numero de almacenes.              *|
|* Parámetro ostatus: puntero a la estructura     *|
|* de mensaje donde se almacenan los datos        *|
|* generados.                                     *|
\* ---------------------------------------------- */

  int x;			/*variable auxiliar */
  char cad[20];

  clien_os = 0;			/*al modo de selección de cliente se inicializa a 0 */
  ostatus->w_id = w_id;		/*se asigna el terminal del cliente */
/*Se genera el número de distrito*/
  setstate (estado[E_OS_D_ID]);
  do
     {
       sprintf (entrada, "%d\n", aleat_int (1, 10));	/*se genera el dato */
     }
  while (!es_entero (entrada));	/*se comprueba el dato */
  sscanf (entrada, "%d\n", &ostatus->d_id);	/*se escribe el dato */

/*Se determina si el cliente se selecciona por c_last o por c_id*/
  setstate (estado[E_OS_SELEC]);
  x = aleat_int (1, 100);
  if (x <= 60)
     {				/*selección de cliente por c_last */
       do
	  {			/*Se genera c_last */
	    crea_clast (nurand
			(255, 0, MAX_C_LAST, C_C_LAST, estado[E_OS_C_LAST],
			 estado[E_OS_A_C_LAST]), cad);
	    sprintf (entrada, "%s\n", cad);
	  }
       while (!es_alfa (entrada));	/*se comprueba el dato */
       sscanf (entrada, "%s\n", ostatus->c_last);	/*se escribe el dato */
       ostatus->c_id = 0;
     }
  else
     {				/*selección de cliente por c_id */
       do
	  {			/*se genera c_id */
	    sprintf (entrada, "%d\n",
		     nurand (255, 0, NUM_CLIENT, C_C_ID, estado[E_OS_C_ID],
			     estado[E_OS_A_C_ID]));
	  }
       while (!es_entero (entrada));	/*se comprueba el dato */
       sscanf (entrada, "%d\n", &ostatus->c_id);	/*se escribe el dato */
       clien_os = 1;		/*se indica que se ha seleccionado por c_id */
     }				/*de else */
}				/*de genera_datos_order_status */

void
genera_datos_stock_level (int w, struct tstock_level_men *stock_level)
{
/* ---------------------------------------------- *\
|* Función de generación de datos de transacción  *|
|* Stock-Level                                    *|
|* ---------------------------------------------- *|
|* Genera los datos de transacción según la       *|
|* cláusula 2.8 del TPC-C                         *|
|* ---------------------------------------------- *|
|* Parámetro w: numero de almacenes.              *|
|* Parámetro stock_level: puntero a la estructura *|
|* de mensaje donde se almacenan los datos        *|
|* generados.                                     *|
\* ---------------------------------------------- */

  stock_level->w_id = w_id;	/*almacén del terminal */
  stock_level->d_id = d_id;	/*distrito del terminal */

/*se genera el umbral*/
  setstate (estado[E_SL_THR]);
  do
     {
       sprintf (entrada, "%d\n", aleat_int (10, 20));	/*se genera el dato */
     }
  while (!es_entero (entrada));	/*se comprueba el dato */
  sscanf (entrada, "%d\n", &stock_level->threshold);	/*se escribe el dato */
}				/*de genera_datos_stock_level */

void
genera_datos_delivery (int w, struct tdelivery_men *delivery)
{
/* ---------------------------------------------- *\
|* Función de generación de datos de transacción  *|
|* Delivery                                       *|
|* ---------------------------------------------- *|
|* Genera los datos de transacción según la       *|
|* cláusula 2.7 del TPC-C                         *|
|* ---------------------------------------------- *|
|* Parámetro w: numero de almacenes.              *|
|* Parámetro delivery: puntero a la estructura    *|
|* de mensaje donde se almacenan los datos        *|
|* generados.                                     *|
\* ---------------------------------------------- */

  delivery->w_id = w_id;	/*almacén del terminal  */
  delivery->d_id = d_id;	/*distrito del terminal */

/*Se genera o_carrier_id*/
  setstate (estado[E_D_O_CARR]);
  do
     {
       sprintf (entrada, "%d\n", aleat_int (1, 10));	/*se genera el dato */
     }
  while (!es_entero (entrada));	/*se comprueba el dato */
  sscanf (entrada, "%d\n", &delivery->o_carrier_id);	/*se escribe el dato */
}				/*de genera_datos_delivery */

void
pant_new_order_pet ()
{
/* ---------------------------------------------- *\
|* Función que muentra la pantalla de petición de *|
|* datos de la transacción New-Order, según la    *|
|* cáusula 2.4 del TPC-C.                         *|
\* ---------------------------------------------- */

  int i;
  fprintf (out, "%33cNew Order\n", 32);
  fprintf (out,
	   "Warehouse: %4d   District: ##%24cDate: YYYY-MM-DD hh:mm:ss\n",
	   w_id, 32);
  fprintf (out,
	   "Customer:  ####   Name: ----------------   Credit: --   %Disc: --,--\n");
  fprintf (out,
	   "Order Number: --------  Number of Lines: --%8cW_tax: --,--   D_tax: --,--\n\n",
	   32);
  fprintf (out,
	   " Supp_W  Item_Id  Item Name%17cQty  Stock  B/G   Price    Amount\n",
	   32);
  for (i = 0; i < 15; i++)
    fprintf (out,
	     "  ####   ######   ------------------------  ##    ---    -    ---,--E  ----,--E\n");
  fprintf (out,
	   "Execution Status: ------------------------%20cTotal:  -----,--E\n",
	   32);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  N\n");

}


void
pant_new_order_muest (struct tnew_order_men *new_order)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de muestra de  *|
|* de datos generados de la transacción New-Order,*|
|* según la cláusula 2.4 del TPC-C                *|
|* ---------------------------------------------- *|
|* Parámetro new_order: puntero a la estructura de*|
|* datos de transacción generados                 *|
\* ---------------------------------------------- */

  int i;

  fprintf (out, "%33cNew Order\n", 32);
  fprintf (out,
	   "Warehouse: %4d   District: %2d%24cDate: YYYY-MM-DD hh:mm:ss\n",
	   new_order->w_id, new_order->d_id, 32);
  fprintf (out,
	   "Customer:  %4d   Name: ----------------   Credit: --   %Disc: --.--\n",
	   new_order->c_id);
  fprintf (out,
	   "Order Number: --------  Number of Lines: --%8cW_tax: --,--   D_tax: --,--\n\n",
	   32);
  fprintf (out,
	   " Supp_W  Item_Id  Item Name%17cQty  Stock  B/G   Price    Amount\n",
	   32);
  for (i = 0; i < 15; i++)
     {
       if (new_order->item[i].flag == 1)
	 fprintf (out,
		  "  %4d   %6d   ------------------------  %2d    ---    -    ---,--E  ----,--E\n",
		  new_order->item[i].ol_supply_w_id,
		  new_order->item[i].ol_i_id, new_order->item[i].ol_quantity);
       else
	 fprintf (out, "\n");
     }
  fprintf (out,
	   "Execution Status: ------------------------%20cTotal:  -----,--E\n",
	   32);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  N\n");
}



void
pant_new_order_menu (struct tnew_order_men *new_order)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de resultados  *|
|* de la transacción New-Order, y muestra el menú *|
|* de selección de la siguiente transacción, según*|
|* la cláusula 2.4 del TPC-C.                     *|
|* ---------------------------------------------- *|
|* La función lee de la memoria compartida los    *|
|* los resultados de la transacción               *|
|* ---------------------------------------------- *|
|* Parámetro new_order: puntero a la estructura de*|
|* datos de transacción generados                 *|
\* ---------------------------------------------- */

  int i;

  if (shm->new_order.ctl == 0)
     {
       fprintf (out, "%33cNew Order\n", 32);
       fprintf (out, "Warehouse: %4d   District: %2d%24cDate: %s\n",
		new_order->w_id, new_order->d_id, 32,
		shm->new_order.o_entry_d);
       fprintf (out,
		"Customer:  %4d   Name: %16s   Credit: %s   %Disc: %#2d,%-2d\n",
		new_order->c_id, shm->new_order.c_last,
		shm->new_order.c_credit,
		double2int (shm->new_order.c_discount),
		decimal (shm->new_order.c_discount, 2));
       fprintf (out,
		"Order Number: %8d  Number of Lines: %2d%8cW_tax: %#2d,%-2d   D_tax: %#2d,%-2d\n\n",
		shm->new_order.o_id, shm->new_order.o_ol_cnt, 32,
		double2int (shm->new_order.w_tax),
		decimal (shm->new_order.w_tax, 2),
		double2int (shm->new_order.d_tax),
		decimal (shm->new_order.d_tax, 2));
       fprintf (out,
		" Supp_W  Item_Id  Item Name%17cQty  Stock  B/G   Price    Amount\n",
		32);

       /*Para cada uno de los ol_cnt artículos */
       for (i = 0; i < 15; i++)
	  {
	    if (new_order->item[i].flag == 1)
	      fprintf (out,
		       "  %4d   %6d   %-24s  %2d    %3d    %c    %#3d,%-2dE  %#4d,%-2dE\n",
		       new_order->item[i].ol_supply_w_id,
		       new_order->item[i].ol_i_id,
		       shm->new_order.item[i].i_name,
		       new_order->item[i].ol_quantity,
		       shm->new_order.item[i].s_quantity,
		       shm->new_order.item[i].b_g,
		       double2int (shm->new_order.item[i].i_price),
		       decimal (shm->new_order.item[i].i_price, 2),
		       double2int (shm->new_order.item[i].ol_amount),
		       decimal (shm->new_order.item[i].ol_amount, 2));
	    else
	      fprintf (out, "\n");
	  }
       fprintf (out,
		"Execution Status: TRANSACTION COMMITED    %20cTotal:  %#5d,%-2dE\n",
		32, double2int (shm->new_order.total_amount),
		decimal (shm->new_order.total_amount, 2));
       fprintf (out,
		"Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
       fprintf (out,
		"                              D (Delivery), L (Stock Level). SELECTION:  _\n");
     }
  else
     {
       fprintf (out, "%33cNew Order\n", 32);
       fprintf (out,
		"Warehouse: %4d   District: %2d%24cDate: YYYY-MM-DD hh:mm:ss\n",
		new_order->w_id, new_order->d_id, 32);
       fprintf (out,
		"Customer:  %4d   Name: %16s   Credit: %s   %Disc: --.--\n",
		new_order->c_id, shm->new_order.c_last,
		shm->new_order.c_credit);
       fprintf (out,
		"Order Number: %8d  Number of Lines: --%8cW_tax: --,--   D_tax: --,--\n\n",
		shm->new_order.o_id, 32);
       fprintf (out,
		" Supp_W  Item_Id  Item Name%17cQty  Stock  B/G   Price    Amount\n",
		32);
       for (i = 0; i < 15; i++)
	  {
	    if (new_order->item[i].flag == 1)
	      fprintf (out,
		       "  %4d   %6d   ------------------------  %2d    ---    -    ---,--E  ----,--E\n",
		       new_order->item[i].ol_supply_w_id,
		       new_order->item[i].ol_i_id,
		       new_order->item[i].ol_quantity);
	    else
	      fprintf (out, "\n");
	  }
       fprintf (out,
		"Execution Status: ITEM NUMBER IS NO VALID %20cTotal:  -----,--E\n",
		32);
       fprintf (out,
		"Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
       fprintf (out,
		"                              D (Delivery), L (Stock Level). SELECTION:  _\n");
     }				/*de else */

}

void
pant_payment_pet ()
{
/* ---------------------------------------------- *\
|* Función que muentra la pantalla de petición de *|
|* datos de la transacción Payment, según la      *|
|* cáusula 2.5 del TPC-C.                         *|
\* ---------------------------------------------- */
  fprintf (out, "%35cPayment\n", 32);
  fprintf (out, "Date: YYYY-MM-DD hh:mm:ss\n\n");
  fprintf (out, "Warehouse: %4d%21cDistrict: ##\n", w_id, 32);
  fprintf (out, "--------------------%21c--------------------\n", 32);
  fprintf (out, "--------------------%21c--------------------\n", 32);
  fprintf (out,
	   "-------------------- -- ----------%7c-------------------- -- ----------\n\n",
	   32);
  fprintf (out, "Customer: ####  Cust-Warehouse: ####  Cust-District: ##\n");
  fprintf (out,
	   "Name:   ---------------- -- ################%5cSince:  YYYY-MM-DD\n",
	   32);
  fprintf (out, "        --------------------%21cCredit: --\n", 32);
  fprintf (out, "        --------------------%21c%Disc: --,--\n", 32);
  fprintf (out,
	   "        -------------------- -- ----------%7cPhone: -------------------\n\n",
	   32);
  fprintf (out,
	   "Amount Paid:%10c ####,##E      New-Cust-Balance: -----------,--E\n",
	   32);
  fprintf (out, "Credit Limit:   ----------,--E\n\n");
  fprintf (out,
	   "Cust-Data: --------------------------------------------------\n");
  fprintf (out, "%11c--------------------------------------------------\n",
	   32);
  fprintf (out, "%11c--------------------------------------------------\n",
	   32);
  fprintf (out, "%11c--------------------------------------------------\n\n",
	   32);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  P\n");
}

void
pant_payment_muest (struct tpayment_men *payment)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de muestra de  *|
|* de datos generados de la transacción Payment,  *|
|* según la cláusula 2.5 del TPC-C.               *|
|* ---------------------------------------------- *|
|* Parámetro payment: puntero a la estructura de  *|
|* datos de transacción generados.                *|
\* ---------------------------------------------- */
  fprintf (out, "%35cPayment\n", 32);
  fprintf (out, "Date: YYYY-MM-DD hh:mm:ss\n\n");
  fprintf (out, "Warehouse: %4d%21cDistrict: %2d\n", payment->w_id, 32,
	   payment->d_id);
  fprintf (out, "--------------------%21c--------------------\n", 32);
  fprintf (out, "--------------------%21c--------------------\n", 32);
  fprintf (out,
	   "-------------------- -- ----------%7c-------------------- -- ----------\n\n",
	   32);
  if (payment->c_id == 0)
     {
       fprintf (out,
		"Customer: ####  Cust-Warehouse: %4d  Cust-District: %2d\n",
		payment->c_w_id, payment->c_d_id);
       fprintf (out,
		"Name:   ---------------- -- %-16s%5cSince:  YYYY-MM-DD\n",
		payment->c_last, 32);
     }
  else
     {
       fprintf (out,
		"Customer: %4d  Cust-Warehouse: %4d  Cust-District: %2d\n",
		payment->c_id, payment->c_w_id, payment->c_d_id);
       fprintf (out,
		"Name:   ---------------- -- ################%5cSince:  YYYY-MM-DD\n",
		32);
     }

  fprintf (out, "        --------------------%21cCredit: --\n", 32);
  fprintf (out, "        --------------------%21c%Disc:  --,--\n", 32);
  fprintf (out,
	   "        -------------------- -- ----------%7cPhone:  -------------------\n\n",
	   32);
  fprintf (out,
	   "Amount Paid:%11c%#4d,%-2dE      New-Cust-Balance: -----------,--E\n",
	   32, double2int (payment->h_amount), decimal (payment->h_amount,
							2));
  fprintf (out, "Credit Limit:   ----------,--E\n\n");
  fprintf (out,
	   "Cust-Data: --------------------------------------------------\n");
  fprintf (out, "%11c--------------------------------------------------\n",
	   32);
  fprintf (out, "%11c--------------------------------------------------\n",
	   32);
  fprintf (out, "%11c--------------------------------------------------\n\n",
	   32);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  P\n");
}


void
pant_payment_menu (struct tpayment_men *payment)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de resultados  *|
|* de la transacción New-Order, y muestra el menú *|
|* de selección de la siguiente transacción, según*|
|* la cláusula 2.5 del TPC-C.                     *|
|* ---------------------------------------------- *|
|* La función lee de la memoria compartida los    *|
|* los resultados de la transacción.              *|
|* ---------------------------------------------- *|
|* Parámetro payment: puntero a la estructura de  *|
|* datos de transacción generados                 *|
\* ---------------------------------------------- */
  int i, l;
  fprintf (out, "%35cPayment\n", 32);
  fprintf (out, "Date: %s\n\n", shm->payment.h_date);
  fprintf (out, "Warehouse: %4d%21cDistrict: %2d\n", payment->w_id, 32,
	   payment->d_id);
  fprintf (out, "%-20s%21c%-20s\n", shm->payment.w_street_1, 32,
	   shm->payment.d_street_1);
  fprintf (out, "%-20s%21c%-20s\n", shm->payment.w_street_2, 32,
	   shm->payment.d_street_2);
  fprintf (out, "%-20s %2s %.5s-%4s%7c%-20s %2s %.5s-%4s\n\n",
	   shm->payment.w_city, shm->payment.w_state, shm->payment.w_zip,
	   &(shm->payment.w_zip[5]), 32, shm->payment.d_city,
	   shm->payment.d_state, shm->payment.d_zip,
	   &(shm->payment.d_zip[5]));
  if (payment->c_id == 0)
     {
       fprintf (out,
		"Customer: %4d  Cust-Warehouse: %4d  Cust-District: %2d\n",
		shm->payment.c_id, payment->c_w_id, payment->c_d_id);
       fprintf (out, "Name:   %16s %s %-16s%5cSince:  %.10s\n",
		shm->payment.c_first, shm->payment.c_middle,
		shm->payment.c_last, 32, shm->payment.c_since);
     }
  else
     {
       fprintf (out,
		"Customer: %4d  Cust-Warehouse: %4d  Cust-District: %2d\n",
		payment->c_id, payment->c_w_id, payment->c_d_id);
       fprintf (out, "Name:   %16s %2s %-16s%5cSince:  %.10s\n",
		shm->payment.c_first, shm->payment.c_middle,
		shm->payment.c_last, 32, shm->payment.c_since);
     }
  fprintf (out, "        %-20s%21cCredit: %2s\n", shm->payment.c_street_1, 32,
	   shm->payment.c_credit);
  fprintf (out, "        %-20s%21c%Disc:  %#2d,%-2d\n",
	   shm->payment.c_street_2, 32, double2int (shm->payment.c_discount),
	   decimal (shm->payment.c_discount, 2));

  // New for version 1.2.3: Incorrect number of descriptors in fprintf().
  // fprintf(out, "        %-20s %s %.3s-%.4s%7cPhone:  %.6s-%.3s-%.3s-%.4s\n\n"
  fprintf (out, "        %-20s %s %.3s-%.4s%7cPhone:  %.6s-%.3s-%.4s\n\n",
	   shm->payment.c_city, shm->payment.c_state, shm->payment.c_zip,
	   &(shm->payment.c_zip[5]), 32, shm->payment.c_phone,
	   &(shm->payment.c_phone[6]), &(shm->payment.c_phone[9]));

  fprintf (out,
	   "Amount Paid:%11c%#4d,%-2dE       New-Cust-Balance: %#10d,%-2dE\n",
	   32, double2int (payment->h_amount), decimal (payment->h_amount, 2),
	   double2int (shm->payment.c_balance),
	   decimal (shm->payment.c_balance, 2));

  fprintf (out, "Credit Limit:   %#10d,%-2dE\n\n",
	   double2int (shm->payment.c_credit_lim),
	   decimal (shm->payment.c_credit_lim, 2));

  fprintf (out, "Cust-Data: ");
  i = 0;
  l = 0;
  while ((shm->payment.c_data[i] != '\0') && (i < 200))
     {
       if (((i + 1) % 50) == 0)
	  {
	    fprintf (out, "%c\n           ", shm->payment.c_data[i]);
	    i++;
	    l++;
	  }
       else
	  {
	    fprintf (out, "%c", shm->payment.c_data[i]);
	    i++;
	  }
     }				/*de while */
  for (; l < 5; l++)
    fprintf (out, "\n");

  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  _\n");
}

void
pant_ostatus_pet ()
{
/* ---------------------------------------------- *\
|* Función que muentra la pantalla de petición de *|
|* datos de la transacción Order-Status, según la *|
|* cáusula 2.6 del TPC-C.                         *|
\* ---------------------------------------------- */

  int i;

  fprintf (out, "%34cOrder-Status\n", 32);
  fprintf (out, "Warehouse: %4d   District: ##\n", w_id);
  fprintf (out,
	   "Customer: ####   Name: ---------------- -- ################\n");
  fprintf (out, "Cust-Balance: ------,--E\n\n");
  fprintf (out,
	   "Order-Number: --------   Entry-Date: YYYY-MM-DD hh:mm:ss   Carrier-Number: --\n");
  fprintf (out,
	   "Supply_W     Item_Id    Qty     Amount      Delivery_Date\n");
  for (i = 0; i < 15; i++)
    fprintf (out,
	     "  ----       ------     --     -----.--E       YYYY-MM-DD\n",
	     32);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  S\n");
}

void
pant_ostatus_muest (struct torder_status_men *ostatus)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de muestra de  *|
|* de datos generados de la transacción           *|
|* Order-Status según la cláusula 2.6 del TPC-C.  *|
|* ---------------------------------------------- *|
|* Parámetro ostatus: puntero a la estructura de  *|
|* datos de transacción generados.                *|
\* ---------------------------------------------- */

  int i;

  fprintf (out, "%34cOrder-Status\n", 32);
  fprintf (out, "Warehouse: %4d   District: %2d\n", ostatus->w_id,
	   ostatus->d_id);
  if (ostatus->c_id == 0)
    fprintf (out, "Customer: ####   Name: ---------------- -- %-16s\n",
	     ostatus->c_last);
  else
    fprintf (out,
	     "Customer: %4d   Name: ---------------- -- ################\n",
	     ostatus->c_id);
  fprintf (out, "Cust-Balance: ------,--E\n\n");
  fprintf (out,
	   "Order-Number: --------   Entry-Date: YYYY-MM-DD hh:mm:ss   Carrier-Number: --\n");
  fprintf (out,
	   "Supply_W     Item_Id    Qty     Amount      Delivery_Date\n");
  for (i = 0; i < 15; i++)
    fprintf (out,
	     "  ----       ------     --     -----.--E       YYYY-MM-DD\n",
	     32);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  S\n");
}

void
pant_ostatus_menu (struct torder_status_men *ostatus)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de resultados  *|
|* de la transacción Order-Status, y muestra el   *|
|* menú de selección de la siguiente transacción, *|
|* según la cláusula 2.6 del TPC-C.               *|
|* ---------------------------------------------- *|
|* La función lee de la memoria compartida los    *|
|* los resultados de la transacción               *|
|* ---------------------------------------------- *|
|* Parámetro ostatus: puntero a la estructura de  *|
|* datos de transacción generados.                *|
\* ---------------------------------------------- */

  int i;

  fprintf (out, "%34cOrder-Status\n", 32);
  fprintf (out, "Warehouse: %4d   District: %2d\n", ostatus->w_id,
	   ostatus->d_id);
  fprintf (out, "Customer: %4d   Name: %16s %2s %-16s\n", shm->ostatus.c_id,
	   shm->ostatus.c_first, shm->ostatus.c_middle, shm->ostatus.c_last);
  fprintf (out, "Cust-Balance: %#5d,%-2dE\n\n",
	   double2int (shm->ostatus.c_balance),
	   decimal (shm->ostatus.c_balance, 2));
  fprintf (out, "Order-Number: %8d   Entry-Date: %.19s   Carrier-Number:",
	   shm->ostatus.o_id, shm->ostatus.o_entry_d);
  if (shm->ostatus.o_carrier_id == 0)
    fprintf (out, " --\n");
  else
    fprintf (out, " %2d\n", shm->ostatus.o_carrier_id);
  fprintf (out,
	   "Supply_W     Item_Id    Qty     Amount      Delivery_Date\n");
  for (i = 0; i < 15; i++)
     {
       if ((i + 1) <= shm->ostatus.num_art)
	  {
	    fprintf (out, "  %4d       %6d     %2d     %#5d,%-2dE       ",
		     shm->ostatus.item[i].ol_supply_w_id,
		     shm->ostatus.item[i].ol_i_id,
		     shm->ostatus.item[i].ol_quantity,
		     double2int (shm->ostatus.item[i].ol_amount),
		     decimal (shm->ostatus.item[i].ol_amount, 2));
	    if (strncmp (shm->ostatus.item[i].ol_delivery_d, "1970-01-01", 8)
		== 0)
	      fprintf (out, "YYYY-MM-DD\n");
	    else
	      fprintf (out, "%.10s\n", shm->ostatus.item[i].ol_delivery_d);
	  }			/*if */
       else
	 fprintf (out, "\n");
     }				/*de for */
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  _\n");
}

void
pant_delivery_pet ()
{
/* ---------------------------------------------- *\
|* Función que muentra la pantalla de petición de *|
|* datos de la transacción Delivery, según la     *|
|* cáusula 2.7 del TPC-C.                         *|
\* ---------------------------------------------- */
  fprintf (out, "%36cDelivery\n", 32);
  fprintf (out, "Warehouse: %4d\n\n", w_id);
  fprintf (out, "Carrier Number: ##\n\n");
  fprintf (out,
	   "Execution Status: -------------------------\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  D\n");
}

void
pant_delivery_muest (struct tdelivery_men *delivery)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de datos       *|
|* generados de la transacción Payment, según la  *|
|* cláusula 2.7 del TPC-C.                        *|
|* ---------------------------------------------- *|
|* Parámetro payment: puntero a la estructura de  *|
|* datos de transacción generados.                *|
\* ---------------------------------------------- */
  fprintf (out, "%36cDelivery\n", 32);
  fprintf (out, "Warehouse: %4d\n\n", delivery->w_id);
  fprintf (out, "Carrier Number: %2d\n\n", delivery->o_carrier_id);
  fprintf (out,
	   "Execution Status: -------------------------\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  D\n");
}

void
pant_delivery_menu (struct tdelivery_men *delivery)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de resultados  *|
|* de la transacción Delivery, y muestra el       *|
|* menú de selección de la siguiente transacción, *|
|* según la cláusula 2.7 del TPC-C.               *|
|* ---------------------------------------------- *|
|* La función lee de la memoria compartida los    *|
|* los resultados de la transacción               *|
|* ---------------------------------------------- *|
|* Parámetro delivery: puntero a la estructura de *|
|* datos de transacción generados.                *|
\* ---------------------------------------------- */

  fprintf (out, "%36cDelivery\n", 32);
  fprintf (out, "Warehouse: %4d\n\n", delivery->w_id);
  fprintf (out, "Carrier Number: %2d\n\n", delivery->o_carrier_id);
  fprintf (out,
	   "Execution Status: TRANSACTION QUEUED          \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  _\n");
}

void
pant_stock_level_pet ()
{
/* ---------------------------------------------- *\
|* Función que muentra la pantalla de petición de *|
|* datos de la transacción Stock-Level, según la  *|
|* cáusula 2.8 del TPC-C.                         *|
\* ---------------------------------------------- */
  fprintf (out, "%33cStock-Level\n", 32);
  fprintf (out, "Warehouse: %4d    District: %2d\n\n", w_id, d_id);
  fprintf (out, "Stock-Level Threshold: ##\n\n");
  fprintf (out, "low stock: ---\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  L\n");
}

void
pant_stock_level_muest (struct tstock_level_men *stock_level)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de datos       *|
|* generados de la transacción Payment, según la  *|
|* cláusula 2.8 del TPC-C.                        *|
|* ---------------------------------------------- *|
|* Parámetro payment: puntero a la estructura de  *|
|* datos de transacción generados.                *|
\* ---------------------------------------------- */
  fprintf (out, "%33cStock-Level\n", 32);
  fprintf (out, "Warehouse: %4d    District: %2d\n\n", stock_level->w_id,
	   stock_level->d_id);
  fprintf (out, "Stock-Level Threshold: %2d\n\n", stock_level->threshold);
  fprintf (out, "low stock: ---\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  L\n");
}


void
pant_stock_level_menu (struct tstock_level_men *stock_level)
{
/* ---------------------------------------------- *\
|* Función que muestra la pantalla de resultados  *|
|* de la transacción Stock-Level, y muestra el    *|
|* menú de selección de la siguiente transacción, *|
|* según la cláusula 2.8 del TPC-C.               *|
|* ---------------------------------------------- *|
|* La función lee de la memoria compartida los    *|
|* los resultados de la transacción               *|
|* ---------------------------------------------- *|
|* Parámetro stock_level: puntero a la estructura *|
|* de datos de transacción generados.             *|
\* ---------------------------------------------- */

  fprintf (out, "%33cStock-Level\n", 32);
  fprintf (out, "Warehouse: %4d    District: %2d\n\n", stock_level->w_id,
	   stock_level->d_id);
  fprintf (out, "Stock-Level Threshold: %2d\n\n", stock_level->threshold);
  fprintf (out, "low stock: %3d\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",
	   shm->stock_level.lowstock);
  fprintf (out,
	   "Select next transaction type: N (New Order), P (Payment), S (Order Status),\n");
  fprintf (out,
	   "                              D (Delivery), L (Stock Level). SELECTION:  _\n");

}

void
leyenda ()
{
/* ---------------------------------------------- *\
|* Función que muestra los parámetros con los que *|
|* se debe llamar al programa                     *|
\* ---------------------------------------------- */


  fprintf (stdout, "\nHa llamado mal al programa. Uso: \n");
  fprintf (stdout, " $ clien <ARG1> <ARG2> <ARG3> <ARG4> <ARG5> \n\n");
  fprintf (stdout,
	   "\t<ARG1>: llave del semáforo y de la memoria compartida\n");
  fprintf (stdout, "\t<ARG2>: número de almacén\n");
  fprintf (stdout, "\t<ARG3>: número de distrito\n");
  fprintf (stdout, "\t<ARG4>: número total de almacenes\n");
  fprintf (stdout, "\t<ARG5>: modo de ejecución\n\n");
}

int
main (int argc, char *argv[])
{

/* ---------------------------------------------- *\
|* Función principal del programa                 *|
|* ---------------------------------------------- *|
|* Argumento 1= llave del semáforo y de la memoria*|
|* compartida.                                    *|
|* Argumento 2= almacén del ETR                   *|
|* Argumento 3= distrito del ETR                  *|
|* Argumento 4= número total de almacenes         *|
|* Argumento 5= modo de ejecución:                *|
|* Argumento 6= server id                         *|
|* salida por pantalla o a /dev/null              *|
\* ---------------------------------------------- */

  void sigterm ();		/*puntero a la función sigterm()  */
  void ctl_c ();		/*puntero a la función ctl_c()    */

  int w;			/*almacén del terminal            */
  struct sembuf operacion;	/*estrctura de operación sobre el semáforo */
  char n_fichero[30];		/*cadena que contiene el nombre del fichero de bitácora */
  char ssystem[200];		/*cadena donde se almacena la oden de copia de focheros de bitácora */
  struct timeb sellohora;
  struct timeb sellohora2;	/* estructuras para almacenar sellos de hora */
  int tpensar;			/*tiempo de pensar del cliente */
  int carta = 0;
/*   Baraja para la implementación de las transacciones  */
  char baraja[] = { NEW_ORDER, NEW_ORDER, NEW_ORDER, NEW_ORDER, NEW_ORDER,
    NEW_ORDER, NEW_ORDER, NEW_ORDER, NEW_ORDER, NEW_ORDER,
    PAYMENT, PAYMENT, PAYMENT, PAYMENT, PAYMENT,
    PAYMENT, PAYMENT, PAYMENT, PAYMENT, PAYMENT,
    ORDER_STATUS, DELIVERY, STOCK_LEVEL
  };
  int ind_baraja[23];		/*vector de apuntadores a la baraja */
  union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
  } semun_arg;

  int srv_id;

/*Se comprueba que se han pasado correctamente los parámetros al programa*/
  if (argc != 7)
     {
       leyenda ();		/*Se avisa mediante la función leyenda si se ha pasado */
       /* un número incorrecto de parámetros */
       exit (-1);		/*y se sale del programa */
     }
  else
    /*Si alguno de los argumentos no es un número entero se informa */
    /*mediante leyenda() y se sale del programa                     */
  if (!es_entero (argv[1]) || !es_entero (argv[2]) || !es_entero (argv[3])
	|| !es_entero (argv[4]) || !es_entero (argv[5]))
     {
       leyenda ();
       exit (-1);
     }

/* Extraemos los parámetros */
  w = atoi (argv[4]);		/*número de almacenes */
  w_id = atoi (argv[2]);	/*asignamos un almacén al terminal */
  d_id = atoi (argv[3]);	/*asignamos un distrito al terminal */
  llave = LLAVE_COLA;		/*llave de la cola TODOS LOS CLIENTES COMPARTEN LA MISMA LLAVE */
  srv_id = atoi(argv[6]);


/*Se activan las funciones de tratamiento de las señales SIGTERM y SIGINT*/
  if (signal (SIGTERM, sigterm) == SIG_ERR)
     {
       fprintf (stderr, "Error en SIGNAL (SIGTERM)\n");
       exit (-1);
     }


  if (signal (SIGINT, ctl_c) == SIG_ERR)
     {
       fprintf (stderr, "Error en SIGNAL (SIGINT)\n");
       exit (-1);
     }


/*Se determina el modo de ejecución en función del quinto parámetro de la llamada*/
  if (argv[5][0] == '1')
     {
       /* Salida a null */
       if ((out = fopen ("/dev/null", "w")) == NULL)
	 fprintf (stderr, "ERROR AL ABRIR /dev/null\n");
     }
  else
     {
       /* Salida por stdout */
       out = stdout;
     }


/*Se genera el nombre del fichero de bitácora a partir del número*/
/*de almacén y de terminal que se pasan como parámetros          */

  sprintf (n_fichero, "/tmp/F_%d_%d.log\0", w_id, d_id);
  if ((flog = fopen (n_fichero, "w")) == NULL)
     {
       fprintf (stderr, "ERROR AL ABRIR EL FICHERO DE BITÁCORA\n");
       exit (-1);
     }


/*se almacena el nombre del fichero de bitácora para usarlo posteriormente*/
  sprintf (n_fichero, "F_%d_%d.log\0", w_id, d_id);

/*Se determina la semilla inicial para la creación de vectores de estaso*/
/*para la generación de números aleatórios                              */
  semilla = (int) (w_id * 100 + d_id);

/*	estado de uso general		*/
  initstate (semilla++, e_global, 32);

/*	generación de datos new_order      */
  initstate (semilla++, estado[E_NO_D_ID], 32);
  initstate (semilla++, estado[E_NO_C_ID], 32);
  initstate (semilla++, estado[E_NO_A_C_ID], 32);
  initstate (semilla++, estado[E_NO_OL_CNT], 32);
  initstate (semilla++, estado[E_NO_RBK], 32);
  initstate (semilla++, estado[E_NO_OL_I_ID], 32);
  initstate (semilla++, estado[E_NO_A_OL_I_ID], 32);
  initstate (semilla++, estado[E_NO_LOCAL], 32);
  initstate (semilla++, estado[E_NO_R_W], 32);
  initstate (semilla++, estado[E_NO_OL_QUAN], 32);

/*generación de datos payment*/
  initstate (semilla++, estado[E_P_D_ID], 32);
  initstate (semilla++, estado[E_P_SELEC], 32);
  initstate (semilla++, estado[E_P_C_LAST], 32);
  initstate (semilla++, estado[E_P_A_C_LAST], 32);
  initstate (semilla++, estado[E_P_C_ID], 32);
  initstate (semilla++, estado[E_P_A_C_ID], 32);
  initstate (semilla++, estado[E_P_LOCAL], 32);
  initstate (semilla++, estado[E_P_R_W], 32);
  initstate (semilla++, estado[E_P_C_D_ID], 32);
  initstate (semilla++, estado[E_P_H_AMOUNT], 32);

/*generación de datos order status*/
  initstate (semilla++, estado[E_OS_D_ID], 32);
  initstate (semilla++, estado[E_OS_SELEC], 32);
  initstate (semilla++, estado[E_OS_C_LAST], 32);
  initstate (semilla++, estado[E_OS_A_C_LAST], 32);
  initstate (semilla++, estado[E_OS_C_ID], 32);
  initstate (semilla++, estado[E_OS_A_C_ID], 32);

/*generación de datos order delivery*/
  initstate (semilla++, estado[E_D_O_CARR], 32);

/*generación de datos order stock level*/
  initstate (semilla++, estado[E_SL_THR], 32);

/*generación de estado para la elección de transacciones*/
  initstate (semilla++, estado[E_TRAN], 32);

/*generacion de estados para los tiempos de pensar */
  initstate (semilla++, estado[E_TP_NEW_ORDER], 32);
  initstate (semilla++, estado[E_TP_PAYMENT], 32);
  initstate (semilla++, estado[E_TP_OSTATUS], 32);
  initstate (semilla++, estado[E_TP_STOCK_LEVEL], 32);
  initstate (semilla++, estado[E_TP_DELIVERY], 32);

  /* Se leen las constantes C_RUN */

  strcpy (filenameBuffer, VARDIR);
  strcat (filenameBuffer, "cons.dat");

  if ((fcons = fopen (filenameBuffer, "r")) == NULL)
     {
       fprintf (stderr,
		"clien: Error: Constants file cons.dat can not be opened.\n");
       exit (-1);		/*En este caso salimos del programa */
     }

  fscanf (fcons, "%d %d %d\n", &C_C_LAST, &C_C_ID, &C_OL_I_ID);	/* En la primera fila estan los C_LOAD */
  fscanf (fcons, "%d %d %d\n", &C_C_LAST, &C_C_ID, &C_OL_I_ID);	/* Se capturan los C_RUN */
  fclose (fcons);

/*creamos la cola de mensajes*/
  if ((colid = msgget (llave, IPC_CREAT | 0600)) == -1)
     {
       switch (errno)
	  {			/*se trata el posible error */
	  case ENOENT:		/*la cola no está creada */
	    fprintf (out, "No Hay Cola de Mensajes\n");
	    fprintf (out, "Compruebe que el tm se esté ejecutando \n");
	    exit (-1);		/*Salimos del programa en caso de error */
	  default:		/*si es otro error */
	    fprintf (out,
		     "Ha ocurrido un error al conectar con la cola de mensajes\n");
	    exit (-1);		/*Salimos del programa en caso de error */

	  }			/*de switch */
     }				/*de if */

  llave = atoi (argv[1]);	/* Valor de llave pasado como parametro */

/*creamos la memoria compartida*/
  if ((shmid =
       shmget (llave, sizeof (union tshm),
	       IPC_CREAT | IPC_EXCL | 0600)) == -1)
     {
       do
	  {			/*En caso de error probamos con otras llaves incrementando la anterior */
	    switch (errno)
	       {
	       case EINVAL:
		 fprintf (out, "\nNO PUEDO CREAR EL SEGMENTO DE MEMORIA\n");
		 exit (-1);	/*En este caso salimos del programa */
	       case EEXIST:
		 fprintf (out,
			  "\nHAY UN SEGMENTO PARA ESA LLAVE\n PROBAMOS CON %d\n",
			  ++llave);
		 break;		/*probamos con una llave una unidad mayor */
	       case EIDRM:
		 fprintf (out,
			  "\nHAY UN SEGMENTO LIBERADO PARA ESA LLAVE\n PROBAMOS CON %d\n",
			  ++llave);
		 break;		/*probamos con una llave una unidad mayor */
	       case EACCES:
		 fprintf (stdout, "\nNO TENGO DERECHOS SOBRE ESE SEGMENTO\n");
		 exit (-1);	/*En este caso salimos del programa */
	       default:
		 fprintf (stdout,
			  "\nNO PUEDO CREAR EL SEGMENTO DE MEMORIA\n");
		 exit (-1);	/*En este caso salimos del programa */
	       }		/* switch */
	  }
       while ((shmid =
	       shmget (llave, sizeof (union tshm),
		       IPC_CREAT | IPC_EXCL | 0600)) == -1);
     }				/*En este punto la memoria ha sido creada correctamente */

  fprintf (out, "Llave shm: %d", llave);
  men.tran.msgtrm.shm_llave = llave;	/*Se introdce en el mensaje de conexión la llave de la memoria para */
  /*que el MT se enganche a ese segmento                             */

  shm = shmat (shmid, 0, 0);	/*El cliente enlaza con la memoria compartida */

  llave = atoi (argv[1]);	/* valor de llave pasado como parametro */


// diego: This client will obtain the identifier associated with the semaphore set for its warehouse.
  if ((semid = semget (w_id, 10, IPC_CREAT | 0600)) == -1)
     {
       fprintf (stdout, "\n\nNO PUEDO CREAR EL SEMAFORO DE MEMORIA\n");
       perror ("Error encontrado");
       exit (-1);
     }

/*En este punto ya conocemos el identificador del semaforo */
  fprintf (out, "Llave semaforo: %d", llave);
  men.tran.msgtrm.sem_llave = w_id;	/*se introduce en el mensaje de conexión la llave del set de semaforos */
  men.tran.msgtrm.sem_ident = d_id - 1;	/*se introduce en el mensaje de conexión el indice dentro del set (DESDE CERO!) */

  semun_arg.val = 0;
  semctl (semid, d_id - 1, SETVAL, semun_arg);	/*inicializamos el semaforo a 0: será el TM quien lo abra */
  /* permitiendonos leer la respuesta                      */

  fprintf (out, "ID. SEM: %d, ID. SHM: %d\n", semid, shmid);

/**********EL CLIENTE ENVIA EL MENSAJE DE CONEXION*********/
  men.tipo = MSGTRM;		/*Tipo de mensaje = mensaje de terminal */
  men.tran.msgtrm.codctl = CONECTAR;	/*de conexión */
  msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);	/*Se envia el mensaje */
  fprintf (out,
	   "\n ENVIADA SOLICITUD DE CONEXION \n Esperando respuesta ...\n");

/*Esperamos a que el semaforo este a 1 para recoger la respuesta*/
/*y cerramos el semaforo para esperar otro mensaje*/
  operacion.sem_num = d_id - 1;
  operacion.sem_op = -1;
  operacion.sem_flg = 0;
  semop (semid, &operacion, 1);

/*En este punto el TM ha contestado asignando un número de terminal*/
  fprintf (out,
	   "CONEXION REALIZADA. \nCliente: NUMERO DE TERMINAL ASIGNADO: %d\n",
	   shm->id);
  fflush (out);
  cod_term = shm->id;		/*codigo de terminal asignado */

/*INICIALIZAMOS LA BARAJA: el vector ind_baraja contendrá una permutación*/
/*de número entre 0 y 22 que indicará la posición del vector baraja que  */
/*será el siguiente tipo de transacción seleccionado                     */
  posicion_cartas (ind_baraja, 23, estado[E_TRAN]);

  while (flag == 0)
     {
/*El ETR permanece en este bucle hasta que el Controlador del Benchmark le */
/*envíe la señal SIGTERM, momento en el se pone el flag a 1 mediante la    */
/*función sigterm()                                                        */
/******* ELECCIÓN  DE TRANSACCIÖN SEGÚN MEZCLA DE TRANSACCIONES *******/
       if (carta >= 23)
	  {			/*si se ha racorrido la baraja se renueba */
	    posicion_cartas (ind_baraja, 23, estado[E_TRAN]);
	    carta = 0;
	  }
       /*Se selecciona el tipo de transacción recorriendo el vector de apuntadores */
       /*a la baraja ind_baraja                                                    */
       tipo = baraja[ind_baraja[carta++]];

       ftime (&sellohora);	/* Sello de hora de comienzo de transacción              */
       /*Se escribe en la bitácora el tipo de transacción seleccionado y el sello   */
       /*de hora de comienzo                                                        */
       fprintf (flog, "\n*%d> %d %d ", tipo, sellohora.time,
		sellohora.millitm);

       /*Se discrimina el tipo de transacción a ejecutar                            */
       men.tipo = tipo;    /*Tipo de transección que enviamos */
       men.id = cod_term;  /*Idenentificador de terminal */
       men.srv_id = srv_id; /* Server/warehouse identifier */
       switch (tipo)
	  {
	    /*Dependiendo del tipo de transaccion hacemos lo que corresponda */
	  case NEW_ORDER:		/********TRANSACCION NEW ORDER*********/
	    /*patalla de introducción de datos */
	    pant_new_order_pet ();

	    /*GENERACIÓN DE DATOS DE TRANSACCIÓN */
	    genera_datos_new_order (w, &men.tran.new_order);

	    /* TIEMPO DE TECLADO */
	    sleep (TT_NEW_ORDER);

	    /*Mostrar datos introducidos */
	    pant_new_order_muest (&men.tran.new_order);

	    /*PRIMER SELLO DE HORA: sello de envío */
	    ftime (&sellohora);
	    /*Se escribe en la bitácora el sello de hora */
	    fprintf (flog, "%d %d ", sellohora.time, sellohora.millitm);

	    /*Se envía la transacción a través de la cola de mensajes */
	    msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);

	    /*Se espera a que el MT responda */
	    operacion.sem_num = d_id - 1;
	    operacion.sem_op = -1;
	    operacion.sem_flg = 0;
	    semop (semid, &operacion, 1);	/*esperamos a que el semaforo este a 1 para */
	    /*recoger la respuesta */
	    /*y cerramos el semaforo para esperar otro mensaje */
	    /*SEGUNDO SELLO DE HORA: recepción de resultados */
	    ftime (&sellohora2);

	    /*RESULTADO DE LA TRANSACCION */
	    /*Imprimir pantalla de menu */
	    pant_new_order_menu (&men.tran.new_order);

	    /*Se calcula el tiempo de respuesta de tranasacción como la diferencia */
	    /*de los dos sellos de hora anteriores, y se escribe en el fichero de */
	    /*bitácora                                                            */
	    fprintf (flog, "%f ", resta_tiempos (&sellohora, &sellohora2));

	    /* calculo de Tiempo de Pensar */
	    tpensar =
	      (int) aleat_tpensar (TMP_NEW_ORDER, estado[E_TP_NEW_ORDER]);
	    fprintf (flog, "%d ", tpensar);	/*Se escribe en la bitácora */

	    /*Se espera el tiempo de pensar */
	    sleep (tpensar);
	    /*imprimimos la información acerca de la ejecución de la transacción */
	    fprintf (flog, "%d %d %d ", shm->new_order.ctl,
		     shm->new_order.o_ol_cnt, art_remoto);

	    break;

	  case PAYMENT:	/********TRANSACCION PAYMENT*********/
	    /*patalla de introducción de datos */
	    pant_payment_pet ();

	    /*GENERACIÓN DE DATOS DE TRANSACCIÓN */
	    genera_datos_payment (w, &men.tran.payment);

	    /* TIEMPO DE TECLADO */
	    sleep (TT_PAYMENT);

	    /*Se muestran los datos introducidos */
	    pant_payment_muest (&men.tran.payment);

	    /*PRIMER SELLO DE HORA */
	    ftime (&sellohora);
	    /*Se escribe el sello de hora */
	    fprintf (flog, "%d %d ", sellohora.time, sellohora.millitm);
	    /*Mandar transaccición */
	    msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);
	    /*enviamos el mensaje y esperamos la respuesta */
	    operacion.sem_num = d_id - 1;
	    operacion.sem_op = -1;
	    operacion.sem_flg = 0;
	    semop (semid, &operacion, 1);	/*esperamos a que el semaforo este a 1 para recojer la respuesta */
	    /*y cerramos el semaforo para esperar otro mensaje */
	    /*SEGUNDO SELLO DE HORA */
	    ftime (&sellohora2);
	    /*Imprimir pantalla de menu */
	    pant_payment_menu (&men.tran.payment);
	    /*Se calcula el tiempo de respuesta de transacción y se escribe en */
	    /* la bitácora                                                    */
	    fprintf (flog, "%f ", resta_tiempos (&sellohora, &sellohora2));
	    /* calculo del tiempo de pensar y se escribe en la bitácora */
	    tpensar = (int) aleat_tpensar (TMP_PAYMENT, estado[E_TP_PAYMENT]);
	    fprintf (flog, "%d ", tpensar);
	    /*TIEMPO DE PENSAR */
	    sleep (tpensar);
	    /*imprimimos la información acerca de la ejecución de la transacción */
	    fprintf (flog, "0 %d %d ", paym_remota, clien_paym);

	    break;

	  case ORDER_STATUS:		/********TRANSACCION ORDER_STATUS*********/
	    /*Pantalla de introducción de datos */
	    pant_ostatus_pet ();
	    /*GENERACIÓN DE DATOS DE TRANSACCIÓN */
	    genera_datos_order_status (w, &men.tran.ostatus);

	    /*TIEMPO DE TECLADO */
	    sleep (TT_OSTATUS);

	    /*Mostrar datos introducidos */
	    pant_ostatus_muest (&men.tran.ostatus);

	    /*PRIMER SELLO DE HORA */
	    ftime (&sellohora);
	    /*Se escribe el sello en la bitácora */
	    fprintf (flog, "%d %d ", sellohora.time, sellohora.millitm);
	    /*Mandar transaccición */
	    msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);
	    /*Se espera a que el MT responda */
	    operacion.sem_num = d_id - 1;
	    operacion.sem_op = -1;
	    operacion.sem_flg = 0;
	    semop (semid, &operacion, 1);	/*esperamos a que el semaforo este a 1 para recojer la respuesta */
	    /*y cerramos el semaforo para esperar otro mensaje */
	    /*SEGUNDO SELLO DE HORA */
	    ftime (&sellohora2);

	    /*Imprimir pantalla de menu */
	    pant_ostatus_menu (&men.tran.ostatus);

	    /*Se imprime el tiempo de respuesta y se escribe en la bitácora */
	    fprintf (flog, "%f ", resta_tiempos (&sellohora, &sellohora2));
	    /* Se calcula el tiempo de pensar y se escribe en la bitácora */
	    tpensar = (int) aleat_tpensar (TMP_OSTATUS, estado[E_TP_OSTATUS]);
	    fprintf (flog, "%d ", tpensar);

	    /*TIEMPO DE PENSAR */
	    sleep (tpensar);
	    /*imprimimos la información acerca de la ejecución de la transacción */
	    fprintf (flog, "0 %d 0 ", clien_os);
	    break;

	  case DELIVERY:		  /********TRANSACCION DELIVERY*********/
	    /*Pantalla de introducción de datos */
	    pant_delivery_pet ();
	    /*GENERACIÓN DE DATOS DE TRANSACCIÓN */
	    genera_datos_delivery (w, &men.tran.delivery);
	    /*TIEMPO DE TECLADO */
	    sleep (TT_DELIVERY);
	    /*Mostrar datos introducidos */
	    pant_delivery_muest (&men.tran.delivery);

	    /*PRIMER SELLO DE HORA */
	    ftime (&sellohora);
	    /*Se escribe el sello en la bitácora */
	    fprintf (flog, "%d %d ", sellohora.time, sellohora.millitm);
	    /*Se manda al tm el primen sello de hora para poder calcular */
	    /*el tiempo de ejecución                                                                        */
	    men.tran.delivery.seg = sellohora.time;
	    men.tran.delivery.mseg = sellohora.millitm;
	    /*Mandar transaccición */
	    msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);

	    /*SEGUNDO SELLO DE HORA */
	    ftime (&sellohora2);
	    /*RESULTADO DE LA TRANSACCION */
	    /*Imprimir pantalla de menú */
	    pant_delivery_menu (&men.tran.delivery);
	    /*Se calcula el tiempo de respuesta (encolado) y se escribe en la bitácora */
	    fprintf (flog, "%f ", resta_tiempos (&sellohora, &sellohora2));

	    /* Se calcula el tiempo de pensar y se escribe en la bitácora */
	    tpensar =
	      (int) aleat_tpensar (TMP_DELIVERY, estado[E_TP_DELIVERY]);
	    fprintf (flog, "%d ", tpensar);
	    /*TIEMPO DE PENSAR */
	    sleep (tpensar);
	    fprintf (flog, "0 0 0 ");
	    break;

	  case STOCK_LEVEL:			/********TRANSACCION STOCK_LEVEL*********/
	    /*Pantalla de introducción de datos */
	    pant_stock_level_pet ();
	    /*GENERACIÓN DE DATOS DE TRANSACCIÓN */
	    genera_datos_stock_level (w, &men.tran.stock_level);
	    /*TIEMPO DE TECALDO */
	    sleep (TT_STOCK_LEVEL);
	    /*Mostrar datos introducidos */
	    pant_stock_level_muest (&men.tran.stock_level);

	    /*PRIMER SELLO DE HORA */
	    ftime (&sellohora);
	    /*Se imprime el sello de hora */
	    fprintf (flog, "%d %d ", sellohora.time, sellohora.millitm);
	    /*Mandar transacción */
	    msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);
	    /*Se envia la transacción y se espera a que el MT responda */
	    operacion.sem_num = d_id - 1;
	    operacion.sem_op = -1;
	    operacion.sem_flg = 0;
	    semop (semid, &operacion, 1);	/*esperamos a que el semaforo este a 1 para recojer la respuesta */
	    /*y cerramos el semaforo para esperar otro mensaje */
	    /*SEGUNDO SELLO DE HORA */
	    ftime (&sellohora2);
	    /*Imprimir pantalla de menu */
	    pant_stock_level_menu (&men.tran.stock_level);
	    /*Se calcula el tiempo de respuesta y se escribe en la bitácora */
	    fprintf (flog, "%f ", resta_tiempos (&sellohora, &sellohora2));
	    /* Se calcula el tiempo de pensar y se escribe en la bitácora */
	    tpensar =
	      (int) aleat_tpensar (TMP_STOCK_LEVEL, estado[E_TP_STOCK_LEVEL]);
	    fprintf (flog, "%d ", tpensar);
	    /*TIEMPO DE PENSAR */
	    sleep (tpensar);
	    fprintf (flog, "0 0 0 ");
	    break;

	  default:		/*Se sale del programa en caso de que la selección del tipo */
	    /*de transacción haya fallado */
	    fprintf (out, "Aleat_int ha sobrepasado el rango\n");
	    exit (-1);
	  }			/* switch */
       /*Se toma el sello de hora de finalización de la transacción y */
       /*se escreibe en la bitácora                                 */
       ftime (&sellohora);
       fprintf (flog, "%d %d ", sellohora.time, sellohora.millitm);
     }				/* de while */

/*Cuando el Controlador del Benchmark envía la señal SIGTERM se deja de enviar transacciones*/
/*y se manda al TM un mensaje de desconexión.                                               */
/* MENSAJE DE DESCONEXION DEL TM*/
  fprintf (out, "\nTerminal %d: ENVIANDO SOLICITUD DE DESCONEXION ... \n",
	   cod_term);
/*Escribimos el mensaje de desconexión*/
  men.tipo = MSGTRM;		/*tipo de mensaje: mensaje de terminal */
  men.tran.msgtrm.codctl = DESCONECTAR;	/*de desconexión */
  men.id = cod_term;		/*identificador de terminal */

/*forzamos el valor del semaforo a 0*/
  semun_arg.val = 0;
  if (semctl (semid, d_id - 1, SETVAL, semun_arg) == -1)
     {
       fprintf (out, "EROR AL ASIGNAR VALOR AL SEMAFORO\n");
     };

/*Se envia el mensaje de desconexión*/
  msgsnd (colid, &men, sizeof (men) - sizeof (men.tipo), 0);
  /*Se espera a que el MT responda */
  operacion.sem_num = d_id - 1;
  operacion.sem_op = -1;
  operacion.sem_flg = 0;
  if (semop (semid, &operacion, 1) == -1)
     {
       fprintf (out, "ERROR EN EL SEMAFORO EN LA DESCONEXIÓN DEL SERVIDOR\n");
       fprintf (out, "error no: %d\n, %s", errno, strerror (errno));
     };
  fprintf (out, "DESCONECTADO DEL SERVIDOR\n");


/*Removing semaphores*/
// diego: Now this is done in bench.c.
// if (semctl(semid,d_id-1,IPC_RMID,0) == -1){
  // fprintf(out,"PROBLEMAS PARA BORRAR EL SEMAFORO EN LA DESCONEXIÓN\n");
  // switch(errno){
  // case EIDRM: fprintf(out,"El semaforo ya estaba eliminada\n");
  // break;
  // case EINVAL: fprintf(out,"Valor invalido de semid\n");
  // break;
  // }    
// }


/*Desconectamos de la memoria compartida*/
  if (shmdt (shm) == -1)
     {
       fprintf (out,
		"PROBLEMAS PARA DESENGANCHARSE DE MEMORIA EN LA DESCONEXIÓN\n");
       switch (errno)
	  {
	  case EIDRM:
	    fprintf (out, "La memoria ya estaba eliminada\n");
	    break;
	  case EINVAL:
	    fprintf (out, "Valor invalido de shmdid\n");
	    break;
	  }
     }
  /*Eliminamos la memoria compartida */
  if (shmctl (shmid, IPC_RMID, 0) == -1)
     {
       fprintf (out, "PROBLEMAS PARA BORRAR MEMORIA EN LA DESCONEXIÓN\n");
       switch (errno)
	  {
	  case EIDRM:
	    fprintf (out, "La memoria ya estaba eliminada\n");
	    break;
	  case EINVAL:
	    fprintf (out, "Valor invalido de shmid\n");
	    break;
	  }
     };

  fprintf (out, "ENLACES BORRADOS\n");
  fprintf (out, "MOVIENDO BITÁCORA... ");

  fclose (flog);

/*Se mueve la bitácora al directorio VARDIR. Se introduce la orden en */
/*la cadena ssystem                                                   */

  strcpy (filenameBuffer, VARDIR);
  sprintf (ssystem, "mv /tmp/%s %s%s\0", n_fichero, filenameBuffer,
	   n_fichero);
/*se ejecuta el comando*/
  if (system (ssystem) != 0)
    fprintf (stderr, "clien: Error en la ejecucion del comando %s.\n",
	     ssystem);
  fprintf (out, "OK\n");
  fprintf (out, "TERMINAL %d APAGADO\n\n", cod_term);

  fclose (out);

}				/*main */

void
sigterm ()
{
/* ---------------------------------------------- *\
|* Función de tratamiento de la señal SIGTERM     *|
|* ---------------------------------------------- *|
|* Modifica el flag que hará que se dejen de      *|
|* enviar transacciones cuando llege la señal     *|
|* SIGTERM                                        *|
\* ---------------------------------------------- */
  if (signal (SIGTERM, sigterm) == SIG_ERR)
     {
       fprintf (out, "error en signal (SIGTERM)\n");
       exit (-1);
     }

  flag = 1;			/*Modificación del flag */
  fprintf (out, "INICIANDO EL PROCESO DE APAGADO DEL TERMINAL %d ...\n",
	   cod_term);
  fprintf (out, "ESTOY REALIZANDO TRANSACCION nº %d\n", tipo);
}

void
ctl_c ()
{
/* ---------------------------------------------- *\
|* Función de tratamiento de la señal SIGINT      *|
|* ---------------------------------------------- *|
|* No realiza ninguna función. Su función en que  *|
|* el ETR no se vea afectado por el ctrl-c        *|
|* que introduce el usuario para parar el test.   *|
\* ---------------------------------------------- */
  if (signal (SIGINT, ctl_c) == SIG_ERR)
     {
       fprintf (out, "Error en signal (SIGINT)\n");
       exit (-1);
     }
}
