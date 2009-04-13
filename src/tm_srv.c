/*
 * TPCC-UVa Project: Open-source implementation of TPCC-v5
 *
 * tm.c: Transaction Monitor
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


/**********************************************************************\
|*               PROGRAMA MONITOR DE TRANSACCIONES                    *|
|* ------------------------------------------------------------------ *|
|* Ejecuta las transacciones enviadas por el ETR sugún su orden de    *| 
|* llegada. Registra los sellos de hora de comienzo y finalización    *|
|* de la transacción Delivery en un fichero de bitácora. Resgistra los*|
|* los resultados de la ejecución de la transacción Delivery en un    *|
|* fichero de bitácora. Registra los posibles errores de la ejecición *|
|* de las transacciones en un fichero de bitácora.                    *|
\**********************************************************************/

/**********************************************************************\
|* Ficheros de Cabecera.                                              *|
\* ------------------------------------------------------------------ */
#include <stdio.h>
#include <pthread.h>
#include "../include/tpcc.h"
#include "msg_marshal.h"
#include "SP_wrapper.h"

/*Fichero de cabecera para el tratamiento de errores en SQL*/
exec sql include sqlca;
/**********************************************************************/

/**********************************************************************\
|* Definición de Constantes                                           *|
\* ------------------------------------------------------------------ */
#define TAM_MEM 1024    /* tamaño de la memoria compartida            */
#define NUM_MAX_CLIENT 10 * NUM_MAX_ALM  /* número maximo de clientes */
                                         /* que pueden hacer conexion */
#define NO_HAY_LINEAS 100 /* error de postgreSQL 'no encontrado'      */
/**********************************************************************/

/********************************************************************\
|* Variables Globales.                                              *|
\* ---------------------------------------------------------------- */

FILE *f_tr_deli;  /* Fichero de tiempos de respuesta de la transacion delivery          */
FILE *f_res_deli; /* Fichero de resultado de la ejecucion de las transacciones delivery */
FILE *ferr;       /* Fichero de errores                                                 */

int salir=0;      /* Flag de permanencia en el bucle principal                          */
		  /* Modificado por las funciones: main y sigterm                       */


/* Global variables used by threads */
int ncli;       /*variable que contendrá los identificadores de*/
                /* terminal que se van asignando e los terminales en las conexiones*/
struct sembuf operacion; /*estructura que contiene operación que se realiza sobre el semáforo*/
/* -------------------------------- */

/* ---------------------------------------------------------------- *\
|* Variables SQL COMPARTIDAS                                        *|
\* ---------------------------------------------------------------- */

EXEC SQL BEGIN DECLARE SECTION;
	/* warehouse table*/
	int w_id;
	char w_name[11];
	char w_street_1[21];
	char w_street_2[21];
	char w_city[21];
	char w_state[3];
	char w_zip[10];
	float w_tax;
	double w_ytd;

	/* district table */
	int d_id;
	int d_w_id;
	char d_name[11];
	char d_street_1[21];
	char d_street_2[21];
	char d_city[21];
	char d_state[3];
	char d_zip[10];
	float d_tax;
	double d_ytd;
	int d_next_o_id;

	/* item table */
	int i_id;
	int i_im_id;
	char i_name[25];
	float i_price;
	char i_data[51];

	/* stock table */
	int s_i_id;
	int s_w_id;
	int s_quantity;
	char s_dist_01[25];
	char s_dist_02[25];
	char s_dist_03[25];
	char s_dist_04[25];
	char s_dist_05[25];
	char s_dist_06[25];
	char s_dist_07[25];
	char s_dist_08[25];
	char s_dist_09[25];
	char s_dist_10[25];
	double s_ytd;
	int s_order_cnt;
	int s_remote_cnt;
	char s_data[51];

	/* customer table */
	int c_id;
	int c_d_id;
	int c_w_id;
	char c_first[17];
	char c_middle[3];
	char c_last[17];
	char c_street_1[21];
	char c_street_2[21];
	char c_city[21];
	char c_state[3];
	char c_zip[10];
	char c_phone[17];
	char c_since[21];
	char c_credit[3];
	double c_credit_lim;
	float c_discount;
	double c_balance;
	double c_ytd_payment;
	int c_payment_cnt;
	int c_delivery_cnt;
	char c_data[501];
	char c_new_data[501];

	/* orderr table */
	int o_id;
	int o_d_id;
	int o_w_id;
	int o_c_id;
	char o_entry_d[21];
	int o_carrier_id;
	int o_ol_cnt;
	int o_all_local;
	int ol_o_id;
	int ol_d_id;
	int ol_w_id;
	int ol_number;
	int ol_i_id;
	int ol_supply_w_id;
	char ol_delivery_d[21];
	int ol_quantity;
	float ol_amount;
	char ol_dist_info[25];		

	/* history table */
	int h_c_id;
	int h_c_d_id;
	int h_c_w_id;
	int h_d_id;
	int h_w_id;
	char h_date[21];
	double h_amount;
	char h_data[25];

	/* new_order table */
	int no_o_id;
	int no_d_id;
	int no_w_id;

	/* auxiliar */
	int ol_cnt;
	int threshold;
	int lowstock;
EXEC SQL END DECLARE SECTION;

/* ---------------------------------------------------------------- *\
|* Vector para la Cominunicación con los terminales                 *|
\* ---------------------------------------------------------------- */
/*Este vector almacena los identificadores de semáforo y de memoria y*/
/*el puntero a esta, de cada ETR que se ha conectado al MT*/
struct tclientes{ 
	int shmid;      /* Identificador de Memoria compartida con el Terminal*/
	int semid;      /* Semaphore set identifier */
	int ident;      /* Semaphore identifier (inside the set) */
	union tshm *shm;	/* Puntero a Memoria Compartida con el Teminal */
	} clientela [NUM_MAX_CLIENT];  

/* ---------------------------------------------------------------- *\
|* Estructura del mensaje procedente de los terminales.             *|
|*              - definida en tpcc.h -                              *|
\* ---------------------------------------------------------------- */
struct mensaje men; 
int srv_id;

/********************************************************************/

int trans_new_order(struct tnew_order_men *new_order, union tshm *shm){
/* ---------------------------------------------------------------- *\
|* Función que implementa la transacción New-Order según el perfil  *|
|* de la cláusula 2.4 del TPC-C.                                    *|
|*------------------------------------------------------------------*|
|* Parámetro new_order: puntero a la estructura de mensaje recibida.*|
|* Parámetro shm: Puntero a la memoria compartida donde se escriben.*|
|* losresultados. If shm is NULL then result is not saved in shm.   *|
\* ---------------------------------------------------------------- */

int i;
double sum_amount; /*Suma de cantidades de artículo*/

getfechahora(o_entry_d); /*Se toma la fecha y la hora del sistema*/
/******RESPUESTA EN MEMORIA COMPARTIDA***/
if (shm == NULL)
	fprintf(stdout, "Not copying o_entry_d to shm.\n");
else
	strcpy(shm->new_order.o_entry_d,o_entry_d);

o_ol_cnt=0; /*Se inicializa el contador de artículos de la orden*/

/*Extracción de datos de la estructura de mensaje recibida*/
w_id=new_order->w_id;
d_id=new_order->d_id;
c_id=new_order->c_id;

EXEC SQL BEGIN; /*COMIENZO DE TRANSACCION*/
	EXEC SQL SELECT w_tax, c_discount, c_last, c_credit  
		INTO :w_tax, :c_discount, :c_last, :c_credit
	 	FROM warehouse, customer
		WHERE w_id=:w_id AND c_w_id=:w_id AND c_d_id=:d_id AND c_id=:c_id;
	
	if (sqlca.sqlcode<0){
		/********ERROR*********/
		fprintf(ferr, "NEW - ORDER linea 1\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECCIAR EN WARE Y CUST\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "w_id= %d, c_w_id = %d, c_d_id = %d, c_id = %d\n", w_id, w_id, d_id, c_id);
		EXEC SQL ROLLBACK; /*Se redhaza la transacción*/
		if (shm == NULL)
			fprintf(stdout, "Transaction cancelled.\n");
		else
			shm->new_order.o_ol_cnt=o_ol_cnt; /*se indica en la respuesta que la transacción */
										  /*ha sido cancelada*/
		return -1;
	}
	if (shm == NULL)
		fprintf(stdout, "Not copying w_tax, c_discount, c_last and c_credit into shm.\n");
	else{
		/*****RESPUESTA******/
		shm->new_order.w_tax=w_tax;
		shm->new_order.c_discount=c_discount;
		strcpy(shm->new_order.c_last,c_last);
		strcpy(shm->new_order.c_credit,c_credit);
		/********************/
	}
	/*Se actualiza el número para la siguiente orden de ese distrito*/
	EXEC SQL SELECT d_next_o_id, d_tax 
		INTO :d_next_o_id, :d_tax 
		FROM district
		WHERE d_id=:d_id AND d_w_id=:w_id;
	if (sqlca.sqlcode<0){
		/********ERROR*********/
		fprintf(ferr, "NEW - ORDER linea 2\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECIONAR EN DIST.\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "d_id= %d, d_w_id = %d\n", d_id, w_id);
		EXEC SQL ROLLBACK;
		if (shm == NULL)
			fprintf(stdout, "Transaction cancelled.\n");
		else
			shm->new_order.o_ol_cnt=o_ol_cnt;
		return -1;
	}

	if (shm == NULL)
		fprintf(stdout, "Not copying d_tax and d_next_o_id into shm.\n");
	else {
		/**RESPUESTA*****/
		shm->new_order.d_tax=d_tax;
		shm->new_order.o_id=d_next_o_id;
		/****************/
	}

	EXEC SQL UPDATE district SET  d_next_o_id=:d_next_o_id+1
		WHERE d_id=:d_id AND d_w_id=:w_id;
	if (sqlca.sqlcode<0){
		/********ERROR*********/
		fprintf(ferr, "NEW - ORDER linea 3\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR DIST.\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "d_id= %d, d_w_id = %d\n", d_id, w_id);
		EXEC SQL ROLLBACK;
		if (shm == NULL)
			fprintf(stdout, "Transaction cancelled.\n");
		else
			shm->new_order.o_ol_cnt=o_ol_cnt;
		return -1;
	} /* if */

	sum_amount=0; 
	o_id = d_next_o_id;
	o_all_local=1;
	/*se inicializa la fila de orderr suponiendo que tódos los artículos son locales: o_all_local=1*/
	EXEC SQL INSERT INTO orderr (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_carrier_id, o_all_local) 
		VALUES (:o_id, :d_id, :w_id, :c_id, :o_entry_d, 0, :o_all_local); 
	if (sqlca.sqlcode<0){
		/********ERROR*********/
		fprintf(ferr, "NEW - ORDER linea 4\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR INSER EN ORDERR.\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "o_id = %d, d_id = %d, w_id = %d, c_id = %d, o_entry_d = %d, o_all_local = %d\n", o_id, d_id, w_id, c_id, o_entry_d, o_all_local);
		EXEC SQL ROLLBACK;
		if (shm == NULL)
			fprintf(stdout, "Transaction cancelled.\n");
		else
			shm->new_order.o_ol_cnt=o_ol_cnt;
		return -1;
	} /* if */
	/*Se inserta una nueva fila en new_order para reflejar la transacción*/
	EXEC SQL INSERT INTO new_order (no_o_id, no_d_id, no_w_id) 
		VALUES (:o_id, :d_id, :w_id);	
	if (sqlca.sqlcode<0){
		/********ERROR*********/
		fprintf(ferr, "NEW - ORDER linea 5\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR INSER EN ORDER_LINE.\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "o_id = %d, d_id = %d, w_id = %d\n", o_id, d_id, w_id);
		EXEC SQL ROLLBACK;
		if (shm == NULL)
			fprintf(stdout, "Transaction cancelled.\n");
		else
			shm->new_order.o_ol_cnt=o_ol_cnt;
		return -1;
	} /* if */

	/************* procesamos cada artículo de la orden***************/		

	i=0;
	while((i<15) && (new_order->item[i].flag==1)) /*mientras no se llegue al artículo 15 y siga */
		{	                                  /*habiendo artículos en el vector             */
		/*extracción los datos del mensaje*/
		ol_i_id=new_order->item[i].ol_i_id;
		ol_supply_w_id=new_order->item[i].ol_supply_w_id;
		ol_quantity=new_order->item[i].ol_quantity;
		ol_number = i+1;

		EXEC SQL SELECT i_price, i_name, i_data INTO :i_price, :i_name, :i_data 
			FROM item
			WHERE i_id=:ol_i_id;

		if (sqlca.sqlcode==NO_HAY_LINEAS){
			/******ROLLBACK DE TRANSACCION POR ARTÏCULO INVÁLIDO*******/			
			if (shm == NULL)
				fprintf(stdout, "Transaction cancelled.\n");
			else
				shm->new_order.ctl=-1; /**Indicación de transaccción rechazada**/
			EXEC SQL ROLLBACK WORK;
			if (shm != NULL)
				shm->new_order.o_ol_cnt=o_ol_cnt;
			return(0);
		}
		if (sqlca.sqlcode<0){
			/*Otro error distinto al anterior*/
			fprintf(ferr, "NEW - ORDER linea 6\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN SELECT ITEM.\n");
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
			fprintf(ferr, "i_id = %d\n", i_id);
			EXEC SQL ROLLBACK;
			if (shm == NULL)
				fprintf(stdout, "Transaction cancelled.\n");
			else
				shm->new_order.o_ol_cnt=o_ol_cnt;
			return -1;
		} /* if */
		if (shm != NULL) {
			/**************************************/
			/***RESPUESTA***/
			shm->new_order.item[i].i_price=i_price;
			strcpy(shm->new_order.item[i].i_name,i_name);
			/***************/
		} else {
			fprintf(stdout, "Not copying i_price and i_name.\n");
		}
		EXEC SQL SELECT s_quantity, s_data, s_dist_01, s_dist_02, s_dist_03, s_dist_04, s_dist_05, s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10
 			INTO :s_quantity, :s_data, :s_dist_01, :s_dist_02, :s_dist_03, :s_dist_04, :s_dist_05, :s_dist_06, :s_dist_07, :s_dist_08, :s_dist_09, :s_dist_10 
			FROM stock 	
			WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id;
		if (sqlca.sqlcode<0){
			/********ERROR*********/
			fprintf(ferr, "NEW - ORDER linea 7\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN SELECT STOCK.\n");
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
			fprintf(ferr, "s_i_id = %d, s_w_id = %d, s_w_id = %d\n", ol_i_id, s_w_id, ol_supply_w_id);
			EXEC SQL ROLLBACK;
			if (shm == NULL)
				fprintf(stdout, "Transaction cancelled.\n");
			else
				shm->new_order.o_ol_cnt=o_ol_cnt;
			return -1;
		} /* if */
		if (shm == NULL)
			fprintf(stdout, "Not copying s_quantity.\n");
		else {
			/**RESPUESTA**/
			shm->new_order.item[i].s_quantity=s_quantity;
			/*************/
		}
		switch(d_id){
			case 1: strcpy(ol_dist_info,s_dist_01); break;
			case 2: strcpy(ol_dist_info,s_dist_02); break;
			case 3: strcpy(ol_dist_info,s_dist_03); break;
 			case 4: strcpy(ol_dist_info,s_dist_04); break;
			case 5: strcpy(ol_dist_info,s_dist_05); break;
			case 6: strcpy(ol_dist_info,s_dist_06); break;
			case 7: strcpy(ol_dist_info,s_dist_07); break;
			case 8: strcpy(ol_dist_info,s_dist_08); break;
			case 9: strcpy(ol_dist_info,s_dist_09); break;
			case 10: strcpy(ol_dist_info,s_dist_10); break;
		} /* switch */
		
		if (s_quantity>=ol_quantity+10){ /*Se actualiza el Stock del artículo*/
			s_quantity=s_quantity-ol_quantity;
			EXEC SQL UPDATE stock SET  s_quantity=:s_quantity
				WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id;
			if (sqlca.sqlcode<0){
				/********ERROR*********/
				fprintf(ferr, "NEW - ORDER linea 7\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN SELECT STOCK.\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
				fprintf(ferr, "s_i_id = %d, s_w_id = %d\n", ol_i_id, ol_supply_w_id);
				EXEC SQL ROLLBACK;
				if (shm == NULL)
					fprintf(stdout, "Transaction cancelled.\n");
				else
					shm->new_order.o_ol_cnt=o_ol_cnt;
				return -1;
			} /* if */
		}
		else{ /*Se renueba el stock del artículo*/			
			s_quantity=(s_quantity-ol_quantity)+91;
			EXEC SQL UPDATE stock SET  s_quantity=:s_quantity
				WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id;
			if (sqlca.sqlcode<0){
				/********ERROR*********/
				fprintf(ferr, "NEW - ORDER linea 8\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN UPDATE STOCK.\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
				fprintf(ferr, "s_i_id = %d, s_w_id = %d\n", ol_i_id, ol_supply_w_id);
				EXEC SQL ROLLBACK;
				if (shm == NULL)
					fprintf(stdout, "Transaction cancelled.\n");
				else
					shm->new_order.o_ol_cnt=o_ol_cnt;
				return -1;
			} /* if */
		}
		EXEC SQL UPDATE stock SET s_ytd=:s_ytd+:ol_quantity, s_order_cnt=:s_order_cnt+1
				WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id;	
		if (sqlca.sqlcode<0){
			/********ERROR*********/
			fprintf(ferr, "NEW - ORDER linea 9\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN UPDATE STOCK.\n");
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
			fprintf(ferr, "s_i_id = %d, s_w_id = %d\n", ol_i_id, ol_supply_w_id);
			EXEC SQL ROLLBACK;
			if (shm == NULL)
				fprintf(stdout, "Transaction cancelled.\n");
			else
				shm->new_order.o_ol_cnt=o_ol_cnt;
			return -1;
		} /* if */
		/*ARTICULO REMOTO*/
		if(ol_supply_w_id!=w_id){
			EXEC SQL UPDATE stock SET s_remote_cnt=:s_remote_cnt+1
					WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id;
			if (sqlca.sqlcode<0){
				/********ERROR*********/
				fprintf(ferr, "NEW - ORDER linea 10\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN UPDATE STOCK.\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
				fprintf(ferr, "s_i_id = %d, s_w_id = %d\n", ol_i_id, ol_supply_w_id);
				EXEC SQL ROLLBACK;
				if (shm == NULL)
					fprintf(stdout, "Transaction cancelled.\n");
				else
					shm->new_order.o_ol_cnt=o_ol_cnt;
				return -1;
			} /* if */
			o_all_local=0;
		}/*de if*/

		if (shm == NULL)
			fprintf(stdout, "Not copying b_g into shm.\n");
		else {
			/*Se comprueba si i_data contiene la cadena 'original'*/
			if ((strstr(i_data,"original") != (char *)NULL) && (strstr(s_data,"original") != (char *)NULL)) 
				shm->new_order.item[i].b_g= 'B';  /**RESPUESTA**/
			else shm->new_order.item[i].b_g= 'G'; /**RESPUESTA**/
		}
		
		/*Se calcula la cuantia de la orden*/
		ol_amount=ol_quantity*i_price;
		if (shm == NULL)
			fprintf(stdout, "Not copying ol_amount into shm.\n");
		else
			shm->new_order.item[i].ol_amount=ol_amount; /**RESPUESTA**/
		sum_amount=sum_amount+ol_amount;

		EXEC SQL INSERT INTO order_line (ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_dist_info)
			VALUES (:o_id, :d_id, :w_id, :ol_number, :ol_i_id, :ol_supply_w_id, :ol_quantity, :ol_amount, :ol_dist_info);
		if (sqlca.sqlcode<0){
			/********ERROR*********/
			fprintf(ferr, "NEW - ORDER linea 11\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN INSERT ORDER_LINE.\n");
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
			fprintf(ferr, "ol_o_id = %d, ol_d_id = %d, ol_w_id = %d, ol_number = %d, ol_i_id = %d, ol_supply_w_id = %d, ol_quantity = %d, ol_amount = %f, ol_dist_info = %s\n", o_id, d_id, ol_w_id, ol_number);
			EXEC SQL ROLLBACK;
			if (shm == NULL)
				fprintf(stdout, "Transaction cancelled.\n");
			else
				shm->new_order.o_ol_cnt=o_ol_cnt;
			return -1;
		} /* if */
		i++; /*se incrementa el contador de artículos*/
	}/*de while*/

	o_ol_cnt=i; /*recuento de artículos*/
	/*Si algún artículo ha sido remoto se indica en la línea correspondiente*/
	if (o_all_local==0){
		EXEC SQL UPDATE orderr SET o_all_local=:o_all_local
				WHERE o_id=:o_id AND o_d_id=:d_id AND o_w_id=:w_id;
		if (sqlca.sqlcode<0){
			/********ERROR*********/
			fprintf(ferr, "NEW - ORDER linea 12\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR ORDERR.\n");
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
			fprintf(ferr, "o_id = %d, o_d_id = %d, o_w_id = %d\n", o_id, d_id, w_id);
			EXEC SQL ROLLBACK;
			if (shm == NULL)
				fprintf(stdout, "Transaction cancelled.\n");
			else
				shm->new_order.o_ol_cnt=o_ol_cnt;
			return -1;
		} /* if */
	}/*de if*/

	EXEC SQL UPDATE orderr SET o_ol_cnt=:o_ol_cnt
		WHERE o_id=:o_id AND o_d_id=:d_id AND o_w_id=:w_id;
	if (sqlca.sqlcode<0){
		/********ERROR*********/
		fprintf(ferr, "NEW - ORDER linea 13\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR ORDERR.\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "o_id = %d, o_d_id = %d, o_w_id = %d\n", o_id, d_id, w_id);
		EXEC SQL ROLLBACK;
		if (shm == NULL)
			fprintf(stdout, "Transaction cancelled.\n");
		else
			shm->new_order.o_ol_cnt=o_ol_cnt;
		return -1;
	} /* if */

	if (shm == NULL)
		fprintf(stdout, "Not copying o_ol_cnt, total_amount and ctl into shm");
	else {
		/**RESPUESTA**/
		shm->new_order.o_ol_cnt=o_ol_cnt;
		shm->new_order.total_amount=sum_amount*(1-c_discount)*(1+w_tax+d_tax);
		shm->new_order.ctl=0; /**Indicación de transacción realizada**/
		/*************/
	}
	/*SE CONFURMA LA TRANSACCIÓN*/
	EXEC SQL COMMIT WORK; 
	/********ERROR*********/
	if (sqlca.sqlcode<0){
		fprintf(ferr, "NEW - ORDER linea 14\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL COMMIT.\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
 return(0);
} /*de trans_new_order*/

int trans_payment(struct tpayment_men *payment, union tshm *shm){
/* ---------------------------------------------------------------- *\
|* Función que implementa la transacción Payment según el perfil    *|
|* de la cláusula 2.5 del TPC-C.                                    *|
|*------------------------------------------------------------------*|
|* Parámetro payment: puntero a la estructura de mensaje recibida.  *|
|* Parámetro shm: Puntero a la memoria compartida donde se escriben.*|
|* losresultados. If shm is NULL then result is not saved in shm.   *|
\* ---------------------------------------------------------------- */

int i;
/*   Variable compartida       */
EXEC SQL BEGIN DECLARE SECTION;
	int cont;
EXEC SQL END DECLARE SECTION;

/*Se extraen los datos de la memoria compartida*/
w_id = payment->w_id;
d_id = payment->d_id;
c_w_id=payment->c_w_id;
c_d_id=payment->c_d_id;
h_amount = payment->h_amount;
strcpy(c_last, payment->c_last);
c_id = payment->c_id;

getfechahora(h_date); /*Se toma la fecha y la hora del sistema*/

EXEC SQL BEGIN; /*Se comienza la transacción*/
	EXEC SQL SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
		INTO :w_name, :w_street_1, :w_street_2, :w_city, :w_state, :w_zip
		FROM warehouse 
		WHERE w_id = :w_id;
	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 1\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECCIAR EN WARE\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "w_id = %d\n", w_id);
		EXEC SQL ROLLBACK; /*Se rechaza la transacción en caso de error*/
		return -1;
	} /* if */

	if (shm) {
		/*********RESPUESTA*********/
		strcpy(shm->payment.w_street_1,w_street_1);
		strcpy(shm->payment.w_street_2,w_street_2);
		strcpy(shm->payment.w_city,w_city);
		strcpy(shm->payment.w_state,w_state);
		strcpy(shm->payment.w_zip,w_zip);
	}

	
	EXEC SQL UPDATE warehouse
		 SET w_ytd = w_ytd + :h_amount
		 WHERE w_id = :w_id;
	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 2\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR EN WARE\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "w_id = %d\n", w_id);
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	EXEC SQL SELECT d_name, d_street_1, d_street_2, d_city, d_state, d_zip
		INTO :d_name, :d_street_1, :d_street_2, :d_city, :d_state, :d_zip
		FROM district
		WHERE d_w_id = :w_id AND d_id = :d_id;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 3\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECCIAR EN DIST\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "d_w_id = %d, d_id = %d\n", w_id, d_id);
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	if (shm) {
		/**RESPUESTA**/
		strcpy(shm->payment.d_street_1,d_street_1);
		strcpy(shm->payment.d_street_2,d_street_2);
		strcpy(shm->payment.d_city,d_city);
		strcpy(shm->payment.d_state,d_state);
		strcpy(shm->payment.d_zip,d_zip);
		/************/
	}

	EXEC SQL UPDATE district
		 SET d_ytd = d_ytd + :h_amount
		 WHERE d_id = :d_id AND d_w_id = :w_id;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 4\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR EN DIST\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
		fprintf(ferr, "d_id = %d, d_w_id = %d\n", d_id, w_id);
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	if (c_id == 0) { /*CLIENTE SELECCIONADO POR C_LAST*/
		/*Se declara un cursor para recorrer todas las filas coincidentes*/
		EXEC SQL DECLARE  c_porlast CURSOR FOR
			SELECT c_id, c_first, c_middle, c_street_1, c_street_2, c_city, c_state, c_zip, c_phone, c_credit, c_credit_lim, c_discount, c_balance, c_since
			FROM customer
			WHERE c_w_id = :c_w_id AND c_d_id = :c_d_id AND c_last = :c_last
			ORDER BY c_first;

		if (sqlca.sqlcode<0){
			/*********ERROR*****************/
			fprintf(ferr, "PAYMENT linea 5\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL DECLARAR EL CURSOR\n");	
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
			fprintf(ferr, "c_w_id = %d, c_d_id = %d, c_last = %s\n", c_w_id, c_d_id, c_last);
			EXEC SQL ROLLBACK;
			return -1;
		} /* if */	

		/*Se cuenta el número de filas coincidentes*/
		EXEC SQL SELECT count(c_id)
			INTO :cont
			FROM customer
			WHERE c_last = :c_last AND c_d_id = :c_d_id AND c_w_id = :c_w_id; 

		if ((cont%2) != 0) cont++; /* si el número de coincidencias es par, se suma 1 */

		EXEC SQL OPEN c_porlast; /*Se inicializa el cursor*/

		/*Se captura la coincidencia que ocupe un lugar intermedio*/
		for (i = 0; i < cont/2; i++){
			EXEC SQL FETCH FROM c_porlast
				INTO :c_id, :c_first, :c_middle, :c_street_1, :c_street_2, :c_city, :c_state, :c_zip, :c_phone, :c_credit, :c_credit_lim, :c_discount, :c_balance,  :c_since;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr, "PAYMENT linea 6\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL CAPTURAR CON EL CURSOR\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
				fprintf(ferr, "VALORES DE LOS CAMPOS:\n");
				fprintf(ferr, "c_id = %d, c_first = %s, c_middle = %s, c_street_1 = %s, c_street_2 = %s, c_city = %s, c_state = %s, c_zip = %s, c_phone = %s, c_credit = %s, c_credit_lim = %f, c_discount = %f, c_balance = %f, c_since = %s\n",c_id, c_first, c_middle, c_street_1, c_street_2, c_city, c_state, c_zip, c_phone, c_credit, c_credit_lim, c_discount, c_balance, c_since );
				EXEC SQL ROLLBACK;
				return -1;

			} /* if */
			
		}/* en este punto tenemos la llave primaria del cliente seleccionado*/

		EXEC SQL CLOSE c_porlast;

	} else {  /* CLIENTE SELECCIONADO POR C_ID */
		
		EXEC SQL SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, c_state, c_zip, c_phone, c_credit, c_discount, c_balance,c_since
			INTO :c_first, :c_middle, :c_last, :c_street_1, :c_street_2, :c_city, :c_state, :c_zip, :c_phone, :c_credit, :c_discount, :c_balance,:c_since
			FROM customer
			WHERE c_w_id = :w_id AND c_d_id = :d_id AND c_id = :c_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr, "PAYMENT linea 7\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECCIAR EN CLIENTE\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
				EXEC SQL ROLLBACK WORK;
				return -1;
			} /* if */

	} /* if (c_id == 0) */

	if (shm) {
		/**RESPUESTA**/
		shm->payment.c_id=c_id;
		strcpy(shm->payment.c_first,c_first);
		strcpy(shm->payment.c_middle,c_middle);
		strcpy(shm->payment.c_last,c_last);
		strcpy(shm->payment.c_street_1,c_street_1);
		strcpy(shm->payment.c_street_2,c_street_2);
		strcpy(shm->payment.c_city,c_city);
		strcpy(shm->payment.c_state,c_state);
		strcpy(shm->payment.c_zip,c_zip);
		strcpy(shm->payment.c_phone,c_phone);
		strcpy(shm->payment.c_credit,c_credit);
		strcpy(shm->payment.c_since,c_since);
		shm->payment.c_credit_lim=c_credit_lim;
		shm->payment.c_discount=c_discount;
		shm->payment.c_balance=c_balance;
		strcpy(shm->payment.h_date,h_date);
		/************/
	}

	EXEC SQL UPDATE customer
		SET c_balance = c_balance - :h_amount, c_ytd_payment = c_ytd_payment + :h_amount, c_payment_cnt = c_payment_cnt +1
		WHERE c_w_id = :c_w_id AND c_d_id = :c_d_id AND c_id = :c_id;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 8\n");
		fprintf(ferr, "ERROR EN TRANSACCION\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR CLIENTE\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK WORK;
		return -1;
	} /* if */

	/* Tratamiento de  C_DATA  */
	if (c_credit[0]=='B'){
		EXEC SQL SELECT c_data 
			INTO :c_data 
			FROM customer 
			WHERE c_id =:c_id AND c_w_id=:c_w_id AND c_d_id=:c_d_id;

		if (sqlca.sqlcode<0){
			/*********ERROR*****************/
			fprintf(ferr, "PAYMENT linea 9\n");
			fprintf(ferr, "ERROR EN TRANSACCION\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECCIONAR CLIENTE\n");	
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			EXEC SQL ROLLBACK WORK;
			return -1;
		} /* if */
		/*Se concatenan las cadenas*/
		sprintf(c_new_data,"%4d %2d %4d %2d %4d %7.2fEur %.10s %.24s", c_id, c_d_id, c_w_id, d_id, w_id, h_amount, h_date, h_data);
		strncat(c_new_data, c_data,500-strlen(c_new_data));
			EXEC SQL UPDATE customer SET c_data=:c_new_data
				WHERE c_w_id=:c_w_id AND c_d_id = :c_d_id AND c_id = :c_id;

		if (sqlca.sqlcode<0){
			/*********ERROR*****************/
			fprintf(ferr, "PAYMENT linea 10\n");
			fprintf(ferr, "ERROR EN TRANSACCION\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR CLIENTE\n");	
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			EXEC SQL ROLLBACK WORK;
			return -1;
		} /* if */
		if (shm)
			strcpy(shm->payment.c_data,c_new_data);
	} else 
		c_new_data[0]='\0';

	 
	/* Tratamiento de h_data*/
	sprintf(h_data,"%s    %s", w_name, d_name);

	EXEC SQL INSERT INTO  history (h_c_d_id, h_c_w_id, h_c_id, h_d_id, h_w_id, h_date, h_amount, h_data)
			VALUES (:c_d_id, :c_w_id, :c_id, :d_id,:w_id,:h_date, :h_amount, :h_data);

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 11\n");
		fprintf(ferr, "ERROR EN TRANSACCION\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL INSERTAR EN HISTORY\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */
	
	/*SE CONFIRMA LA TRANSACCIÓN*/
	EXEC SQL COMMIT WORK;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "PAYMENT linea 12\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL COMMIT\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK WORK;
		return -1;
	} /* if */

return(0);
} /* de trans_payment */

int trans_ostatus(struct torder_status_men *ostatus, union tshm *shm){

/* ---------------------------------------------------------------- *\
|* Función que implementa el perfil de transacción Order-Status     *|
|* según la cláusula 2.6 del TPC-C.                                 *|
|*------------------------------------------------------------------*|
|* Parámetro ostatus: puntero a la estructura de mensaje recibida.  *|
|* Parámetro shm: Puntero a la memoria compartida donde se escriben.*|
|* losresultados.                                                   *|
\* ---------------------------------------------------------------- */

int i;
int art;
/*             Variable compartida									*/
EXEC SQL BEGIN DECLARE SECTION;
	int cont;
EXEC SQL END DECLARE SECTION;

/*Se extraen los datos del mensaje de transacción*/
c_id = ostatus->c_id;
d_id = ostatus->d_id;
w_id = ostatus->w_id;
strcpy(c_last, ostatus->c_last);

EXEC SQL BEGIN; /*Se comienza la transacción*/
	if (c_id != 0){ /*CLIENTE SELECCIONADO POR C_ID */

		EXEC SQL SELECT c_balance, c_first, c_middle, c_last 
				INTO :c_balance, :c_first, :c_middle, :c_last 
				FROM customer
				WHERE c_w_id = :w_id AND c_d_id = :d_id AND c_id = :c_id;
		if (sqlca.sqlcode<0){
				fprintf(ferr,"TRANSACCION ORDER-STATUS linea 1\n");
				fprintf(ferr,"SE HA PRODUCIDO UN ERROR AL DECLARAR EL CURSOR\n");	
				fprintf(ferr,"TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr,"      %s\n", sqlca.sqlerrm.sqlerrmc);	
				EXEC SQL ROLLBACK;
				return -1;
		} /* if */
		
	} else { /*CLIENTE SELECCIONADO POR C_LAST*/
		/*Se Declara un cursor para recorrer todas las filas coincidentes*/
		EXEC SQL DECLARE  c_porlast2 CURSOR FOR
				SELECT c_id, c_first, c_middle, c_balance
				FROM customer
				WHERE c_w_id = :w_id AND c_d_id = :d_id AND c_last = :c_last
				ORDER BY c_first;

		if (sqlca.sqlcode<0){
			/*********ERROR*****************/
			fprintf(ferr,"TRANSACCION ORDER-STATUS linea 2\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL DECLARAR EL CURSOR\n");	
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			EXEC SQL ROLLBACK;
			return -1;
		} /* if */

		cont = 0;	
		/*Se cuenta el número de filas coincidentes*/
		EXEC SQL SELECT count(c_id)
			INTO :cont
			FROM customer
			WHERE c_last = :c_last AND c_w_id = :w_id AND c_d_id = :d_id;
		if (sqlca.sqlcode<0){
			/*********ERROR*****************/
			fprintf(ferr,"TRANSACCION ORDER-STATUS linea 3\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL CONTAR EN CUSTOMER\n");	
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			EXEC SQL ROLLBACK;
			return -1;
		} /* if */

		if ((cont%2) != 0) cont++;

		EXEC SQL OPEN c_porlast2;

		/*Se captura la fila coincidente de la posición intermedia*/
		for (i = 0; i < cont/2; i++){
			EXEC SQL FETCH FROM c_porlast2
				INTO :c_id, :c_first, :c_middle, :c_balance;
	
			if (sqlca.sqlcode<0){
			/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION ORDER-STATUS linea 4\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL CAPTURAR CON EL CURSOR\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} /* if */
			
		}/*de for*/
		/* en este punto tenemos la llave primaria del cliente seleccionado*/
		EXEC SQL CLOSE c_porlast2;
	}

	/**RESPUESTA**/
	shm->ostatus.c_id=c_id;
	strcpy(shm->ostatus.c_middle,c_middle);
	strcpy(shm->ostatus.c_last,c_last);
	strcpy(shm->ostatus.c_first,c_first);
	shm->ostatus.c_balance=c_balance;
	/*************/

	/*Se declara un cursor para las ordenes de ese almacen, distrito y cliente*/
	EXEC SQL DECLARE cur_ordenes CURSOR FOR	
		SELECT o_id, o_entry_d, o_carrier_id
		FROM orderr
		WHERE o_w_id = :w_id AND o_d_id = :d_id AND o_c_id = :c_id
		ORDER BY o_id DESC; /*en orden descendente del número de orden*/

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 5\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL DECLARAR EL CURSOR\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	EXEC SQL OPEN cur_ordenes;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 6\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ABRIR EL CURSOR\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	/*se captura la orden con mayor o_id*/
	EXEC SQL FETCH FROM cur_ordenes  
		INTO :o_id, :o_entry_d, :o_carrier_id;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 7\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL CAPTURAR DEL CURSOR\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	EXEC SQL CLOSE cur_ordenes;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 8\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL CERRAR EL CURSOR\n");
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	/**RESPUESTA**/
	shm->ostatus.o_id=o_id;
	strcpy(shm->ostatus.o_entry_d,o_entry_d);	
	shm->ostatus.o_carrier_id=o_carrier_id;
	/*************/
	
	/*Se declara uncursor para las fils de la tabla order_line que porteneccan a ese almacén*/
	/*distrito, cliente, y la orden seleccinada anteriormente								*/
	EXEC SQL DECLARE cur_ord_lines CURSOR FOR	
		SELECT ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_delivery_d
		FROM order_line
		WHERE ol_w_id = :w_id AND ol_d_id = :d_id AND ol_o_id = :o_id;
	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 9\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL DECLARAR EL CURSOR\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	EXEC SQL OPEN cur_ord_lines;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 10\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ABRIR EL CURSOR\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	art=0;
	while (1){
		/*Se capturan las filas hasta que no haya mas*/
		EXEC SQL FETCH FROM cur_ord_lines 
			INTO :ol_i_id, :ol_supply_w_id, :ol_quantity, :ol_amount, :ol_delivery_d;

		/*si ya no hay mas filas salimos del while*/
		if (sqlca.sqlcode!=0){
			if (sqlca.sqlcode != NO_HAY_LINEAS){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION ORDER-STATUS linea 8\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL LEER EL CURSOR\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} else 
				break;

		}  /*if */

		/***RESPUESTA***/
		shm->ostatus.item[art].ol_i_id=ol_i_id;	
		shm->ostatus.item[art].ol_supply_w_id=ol_supply_w_id;
		shm->ostatus.item[art].ol_quantity=ol_quantity;
		shm->ostatus.item[art].ol_amount=ol_amount;
		strcpy(shm->ostatus.item[art].ol_delivery_d,ol_delivery_d);
		/**********************/
		art++;
	}/*de while*/ 

	shm->ostatus.num_art=art;
	EXEC SQL CLOSE cur_ord_lines;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 11\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL CERRAR EL CURSOR\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	/*SE CONFIRMA LA TRANSACCIÓN*/
	EXEC SQL COMMIT WORK;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION ORDER-STATUS linea 12\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL COMMIT\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

} /* de trans_order_status */

int trans_stock_level(struct tstock_level_men *stock_level, union tshm *shm){

/* ---------------------------------------------------------------- *\
|* Función que implementa el perfíl de transacción Stock-Level según*|
|* la cláusula 2.8 del TPC-C.                                       *|
|*------------------------------------------------------------------*|
|* Parámetro stock_level: puntero a la estructura de mensaje        *|
|* recibida.                                                        *|
|* Parámetro shm: Puntero a la memoria compartida donde se escriben.*|
|* los resultados.                                                  *|
\* ---------------------------------------------------------------- */

/**************** EXTRACCIÓN DE DATOS *****************/
w_id = stock_level->w_id;
d_id = stock_level->d_id;
threshold = stock_level->threshold;
	
EXEC SQL BEGIN; /*Se comienza la transacción*/
	EXEC SQL SELECT d_next_o_id 
		INTO :d_next_o_id
		FROM district
		WHERE d_id = :d_id AND d_w_id = :w_id;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION  STOCK-LEVEL linea 1\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL SELECT\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */

	EXEC SQL SELECT COUNT(DISTINCT (s_i_id)) 
		INTO :lowstock
		FROM stock, order_line
		WHERE ol_w_id = :w_id AND ol_d_id = :d_id AND 
			  ol_o_id < :d_next_o_id AND ol_o_id >= :d_next_o_id -20
			  AND s_w_id = :w_id AND s_i_id = ol_i_id AND s_quantity < :threshold;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL SELECT\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */	

	shm->stock_level.lowstock=lowstock;
EXEC SQL COMMIT WORK;

} /* de trans_stock_level */


int trans_delivery(struct tdelivery_men *delivery){
/* ---------------------------------------------------------------- *\
|* Función que implementa el perfil de transacción Delivery según   *|
|* la cláusula 2.7 del TPC-C.                                       *|
|*------------------------------------------------------------------*|
|* Parámetro delivery: puntero a la estructura de mensaje recibida. *|
\* ---------------------------------------------------------------- */

/**************** EXTRACCIÓN DE DATOS ***************/
w_id = delivery->w_id;
o_carrier_id = delivery->o_carrier_id;

/*Se indica en el fichero de resultados el comienzo de la transacción*/ 
fprintf(f_res_deli, "Inicio Transaccion. Fecha Encolado: %d %d, Almacen %d, Repartidor %d\n", delivery->seg, delivery->mseg, delivery->w_id, delivery->o_carrier_id);

EXEC SQL BEGIN; /*se comienza la transacción*/
	for (no_d_id = 1; no_d_id <= 10; no_d_id++){
		EXEC SQL SELECT min(no_o_id)
			INTO :no_o_id
			FROM new_order
			WHERE no_w_id = :w_id AND no_d_id = :no_d_id;

		if (sqlca.sqlcode<0){
			/*********ERROR*****************/
			fprintf(ferr,"TRANSACCION  DELIVERY linea 1\n");
			fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL SELECT\n");	
			fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
			EXEC SQL ROLLBACK;
			return -1;
		} /* if */	

		if (sqlca.sqlcode == NO_HAY_LINEAS){ 
			/*Si no hay ninguna orden pendiente para ese distrito se indica en el fichero de*/
			/*resultados y se salta al siguiente distrito*/
			fprintf(f_res_deli, "	* %d\n",no_d_id);
		}else{
			/*Se elimina la orden pendiente de la tabla new_order*/
			EXEC SQL DELETE FROM new_order WHERE no_o_id = :no_o_id AND no_w_id = :w_id AND no_d_id = :no_d_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION  DELIVERY linea 2\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ELIMINAR NEW_ORDER \n");	
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK; 
				return -1;
			} /* if */

			EXEC SQL SELECT o_c_id 
					INTO :o_c_id
					FROM orderr
					WHERE o_w_id = :w_id AND o_d_id = :no_d_id AND o_id = :no_o_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION  DELIVERY linea 3\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL SELECCIONAR o_c_id \n");	
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} /* if */
			/*Se actualiza en la orden correspondiente el número de transportista*/
			EXEC SQL UPDATE orderr
				SET o_carrier_id = :o_carrier_id
				WHERE o_w_id = :w_id AND o_d_id = :no_d_id AND o_id = :no_o_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION  DELIVERY linea 4\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR o_carrier_id \n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} /* if */

			getfechahora(ol_delivery_d); /*Se toma la fecha de reparto de la orden*/
			/*Se escribe en las filas de order_line la fecha de reparto*/
			EXEC SQL UPDATE order_line
					SET ol_delivery_d = :ol_delivery_d
					WHERE ol_o_id = :no_o_id AND ol_w_id = :w_id AND ol_d_id = :no_d_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION  DELIVERY linea 5\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR ol_delivery_d \n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} /* if */

			EXEC SQL SELECT sum(ol_amount)
				INTO :c_balance
				FROM order_line
				WHERE ol_o_id = :no_o_id AND ol_w_id = :w_id AND ol_d_id = :no_d_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION  DELIVERY linea 6\n");
				fprintf(ferr, "SE HA PRODUCIDO AL CONTAR ol_amount\n");	
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} /* if */

			/*Se actualiza el balance del cliente, y su número de repartos*/
			EXEC SQL UPDATE customer
				SET c_balance = c_balance + :c_balance, c_delivery_cnt = c_delivery_cnt + 1
				WHERE c_w_id = :w_id AND c_d_id = :no_d_id AND c_id = :o_c_id;

			if (sqlca.sqlcode<0){
				/*********ERROR*****************/
				fprintf(ferr,"TRANSACCION  DELIVERY linea 7\n");
				fprintf(ferr, "SE HA PRODUCIDO UN ERROR AL ACTUALIZAR c_balance y c_delivery_cnt\n");
				fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
				fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				EXEC SQL ROLLBACK;
				return -1;
			} /* if */
			/*Se refleja el reparto de esa orden en el fichero de resultados*/
			fprintf(f_res_deli, "\t#%d %d\n",no_d_id, no_o_id);
		} /* if */		
	} /* for */
	
	/*Se confirma la transacción*/
	EXEC SQL COMMIT WORK;

	if (sqlca.sqlcode<0){
		/*********ERROR*****************/
		fprintf(ferr,"TRANSACCION  DELIVERY linea 8\n");
		fprintf(ferr, "SE HA PRODUCIDO UN ERROR EN EL COMMIT\n");	
		fprintf(ferr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(ferr, "      %s\n", sqlca.sqlerrm.sqlerrmc);			
		EXEC SQL ROLLBACK;
		return -1;
	} /* if */
} /* de trans_delivery */

int t_consumer(){
  struct mensaje msg;
  char buf_rcv[BUFSIZE];
  struct timeb sellohora; /*sello de hora*/

  fprintf(stdout, "Starting consumer thread\n");
  while(salir == 0){
    /* Get message from spread */
    if (SP_recv(buf_rcv, BUFSIZE)){
	decode(&msg, buf_rcv, BUFSIZE);
    }
    else{
    	salir = 1;
    }
    switch (msg.tipo){
 	      case NEW_ORDER: /******************TRANSACCIÓN NEW_ORDER************************/
			fprintf(stdout, "NEW_ORDER Transaction.    ");
			fprintf(stdout, "Terminal %d.", msg.id);
			if (msg.id >= ncli) {
				fprintf(stdout, "Nº de terminal no reconocido\n");
				break;
			}
			/********************** EJECUCIÓN DE TRANSACCIÓN *************************/
			if (srv_id == msg.srv_id){
				trans_new_order(&msg.tran.new_order, clientela[msg.id].shm);
			else
				trans_new_order(&msg.tran.new_order, NULL);

			/*********************	RESPUESTA DE TRANSACCIÓN**************************/
			/* Answer given just if the question came from same warehouse */
			if (srv_id == msg.srv_id){
				operacion.sem_num= clientela[msg.id].ident; 
				operacion.sem_op=1;  /*abrimos el semaforo: el terminal podrá acceder a los resultados*/
				operacion.sem_flg=0;
          			if (semop(clientela[msg.id].semid, &operacion, 1) == -1){
					/* Se ha producido un error al abrir el semáforo */
					fprintf(stderr, "Error al abrir el semáforo\n");
					fprintf(stderr, "Error: %s", strerror(errno));
					break;
				}
			}
			fprintf(stdout, "\tDone.\n");
			break;

	      case PAYMENT: /*******TRANSACCIÓN PAYMENT******/
			fprintf(stdout, "PAYMENT Transaction.      ");
			fprintf(stdout, "Terminal %d.", msg.id);

			if (msg.id >= ncli) {
				fprintf(stdout, "Nº de terminal no reconocido\n");
				break;
			}
			/********************** EJECUCIÓN DE TRANSACCIÓN *************************/
			if (srv_id == msg.srv_id)
				trans_payment(&msg.tran.payment, clientela[msg.id].shm);
			else
				trans_payment(&msg.tran.payment, NULL);

			/*********************	RESPUESTA DE TRANSACCIÓN**************************/
			/* Answer given just if the question came from same warehouse */
			if (srv_id == msg.srv_id){
				operacion.sem_num= clientela[msg.id].ident; 
				operacion.sem_op=1;  /*abrimos el semaforo: el terminal podrá acceder a los resultados*/
				operacion.sem_flg=0;
          			if (semop(clientela[msg.id].semid, &operacion, 1) == -1){
					/* Se ha producido un error al abrir el semáforo */
					fprintf(stderr, "Error al abrir el semáforo\n");
					fprintf(stderr, "Error: %s\n", strerror(errno));
					break;
				}
			}
			fprintf(stdout, "\tDone.\n");
			break;

	      case ORDER_STATUS:  /*******TRANSACCIÓN ORDER-STATUS******/
	      		/* Executed just if the msg came from a client conected to this warehouse */
			if (srv_id == msg.srv_id){
				fprintf(stdout, "ORDER_STATUS Transaction. ");
				fprintf(stdout, "Terminal %d.", msg.id);

				if (msg.id >= ncli) {
					fprintf(stdout, "Nº de terminal no reconocido\n");
					break;
				}
				/********************** EJECUCIÓN DE TRANSACCIÓN *************************/
				trans_ostatus(&msg.tran.ostatus, clientela[msg.id].shm);

				/*********************	RESPUESTA DE TRANSACCIÓN**************************/
				/* Answer given just if the question came from same warehouse */
				operacion.sem_num= clientela[msg.id].ident; 
          		    	operacion.sem_op=1; /*abrimos el semaforo: el terminal podrá acceder a los resultados*/
    	       	    		operacion.sem_flg=0;
	          		if (semop(clientela[msg.id].semid, &operacion, 1) == -1){
					/* Se ha producido un error al abrir el semáforo */
					fprintf(stderr, "Error al abrir el semáforo\n");
					fprintf(stderr, "Error: %s\n", strerror(errno));
					break;
				}
				fprintf(stdout, "\tDone.\n");
			}
			break;

	      case DELIVERY: /*******TRANSACCIÓN DELIVERY******/
			fprintf(stdout, "DELIVERY Transaction.     ");
			fprintf(stdout, "Terminal %d.", msg.id);

			if (msg.id >= ncli) {
				printf("Nº de terminal no reconocido\n");
				break;
			}
			/********************** EJECUCIÓN DE TRANSACCIÓN *************************/
			trans_delivery(&msg.tran.delivery);

			/*********************	RESPUESTA DE TRANSACCIÓN**************************/
			/* TODO: Answer given just if the question came from same warehouse */
			/*Se toma el sello de hora que indica la finalización de la transacción y se*/
			/*escribe en el fichero de tiempos de respuesta junto con el sello de hora que ha enviado el */
			/*terminal y que indica el inicio de la transacción. De esta forma se podrá calcular */
			/*el tiempo de respuesta en el recuento de resultados*/
			ftime(&sellohora);
			fprintf(f_tr_deli,"\n %d %d %d %d %d %d ",msg.tran.delivery.w_id,msg.tran.delivery.d_id,msg.tran.delivery.seg,msg.tran.delivery.mseg,sellohora.time,sellohora.millitm);
			/*Se escribe en el fichero de resultados el sollo de hora de finalización*/
			fprintf(f_res_deli, "Terminada a las: %d %d\n\n", sellohora.time, sellohora.millitm);

			fprintf(stdout, "\tDone.\n");
			break;

	      case STOCK_LEVEL:/*se ha mandado una transaccion*/
	      		/* Executed just in same warehouse */
			if (srv_id == msg.srv_id){
				fprintf(stdout, "STOCK_LEVEL Transaction.  ");
				fprintf(stdout, "Terminal %d.", msg.id);
	
				if (msg.id >= ncli) {
					fprintf(stdout, "Nº de terminal no reconocido\n");
					break;
				}
				/********************** EJECUCIÓN DE TRANSACCIÓN *************************/
				trans_stock_level(&msg.tran.stock_level,clientela[msg.id].shm);

				/*********************	RESPUESTA DE TRANSACCIÓN**************************/
				/* Answer given just if the question came from same warehouse */
				operacion.sem_num= clientela[msg.id].ident; 
				operacion.sem_op=1;  /*abrimos el semaforo: el terminal podrá acceder a los resultados*/
				operacion.sem_flg=0;
	          		if (semop(clientela[msg.id].semid, &operacion, 1) == -1){
					/* Se ha producido un error al abrir el semáforo */
					fprintf(stderr, "Error al abrir el semáforo\n");
					fprintf(stderr, "Error: %s\n", strerror(errno));
					break;
				}
				fprintf(stdout, "\tDone.\n");
			}
			break;
    }
  }
}

int main(int argc, char *argv[]){          
  
  /* ---------------------------------------------- *
   * Función principal del programa                 *
   * ---------------------------------------------- */
  
  void sigterm(); /* Función de tratamiento de la señal SIGTERM */
  void ctl_c();   /* Función de tratamiento de la señal SIGINT */
  
  int semid, shmid, colid; /*identificadores de semáforo, memoria compartida, y cola*/
  key_t llave;  /*variable que contendrá las llaves para los recursos IPC*/
  key_t ident;  /*variable que contendrá los identificadores dentro de cada llave para los recursos IPC*/

  pthread_t t_spread_srv;
  char buf_snd[BUFSIZE];

  if (argc != 2){
	fprintf(stdout, "You should run: ./tm_srv <srv_id>\n");
  }
  else{
	srv_id = atoi(argv[1]);
  }

/* Conexión con el servidor SQL */
EXEC SQL CONNECT TO tpcc; /*USER USERNAME;*/
/*Se activa el modo autocommit para poder realizar transacciones*/
EXEC SQL SET autocommit = ON;

llave= LLAVE_COLA;     /*llave de la cola */

/* Se activan las funciones de tratamiento de las señales */
	/* Señal SIGTERM */
	if (signal (SIGTERM, sigterm) == SIG_ERR){
		/* Error al Activar la función */
		fprintf(stdout, "ERROR EN SIGNAL (SIGTERM)\n");
		exit (-1);
	}

	/* Señal SIGINT */
	if (signal (SIGINT, ctl_c) == SIG_ERR){
		/* Error al Activar la función */
		fprintf(stdout, "ERROR EN SIGNAL (SIGINT)\n");
		exit (-1);
	}

/* Se crea la cola de mensajes */
	if((colid=msgget(llave, IPC_CREAT|0600)) == -1){
		fprintf(stdout, "ERROR al CREAR la COLA DE MENSAJES\n");
		exit(-1);
	}

/* Se abren los ficheros de bitácora de transacciones DELIVERY */
	strcpy(filenameBuffer,VARDIR);
	strcat(filenameBuffer,"tm_delivery_tr.log");
	f_tr_deli=fopen(filenameBuffer,"w"); /*fichero de segistro de tiempos de respuesta*/

	strcpy(filenameBuffer,VARDIR);
	strcat(filenameBuffer,"tm_delivery_res.log");
	f_res_deli=fopen(filenameBuffer,"w");  /*fichero de resultados*/

/* Se abre el fichero de bitácora de errores */
	strcpy(filenameBuffer,VARDIR);
	strcat(filenameBuffer,"tm_err.log");
	ferr = fopen(filenameBuffer, "w");

/* Se inicializa el valor del identificador del siguiente terminal que intente conectar*/
	ncli=0;

	SP_init();
	/* Start server thread */
	pthread_create(&t_spread_srv, NULL, (void *) t_consumer, NULL);
	
/* Bucle Principal */
/*El flag 'salir' determina la permanencia en el bucle, y se modificará cuando el MT reciba la señal SIGTERM*/
	while (salir == 0){
          fprintf(stdout, ">> ");
	  fflush(stdout);

	  /* se recoge el siguiente mensaje de la cola*/
	  msgrcv(colid, &men, sizeof(men)-sizeof(men.tipo),0,0);

	  if (salir == 0) {
	    switch (men.tipo){ /*Se discrimina el tipo de mensaje recibido*/
	    case MSGTRM: /*MENSAJE DE TERMINAL*/	
	      switch (men.tran.msgtrm.codctl){
	      case CONECTAR:	/********CONEXIÓN DE UN NUEVO TERMINAL********/
		
		fprintf(stdout, "tm: Connection request. ");
		if (ncli >= NUM_MAX_CLIENT){
		  fprintf(stderr, "tm: Error: Connection rejected.\n");
		  break;
		}

		/*obtenemos del mensaje el identificador para la memoria compartida y enganchamos con ella*/
		if( (shmid=shmget(men.tran.msgtrm.shm_llave,sizeof(union tshm), IPC_CREAT | 0600))==-1){
		  /* Ha habido un error al crear la memoria compartida */
		  fprintf(stderr, "Error en la creacion de la memoria\n");
		  break;
		}
		
		/*obtenemos del mensaje el identificador para el semaforo y enganchamos con él*/
		if ((semid = semget(men.tran.msgtrm.sem_llave, 10, IPC_CREAT|0600)) == -1){
		  /* Ha habido un error al crear el semáforo */	
		  perror("tm: Error in semaphore creation");
		  break;
		}
				
		// diego: We get the identifier inside the semaphore set for this particular client.
		clientela[ncli].ident = men.tran.msgtrm.sem_ident;
		// diego: end of change
		
		/*Almacenamos los identificadores en el vector de clientela*/
		clientela[ncli].shmid= shmid; /*identificador de la memoria*/
		clientela[ncli].semid= semid; /*identificador del semáforo*/
		clientela[ncli].shm=shmat(shmid,0 ,0); /* puntero a la memoria compartida*/
		
		/*Se escribe en la memoria compartida el identificador de terminal*/
		clientela[ncli].shm->id=ncli;
		
		/*Se informa de la conexión al terminal correspondiente*/
		operacion.sem_num= clientela[ncli].ident; 
		operacion.sem_op=1;  /*Se abre el semáforo: el terminal */ 
		operacion.sem_flg=0; /*recojerá su identificador de terminal*/
		
		if ((semop(clientela[ncli].semid, &operacion, 1))== -1){
		  fprintf(stderr, "tm: Error in semop() call.\n");
		  fprintf(stderr, "    Parameters: sem_num=%i, sem_op=%i, sem_flg=%i.\n",
			  operacion.sem_num, operacion.sem_op, operacion.sem_flg);
		  fprintf(stderr, "    Error identifier: %s\n", strerror(errno));
		  break;
		}
		fprintf(stdout, "SEM %d. ",clientela[ncli].semid);
		fprintf(stdout, "SHM %d. ",clientela[ncli].shmid);
		fprintf(stdout, "Assigned Terminal no. %d.\n", ncli);

		/*Se prepara el número de identificación de terminal para una nueva conexión*/
		ncli++;
		
		break;                     

	      case DESCONECTAR: /********DESCONEXIÓN DE UN TERMINAL********/

		fprintf(stdout, "Disconnecting ");
		fprintf(stdout, "Terminal %d. ", men.id);

		if (men.id >= ncli) {
		  fprintf(stderr, "tm: Error: Unknown terminal number.\n");
		  break;
		}
		/*El MT se desengancha de la memoria compartida*/
		if (shmdt(clientela[men.id].shm) == -1){
		  /*Se ha producido un error en la desconexión con la memoria*/ 
		  fprintf(stdout, "Error al desconectar la memoria\n");
		};
		
		operacion.sem_num= clientela[men.id].ident; 
		operacion.sem_op=1;  /*abrimos el semaforo*/
		operacion.sem_flg=0;
		if (semop(clientela[men.id].semid, &operacion, 1) == -1){
		  /* Se ha producido un error al abrir el semáforo */
		  fprintf(stdout, "Error al abrir el semáforo\n");
		  fprintf(stdout, "Error: %s", strerror(errno));
		  fprintf(stdout, "SEMID: %i\n",clientela[men.id].semid);
		  fprintf(stdout, "men.id = %i\n", men.id);
		}
		
		fprintf(stdout, "\tOK \n");
		break;         
		
	      } /* switch */
	      break;
	    default:
	      if (encode(&men, buf_snd, BUFSIZE)){
		/* Send message through spread */
		SP_send(buf_snd, BUFSIZE);
	      }
	      else{
		salir = 1;
	      }
	      
	    } /*de switch*/       
	  } /* de if (salir) */
	}/*de while*/

/*Se sale del bucle cuando se ha recibido la señal SIGTERM y se ha modificado el flag 'salir'*/

	/* Se elimina la cola de mensajes */
	if (msgctl(colid, IPC_RMID, 0) == -1) 
		/* Hay un problema al borrar la cola */
		fprintf(stderr, "PROBLEMA AL BORRAR LA COLA\n");

	/* Se cierran los fichero de bitácora */
	fclose(f_tr_deli); /*fichero de tiempos de respuesta de transacción Delivery*/
	fclose(f_res_deli); /*fichero de resultados de transacción Delivery*/
	fclose(ferr); /*Fichero de errores*/

	/* Desconecta con USERNAME */
	EXEC SQL DISCONNECT;

	fprintf(stdout, "MT Shut down.\n\n");

} /*main*/

void sigterm(){
/* ---------------------------------------------- *\
|* Función de tratamiento de la señal SIGTERM     *|
|* ---------------------------------------------- *|
|* Modifica el flag que hará que se dejen de      *|
|* recibir transacciones cuando llege la señal    *|
|* SIGTERM                                        *|
\* ---------------------------------------------- */
	if (signal (SIGTERM, sigterm) == SIG_ERR){
		/* Error al Activar la función */
		fprintf(stdout, "ERROR EN SIGNAL (SIGTERM)\n");
		exit (-1);
	}

	fprintf(stdout,"Starting shut down MT process... \n");
	salir = 1; /*se modifica el flag que marca la permanencia en el bucle pricipal*/
}

void ctl_c(){
/* ---------------------------------------------- *\
|* Función de tratamiento de la señal SIGINT      *|
|* ---------------------------------------------- *|
|* No realiza ninguna función. Su función en que  *|
|* el MT no se vea afectado por el ctrl-c         *|
|* que introduce el usuario para parar el test.   *|
\* ---------------------------------------------- */ 
	if (signal (SIGINT, ctl_c) == SIG_ERR){
		/* Error al Activar la función */
		fprintf(stdout, "ERROR EN SIGNAL (SIGINT)\n");
		exit (-1);
	}
}

