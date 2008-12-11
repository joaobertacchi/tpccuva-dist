
/*
 * TPCC-UVa Project: Open-source implementation of TPCC-v5
 *
 * bench.c: Benchmark Control program
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
|*                     BENCHMARK CONTROL PROGRAM                    *|
|* -----------------------------------------------------------------*|
|* Gives the user interface for accesing the benchmark functions.   *|
|* Furthermore, perfroms the functions below:                       *|
|* 	- Initial database population.                              *|
|* 	- Existing database recovery.                               *|
|* 	- Processes performance text related launching.             *|
|* 	- Database consistency checking.                            *|
|* 	- Database droping.                                         *|
|* 	- Database tables cardinality checking.                     *|
|* ---------------------------------------------------------------- *|
|* During the test execution, the benchmark control program needs   *|
|* that the RTE and MT programs were in its directory.              *|
|* During the population and recovery tasks the benchmark control   *|
|* program forks into two processes, one of them will perform the   *|
|* operations while the other process will control the task.        *|
\********************************************************************/

/********************************************************************\
|* Header Files.                                                    *|
\* ---------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <values.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/tpcc.h"

EXEC SQL INCLUDE sqlca; /* postgreSQL library */
/********************************************************************/

/**********************************************************************\
|* Constant definition.                                               *|
\* ------------------------------------------------------------------ */
#define NEW_ORDER 3       /* NEW-ORDER transaction identificator      */
#define PAYMENT 4         /* PAYMENT transaction identificator        */
#define ORDER_STATUS 5    /* ORDER-STATUS transaction identificator   */
#define DELIVERY 6        /* DELIVERY transaction identificator       */
#define STOCK_LEVEL 7     /* STOCK-LEVEL transaction identificator    */
#define NUM_INT_TP 20     /* Number of intervals for the think time   */
                          /* graph.                                   */
#define NUM_INT_TR 500    /* Number of intervals for the response     */
                          /* time graph.                              */
#define INT_REND 30       /* Interval in seconds for the throughput   */
                          /* graph.                                   */
#define NO_HAY_LINEAS 100 /* Value returned by postgreSQL when the    */ 
                          /* query return no matches or the end of    */
                          /* a cursor has been reached.               */
/**********************************************************************/

/********************************************************************\
|* Global variables.                                                *|
\* ---------------------------------------------------------------- */
int terminado = 0;  /* Flag that indicates the termination of some  */
                    /* of the active processes.                     */
int w = 0;          /* Number of warehouses involved in the test.   */
int term = 0;       /* Number of terminals per warehouse.           */
pid_t pidp;         /* Control process PID                          */
pid_t pidh;         /* Execution process PID.                       */
pid_t *pidtm;       /* Pointer to the Transaction Monitor PID.      */
pid_t *pidterm;     /* Pointer to the terminals PID vector.         */
pid_t *pidcheck;    /* Pointer to the Checkpoints Controller PID.   */
pid_t *pidvacuum;   /* Pointer to the Vacuums Controller PID.       */
int C_C_LAST;      /* C_LOAD constant for the field C_LAST.        */
int C_C_LAST_RUN;   /* C_RUN constant for the field C_LAST.        */
int C_C_ID;         /* C_LOAD constant for the field C_ID.         */
int C_OL_I_ID;      /* C_LOAD constant for the field OL_I_ID.      */
int timeout= 0;      /* Flag for controlling the waiting loops.     */
int salir = 0;       /* Flag for controling the main loop of the    */ 
                     /* benchmark control program.                  */
char e_global[32];   /* Global state vector.                        */
unsigned short semilla; /* Seed used to generate the state vectors. */
extern char **environ;  /* Pointer to Environment variableses.      */
FILE *med;              /* Pontert to the test results output file. */
struct timeb sellohora1, sellohora2; /* Auxiliar timestamps.        */
/********************************************************************/


int lanzador(int w, int term, int srv_id){
/* ---------------------------------------------- *\
|* Lanzador function.                             *|
|* ---------------------------------------------- *|
|* Launches the terminals and the Transaction     *|
|* Monitor.                                       *|
|* ---------------------------------------------- *|
|* <w> number of warehouses involved in the test. *|
|* <term> numbers of RTE for each warehouse.      *|
|* ---------------------------------------------- *|
|* The processes are lauched by means of the      *|
|* system call execve() after creating a child    *|
|* process with the system call fork().           *|
|* The terminals PIDs are stored into a vector    *|
|* pointed by the globar pointer *pidterm.        *|
|* The Transaction Monitor PID is stored into a   *|
|* variable pointed by the global pointer *pidtm. *|
\* ---------------------------------------------- */

char *param[8]; /* Vector of parameters for the function execve */
char men0[8], men1[3], men2[3], men3[3], men4[3], men5[3]; /* Parameters for the function execve */
int llave = 10; /* Key, passed to the clients, for the memory and semaphores management. */
int i= 0, d = 1;
const int n_chars_sid = 4;
char s_srv_id[n_chars_sid]; /* Max of 1000 srv_ids (000 - 999) */

	sprintf(s_srv_id, "%d", srv_id);

	if (srv_id > (int)powf((float)10, (float)(n_chars_sid - 1) - 1) ){
		fprintf(stdout, "\n\nsrv_id invalid! Continuing anyway...\n\n\n");
		sleep(1);
	}

	/* Allocating memory for the TM PID */
	if ((pidtm = (pid_t *)malloc(sizeof(pid_t))) == NULL){
		fprintf(stdout, "Launcher: Error allocating memory for PIDs\n");
		return 0;
	}

	/* Allocating memory for the terminals PID vector ('term' terminals per warehouse) */
	if ((pidterm = (pid_t *)malloc(sizeof(pid_t) * term * w)) == NULL){
		fprintf(stdout, "Launcher: Error allocating memory for PIDs\n");
		return 0;
	}

	/* Launching the Transaction Monitor. */
	fprintf(stdout, "Launching TM...");
	fflush(stdout);
	if ((*pidtm = fork()) == 0){
		/* Hijo */
		strcpy(filenameBuffer,EXECDIR);
		strcat(filenameBuffer,"tm_srv");
		// debug
		fprintf(stdout,"Command: ++%s++\n",filenameBuffer);
		sprintf(men0, "tm_srv\0");     /* Name of the program.                      */
		param[0] = men0;
		param[1] = s_srv_id;
		param[2] = NULL;
		if (execve(filenameBuffer, param, environ)){
			fprintf(stderr, "bench: Error launching the Transaction Monitor\n");
			return(-1);
		} /* if */ 
	}
	/* The code below is not executed by the child process.       */
	fprintf(stdout, "OK\n");

	/* Launching the terminals */
	fprintf(stdout, "Launching terminals...");
	for (i = 0; i<w; i++){
		// diego: Instead of a single key equal to 10 for everyone, 
		// we will use one different key per set of TERM terminals. 
		// llave = i;
		// diego: End of change.
		
		for (d = 1; d <= term; d ++){
			if ((pidterm[i*10+d-1] = fork()) == 0){
				/* Child */
				/*Parameters passed to clients.                                            */
			
				sprintf(men0, "clien\0");     /* Name of the program.                      */
				sprintf(men1, "%i", llave); /* Key for the memory and semaphores.        */
				sprintf(men2, "%i", i+1);   /* Terminal warehouse.                       */
				sprintf(men3, "%i", d);     /* Terminal district.                        */
				sprintf(men4, "%i", w);     /* Total number of warehouses                */
				sprintf(men5, "1");         /* Execution mode. (1 = stdout to /dev/null) */
				param[0]=men0;
				param[1]=men1;
				param[2]=men2;
				param[3]=men3;
				param[4]=men4;
				param[5]=men5;
				param[6]=s_srv_id;
				param[7]=NULL;
				strcpy(filenameBuffer,EXECDIR);
				strcat(filenameBuffer,"clien");
				fprintf(stdout,"Command: ++%s++\n",filenameBuffer);
				if (execve(filenameBuffer, param, environ)){
					fprintf(stderr, "bench: Error launching terminal number %d\n", i);
					return(-1);
				} /* if */
				sleep(1);
			}
		}
	}
	/* The code bellow is not executed by the child process.       */
	fprintf(stdout, " OK\n");
return 0;
} /* lanzador */

int lanza_check(){
/* ----------------------------------------------- *\
|* lanza_check function.                           *|
|* ----------------------------------------------- *|
|* Lauches the Checkpoints Controller.             *|
|* ----------------------------------------------- *|
|* The "CC" is launched using the execve() system  *|
|* call after calling the fork() function          *|
|* The PID is stored into a variable pointed by    *|
|* the global pointer *pidcheck.                   *|
\* ----------------------------------------------- */

/* Allocating memory for the Checkpoint Controller PID.        */
if ((pidcheck = (pid_t *)malloc(sizeof(pid_t))) == NULL){
	fprintf(stdout, "Laucher: Error allocating memory for pids\n");
	return 0;
}

/* Launching the program.            */
fprintf(stdout, "Launching the Checkpoint Controller...");
fflush(stdout);
if ((*pidcheck = fork()) == 0){
	/* Hijo */
	strcpy(filenameBuffer,EXECDIR);
	strcat(filenameBuffer,"check");
	if (execve(filenameBuffer, NULL,environ)){
		fprintf(stderr, "bench: Error launching the Checkpoint Controller.\n");
		fprintf(stderr, "%s\n", strerror(errno));
		exit(-1);
	}/* if */ 
}	
fprintf(stdout, " OK\n");
return 1;
}

int lanza_vacuum(int intv, int num){
/* ----------------------------------------------- *\
|* lanza vacuum function                           *|
|* ----------------------------------------------- *|
|* Launches the Vacuums Controller.                *|
|* ----------------------------------------------- *|
|* <intv> interval in seconds between two vacuums. *|
|* <num> maximum number of vacuums to perform.     *|
|* ----------------------------------------------- *|
|* The Vacuums Controller is launched by the       *|
|* system call execve() after calling the function *|
|* fork().                                         *|
|* The PID is pointed by the global pointer        *|
|* *pidvacuum.                                     *|
\* ----------------------------------------------- */

char *param[4]; /* Vector of parameters for the function execve */
char  men0[7],men1[6],men2[6]; /* parameters for the functions execve */

/* Allocating memory for storing the Vacuums Controller PID */
if ((pidvacuum = (pid_t *)malloc(sizeof(pid_t))) == NULL){
		fprintf(stdout, "Launcher: Error allocating memory for PID\n");
		return 0;
}

/* Launching program */
fprintf(stdout, "Launching Vacuums Controller ...");
fflush(stdout);
if ((*pidvacuum = fork()) == 0){
	/* Hijo */
	sprintf(men0, "vacuum\0"); /* Program Name*/
	sprintf(men1, "%d\0", intv); /*Interval between vacuums*/
	sprintf(men2, "%d\0", num);  /*Maximum number of vacuums*/
	param[0]=men0;
	param[1]=men1;
	param[2]=men2;
	param[3]=NULL;
	strcpy(filenameBuffer,EXECDIR);
	strcat(filenameBuffer,"vacuum");
	if (execve(filenameBuffer, param, environ)){
		fprintf(stderr, "bench: Error launching Vacuums Controller\n");
		fprintf(stderr, "%s\n", strerror(errno));
		exit(-1);
	}/* if */ 
}/*if*/	
fprintf(stdout, " OK\n");
return 1;
}/*lanza_vacuum*/

void restaura_new_order(int num_alma){
/* ----------------------------------------------- *\
|* restaura_new_order function.                    *|
|* ----------------------------------------------- *|
|* <num_alma> number of warehouses in the database *|
|* ----------------------------------------------- *|
|* Restore the database from changes performed by  *|
|* the new-order transaction.                      *|
\* ----------------------------------------------- */

int i,j,z;
char hora[21];    /* Sting for the system date. */
char estados[32]; /* State vector for the random variables. */


/* SQL variables declaration */
EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	int d_id;
	double d_ytd;
	int d_next_o_id;
	int no_o_id;
	int no_d_id;
	int no_w_id;
	int s_i_id;
	int s_quantity;
	int orden;
EXEC SQL END DECLARE SECTION;

/*Restoring d_next_o_id*/
	fprintf(stdout,"	Restoring DISTRICT... ");
	fflush(stdout);
	orden=NEXT_O_ID;
	EXEC SQL UPDATE district SET  d_next_o_id=:orden;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: RESTORING THE NEXT_O_ID VALUE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");
	fflush(stdout);
	
/*Restoring the new_order table */
	fprintf(stdout,"	Restoring NEW_ORDER... ");
	fflush(stdout);

	/* Deleting new_order table */
	EXEC SQL DELETE FROM new_order; 

	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: DELETING NEW_ORDER TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */


        /* Populating again the neworder table*/
	for (i = 1; i <= num_alma; i++){/* for each district*/
     	 for (j = 1; j <= 10; j++){/*for each warehouse*/
		/* inserting NUM_ORDER-N_OR_UNDELIVER+1 rows in the table */
		for (z = N_OR_UNDELIVER; z <= NUM_ORDER; z++){ 
			no_o_id=z;
			no_d_id=j;
			no_w_id=i;
			EXEC SQL INSERT 
		 		  INTO new_order (no_o_id, no_d_id, no_w_id)
					VALUES (:no_o_id, :no_d_id, :no_w_id);
	
			if (sqlca.sqlcode<0){
				fprintf(stderr, "ERROR: INSERTING ROW IN THE ORDERR TABLE\n");
				fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
				fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			} /* if */
	
		} /* for z */
    	  }/* for j*/
	 } /* for i*/

	fprintf(stdout,"OK.\n");
	fflush(stdout);

/*Restoring the orderr and order_line tables */

	/*Deleting the created rows in the order_line table */
	fprintf(stdout,"	Restoring ORDERR Y ORDER_LINE... ");
	fflush(stdout);
	orden=NEXT_O_ID;
	EXEC SQL DELETE FROM order_line WHERE ol_o_id>=:orden;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: DELETING ROWS IN THE NEW_ORDER TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

	/*Deleting the created rows in the orderr table */
	EXEC SQL DELETE FROM orderr WHERE o_id>=:orden;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: DELETING ROWS IN THE ORDERR TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");
	fflush(stdout);

/*Restoring the stock table*/
	fprintf(stdout,"	Restoring STOCK... ");

	/* Initializing the state vector with the seed */
	/* used in the database population. */
	initstate(SEMILLACARGA+14, estados,32);
	setstate(estados);
	for (w_id=1;w_id<=num_alma;w_id++){/*for each warehouse*/
		for(s_i_id=1;s_i_id<=NUM_ART;s_i_id++){/*for each item */
				s_quantity = (int)aleat_int(10,100); 
				EXEC SQL UPDATE stock SET s_quantity=:s_quantity, s_ytd=0, 
						s_order_cnt=0, s_remote_cnt=0
				WHERE s_w_id=:w_id AND s_i_id=:s_i_id;
			if (sqlca.sqlcode<0){
				fprintf(stderr, "ERROR RESTORING STOCK TABLE\n");
				fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
				fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			} /* if */
		} /* for item  */
	}/*for warehouses */
	fprintf(stdout, "OK.\n");
	fflush(stdout);
}/*restaura_new_order*/

void restaura_payment(int num_alma){
/* ----------------------------------------------- *\
|* restaura_payment function.                      *|
|* ----------------------------------------------- *|
|* <num_alma> number of warehouses in the database *|
|* ----------------------------------------------- *|
|* Restores the changes performed in the database  *|
|* by the payment transaction.                     *|
\* ----------------------------------------------- */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	double w_ytd;
	int d_id;
	double d_ytd;
	int o_id;
	char c_data[501];
	int c_id;
	char h_date[21];
	char h_data[25];
EXEC SQL END DECLARE SECTION;

char estado[32];   /* state vector for the c_data string lenght */
char estado2[32];  /* state vector for the h_data string lenght */
char e_global[32]; /* state vector for the strings characters */

/*Restoring the warehouse and district tables */
	fprintf(stdout,"	Restoring WAREHOUSE... ");
	EXEC SQL UPDATE warehouse SET w_ytd=300000;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR RESTORING W_YTD\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");
	fprintf(stdout,"	Restoring DISTRICT... ");
	EXEC SQL UPDATE district SET d_ytd=30000;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR RESTORING D_YTD\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");


/*Restoring customer and history tables*/
  
	/* Initializing the state vectors with the values used in the database population */
	initstate(SEMILLACARGA,e_global,32);
	initstate(SEMILLACARGA+21, estado,32);/*22*/
	initstate(SEMILLACARGA+24, estado2,32);/*25*/

	getfechahora(h_date);

	fprintf(stdout,"	Restoring HISTORY and CUSTOMER... ");
	/*se borra toda la tabla history*/
	EXEC SQL DELETE  FROM history;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: DELETING THE HISTORY TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	for(w_id=1;w_id<=num_alma;w_id++){ /*for each warehouse*/
		for(d_id=1;d_id<=10;d_id++){/*for each district*/
			for(c_id=1;c_id<=NUM_CLIENT;c_id++){/*for each customer*/
				cad_alfa_num(300,500,c_data,estado,e_global);
				EXEC SQL UPDATE customer SET c_balance=-10,c_ytd_payment=10, 
						c_payment_cnt=1,  c_data=:c_data
				WHERE c_w_id=:w_id AND c_d_id=:d_id AND c_id=:c_id;
				if (sqlca.sqlcode<0){
					fprintf(stderr, "ERROR: RESTORING THE CUSTOMER TABLE\n");
					fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
					fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				} /* if */

				/*inserting the corresponding row in the history table*/
				cad_alfa_num(12,24,h_data,estado2,e_global);
				EXEC SQL INSERT 
			    INTO history ( h_c_id, h_c_d_id, h_c_w_id, h_d_id, h_w_id, h_date, h_amount, h_data)
				VALUES ( :c_id, :d_id, :w_id, :d_id, :w_id, :h_date, 10, :h_data);
			if (sqlca.sqlcode<0){
				fprintf(stderr, "ERROR RESTORING THE HISTORY TABLE\n");
				fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
				fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			} /* if */

			}/* for customer */
		}/*for district*/
	}/*for warehouse*/
	fprintf(stdout,"OK.\n");

}/* restaura_payment*/

void restaura_delivery(int num_alma){
/* ----------------------------------------------- *\
|* restaura_delivery function.                     *|
|* ----------------------------------------------- *|
|* <num_alma> number of warehouses in the database *|
|* ----------------------------------------------- *|
|* Restores the database from the changes          *|
|* performed by the delivery transaction.          *|
\* ----------------------------------------------- */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int o_id;
EXEC SQL END DECLARE SECTION;

/* restoring the customer table */
	fprintf(stdout,"	Restoring CUSTOMER... ");
	EXEC SQL UPDATE customer SET c_delivery_cnt=0;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: RESTORING DISTRICT TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");
	
/* restoring the orderr table */
	fprintf(stdout,"	Restoring ORDERR... ");
	o_id=N_OR_UNDELIVER;
	EXEC SQL UPDATE orderr SET o_carrier_id=0
		WHERE o_id>=:o_id AND o_id <= 3000;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: RESTORING ORDERR TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");

/* restoring order_line table */
	fprintf(stdout,"	Restoring ORDER_LINE... ");
	EXEC SQL UPDATE order_line SET ol_delivery_d='1970-01-01'
	WHERE ol_o_id>=:o_id AND ol_o_id <= 3000;
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR: RESTORING ORDER-LINE TABLE\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
	fprintf(stdout,"OK.\n");

}/* restaura_delivery*/

void restaura(){
/* ----------------------------------------------- *\
|* restaura function.                              *|
|* ----------------------------------------------- *|
|* This function obtains the number of warehouses  *|
|* in the database and calls the database          *|
|* restoring functions.                            *|
|* After restoring the database executes the       *|
|* postgreSQL command VACUUM ANALYCE.              *|
|* ----------------------------------------------- *|
|* Required functions:                             *|
|*      restaura_new_order().                      *|
|*      restaura_payment().                        *|
|*      restaura_delivery().                       *|
\* ----------------------------------------------- */

/* SQL variables */
char comienzo[21]; /* Date string of the process begin */
char final[21];    /* Date string of the process end */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w;
EXEC SQL END DECLARE SECTION;

/* Conecting with the database */
EXEC SQL CONNECT TO tpcc;/* USER USERNAME;*/

EXEC SQL SET autocommit = ON;

/* Obtaining the number of warehouses */
EXEC SQL SELECT count(*) INTO :w FROM warehouse;

getfechahora(comienzo);
fprintf(stdout, "Start date: %s\n",comienzo);

fprintf(stdout,">> RESTORING THE TABLES CHANGED BY NEW_ORDER \n");
restaura_new_order(w);
fprintf(stdout,">> RESTORING THE TABLES CHANGED BY PAYMENT\n");
restaura_payment(w);
fprintf(stdout,">> RESTORING THE TABLES CHANGED BY DELIVERY\n");
restaura_delivery(w);
fprintf(stdout,">> PERFORMING VACUUM\n");

/* doing vacuum */
EXEC SQL VACUUM ANALYZE;
if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR WHILE PERFORMING VACUUM\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
} /* if */

/* printing end */
fprintf(stdout,"   RESTORE COMPLETED\n");
getfechahora(final);
fprintf(stdout, "\nStart date: %s\n",comienzo);
fprintf(stdout, "End date: %s\n",final);

/* disconnecting */
EXEC SQL DISCONNECT;
} /* restaura */

void condicion_1(int num_alm){
/* ----------------------------------------------- *\
|* condicion_1 function.                           *|
|* ----------------------------------------------- *|
|* <num_alm> number of warehouses in the database  *|
|* ----------------------------------------------- *|
|* This function checks the first database         *|
|* consistency condition defined in the TPC-C      *|
|* standard and outputs to stdout the results.     *|
\* ----------------------------------------------- */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	double w_ytd;
	double d_ytd;
EXEC SQL END DECLARE SECTION;

	/* Checking the condition for each warehouse */
	for(w_id=1;w_id<=num_alm;w_id++){
		EXEC SQL SELECT w_ytd INTO :w_ytd FROM warehouse WHERE w_id=:w_id;
		/* Each district */
		EXEC SQL SELECT SUM(d_ytd) INTO :d_ytd FROM district WHERE d_w_id=:w_id;

		if (fabs(w_ytd -  d_ytd) > 0.001){
			fprintf(stdout,"WAREHOUSE %d DOES NOT MEET CONDITION 1\n",w_id);
			fprintf(stdout,"w_ytd = %.10f, sum_ytd = %.10f\n", w_ytd, d_ytd);
		}else fprintf(stdout,"WAREHOUSE %d MEETS CONDITION 1\n",w_id);

	}/*warehouse*/
}

void condicion_2(int num_alm){
/* ----------------------------------------------- *\
|* condicion_2 function.                           *|
|* ----------------------------------------------- *|
|* <num_alm> number of warehouses in the database  *|
|* ----------------------------------------------- *|
|* Checks the second database consistency          *|
|* condition defined in the TPC-C standard. The    *|
|* results are printed on stdout.                  *|
\* ----------------------------------------------- */

int cumplido=1; /* condition flag */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	int d_id;
	int d_next_o_id;
	int o_id;
	int cuenta;
	int no_o_id;
EXEC SQL END DECLARE SECTION;

/*Each warehouse */
for(w_id=1;w_id<=num_alm;w_id++){
	/* Each district */
	for(d_id=1;d_id<=10;d_id++){
		EXEC SQL SELECT d_next_o_id 
			INTO :d_next_o_id 
			FROM district 
			WHERE d_w_id=:w_id AND d_id=:d_id;

		EXEC SQL SELECT max(o_id) 
			INTO :o_id 
			FROM orderr
			WHERE o_w_id=:w_id AND o_d_id=:d_id;

		if ((d_next_o_id-1)!=o_id){
			fprintf(stdout,"DISTRICT %d FROM WAREHOUSE %d DOES NOT MEET CONDITION 2 FOR THE ORDERR TABLE\n",d_id, w_id);
			fprintf(stdout,"  ->d_next_o_id  = %d max o_id = %d \n",d_next_o_id,o_id);
			cumplido=0; 
		}/*if*/
		/* if exists, outstanding orders are stored in the new_order table */
		EXEC SQL SELECT count(*) 
			INTO :cuenta
			FROM new_order
			WHERE no_w_id=:w_id AND no_d_id=:d_id;
		if (cuenta!=0){/* There are outstanding orders for this warehouse */
			EXEC SQL SELECT max(no_o_id) 
				INTO :no_o_id 
				FROM new_order
				WHERE no_w_id=:w_id AND no_d_id=:d_id;
			if ((d_next_o_id-1)!=no_o_id){
				fprintf(stdout,"DISTRICT %d FROM WAREHOUSE %d DOES NOT MEET CONDITION 2 FOR THE NEW_ORDER TABLE\n",d_id, w_id);
				fprintf(stdout,"  ->d_next_o_id  = %d max no_o_id = %d \n",d_next_o_id,no_o_id);
				cumplido=0; 
				}/*if*/
			}/*if*/
		}/* for district */
	}/*for warehouse*/

if (cumplido!=1)
	fprintf(stdout," DATABASE DOES NOT MEET CONDITION 2\n");
 else 	fprintf(stdout," ALL THE DISTICTS MEET CONDITION 2\n");
} /*de condicion 2*/

void condicion_3(int num_alm){
/* ----------------------------------------------- *\
|* condicion_3 function.                           *|
|* ----------------------------------------------- *|
|* <num_alm> number of warehouses in the database  *|
|* ----------------------------------------------- *|
|* Checks the third database consistency condition *|
|* defined in the TPC-C standard. The results are  *|
|* printed on stdout.                              *|
\* ----------------------------------------------- */

int cumplido=1; /* condition flag */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	int d_id;
	int d_next_o_id;
	int o_id;
	int cuenta;
	int no_o_id;
	int max_no_o_id;
	int min_no_o_id;
EXEC SQL END DECLARE SECTION;

/* Each warehouse */
for(w_id=1;w_id<=num_alm;w_id++){
	/* Each district */
	for(d_id=1;d_id<=10;d_id++){
		/*if exists, outstanding orders are stored in the new_order table*/
		cuenta=0;
		EXEC SQL SELECT count(*) 
			INTO :cuenta
			FROM new_order
			WHERE no_w_id=:w_id AND no_d_id=:d_id;
		if (cuenta!=0){/*There are outstanding orders for this warehouse*/
			EXEC SQL SELECT max(no_o_id) 
				INTO :max_no_o_id 
				FROM new_order
				WHERE no_w_id=:w_id AND no_d_id=:d_id;
			EXEC SQL SELECT min(no_o_id) 
				INTO :min_no_o_id 
				FROM new_order
				WHERE no_w_id=:w_id AND no_d_id=:d_id;
			if ((max_no_o_id-min_no_o_id+1)!=cuenta){
				fprintf(stdout,"DISTRICT %d FROM WAREHOUSE %d DOES NOT MEET CONDITION 3\n",d_id, w_id);
				fprintf(stdout,"  ->max_no_o_id  = %d min_no_o_id = %d new_order cardinality= %d \n",max_no_o_id,min_no_o_id,cuenta);
				cumplido=0; 
				}/*de if*/
			}/* if cuenta !=0 */
	}/* for district */
}/*for warehouse */

if (cumplido!=1)
	fprintf(stdout," THE DATABASE DOES NOT MEET CONDITION 3\n");
 else 	fprintf(stdout," ALL DISTRICTS MEET CONDICION 3\n");

}/*condicion 3*/

void condicion_4(int num_alm){
/* ----------------------------------------------- *\
|* condicion_4 function.                           *|
|* ----------------------------------------------- *|
|* <num_alm> number of warehouses in the database. *|
|* ----------------------------------------------- *|
|* Checks the fourth database consistency          *|
|* condition defined in the TPC-C standard.        *|
|* The results are printed on stdout.              *|
\* ----------------------------------------------- */

int cumplido=1; /* condition flag  */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	int d_id;
	int cuenta1;
	int cuenta2;
EXEC SQL END DECLARE SECTION;

/* Each warehouse */
for(w_id=1;w_id<=num_alm;w_id++){
	/* Each district*/
	for(d_id=1;d_id<=10;d_id++){
		cuenta1=0;
		cuenta2=0;
		EXEC SQL SELECT sum(o_ol_cnt) 
			INTO :cuenta1
			FROM orderr
			WHERE o_w_id=:w_id AND o_d_id=:d_id;

			EXEC SQL SELECT count(*) 
				INTO :cuenta2 
				FROM order_line
				WHERE ol_w_id=:w_id AND ol_d_id=:d_id;
			if (cuenta1!=cuenta2){
				fprintf(stdout,"DISTRICT %d FROM WAREHOUSE %d DOES NOT MEET CONDITION 4\n",d_id, w_id);
				fprintf(stdout,"  ->sum o_ol_cnt = %d order_line cardinality= %d \n",cuenta1,cuenta2);
				cumplido=0; 

			}/* if cuenta1!= cuenta2*/
	}/*for district */
}/*for warehouse*/

if (cumplido!=1)
	fprintf(stdout," THE DATABASE DOES NOT MEET CONDITION 4\n");
 else 	fprintf(stdout," ALL THE DISTRICTS MEET CONDITION 4\n");
}/*condicion 4*/

void consistencia(){
/* ----------------------------------------------- *\
|* consistencia function.                          *|
|* ----------------------------------------------- *|
|* This function obtains the number of warehouses  *|
|* in the database and calls the functions for     *|
|* checking the database consistency conditions.   *|
|* ----------------------------------------------- *|
|* Required functions:                             *|
|*      condicion_1().                             *|
|*      condicion_2().                             *|
|*      condicion_3().                             *|
|*      condicion_4().                             *|
\* ----------------------------------------------- */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int w;
EXEC SQL END DECLARE SECTION;

/* Conecting to database   */
EXEC SQL CONNECT TO tpcc;/* USER USERNAME;*/

/* Obtainint the number of warehouses */
EXEC SQL SELECT count(*) INTO :w FROM warehouse;

fprintf(stdout, "Number of warehouses: %d\n",w);

/* Checking consistency */
condicion_1(w);
condicion_2(w);
condicion_3(w);
condicion_4(w);

/* disconnecting */
EXEC SQL DISCONNECT;
}

void poblar_warehouse(int w){
/* ----------------------------------------------- *\
|* poblar_warehouse function.                      *|
|* ----------------------------------------------- *|
|* <w> number of warehouses to insert in the       *|
|* database.                                       *|
|* ----------------------------------------------- *|
|* Inserts in the table warehouse <w> rows with    *|
|* the data defined by the TPC-C standard.         *|
\* ----------------------------------------------- */

#define E_W_NAME 0      /* Constant for accesing to the state vector corresponding to w_name */
#define E_W_STREET_1 1  /* Constant for accesing to the state vector corresponding to w_street_1 */
#define E_W_STREET_2 2  /* Constant for accesing to the state vector corresponding to w_street_2 */
#define E_W_TAX 3       /* Constant for accesing to the state vector corresponding to w_tax */
#define E_W_CITY 4      /* Constant for accesing to the state vector corresponding to w_city */

char estados[5][32];    /* State vectors array for the random variables */
char hora[21];          /* System date string */
int i,j;                

/* SQL variables */
  EXEC SQL BEGIN DECLARE SECTION;
	int w_id;
	char w_name[11];
	char w_street_1[21];
	char w_street_2[21];
	char w_city[21];
	char w_state[3];
	char w_zip[10];
	float w_tax;
	double w_ytd;
  EXEC SQL END DECLARE SECTION;

/* Initializing state vectors */
/* for each state vector seed++ */
initstate(semilla++, estados[E_W_NAME],32); /*seed (semilla) = 2*/
initstate(semilla++, estados[E_W_STREET_1],32);
initstate(semilla++, estados[E_W_STREET_2],32);
initstate(semilla++, estados[E_W_TAX],32);
initstate(semilla++, estados[E_W_CITY],32); /*seed (semilla) = 6*/

  for (i=1; i <= w; i++){
	w_id = i;
	/* generating fields */
	cad_alfa_num(6,10,w_name, estados[E_W_NAME],e_global);
	cad_alfa_num(10,20,w_street_1, estados[E_W_STREET_1],e_global);
	cad_alfa_num(10,20,w_street_2, estados[E_W_STREET_2],e_global);
	cad_alfa_num(10,20,w_city, estados[E_W_CITY],e_global);
	cad_alfa_num(2,2,w_state, e_global,e_global);
	cad_num(4,w_zip,e_global);
	// for (j=4; j < 10; j++)
	for (j=4; j < 8; j++)
		w_zip[j] = '1';
	// w_zip[10] = '\0';
	 w_zip[8] = '\0';
	/* getting state E_W_TAX */
	setstate(estados[E_W_TAX]);
	w_tax = aleat_dbl(0,0.2);
	w_ytd = 300000;
	/* Inserting the row */
	EXEC SQL INSERT 
		INTO warehouse (w_id, w_name, w_street_1, w_street_2, w_city, w_state, w_zip, w_tax, w_ytd)
		VALUES (:w_id, :w_name, :w_street_1, :w_street_2, :w_city, :w_state, :w_zip, :w_tax, :w_ytd);

        /*************ERROR HANDLING********/
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR INSERTING THE ROW\n");
		fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stderr, "ITEM %d, ALMACEN %d\n", j+1, i);
	} /* if */

	getfechahora(hora);
	fprintf(stdout, "%s> Warehouse %d Created\n", hora, w_id);
	EXEC SQL COMMIT;
        
	/*************ERROR HANDLING********/
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR INSERTING THE ROW (commit)\n");
		fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stderr, "ITEM %d, ALMACEN %d\n", j+1, i);
	} /* if */

  } /* for */

}/* poblar_warehouse*/

void poblar_district(int w){
/* ----------------------------------------------- *\
|* poblar_district function.      *|
|* ----------------------------------------------- *|
|* <w> number of warehouses present in the         *|
|* database.                                       *|
|* ----------------------------------------------- *|
|* Inserts in the district table <w*10> rows with  *|
|* data defined by the TPC-C standard.             *|
\* ----------------------------------------------- */

#define E_D_NAME 0      /* Constant for accesing to the state vector corresponding to d_name field */
#define E_D_STREET_1 1  /* Constant for accesing to the state vector corresponding to d_street_1 field */
#define E_D_STREET_2 2  /* Constant for accesing to the state vector corresponding to d_street_2 field */
#define E_D_CITY 3      /* Constant for accesing to the state vector corresponding to d_city field */
#define E_D_TAX 4       /* Constant for accesing to the state vector corresponding to d_tax field */

int i,j,z;
char estados[5][32];    /* State vectors array for the random variables generation */
char hora[21];          /* System date string */

/* SQL variables */
  EXEC SQL BEGIN DECLARE SECTION;
	int d_id;
	int d_w_id;
	char d_name[11];
	char d_street_1[21];
	char d_street_2[21];
	char d_city[21];
	char d_state[3];
	// char d_zip[10];
	char d_zip[9];
	float d_tax;
	double d_ytd;
	int d_next_o_id;
  EXEC SQL END DECLARE SECTION;

initstate(semilla++, estados[E_D_NAME],32); /*seed (semilla) = 7*/
initstate(semilla++, estados[E_D_STREET_1],32);
initstate(semilla++, estados[E_D_STREET_2],32);
initstate(semilla++, estados[E_D_CITY],32);
initstate(semilla++, estados[E_D_TAX],32); /*seed (semilla) = 11*/

  for (i = 1; i <= w; i++){ /*for each warehouse */
      for (j = 1; j <= 10; j++){ /*for each district */

	      	/* Generating fields */
		d_id = j;
		d_w_id = i;
		cad_alfa_num(6,10,d_name,estados[E_D_NAME],e_global);
		cad_alfa_num(10,20, d_street_1,estados[E_D_STREET_1],e_global);
		cad_alfa_num(10,20, d_street_2,estados[E_D_STREET_2],e_global);
		cad_alfa_num(10,20, d_city,estados[E_D_CITY],e_global);
		cad_alfa_num(2,2, d_state,e_global,e_global);
		cad_num(4,d_zip,e_global);
		// for (z=4; z <= 9; z++)
		for (z=4; z < 8; z++)
			d_zip[z] = '1';
		// d_zip[z] = '\0';
		d_zip[8] = '\0';
		setstate(estados[E_D_TAX]);
		d_tax = aleat_dbl(0,0.2);
		d_ytd = 30000;
		d_next_o_id = NEXT_O_ID;

	/* Inserting the row */
	EXEC SQL INSERT 
		INTO district ( d_id, d_w_id, d_name, d_street_1, d_street_2, d_city, d_state, d_zip, d_tax, d_ytd, d_next_o_id)
		VALUES ( :d_id, :d_w_id, :d_name, :d_street_1, :d_street_2, :d_city, :d_state, :d_zip, :d_tax, :d_ytd, :d_next_o_id);

	/**************ERROR HANDLING********/
	if (sqlca.sqlcode<0){
		fprintf(stdout, "SE HA PRODUCIDO UN ERROR AL INSERTAR UNA FILA\n");
		fprintf(stdout, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stdout, "ITEM %d, ALMACEN %d\n", j+1, i);
	} /* if */

	EXEC SQL COMMIT;
	/**************ERROR HANDLING********/
	if (sqlca.sqlcode<0){
		fprintf(stdout, "SE HA PRODUCIDO UN ERROR EN EL COMMIT\n");
		fprintf(stdout, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stdout, "ITEM %d, ALMACEN %d\n", j+1, i);
	} /* if */

	getfechahora(hora);
	fprintf(stdout, "%s> Created District %d, Warehouse %d\n", hora, d_id, d_w_id);
     } /* for */
  } /* for */

}/* poblar_district*/

void poblar_item() {
/* ----------------------------------------------- *\
|* poblar_item function.                           *|
|* ----------------------------------------------- *|
|* Inserts in the item table NUM_ART (defined in   *|
|* tpcc.h) rows with the data required by the      *|
|* TPC-C standard.                                 *|
\* ----------------------------------------------- */

#define E_I_NAME 0    /* Constant for accesing the state vector corresponding to i_name field */
#define E_I_DATA 1    /* Constant for accesing the state vector corresponding to i_data field */
#define E_I_PRICE 2   /* Constant for accesing the state vector corresponding to i_price field */

int art[NUM_ART/10]; /* Vector with the 10% of items identificators */
char estados[3][32];  /* State vector array for the random variables generation */
int tamano,pos; 
int i,j,h;
char hora[21];        /* System date string */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int i_id;
	int i_im_id;
	char i_name[25]; 
	float i_price;
	char i_data[51];
EXEC SQL END DECLARE SECTION;

/* Initiatizing state vectors */
initstate(semilla++, estados[E_I_NAME],32); /*seed (semilla) = 12*/
initstate(semilla++, estados[E_I_DATA],32);
initstate(semilla++, estados[E_I_PRICE],32);/*seed (semilla) = 14*/

/* the vector is filled with the 10% of item identificators */
/* randomly selected. The vector is sorted in ascending order */

aleat_vect(art,NUM_ART/10,1,NUM_ART);
/****************************************/

h=0; /* Index for accesing the vector */

for (i=0;i<NUM_ART;i++){ /* for NUM_ART items*/
	i_id=i+1;
	/* generating the fields */
	tamano= cad_alfa_num(14,24,i_name,estados[E_I_NAME],e_global);
	tamano= cad_alfa_num(26,50,i_data,estados[E_I_DATA],e_global);
	if (art[h]==i+1){
		/* Item i+1 forms part of the 10% of randomly selected items */
		h++;
		setstate(e_global); 
		pos=aleat_int(0,tamano-8);
		i_data[pos]='o';
		i_data[pos+1]='r';
		i_data[pos+2]='i';
		i_data[pos+3]='g';
		i_data[pos+4]='i';
		i_data[pos+5]='n';
 		i_data[pos+6]='a';	
		i_data[pos+7]='l';
	}/*if*/	
	setstate(e_global); 
	i_im_id=aleat_int(1,10000);
	setstate(estados[E_I_PRICE]);
	i_price=aleat_dbl(1,100);

	/* Inserting the row */
	EXEC SQL INSERT INTO item (i_id, i_im_id, i_name, i_price, i_data) 
		values(:i_id,:i_im_id, :i_name, :i_price, :i_data);

	/*************ERROR HANDLING********/
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR INSERTING THE ROW\n");
		fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stderr, "ITEM %d, ALMACEN %d\n", j+1, i);
	} /* if */

	EXEC SQL COMMIT;

	/* Error handling */
	if (sqlca.sqlcode<0){

		fprintf(stderr, "ERROR INSERTING THE ROW (commit)\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);

		fprintf(stderr, "FIELD VALUES AT ERROR:\n");
		fprintf(stderr, "	I_ID: %d, I_NAME: %s\n",i_id, i_name);
		fprintf(stderr, "	I_DATA: %s\n", i_data);
		fprintf(stderr, "	I_PRICE: %f, I_IM_ID: %d\n",i_price, i_im_id);

		fprintf(stdout, "I_NAME:\n");
		for (j=0;i_name[j] != 0;j++){
	
			if ((i_name[j] >= 33) && (i_name[j] <= 47))
				fprintf(stdout, "(%d, %d, %c), ", j, i_name[j], i_name[j]);
		}
		fprintf(stdout, "\nI_DATA: \n");
		for (j=0;i_data[j] != 0;j++){
			if ((i_data[j] >= 33) && (i_data[j] <= 47))
				fprintf(stdout, "(%d,%d,%c), ", j, i_data[j], i_data[j]);
		}
		fprintf(stdout, "\n");

	} 
	if ((i+1)%10000 == 0) {
		getfechahora(hora);
		fprintf(stdout, "%s> %d Items created out of %d\n", hora, i+1, NUM_ART);
	}
}/* for */

}/* poblar_item */

void poblar_stock(int n){
/* ----------------------------------------------- *\
|* poblar_stock function.                          *|
|* ----------------------------------------------- *|
|* <n> number of warehouses in the database.       *|
|* ----------------------------------------------- *|
|* inserts num_art*n rows in the district table    *|
|* with data defined by the TPC-C standard.        *|
\* ----------------------------------------------- */

#define E_S_QUANTITY 0    /* Constant for accesing the state vector of random field s_quantity */
#define E_S_DATA 1        /* Constant for accesing the state vector of random field s_data */

int stock[NUM_ART/10];   /* Vector for storing the 10% of item identificators. */
int tamano,pos; 
int i,j,z,h;
char hora[21];            /* System date string */
char estados[2][32];      /* Matrix of state vector for the random fields generation */

/* SQL variables */
  EXEC SQL BEGIN DECLARE SECTION;
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
  EXEC SQL END DECLARE SECTION;

 /* initializing state vectors */ 
  initstate(semilla++, estados[E_S_QUANTITY],32);/*seed (semilla) = 15*/
  initstate(semilla++, estados[E_S_DATA],32);/*seed (semilla) = 16*/

  for (i=1; i <= n; i++){ /* for each warehouse */

	/* the vector is filled with the 10% of item identificators */
	/* randomly selected. The vector is sorted in ascending order */

	aleat_vect(stock,NUM_ART/10,1,NUM_ART);
	/********************************************************************/

	h=0; /* index for accesing the item identificators vector */

     for(j=0; j < NUM_ART; j++){ /* for each item*/

	/* generating the fields */
	s_i_id = j+1;
	s_w_id = i;
	
	setstate(estados[E_S_QUANTITY]);
	s_quantity = (int)aleat_int(10,100);
	cad_alfa_num(24,24,s_dist_01,e_global,e_global);
	cad_alfa_num(24,24,s_dist_02,e_global,e_global);
	cad_alfa_num(24,24,s_dist_03,e_global,e_global);
	cad_alfa_num(24,24,s_dist_04,e_global,e_global);
	cad_alfa_num(24,24,s_dist_05,e_global,e_global);
	cad_alfa_num(24,24,s_dist_06,e_global,e_global);
	cad_alfa_num(24,24,s_dist_07,e_global,e_global);
	cad_alfa_num(24,24,s_dist_08,e_global,e_global);
	cad_alfa_num(24,24,s_dist_09,e_global,e_global);
	cad_alfa_num(24,24,s_dist_10,e_global,e_global);
	s_ytd = 0;
	s_order_cnt = 0;
	s_remote_cnt = 0;

	tamano= cad_alfa_num(26,50,s_data,estados[E_S_DATA],e_global);

	if (stock[h]==j+1){  /* selected to be "ORIGINAL" */
		h++;
		setstate(e_global);
		pos=aleat_int(0,tamano-8); /* random position into the string */
		s_data[pos]='o';
		s_data[pos+1]='r';
		s_data[pos+2]='i';
		s_data[pos+3]='g';
		s_data[pos+4]='i';
		s_data[pos+5]='n';
 		s_data[pos+6]='a';	
		s_data[pos+7]='l';
	}/*de if*/	

	/* Inserting the row */
	EXEC SQL INSERT 
		INTO stock ( s_i_id, s_w_id, s_quantity, s_dist_01, s_dist_02, s_dist_03, s_dist_04,  s_dist_05,
					 s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10, s_ytd, s_order_cnt, s_remote_cnt, s_data)
		VALUES ( :s_i_id, :s_w_id, :s_quantity, :s_dist_01, :s_dist_02, :s_dist_03, :s_dist_04, :s_dist_05,
					 :s_dist_06, :s_dist_07, :s_dist_08, :s_dist_09, :s_dist_10, :s_ytd, :s_order_cnt, :s_remote_cnt, :s_data); 


	/*************ERROR HANDLING********/
	if (sqlca.sqlcode<0){
		fprintf(stderr, "ERROR INSERTING THE ROW\n");
		fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stderr, "ITEM %d, ALMACEN %d\n", j+1, i);
	} /* if */

	EXEC SQL COMMIT;

	/*************EROR HANDLING********/
	if (sqlca.sqlcode<0){

		fprintf(stderr, "ERROR INSERTING THE ROW (commit)\n");
		fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
		fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
	} else {
		if (((j+1)%(NUM_ART/50)) == 0) {
			getfechahora(hora);
			fprintf(stdout, "%s> %d Items Created of Warehouse %d\n", hora, j+1, i);
		}	
	}
     }
  } /* for */
}/* poblar_stock */

void poblar_customer(int w){
/* ----------------------------------------------- *\
|* poblar_stock function.                          *|
|* ----------------------------------------------- *|
|* <w> number of warehouses in the database.       *|
|* ----------------------------------------------- *|
|* inserts in the customer table NUM_CLIEN rows    *|
|* with data defined by the TPC-C standard.        *|
\* ----------------------------------------------- */

#define E_C_FIRST 0      /* Constant for accesing the state vertor of the field c_first */
#define E_C_STREET_1 1   /* Constant for accesing the state vertor of the field c_street_1 */
#define E_C_STREET_2 2   /* Constant for accesing the state vertor of the field c_street_2 */
#define E_C_CITY 3       /* Constant for accesing the state vertor of the field c_city */
#define E_C_DISCOUNT 4   /* Constant for accesing the state vertor of the field c_discount */
#define E_C_DATA 5       /* Constant for accesing the state vertor of the field c_data */
#define E_C_LAST 6       /* Constant for accesing the state vertor of the field c_last */
#define E_A_C_LAST 7     /* Constant for accesing the state vertor of the field c_c_last */

char estados[8][32];     /* Matrix of state vectors for generating the random fields. */
int i,j,z,e,h;
char hora[21];           /* System date string */


int c_cred[NUM_CLIENT/10];  /* Vector for storing the 10 % of cunstomer identificators */

/* SQL variables */
  EXEC SQL BEGIN DECLARE SECTION;
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
  EXEC SQL END DECLARE SECTION;

/* Initializing state vectors */
  initstate(semilla++, estados[E_C_FIRST],32);/*seed (semilla) = 17*/
  initstate(semilla++, estados[E_C_STREET_1],32);
  initstate(semilla++, estados[E_C_STREET_2],32);
  initstate(semilla++, estados[E_C_CITY],32);
  initstate(semilla++, estados[E_C_DISCOUNT],32);
  initstate(semilla++, estados[E_C_DATA],32);/*seed (semilla) = 22*/
  initstate(semilla++, estados[E_C_LAST],32);
  initstate(semilla++, estados[E_A_C_LAST],32);/*seed (semilla) = 24*/

  for (i = 1; i <= w; i++){ /*for each warehouse*/
      for (j = 1; j <= 10; j++){ /*for each district*/


	/* the vector is filled with the 10% of customer identificators */
	/* randomly selected. The vector is sorted in ascending order */

	aleat_vect(c_cred,NUM_CLIENT/10,1,NUM_CLIENT);
	/**************************************************************************************/

	h=0; /* index for accesing the customers vector */

	for (z = 1; z <= NUM_CLIENT; z++){ 

		if (z <= 1000)
			crea_clast(z-1,c_last);
		else
			crea_clast(nurand(255,0,MAX_C_LAST,C_C_LAST,estados[E_C_LAST],estados[E_A_C_LAST]),c_last);
		c_id = z;
		c_d_id = j;
		c_w_id = i;
		cad_alfa_num(8,16,c_first,estados[E_C_FIRST],e_global);
		strcpy(c_middle, "OE\0");
		cad_alfa_num(10,20,c_street_1,estados[E_C_STREET_1],e_global);
		cad_alfa_num(10,20,c_street_2,estados[E_C_STREET_2],e_global);
		cad_alfa_num(10,20,c_city,estados[E_C_CITY],e_global);
		cad_alfa_num(2,2,c_state,e_global,e_global);
		cad_num(4,c_zip,e_global);
		for (e=4; e<9; e++)
			c_zip[e] = '1';
		c_zip[e]= '\0';
		cad_num(16,c_phone,e_global);
		getfechahora(c_since);
		if (c_cred[h] == z){ 
			/* Customer selected to be "BC" */
			h++;
			strcpy(c_credit, "BC\0");
		}
		else
			strcpy(c_credit, "GC\0");
		c_credit_lim = 50000;
		setstate(estados[E_C_DISCOUNT]);
		c_discount = aleat_dbl(0, 0.5);
		c_balance = -10;
		c_ytd_payment = 10;
		c_payment_cnt = 1;
		c_delivery_cnt = 0;
		cad_alfa_num(300,500,c_data,estados[E_C_DATA],e_global);

		/* Inserting the row */
		EXEC SQL INSERT 
	 	    INTO customer (	c_id, c_d_id, c_w_id, c_first, c_middle, c_last, c_street_1, c_street_2, c_city, c_state,
						c_zip, c_phone, c_since, c_credit, c_credit_lim, c_discount, c_balance, c_ytd_payment,
						c_payment_cnt, c_delivery_cnt, c_data)
			VALUES (:c_id, :c_d_id, :c_w_id, :c_first, :c_middle, :c_last, :c_street_1, :c_street_2, :c_city, :c_state,
						:c_zip, :c_phone, :c_since, :c_credit, :c_credit_lim, :c_discount, :c_balance, :c_ytd_payment,
						:c_payment_cnt, :c_delivery_cnt, :c_data);

		/*************ERROR HANDLING********/
		if (sqlca.sqlcode<0){
			fprintf(stderr, "ERROR INSERTING THE ROW\n");
			fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, ALMACEN %d\n", j+1, i);
		} /* if */

		EXEC SQL COMMIT; 

		/*************ERROR HANDLING********/
		if (sqlca.sqlcode<0){
			fprintf(stderr, "ERROR INSERTING THE ROW (commit)\n");
			fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, ALMACEN %d\n", j+1, i);
		} /* if */

	} /* for */
		getfechahora(hora);
		fprintf(stdout, "%s> Created customers. Warehouse %d, District %d\n", hora, i, j);
     } /* for */
  } /* for */
}/* poblar_customer*/

void poblar_history(int w){
/* ----------------------------------------------- *\
|* poblar_history function.                        *|
|* ----------------------------------------------- *|
|* <w> number of warehouses in the database.       *|
|* ----------------------------------------------- *|
|* inserts in the history table NUM_CLIEN*w*10     *|
|* rows with data defined by the TPC-C standard.   *|
\* ----------------------------------------------- */

char estados[32];  /* State vector for the field h_data */
int i,j,z;
char hora[21];     /* System date string */

/* SQL variables */
  EXEC SQL BEGIN DECLARE SECTION;
	int h_c_id;
	int h_c_d_id;
	int h_c_w_id;
	int h_d_id;
	int h_w_id;
	char h_date[21];
	double h_amount;
	char h_data[25];	
  EXEC SQL END DECLARE SECTION;

/* initializing the vector state */
  initstate(semilla++, estados,32);/*25*/

 
  for (i = 1; i <= w; i++){ /*for each warehouse */
      for (j = 1; j <= 10; j++){ /* for each district */
	for (z = 1; z <= NUM_CLIENT; z++){ /* for each customer */

		h_c_id=z;
		h_d_id=j;
		h_w_id=i;
		h_c_d_id=j;
		h_c_w_id=i;
		getfechahora(h_date);
		// debug.
		// printf("Fecha generada: ;;%s;;\n",h_date);
		// getchar();
		// end debug.

		h_amount=10;
		cad_alfa_num(12,24,h_data,estados,e_global);
		EXEC SQL INSERT 
	 	    INTO history ( h_c_id, h_c_d_id, h_c_w_id, h_d_id, h_w_id, h_date, h_amount, h_data)
			VALUES ( :h_c_id, :h_c_d_id, :h_c_w_id, :h_d_id, :h_w_id, :h_date, :h_amount, :h_data);
		/************ERROR HANDLING********/
		if (sqlca.sqlcode<0){

			fprintf(stderr, "ERROR INSERTING THE ROW.\n");
			fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
		} /* if */
		
		EXEC SQL COMMIT; 

		/************ERROR HANDLING********/
		if (sqlca.sqlcode<0){

			fprintf(stderr, "ERROR INSERTING THE ROW.\n");
			fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
		} /* if */

	} /* for */
		getfechahora(hora);
		fprintf(stdout, "%s> Created. Warehouse %d, District %d\n", hora, i, j);
     } /* for */
  } /* for */
}/* poblar_history*/

void poblar_order(int w){
/* ----------------------------------------------- *\
|* poblar_order function.                          *|
|* ----------------------------------------------- *|
|* <w> number of warehouses in the database.       *|
|* ----------------------------------------------- *|
|* Inserts NUM_ORDER*w*10 rows in the orderr table *|
|* and in the order-line table a mean of 5 rows    *|
|* for each row inserted in the orderr table.      *|
\* ----------------------------------------------- */

#define E_O_OL_CNT 0      /* Constant for accesing the state vector of field o_ol_cnt */
#define E_O_CARRIER_ID 1  /* Constant for accesing the state vector of field o_carrier_id */
#define E_OL_I_ID 2       /* Constant for accesing the state vector of field o_i_id */
#define E_OL_AMOUNT 3     /* Constant for accesing the state vector of field ol_amount */

int i,j,z;
int permut[NUM_ORDER]; 
int q;
char estados[4][32];
char hora[21];

  EXEC SQL BEGIN DECLARE SECTION;
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
  EXEC SQL END DECLARE SECTION;

  
/* Inserting in the vector positions a permutation of natural */
/* numbers from 1 to NUM_ORDER                                */

  permutacion_int(permut, NUM_ORDER,e_global);

/* Initializing state vectors */
  initstate(semilla++, estados[E_O_OL_CNT],32);
  initstate(semilla++, estados[E_O_CARRIER_ID],32);
  initstate(semilla++, estados[E_OL_I_ID],32);
  initstate(semilla++, estados[E_OL_AMOUNT],32);

  for (i = 1; i <= w; i++){ /*for each warehouse*/
      for (j = 1; j <= 10; j++){ /*for each district */
	for (z = 1; z <= NUM_ORDER; z++){ /* NUM_ORDER orders */
		o_id=z; /*order number*/
		o_c_id=permut[z-1]; /* Customer selected from permutation */
		o_d_id=j;
		o_w_id=i;
		getfechahora(o_entry_d);
		setstate(estados[E_O_OL_CNT]);
		o_ol_cnt=(int)aleat_int(5,15);
  
		o_all_local=1;
		if (z<N_OR_UNDELIVER)
		{	setstate(estados[E_O_CARRIER_ID]);
			o_carrier_id=(int)aleat_int(1,10);
			EXEC SQL INSERT 
	 		    INTO orderr (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_carrier_id, o_ol_cnt, o_all_local)
				VALUES (:o_id, :o_d_id, :o_w_id, :o_c_id, :o_entry_d, :o_carrier_id, :o_ol_cnt, :o_all_local);
			EXEC SQL COMMIT; 
			/************ERROR HANDLING********/
			if (sqlca.sqlcode<0){
			fprintf(stderr, "ERROR INSERTING THE ROW IN THE ORDERR TABLE\n");
			fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
			} /* if */
		}else
		{	/* inserting 0 in o_carrier_id*/
			EXEC SQL INSERT 
	 		    INTO orderr (o_id, o_d_id, o_w_id, o_c_id, o_entry_d,o_carrier_id , o_ol_cnt, o_all_local)
				VALUES (:o_id, :o_d_id, :o_w_id, :o_c_id, :o_entry_d, 0, :o_ol_cnt, :o_all_local);
			EXEC SQL COMMIT; 
			/************ERROR HANDLING********/
			if (sqlca.sqlcode<0){

			fprintf(stderr, "ERROR INSERTING THER ROW IN THE ORDERR TABLE\n");
			fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
			} /* if */
		}/* else*/

		/*INSERTING OL_CNT ROWS IN THE ORDER_LINE TABLE FOR EACH ROW IN THE ORDERR TABLE*/

		for (q=1;q<=o_ol_cnt;q++){
			ol_o_id=z;
			ol_d_id=j;
			ol_w_id=i;
			ol_number=q;
			ol_quantity=5;
			cad_alfa_num(24,24,ol_dist_info,e_global,e_global);
			setstate(estados[E_OL_I_ID]);
			ol_i_id = aleat_int(1, NUM_ART);

			if (z<N_OR_UNDELIVER){	
				ol_amount=0;
				EXEC SQL INSERT 
	 			    INTO order_line (ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, ol_supply_w_id, 
									ol_delivery_d, ol_quantity, ol_amount, ol_dist_info)
					VALUES (:ol_o_id, :ol_d_id, :ol_w_id, :ol_number, :ol_i_id, :ol_w_id, 
							:o_entry_d, :ol_quantity, :ol_amount, :ol_dist_info);
				EXEC SQL COMMIT; 
					/*************ERROR HANDLING********/
				if (sqlca.sqlcode<0){
				fprintf(stderr, "ERROR INSERTING THE ROW IN THE ORDER_LINE TABLE\n");
				fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
				fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
			} /* if */
			}else{
				setstate(estados[E_OL_AMOUNT]);
				ol_amount=aleat_dbl(0.01,9999.99);
				EXEC SQL INSERT 
	 			    INTO order_line (ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, ol_supply_w_id, 
							ol_quantity, ol_amount, ol_dist_info)
					VALUES (:ol_o_id, :ol_d_id, :ol_w_id, :ol_number, :ol_i_id, :ol_w_id, 
								:ol_quantity, :ol_amount, :ol_dist_info);
				EXEC SQL COMMIT; 
				/************ERROR HANDLING********/
				if (sqlca.sqlcode<0){
	
				fprintf(stderr, "ERROR INSERTING THE ROW IN THE ORDER_LINE TABLE\n");
				fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
				fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
				fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
				} /* if */
			}/*de else*/
		}/*de for de order_line q*/
	} /* for de z */
		getfechahora(hora);
		fprintf(stdout, "%s> Created orders of Warehouse %d, District %d\n", hora, i, j);
     } /* for D*/
  } /* for w*/
}/*poblar_order*/

void poblar_new_order(int w){
/* ----------------------------------------------- *\
|* poblar_new_order function.                      *|
|* ----------------------------------------------- *|
|* <w> number of warehouses in the database.       *|
|* ----------------------------------------------- *|
|* inserts in the new-order table                  *|
|* NUM_ORDER-N_OR_UNDELIVER+1 rows with data       *|
|* defined by the TPC-C standard.                  *| 
\* ----------------------------------------------- */

int i,j,z;
char hora[21];    /* System date string */

  EXEC SQL BEGIN DECLARE SECTION;
	int no_o_id;
	int no_d_id;
	int no_w_id;
  EXEC SQL END DECLARE SECTION;


  for (i = 1; i <= w; i++){ /*for each warehouse*/
      for (j = 1; j <= 10; j++){ /*for each district*/
	/*Inserting NUM_ORDER-N_OR_UNDELIVER+1 rows in the table*/
	for (z = N_OR_UNDELIVER; z <= NUM_ORDER; z++){ 
		no_o_id=z;
		no_d_id=j;
		no_w_id=i;
		EXEC SQL INSERT 
	 		  INTO new_order (no_o_id, no_d_id, no_w_id)
				VALUES (:no_o_id, :no_d_id, :no_w_id);
		EXEC SQL COMMIT; 
		/************ERROR HANDLING********/
		if (sqlca.sqlcode<0){
			fprintf(stderr, "ERROR INSERTING THE ROW IN THE ORDERR TABLE\n");
			fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
			fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
			fprintf(stderr, "ITEM %d, WAREHOUSE %d\n", j+1, i);
		} /* if */
	} /* for de z */
		getfechahora(hora);
		fprintf(stdout, "%s> New-orders created. Warehouse %d, District %d\n", hora, i, j);
     } /* for de D*/
  } /* for de w*/
}/*de new_order*/

void creartablas(){  
/* ----------------------------------------------- *\
|* creartablas function.                           *|
|* ----------------------------------------------- *|
|* Creates the relations in the database.          *|
|* Does not populate the tables, but also define   *|
|* the fields and constraints.                     *|
\* ----------------------------------------------- */

   /*********************WAREHOUSE TABLE**********************/
   EXEC SQL CREATE TABLE warehouse (
	w_id int4,
	w_name varchar(10),
	w_street_1 varchar(20),
	w_street_2 varchar(20),
	w_city varchar(20),
	w_state char(2),
	w_zip char(9),
	w_tax float4,
	w_ytd float8,
	CONSTRAINT wareh1 PRIMARY KEY (w_id)
   );
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING TABLE WAREHOUSE\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   /*************************ITEM TABLE************************/
   EXEC SQL CREATE TABLE item (
	i_id int4,
	i_im_id int4,
	i_name varchar(24),
	i_price float8,
	i_data varchar(50),
	CONSTRAINT item1 PRIMARY KEY (i_id)
   );

   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE ITEM\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   EXEC SQL COMMIT; 
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING TABLE ITEM\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   /*********************STOCK TABLE**************************/
   EXEC SQL CREATE TABLE stock (
	s_i_id int4,
	s_w_id int4,
	s_quantity int2,
	s_dist_01 char(24),
	s_dist_02 char(24),
	s_dist_03 char(24),
	s_dist_04 char(24),
	s_dist_05 char(24),
	s_dist_06 char(24),
	s_dist_07 char(24),
	s_dist_08 char(24),
	s_dist_09 char(24),
	s_dist_10 char(24),
	s_ytd numeric(8,2),
	s_order_cnt int2,
	s_remote_cnt int2,
	s_data varchar(50),
	CONSTRAINT stock1 PRIMARY KEY (s_w_id, s_i_id),
	CONSTRAINT stock2 FOREIGN KEY (s_w_id) REFERENCES warehouse (w_id) DEFERRABLE,
	CONSTRAINT stock3 FOREIGN KEY (s_i_id) REFERENCES item (i_id) DEFERRABLE
   );
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE STOCK\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   EXEC SQL COMMIT; 
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING TABLE STOCK\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   /************************DISTRICT TABLE*********************/
   EXEC SQL CREATE TABLE district (
	d_id int4,
	d_w_id int4,
	d_name varchar(10),
	d_street_1 varchar(20),
	d_street_2 varchar(20),
	d_city varchar(20),
	d_state char(2),
	d_zip char(9),
	d_tax float4,
	d_ytd float8,
	d_next_o_id int4,
	CONSTRAINT dist1 PRIMARY KEY (d_w_id,d_id),
	CONSTRAINT dist2 FOREIGN KEY (d_w_id) REFERENCES warehouse (w_id) DEFERRABLE

   );
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE DISTRICT\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   EXEC SQL COMMIT; 

   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE DISTRICT\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   /**************CUSTOMER TABLE********/
   EXEC SQL CREATE TABLE customer (
	c_id int4,
	c_d_id int4,
	c_w_id int4,
	c_first varchar(16),
	c_middle char(2),
	c_last varchar(16),
	c_street_1 varchar(20),
	c_street_2 varchar(20),
	c_city varchar(20),
	c_state char(2),
	c_zip char(9),
	c_phone char(16),
	c_since timestamp DEFAULT '1970-01-01',
	c_credit char(2),
	c_credit_lim float8,
	c_discount float4,
	c_balance float8,
	c_ytd_payment float8,
	c_payment_cnt int2,
	c_delivery_cnt int2,
	c_data varchar(500),
	CONSTRAINT custom1 PRIMARY KEY (c_w_id, c_d_id, c_id),
	CONSTRAINT custom2 FOREIGN KEY (c_w_id, c_d_id) REFERENCES district (d_w_id, d_id) DEFERRABLE
   );
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE CUSTOMER\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   EXEC SQL COMMIT; 

   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING TABLE CUSTOMER\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   /**************HISTORY TABLE********/
   EXEC SQL CREATE TABLE history (
	h_c_id int4,
	h_c_d_id int4,
	h_c_w_id int4,
	h_d_id int4,
	h_w_id int4,
	h_date timestamp DEFAULT '1970-01-01', 
	h_amount float4,
	h_data varchar(24),
	CONSTRAINT hist1 FOREIGN KEY (h_c_w_id, h_c_d_id, h_c_id) REFERENCES customer (c_w_id, c_d_id, c_id) DEFERRABLE,
	CONSTRAINT hist2 FOREIGN KEY (h_w_id, h_d_id) REFERENCES district (d_w_id, d_id) DEFERRABLE
   );

   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING TABLE HISTORY\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
    } 

   EXEC SQL COMMIT; 

   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING TABLE HISTORY\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
    } 

  /***********************ORDERR TABLE*******************/
  EXEC SQL CREATE TABLE orderr (
	o_id int4,
	o_w_id int4,
	o_d_id int4,
	o_c_id int4,
	o_entry_d timestamp DEFAULT '1970-01-01',
	o_carrier_id int2 DEFAULT 0,
	o_ol_cnt int2,
	o_all_local int2,
	CONSTRAINT orderr1 PRIMARY KEY (o_w_id, o_d_id, o_id),
	CONSTRAINT orderr2 FOREIGN KEY (o_w_id, o_d_id, o_c_id) REFERENCES customer (c_w_id, c_d_id, c_id) DEFERRABLE 
   ); 
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE ORDERR\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

   EXEC SQL COMMIT; 
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE ORDERR\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

  /****************ORDER_LINE TABLE********************/
  EXEC SQL CREATE TABLE order_line (
	ol_o_id int4,
	ol_d_id int4,
	ol_w_id int4,
	ol_number int2,
	ol_i_id int4,
	ol_supply_w_id int4,
	ol_delivery_d timestamp DEFAULT '1970-01-01',
	ol_quantity int2,
	ol_amount numeric(6,2),
	ol_dist_info char(24),
	CONSTRAINT ol1 PRIMARY KEY (ol_w_id, ol_d_id, ol_o_id, ol_number),
	CONSTRAINT ol2 FOREIGN KEY (ol_w_id, ol_d_id, ol_o_id) REFERENCES orderr (o_w_id, o_d_id, o_id) DEFERRABLE,
	CONSTRAINT ol3 FOREIGN KEY (ol_supply_w_id, ol_i_id) REFERENCES stock (s_w_id, s_i_id) DEFERRABLE
  ); /* exec*/
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE DISTRICT\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

  EXEC SQL COMMIT;

   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE ORDER-LINE\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

  /******************NEW_ORDER TABLE*******************/
  EXEC SQL CREATE TABLE new_order (
	no_o_id int4,
	no_d_id int4,
	no_w_id int4,
	CONSTRAINT no1 PRIMARY KEY (no_w_id, no_d_id, no_o_id),
	CONSTRAINT no2 FOREIGN KEY (no_w_id, no_d_id, no_o_id) REFERENCES orderr (o_w_id, o_d_id, o_id) DEFERRABLE
  );
   /**************ERROR HANDLING********/
   if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE DISTRICT\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }

  EXEC SQL COMMIT; 

   /**************ERROR HANDLING********/
  if (sqlca.sqlcode<0){
	fprintf(stderr, "ERROR CREATING TABLE NEW-ORDER\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
   }
} /* creartablas*/

int carga (int w){
/* ----------------------------------------------- *\
|* carga function.                                 *|
|* ----------------------------------------------- *|
|* populates each table in the database and shows  *|
|* messages about the tasks performed. The total   *|
|* number of warehouses to be populated is         *|
|* determined by the global variable w.            *|
|* ----------------------------------------------- *|
|* Funtions required:                              *|
|*	poblar_warehouse()                         *|
|*	poblar_item()                              *|
|*	poblar_district()                          *|
|*	poblar_customer()                          *|
|*	poblar_stock()                             *|
|*	poblar_history()                           *|
|*	poblar_order()                             *|
|*	poblar_new_order()                         *|
\* ----------------------------------------------- */

char comienzo[21],final[21];    /*  Start and end date strings */
FILE *fcons;                    /*  Pointer to constants file */

/************* Conecting to template1 **************/
/* This database is created by default by postgreSQL */
EXEC SQL CONNECT TO template1;/* USER USERNAME;*/
if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CONNECTING TO template1\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	return -1;
}
EXEC SQL SET autocommit = ON; 

/************** CREATING THE DATABASE *********/
EXEC SQL CREATE DATABASE tpcc;
if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CREATING THE DATABASE template1\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	return -1;
}
/************** DISCONECTING from template1 ********/
EXEC SQL DISCONNECT;

/************* CONNECTING to tpcc ***************/
EXEC SQL CONNECT TO tpcc;/* USER USERNAME;*/

/**************ERROR HANDLING********/
if (sqlca.sqlcode<0){

	fprintf(stderr, "ERROR CONNECTING TO DATABASE tpcc\n");
	fprintf(stderr, "ERROR TYPE: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
}

/****************POPULATING THE TABLES***********************/
semilla = SEMILLACARGA; /* Seed for creating the state vectors */

/* Initializing the global state vector */
initstate(semilla++,e_global,32);

/* Opening the random constants file */
strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"cons.dat");
if ((fcons = fopen(filenameBuffer, "w")) == NULL){
	fprintf(stdout, "Error opening file cons.dat\n");
	fprintf(stdout, "Aborting population ...\n");
	getchar();
	return -1;
}

/* Creating and writing the random constants */
setstate(e_global);
C_C_LAST = aleat_int (0,255); /*C_LOAD: Load CONSTANT FOR C_LAST*/
C_C_ID = aleat_int (1, NUM_CLIENT);
C_OL_I_ID = aleat_int (1, NUM_ART);
	fprintf(fcons, "%d %d %d\n", C_C_LAST, C_C_ID, C_OL_I_ID);
/* Calculating C_C_LAST_RUN */
C_C_LAST_RUN = aleat_int (67, 119);
	if (C_C_LAST_RUN == 96) C_C_LAST_RUN = 65;
	else{
		if (C_C_LAST_RUN == 112) 
			C_C_LAST_RUN = 66;
	 }
C_C_LAST_RUN = labs(C_C_LAST - C_C_LAST_RUN);
	fprintf(fcons, "%d %d %d\n", C_C_LAST_RUN, C_C_ID, C_OL_I_ID);

fclose(fcons);

/* Populating the database */
	fprintf(stdout, "Starting to populate database.\n");
	getfechahora(comienzo);
	fprintf(stdout, "Start time: %s\n",comienzo);
	fprintf(stdout, "POPULATING DATABASE WITH %d WAREHOUSES...\n\n",w);
	creartablas();

	fprintf(stdout, "POPULATING TABLE WAREHOUSE ...\n");
	poblar_warehouse(w);
	fprintf(stdout, "TABLE WAREHOUSE CREATED.\n");

	fprintf(stdout, "POPULATING TABLE ITEM ...\n");
	poblar_item();
	fprintf(stdout, "TABLE ITEM CREATED.\n");

	fprintf(stdout, "POPULATING TABLE DISTRICT ...\n");
	poblar_district(w);
	fprintf(stdout, "TABLE DISTRICT CREATED.\n");

	fprintf(stdout, "POPULATING TABLE CUSTOMER ...\n");
	poblar_customer(w); 
	fprintf(stdout, "TABLE CUSTOMER CREATED.\n");

	fprintf(stdout, "POPULATING TABLE STOCK ...\n");
	poblar_stock(w); 
	fprintf(stdout, "TABLE STOCK CREATED.\n");

	fprintf(stdout, "POPULATING TABLE HISTORY ...\n");
       	poblar_history(w); 
	fprintf(stdout, "TABLE HISTORY CREATED.\n");

	fprintf(stdout, "POPULATING TABLE ORDER ...\n");
	poblar_order(w);
	fprintf(stdout, "TABLE ORDER AND ORDER_LINE CREATED.\n");

	fprintf(stdout, "POPULATING TABLE NEW-ORDER ...\n");
	poblar_new_order(w);
	fprintf(stdout, "TABLE NEW_ORDER CREATED.\n");

	getfechahora(final);
	fprintf(stdout, "\nPOPULATION COMPLETED.\n");
	fprintf(stdout, "Start time: %s\n",comienzo);
	fprintf(stdout, "End time: %s\n\n",final); 
/***************DISCONECTING FROM DATABASE*********************/
EXEC SQL DISCONNECT;
return 1;
}/*carga*/

int menu(int haybd, int haylogs){
/* ----------------------------------------------- *\
|* menu function.                                  *|
|* ----------------------------------------------- *|
|* <haydb> if has value 1 indicates that the       *|
|* database exists. If has value 0  the database   *|
|* not exists.                                     *|
|* <haylogs> if has value 1 indicates that the     *|
|* log files has been created. If has value 0 they *|
|* has not been created.                           *|
|* ----------------------------------------------- *|
|* Shows on stdout a menu screen with the allowed  *|
|* options depending of the <haydb> and <haylogs>  *|
|* values. In addition, gets from the user the     *|
|* selected option and returns its identificator.  *|
\* ----------------------------------------------- */

int d;
	fprintf(stdout, "\n\n\n\n    +---------------------------------------------------+\n");
	fprintf(stdout, "    | BENCHMARK TPCC  --- UNIVERSIDAD DE VALLADOLID --- |\n");
	fprintf(stdout, "    +---------------------------------------------------+\n\n\n\n\n");
	if (haybd) {		
		fprintf(stdout, "	2.- RESTORE EXISTING DATABASE.\n");
		fprintf(stdout, "	3.- RUN THE TEST.\n");
		fprintf(stdout, "	4.- CHECK DATABASE CONSISTENCY.\n");
		fprintf(stdout, "	5.- DELETE DATABASE.\n");		
		if (haylogs)	
			fprintf(stdout, "	6.- SHOW AND STORE TEST RESULTS\n");
		else
			fprintf(stdout,"\n");
		fprintf(stdout, "	7.- CHECK DATABASE STATE.\n");
		
	} else{
		fprintf(stdout, "	1.- CREATE A NEW TEST DATABASE\n");
		fprintf(stdout,"\n\n\n\n");
		if (haylogs)	
			fprintf(stdout, "	6.- SHOW AND STORE TEST RESULTS\n");
		else
			fprintf(stdout,"\n");
	}
	
	fprintf(stdout,"\n\n	8.- Quit\n\n\n\n\n\n");
	while (1){
		fprintf(stdout, "SELECT OPTION: ");
		fscanf(stdin, "%d", &d);
		getchar();
		/* Switch input */
		switch(d){
			case 1: if (!haybd)
					return 1;
				break;
			case 2: if (haybd)
					return 2;
				break;
			case 3: if (haybd)
					return 3;
				break;
			case 4: if (haybd)
					return 4;
				break;
			case 5: if (haybd)
					return 5;
				break;
			case 6: if (haylogs)
					return 6;
				break;
			case 7:if (haybd)
					return 7;
				break;
			case 8: return 8;
		} /*switch*/
	}
}

void zero(char *v, int tam){
/* ----------------------------------------------- *\
|* zero function.                                  *|
|* ----------------------------------------------- *|
|* Forces to "0" value <tam> bytes from the        *|
|* address pointed by <*v>.                        *|
\* ----------------------------------------------- */
int i;
for(i=0;i<tam;i++) v[i]=0;
}

int lectura_bitacora(char op, char filename[]){
/* ----------------------------------------------- *\
|* lectura_bitacora function.                      *|
|* ----------------------------------------------- *|
|* No parameters are needed.                       *|
|* ----------------------------------------------- *|
|*  - Performs the results accounting using the    *|
|*  log files wrote by the Transaction Monitor     *|  
|*  and the Remote Terminal Emulators.             *|
|*  - Shows the results on stdout.                 *|
|*  - Write a file containing the performance      *|
|*  results. The name of the file is selected by   *|
|*  user.                                          *|
|*  - Generates the data files for plotting the    *|
|*  graphs required by the TPC-C standard. The     *|
|*  names of these files are:  'g1New\_Order.dat', *|
|*  'g1Delivery.dat', 'g1Payment.dat',             *|
|*  'g1OrderStatus.dat', 'g1StockLevel.dat',       *|
|*  'g3.dat' y 'g4.dat'.                           *|
|*  - In addition, generates the files             *|
|*  'vacuum.dat' and 'check.dat' that contain      *|
|*  information about the checkpoints and vacuums  *|
|*  performed during the test.                     *|
|* ----------------------------------------------- *|
|*  Returns 1 if all operations have been performed *|
|*  correctly, and 0 if it was not able to read    *|
|*  the log files.                                 *|
\* ----------------------------------------------- */

int d;
int alm;

/* auxiliar file name strings */
char n_fichero[50];
char nombre[100];

/* File pointers */
FILE *fich;
FILE *aux;
FILE *med;
FILE *g3;
FILE *g4;
FILE *g1_0;
FILE *g1_1;
FILE *g1_2;
FILE *g1_3;
FILE *g1_4;

char fecha[30]; /* System date string */
char dec;
int w, term;
int dist_saltado;
int l1,l2,l3,l4;
int cuenta,seg1, seg2, seg_encl, d_seg_encl,d_seg_fin, total_tran= 0;
int tipo, mseg1,mseg2, mseg_encl,d_mseg_encl,d_mseg_fin, tp; 
int d_w_id, d_d_id,resul;
int cont1, cont2;
float tr,porcen,porcen2,porcen_90th;
float t_med,d_tr,intg3,intg1_0,intg1_1,intg1_2,intg1_3,intg1_4;
struct timeb sellohora1;  /* auxiliar   */
struct timeb sellohora2;  /*            */
struct timeb sello_leido; /* timestamps */
struct timeb inicio;     /* Test start timestamp */
struct timeb com_med;    /* Measurement Interval start timestamp*/
struct timeb fin_med;    /* Measurement Interval end timestamp*/
struct timeb fin_test; /*Test end timestamp*/ 

int temp_inicio_time;    /* Test start timestamp*/
int temp_inicio_milisecs;    /* Test start timestamp*/
int temp_com_med_time;    /* Measurement Interval start timestamp*/
int temp_com_med_milisecs;    /* Measurement Interval start timestamp*/
int temp_fin_med_time;    /* Measurement Interval end timestamp*/
int temp_fin_med_milisecs;    /* Measurement Interval end timestamp*/
int temp_fin_test_time; /*Test end timestamp*/ 
int temp_fin_test_milisecs; /*Test end timestamp*/ 

float rendimiento;
double int_test;
int int_vacuum, num_vacuum;
double durac;

/* Vector of each transactions data */
struct tdat{
	int cnt; 	/* Number of transactions performed in the measurement interval */
	int cnt_total; /* Total number of transactions performed */
	int cnt_90th;  /* Total number of transactions completed with a response time less than the 90th percentile specified */
	double sum_tr;  /* Sum of response times (queue acknowledge for deliverys) of each transaction */
	float max_tr;   /* Maximun response time (queue acknowledge for deliverys)*/
	float min_tr;   /* Minimun response time (queue acknowledge for deliverys)*/
	float med_tr;   /* Average response time (queue acknowledge for deliverys)*/
	double sum_tp;  /* Sum of think times of each transaction */
	float max_tp;   /* Maximun think time */
	float min_tp;   /* Minimun think time */
	float med_tp;	/* Average think time */
	double sum_tr_ej;  /* Sum of queue adnoledge for deliverys */
	float max_tr_ej;   /* Maximun execution time for deliverys */
	float min_tr_ej;   /* Minimun execution time for deliverys */
	float med_tr_ej;   /* Average execution time for deliverys y*/  
	int cnt_90th_ej;  /* Total number of transactions completed with an execution time less than the 90th specified */
	float tr90th;	   /* 90th percentile response time */
	int roll_cnt;	   /*counter of rollback transactions*/
	int sum_ol_cnt; /*counter of total number of items in new-order transactions*/
	int art_remoto; /*counter of total number of remote items*/
	int sum_c_id;  /*counter of cunstomer selections by c_id*/
	int paym_remota; /*counter of remote payment transactions*/
	} dat[5];

/* Vector for storing the data of the four intest checkpoints and vacuums */
struct tsello{
	char fecha[30];
	double transcurrido;
	double duracion;
} vacuum[4], check[4];

/* Struct of graphs auxiliar files */
struct punto{
		float x;
		float y;	
	} pto;

/* Initializing data */
for (cuenta = 0; cuenta < 5; cuenta++)
	zero((char *)&dat[cuenta],sizeof(dat[cuenta]));

/*Opening medida.log for obtaining the test information */
strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"medida.log");
if ((med = fopen(filenameBuffer, "r")) == NULL){
	fprintf(stderr, "ERROR OPENING THE TEST INFORMATION FILE.\n");
	return 0;
}

/******Obtainint the test parameters*******/
fscanf(med,"TEST PERFORMED ON %19c .\n", fecha);
if (feof(med)) {
	fprintf(stdout,"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n->Log file medida.log does not contain information.\n");
	fprintf(stdout,"  Computation of performance results has failed.\n\n\n\n\nContinue...");
	
	getchar();		
	fclose(med);
	return 0;
}

fscanf(med,"Number of warehouses: %d .\n",&w);
fscanf(med,"Number of terminals for each warehouse: %d .\n",&term);
fscanf(med,"Measurement interval: %d minutes.\n",&int_vacuum); /* Se utiliza int_vacuum */
fscanf(med,"Ramp-up interval: %d minutes.\n",&int_vacuum); /* para leer unicamente  */
fscanf(med,"Interval between vacuums: %d.\n", &int_vacuum);
fscanf(med,"Maximum number of vacuums: %d .\n", &num_vacuum);

fscanf(med,"Test start timestamp: %d %d .\n",&temp_inicio_time, &temp_inicio_milisecs);
inicio.time = (time_t) temp_inicio_time;
inicio.millitm = (unsigned short) temp_inicio_milisecs;

fscanf(med,"Measurement interval start timestamp: %d %d .\n",&temp_com_med_time, &temp_com_med_milisecs);
com_med.time = (time_t) temp_com_med_time;
com_med.millitm = (unsigned short) temp_com_med_milisecs;

fscanf(med,"Measurement interval end timestamp: %d %d .\n",&temp_fin_med_time, &temp_fin_med_milisecs);
fin_med.time = (time_t) temp_fin_med_time;
fin_med.millitm = (unsigned short) temp_fin_med_milisecs;

fscanf(med,"Test end timestamp: %d %d .\n",&temp_fin_test_time, &temp_fin_test_milisecs);
fin_test.time = (time_t) temp_fin_test_time;
fin_test.millitm = (unsigned short) temp_fin_test_milisecs;

fclose(med); 
/********************************************/

fprintf(stdout,"Computation of performance test done on %.10s at %.8s for %d warehouses.\n\n",fecha,&fecha[11], w);
fprintf(stdout,"Start of measurement interval: %lf m\n", resta_tiempos(&inicio,&com_med)/60);
fprintf(stdout,"\nEnd of measurement interval: %lf m\n\n", resta_tiempos(&inicio,&fin_med)/60);


/******************* FIRST FILE PARSING  ************************/
/******     Obtaining response times and think times     ********/
/** First step to plot graph 1: computation of 90th percentile **/
/**      Generating graph 4: performance/elapsed time.         **/
/****************************************************************/


/* Opening and initializing auxiliar files for the graphs 1 and 4 */
	if ((g1_0 = fopen("g1_0.aux", "w+")) == NULL){
		fprintf(stderr, "ERROR OPENING AUXILIAR FILE g1_0\n");
		return(0);
	}

	if ((g1_1 = fopen("g1_1.aux", "w+")) == NULL){
		fprintf(stderr, "ERROR OPENING AUXILIAR FILE g1_1\n");
		return(0);
	}

	if ((g1_2 = fopen("g1_2.aux", "w+")) == NULL){
		fprintf(stderr, "ERROR OPENING AUXILIAR FILE g1_2\n");
		return(0);
	}

	if ((g1_3 = fopen("g1_3.aux", "w+")) == NULL){
		fprintf(stderr, "ERROR OPENING AUXILIAR FILE g1_3\n");
		return(0);
	}

	if ((g1_4 = fopen("g1_4.aux", "w+")) == NULL){
		fprintf(stderr, "ERROR OPENING AUXILIAR FILE g1_4\n");
		return(0);
	}

	if ((g4 = fopen("g4.aux", "w+")) == NULL){
		fprintf(stderr, "ERROR OPENING AUXILIAR FILE g4\n");
		return(0);
	}

	/* Calculating number of intervals and initializing files */

	/* file g1_0 */
	intg1_0 = ((double)4*TR90thNO)/NUM_INT_TR;
	for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){ /* intervals of INT_REND seconds */
		pto.x = cuenta*(intg1_0);
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g1_0);
	}

	/* file g1_1 */
	intg1_1 = ((double)4*TR90thP)/NUM_INT_TR;
	for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
		pto.x = cuenta*(intg1_1);
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g1_1);
	}

	/* file g1_2 */
	intg1_2 = ((double)4*TR90thOS)/NUM_INT_TR;
	for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
		pto.x = cuenta*(intg1_2);
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g1_2);
	}

	/* file g1_3 */
	intg1_3 = ((double)4*TR90thD)/NUM_INT_TR;
	for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
		pto.x = cuenta*(intg1_3);
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g1_3);
	}

	/* file g1_4 */
	intg1_4 = ((double)4*TR90thSL)/NUM_INT_TR;
	for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
		pto.x = cuenta*(intg1_4);
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g1_4);
	}

	/* file g4 */
	int_test=resta_tiempos(&inicio,&fin_test);
	for (cuenta = 0; cuenta < (int) int_test; cuenta = cuenta+INT_REND){ /* intervals of INT_REND seconds */
		pto.x = cuenta;
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g4);
	}

/* Initializing fields to maximum value */

	dat[0].min_tr = dat[1].min_tr = dat[2].min_tr =	dat[3].min_tr = dat[4].min_tr = MAXFLOAT;
	dat[0].min_tp = dat[1].min_tp = dat[2].min_tp =	dat[3].min_tp = dat[4].min_tp = MAXFLOAT;
	dat[3].min_tr_ej = MAXFLOAT;


  cuenta =0;

  for(alm=1;alm<=w;alm++){ /*for each warehouse */
	for(d=1;d<=term;d++){   /*for each terminal */

		/*getting terminal log file name*/

strcpy(filenameBuffer,VARDIR);
sprintf(n_fichero, "F_%d_%d.log\0", alm, d);
strcat(filenameBuffer,n_fichero);

		/*opening log file */
		if ((fich=fopen(filenameBuffer,"r")) == NULL){
			fprintf(stdout, "Error al abrir el fichero: %s\n", filenameBuffer);
			fprintf(stdout, "%s\n", strerror(errno));
		} else {

		/*read one line from file*/
		fscanf(fich,"\n*%d> %d %d %d %d %f %d %d %d %d %d %d",&tipo, &seg1,&mseg1, &seg_encl, &mseg_encl, &tr,&tp,&resul,&cont1,&cont2 ,&seg2,&mseg2);

		sello_leido.time=seg2;
		sello_leido.millitm=mseg2;
		
		while (!feof(fich)){	
			/******   Select the transaction type  ******/
			switch (tipo){
				case NEW_ORDER:
				/* Total number of New-Order transactions*/
				dat[0].cnt_total++;

			 /* Checking if transactions has been commited into measurement interval*/
				if(resta_tiempos(&com_med,&sello_leido)>=0){ /* Test start */
				if(resta_tiempos(&fin_test,&sello_leido)<=0){ /* Test end */
					cuenta++; /*Updating total number of transactions*/

					/* Updating number of commited new-order transactions */
					dat[0].cnt++;

					/* Calculating response time */
					if (tr<TR90thNO) dat[0].cnt_90th++;
					dat[0].sum_tr=dat[0].sum_tr+tr;
					if (dat[0].max_tr<tr) dat[0].max_tr=tr;
					if (dat[0].min_tr>tr) dat[0].min_tr=tr;

					/* Calculating thinking time */
					dat[0].sum_tp=dat[0].sum_tp+tp;
					if (dat[0].max_tp<tp) dat[0].max_tp=tp;
					if (dat[0].min_tp>tp) dat[0].min_tp=tp;

					/* Checking if transaction has been rolled back */
					if (resul<0){
						dat[0].roll_cnt++;
					}

					/* Accounting total number of items and remote items */
					dat[0].sum_ol_cnt=dat[0].sum_ol_cnt+cont1; /*total number of items*/
					dat[0].art_remoto=dat[0].art_remoto+cont2; /*number of remote items*/

					/* graph 1 */

					if (tr < 4*TR90thNO){

						/* Search and reading interval */
						fseek(g1_0, sizeof(pto)*(int)double2int(tr/intg1_0), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_0);
						if (ferror(g1_0)){
							fprintf(stdout, "Error reading file g1_0\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
						/* Update y */
						pto.y++;
						fseek(g1_0, sizeof(pto)*(int)double2int(tr/intg1_0), SEEK_SET);

						/* Writing the new values of interval */
						fwrite(&pto, sizeof(pto), 1, g1_0);
						if (ferror(g1_0)){
							fprintf(stdout, "Error writing file g1_0\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
				}
				}

				/* For graph 4 all the transactions are considered */
				/* graph 4 */
				sellohora1.time = seg2;
				sellohora1.millitm = mseg2;
				if (resta_tiempos(&inicio,&sellohora1) <= int_test){
					fseek(g4, sizeof(pto)*(int)double2int(resta_tiempos(&inicio,&sellohora1)/INT_REND), SEEK_SET);
					fread(&pto, sizeof(pto), 1, g4);
					if (ferror(g4)){
						fprintf(stdout, "Error reading file g4\n");
						fprintf(stdout, "%s\n", strerror(errno));
					}
					pto.y++;
					fseek(g4, sizeof(pto)*(int)double2int(resta_tiempos(&inicio,&sellohora1)/INT_REND), SEEK_SET);
					fwrite(&pto, sizeof(pto), 1, g4);
					if (ferror(g4)){
						fprintf(stdout, "Error reading file g4\n");
						fprintf(stdout, "%s\n", strerror(errno));
					}
				}
				break;
				case PAYMENT:
				dat[1].cnt_total++;
					 /*Checking if transaction has been commited into measurement interval*/
				if(resta_tiempos(&com_med,&sello_leido)>=0){ /*start of test*/
				if(resta_tiempos(&fin_test,&sello_leido)<=0){/*end of test*/
					cuenta++; /*Updating total number of commited transactions */

					/* Updating number of payment transactions commited into measurement interval */
					dat[1].cnt++;

					/* Calculating Response Times */
					if (tr<TR90thP) dat[1].cnt_90th++;
					dat[1].sum_tr=dat[1].sum_tr+tr;
					if (dat[1].max_tr<tr) dat[1].max_tr=tr;
					if (dat[1].min_tr>tr) dat[1].min_tr=tr;

					/* Calculating think times */
					dat[1].sum_tp=dat[1].sum_tp+tp;
					if (dat[1].max_tp<tp) dat[1].max_tp=tp;
					if (dat[1].min_tp>tp) dat[1].min_tp=tp;

					/*Accounting number of remote payment transactions*/
					dat[1].paym_remota=dat[1].paym_remota+cont1;

					/*Accounting number of customers selected by C-ID*/
					dat[1].sum_c_id=dat[1].sum_c_id+cont2;

					/* graph 1 */

					if (tr < 4*TR90thP){
						fseek(g1_1, sizeof(pto)*(int)double2int(tr/intg1_1), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_1);
						if (ferror(g1_1)){
							fprintf(stdout, "Error reading file g1_1\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
						pto.y++;
						fseek(g1_1, sizeof(pto)*(int)double2int(tr/intg1_1), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_1);
						if (ferror(g1_1)){
							fprintf(stdout, "Error reading file g1_1\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
				}
				}
				break;
				case ORDER_STATUS:
				dat[2].cnt_total++;
					 /*Checking if transaction has been commited into measurement interval*/
				if(resta_tiempos(&com_med,&sello_leido)>=0){ /*start of test */
				if(resta_tiempos(&fin_test,&sello_leido)<=0){/*end of test*/
					cuenta++; /*Updating total number of transactions commited*/

					/* Updating number of Order-Status transactions commited into measurement interval*/
					dat[2].cnt++;

					/* Calculating response times */
					if (tr<TR90thOS) dat[2].cnt_90th++;
					dat[2].sum_tr=dat[2].sum_tr+tr;
					if (dat[2].max_tr<tr) dat[2].max_tr=tr;
					if (dat[2].min_tr>tr) dat[2].min_tr=tr;

					/* Calculating think times */
					dat[2].sum_tp=dat[2].sum_tp+tp;
					if (dat[2].max_tp<tp) dat[2].max_tp=tp;
					if (dat[2].min_tp>tp) dat[2].min_tp=tp;

					/* Accounting number of customers selected by C-ID */
					dat[2].sum_c_id=dat[2].sum_c_id+cont1;

					/* graph 1 */

					if (tr < 4*TR90thOS){
						fseek(g1_2, sizeof(pto)*(int)double2int(tr/intg1_2), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_2);
						if (ferror(g1_2)){
							fprintf(stdout, "Error reading file g1_2\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
						pto.y++;
						fseek(g1_2, sizeof(pto)*(int)double2int(tr/intg1_2), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_2);
						if (ferror(g1_2)){
							fprintf(stdout, "Error reading file g1_2\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
				}
				}
				break;
				case DELIVERY:
				dat[3].cnt_total++;
					 /*Checking if transaction has been commited into measurement interval*/
				if(resta_tiempos(&com_med,&sello_leido)>=0){ /*Start of test */
				if(resta_tiempos(&fin_test,&sello_leido)<=0){/*End of test */
					cuenta++; /*Total number of transactions commited into measurement interval */

					/* Updating number of delivery transactions commited into measurement interval */
					dat[3].cnt++;

					/* Response time (queue time)*/
					dat[3].sum_tr=dat[3].sum_tr+tr;
					if (dat[3].max_tr<tr) dat[3].max_tr=tr;
					if (dat[3].min_tr>tr) dat[3].min_tr=tr;
					if(tr<TR90thD) dat[3].cnt_90th++;

					/*Searching transaction response time data wrote by Transaction Monitor */
					/*Opening TM log file*/

strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"tm_delivery_tr.log");
					if ((aux=fopen(filenameBuffer,"r")) == NULL){
						fprintf(stdout, "Error:\n");
						fprintf(stdout, "\tIt was not possible to open file tm_delivery_tr.log\n");
						fprintf(stdout, "\nAborting process ...\n");
						fprintf(stdout, "\nContinue ...\n");
						getchar();
						return(0);
						}
					/*reading each line until get the line corresponding to current transaction */
                                        /* finding transaction data from current transaction terminal and matching the current transaction queue timestamp. */
					fscanf(aux,"\n %d %d %d %d %d %d",&d_w_id, &d_d_id,&d_seg_encl, &d_mseg_encl, &d_seg_fin, &d_mseg_fin);

					while(((d_w_id !=alm)||(d_d_id!=d)||(d_seg_encl!=seg_encl)||(d_mseg_encl!=mseg_encl))&&(!feof(aux))){
						fscanf(aux,"\n %d %d %d %d %d %d",&d_w_id, &d_d_id,&d_seg_encl, &d_mseg_encl, &d_seg_fin, &d_mseg_fin);
					}/*while*/

					if (feof(aux)){
						fprintf(stdout,"Error reading TM log file\n");
						fprintf(stdout,"Looking for: %d %d %d %d\n",alm, d, seg_encl, mseg_encl);
					} /* if */
					fclose(aux);

					/*copying recovered data into timestamp structs */
					/* start of transaction timestamp */
					sellohora1.time=(time_t)d_seg_encl;
					sellohora1.millitm=(unsigned short)d_mseg_encl;
					/* end of transaction timestamp */
					sellohora2.time=(time_t)d_seg_fin;
					sellohora2.millitm=(unsigned short)d_mseg_fin;

					/* Calculating execution time */
					d_tr=(float)resta_tiempos(&sellohora1,&sellohora2);
					if(d_tr<80) dat[3].cnt_90th_ej++;
					if (d_tr>dat[3].max_tr_ej) dat[3].max_tr_ej=d_tr;
					if (dat[3].min_tr_ej>d_tr) dat[3].min_tr_ej=d_tr;
					dat[3].sum_tr_ej=dat[3].sum_tr_ej+d_tr;

					/* Calculating thinking time */
					dat[3].sum_tp=dat[3].sum_tp+tp;
					if (dat[3].max_tp<tp) dat[3].max_tp=tp;
					if (dat[3].min_tp>tp) dat[3].min_tp=tp;

					/* graph 1 */

					if (tr < 4*TR90thD){
						fseek(g1_3, sizeof(pto)*(int)double2int(tr/intg1_3), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_3);
						if (ferror(g1_3)){
							fprintf(stdout, "Error reading file g1_3\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
						pto.y++;
						fseek(g1_3, sizeof(pto)*(int)double2int(tr/intg1_3), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_3);
						if (ferror(g1_3)){
							fprintf(stdout, "Error writing file g1_3\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
				}/* if */
				}/* if */
				break;

				case STOCK_LEVEL:
				dat[4].cnt_total++;
					 /*Checking if transaction has been commited into measurement interval*/
				if(resta_tiempos(&com_med,&sello_leido)>=0){
				if(resta_tiempos(&fin_test,&sello_leido)<=0){/*fin del test*/
					cuenta++; /*Updating total number of transactions commited into measurement interval*/

					/* Updating number of Stock-Level transactions commited into measurement interval */
					dat[4].cnt++;

					/* Calculating response times */
					if (tr<TR90thSL) dat[4].cnt_90th++;
					dat[4].sum_tr=dat[4].sum_tr+tr;
					if (dat[4].max_tr<tr) dat[4].max_tr=tr;
					if (dat[4].min_tr>tr) dat[4].min_tr=tr;

					/* Calculating think times */
					dat[4].sum_tp=dat[4].sum_tp+tp;
					if (dat[4].max_tp<tp) dat[4].max_tp=tp;
					if (dat[4].min_tp>tp) dat[4].min_tp=tp;

					/* graph 1 */
					if (tr < 4*TR90thSL){
						fseek(g1_4, sizeof(pto)*(int)double2int(tr/intg1_4), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_4);
						if (ferror(g1_4)){
							fprintf(stdout, "Error reading file g1_4\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
						pto.y++;
						fseek(g1_4, sizeof(pto)*(int)double2int(tr/intg1_4), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_4);
						if (ferror(g1_4)){
							fprintf(stdout, "Error writing file g1_4\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
				}/*if*/
				}/*if*/
				break;					
				default:
				break;
			}/* switch*/


			/* reading from file the next line*/
			fscanf(fich,"\n*%d> %d %d %d %d %f %d %d %d %d %d %d",&tipo, &seg1,&mseg1, &seg_encl, &mseg_encl, &tr,&tp,&resul,&cont1,&cont2,&seg2,&mseg2);
		sello_leido.time=seg2;
		sello_leido.millitm=mseg2;
		}/* while*/
		fclose(fich);				
		} /* if */
	}/* for terminals*/
  }/* for warehouses*/

	/* Total number of transactions commited into measurement interval */
	total_tran = cuenta;

	/***************************************************/
	/* Calculating 90th percentile response time.      */
	/* ----------------------------------------------- */
	/* Intervals of each file are read until the 90%   */
	/* of total number of transactions has been        */
	/* reached. Then the response time associated to   */
	/* interval are considered as the 90th percentile. */
	/***************************************************/

	/* New Order */
	rewind(g1_0);
	cuenta = 0;
	fread(&pto,sizeof(pto), 1, g1_0);
	while (!feof(g1_0)){
		if (ferror(g1_0)){
			fprintf(stdout, "Error in file g1_0\n");
			break;
		}
		cuenta = cuenta + (int)pto.y;

		/* Checking if 90% of total has been reached */
		if (((double)cuenta/dat[0].cnt) > 0.9){
			dat[0].tr90th = pto.x;
			break;
		}
		fread(&pto, sizeof(pto), 1, g1_0);
	}
	if (feof(g1_0)){
		/* 90th percentile response time is 4 times greater than the specified by TPC-C*/
		dat[0].tr90th = -1; /* Storing a non valid value */
	}
	fclose(g1_0);

	/* Payment */
	rewind(g1_1);
	cuenta = 0;
	fread(&pto,sizeof(pto), 1, g1_1);
	while (!feof(g1_1)){
		if (ferror(g1_1)){
			fprintf(stdout, "Error in file g1_1\n");
			break;
		}
		cuenta = cuenta + (int)pto.y;

		/* Checking if 90% of total has been reached */
		if (((double)cuenta/dat[1].cnt) > 0.9){
			dat[1].tr90th = pto.x;
			break;
		}
		fread(&pto, sizeof(pto), 1, g1_1);
	}
	if (feof(g1_1)){
		/* 90th percentile response time is 4 times greater than the specified by TPC-C*/
		dat[1].tr90th = -1; /* Storing a non valid value */
	}
	fclose(g1_1);

	/* Order Status */
	rewind(g1_2);
	cuenta = 0;
	fread(&pto,sizeof(pto), 1, g1_2);
	while (!feof(g1_2)){
		if (ferror(g1_2)){
			fprintf(stdout, "Error in file g1_2\n");
			break;
		}
		cuenta = cuenta + (int)pto.y;

		/* Checking if 90% of total has been reached */
		if (((double)cuenta/dat[2].cnt) > 0.9){
			dat[2].tr90th = pto.x;
			break;
		}
		fread(&pto, sizeof(pto), 1, g1_2);
	}
	if (feof(g1_2)){
		/* 90th percentile response time is 4 times greater than the specified by TPC-C*/
		dat[2].tr90th = -1; /* Storing a non valid value  */
	}
	fclose(g1_2);

	/* Delivery */
	rewind(g1_3);
	cuenta = 0;
	fread(&pto,sizeof(pto), 1, g1_3);
	while (!feof(g1_3)){
		if (ferror(g1_3)){
			fprintf(stdout, "Error in file g1_3\n");
			break;
		}
		cuenta = cuenta + (int)pto.y;

		/* Checking if 90% of total has been reached */
		if (((double)cuenta/dat[3].cnt) > 0.9){
			dat[3].tr90th = pto.x;
			break;
		}
		fread(&pto, sizeof(pto), 1, g1_3);
	}
	if (feof(g1_3)){
		/* 90th percentile response time is 4 times greater than the specified by TPC-C*/
		dat[3].tr90th = -1; /* Storing a non valid value */
	}
	fclose(g1_3);

	/* Stock Level */
	rewind(g1_4);
	cuenta = 0;
	fread(&pto,sizeof(pto), 1, g1_4);
	while (!feof(g1_4)){
		if (ferror(g1_4)){
			fprintf(stdout, "Error in file g1_4\n");
			break;
		}
		cuenta = cuenta + (int)pto.y;

		/* Checking if 90% of total has been reached */
		if (((double)cuenta/dat[4].cnt) > 0.9){
			dat[4].tr90th = pto.x;
			break;
		}
		fread(&pto, sizeof(pto), 1, g1_4);
	}
	if (feof(g1_4)){
		/* 90th percentile response time is 4 times greater than the specified by TPC-C*/
		dat[4].tr90th = -1; /* Storing a non valid value */
	}
	fclose(g1_4);
	

	/***************************************************/
	/**  ACOUNTING OF SKIPPED DELIVERY TRANSACTIONS   **/
	/***************************************************/

strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"tm_delivery_res.log");
	if ((aux=fopen(filenameBuffer,"r")) == NULL){
		fprintf(stdout, "Error:\n");
		fprintf(stdout, "\tIt was not possible to open file tm_delivery_res.log\n");
		fprintf(stdout, "\nAborting process ...\n");
		fprintf(stdout, "\nContinue ...\n");
		getchar();
		return(0);
	}
	dist_saltado=0;
	
/* ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ */
	fscanf(aux, "Inicio Transaccion. Fecha Encolado: %d %d, Almacen %d, Repartidor %d\n",&l1,&l2,&l3,&l4);
	while(!feof(aux)){
		fscanf(aux, "\t%c%d %d\n",&l2,&l3,&l4);
		if ((char)l2=='*') dist_saltado++;
		for(l1=2;(l1<=10)&&(!feof(aux));l1++){
			fscanf(aux, "\t%c%d %d\n",&l2,&l3,&l4);
			if ((char)l2=='*') dist_saltado++;
		}/*de for*/
		fscanf(aux, "Terminada a las: %d %d\n\n", &l1,&l2);
		fscanf(aux, "Inicio Transaccion. Fecha Encolado: %d %d, Almacen %d, Repartidor %d\n",&l1,&l2,&l3,&l4);
	}
	fclose(aux);
	

	/***************************************************/
	/**  CHECKPOINTS TIMESTAMPS AND EXECUTION TIME    **/
	/***************************************************/
	/* Initializing checkpoints execution time */
	for (cuenta = 0; cuenta < 4; cuenta++)
		check[cuenta].duracion = 0;

	/* Opening checkpoints data file */
	if((fich=fopen("check.dat", "w")) == NULL){
		fprintf(stdout, "Error opening file check.dat\n");
	} else {
	/* Opening checkpoints log file */

strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"check.log");
	if ((aux=fopen(filenameBuffer,"r")) == NULL){
		fprintf(stdout, "Error opening file check.log\n");
	} else {

	/* Reading the first line from file*/
	/* ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ */
	fprintf(fich, "Hora de comienzo\tTiempo desde el Inicio (s)\tDuración (s)\n");
	fscanf(aux, "1º sello %d %d 2º sello %d %d", &sellohora1.time, &sellohora1.millitm, &sellohora2.time, &sellohora2.millitm);
	while(!feof(aux)){

		/* Obtaining the current checkpoing execution time */
		fprintf(fich, "%s\t%lf\t\t%lf\n", ctime_sin_salto(&sellohora1.time), resta_tiempos(&inicio, &sellohora1), resta_tiempos(&sellohora1, &sellohora2));
		durac = resta_tiempos(&sellohora1, &sellohora2);
		cuenta = 0;

		/* finding the intest checkpoint */
		while (cuenta < 4){
			if (check[cuenta].duracion < durac) {
				if (check[cuenta].duracion != 0){
					/* move 1 place */
					cont1 = 3;
					while (cont1 > cuenta){
						check[cont1].duracion = check[cont1-1].duracion;
						check[cont1].transcurrido = check[cont1-1].transcurrido;
						strcpy(check[cont1].fecha, check[cont1-1].fecha);
						cont1--;
					}	
				}
				check[cuenta].duracion = durac;
				check[cuenta].transcurrido = resta_tiempos(&inicio, &sellohora1);
				strcpy(check[cuenta].fecha, ctime_sin_salto(&sellohora1.time));
				break; /* exits while */
			}
			cuenta++;
		}

		/* Reading next line */
		fscanf(aux, " \n1º sello %d %d 2º sello %d %d", &sellohora1.time, &sellohora1.millitm, &sellohora2.time, &sellohora2.millitm);
	} /* while */
	fclose(fich);
	fclose(aux);
	} /* if */
	} /* if */

	/***************************************************/
	/**    VACUUMS TIMESTAMPS AND EXECUTION TIME      **/
	/***************************************************/
	if (int_vacuum != 0){
	for (cuenta = 0; cuenta < 4; cuenta++)
		vacuum[cuenta].duracion = 0;


	/* Opening vacuums data file */
	if ((fich=fopen("vacuum.dat", "w")) == NULL){
		fprintf(stdout, "Error opening file vacuum.dat\n");
	} else {

	/* Opening vacuums log file */
strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"vacuum.log");
	if ((aux=fopen(filenameBuffer,"r")) == NULL){
		if (int_vacuum > 0)
			fprintf(stdout, "Error opening file %s\n",filenameBuffer);
	} else {

	/* Read the first line from file */
	/* ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ */
	fprintf(fich, "Hora de comienzo\tTiempo desde el Inicio (s)\tDuración (s)\n");
	fscanf(aux, "1º sello %d %d 2º sello %d %d", &sellohora1.time, &sellohora1.millitm, &sellohora2.time, &sellohora2.millitm);
	while(!feof(aux)){

		/* Obtaining the current vacuum execution time */
		fprintf(fich, "%s\t%lf\t\t%lf\n", ctime_sin_salto(&sellohora1.time), resta_tiempos(&inicio, &sellohora1), resta_tiempos(&sellohora1, &sellohora2));
		durac = resta_tiempos(&sellohora1, &sellohora2);
		cuenta = 0;

		/* finding the intest vacuum */
		while (cuenta < 4){
			if (vacuum[cuenta].duracion < durac) {
				if (vacuum[cuenta].duracion != 0){
					/* move 1 place */
					cont1 = 3;
					while (cont1 > cuenta){
						vacuum[cont1].duracion = vacuum[cont1-1].duracion;
						vacuum[cont1].transcurrido = vacuum[cont1-1].transcurrido;
						strcpy(vacuum[cont1].fecha, vacuum[cont1-1].fecha);
						cont1--;
					}	
				}
				vacuum[cuenta].duracion = durac;
				vacuum[cuenta].transcurrido = resta_tiempos(&inicio, &sellohora1);
				strcpy(vacuum[cuenta].fecha, ctime_sin_salto(&sellohora1.time));
				break; /* exits while */
			}
			cuenta++;
		}

		/* reading next line */
		fscanf(aux, " \n1º sello %d %d 2º sello %d %d", &sellohora1.time, &sellohora1.millitm, &sellohora2.time, &sellohora2.millitm);
	} /* while */
	fclose(fich);
	fclose(aux);
	} /* if */
	} /* if */
	} /* if (vacuum != 0) */

	/***************************************************/
	/*************    SHOWING RESULTS   ****************/
	/***************************************************/

	if ((total_tran == 0 ) || (resta_tiempos(&com_med, &fin_med) < 60)) {

		/* Measurement interval data have not been read */
		fprintf(stdout, "\nOne of these errors have ocurred:\n");
		fprintf(stdout, "\tThere was not measurement interval.\n");
		fprintf(stdout, "\tperformance log files have been removed.\n");
		fprintf(stdout, "\tperformance log files have errors.\n");
		fprintf(stdout, "Continue ... ");
		getchar();
		return 0;
	} else {
	rendimiento=((float)dat[0].cnt/((float)resta_tiempos(&com_med,&fin_med)/60));
	fprintf(stdout,"COMPUTED THROUGHPUT: %.3f tpmC-uva using %d warehouses.\n",rendimiento,w);
	fprintf(stdout," %d Transactions committed.\n", total_tran);

	/* For each transaction */
	for (cuenta = 0; cuenta < 5; cuenta++){

		/* Computing average response and think times */
		dat[cuenta].med_tr = dat[cuenta].sum_tr/dat[cuenta].cnt;
		dat[cuenta].med_tp = dat[cuenta].sum_tp/dat[cuenta].cnt;

		switch (cuenta){
			case 0:
				fprintf(stdout,"NEW-ORDER TRANSACTIONS:\n");
				break;
			case 1:
				fprintf(stdout,"PAYMENT TRANSACTIONS:\n");
				break;
			case 2:
				fprintf(stdout,"ORDER-STATUS TRANSACTIONS:\n");
				break;
			case 3:
				fprintf(stdout,"DELIVERY TRANSACTIONS:\n");
				break;
			case 4:
				fprintf(stdout,"STOCK-LEVEL TRANSACTIONS:\n");
				break;
			default: break;
		} /* switch */

		/* Printing common data */

		/* Number of transactions commited */
		fprintf(stdout,"  %d Transactions committed into measurement interval. Total transactions: %d.\n",dat[cuenta].cnt,dat[cuenta].cnt_total);
		porcen=(float)((float)dat[cuenta].cnt/total_tran)*100;
		fprintf(stdout,"  Percentage of Total transactions: %.3f%%\n",porcen);

		/* Response times */
		porcen_90th=(float)((float)dat[cuenta].cnt_90th/dat[cuenta].cnt)*100;
		fprintf(stdout,"  Percentage of \"well done\": %.3f%%\n",porcen_90th);
		if (dat[cuenta].tr90th >= 0)
			fprintf(stdout,"  Response time (min/avg/max/90th): %.3f / %.3f / %.3f / %.3f\n", dat[cuenta].min_tr, dat[cuenta].med_tr, dat[cuenta].max_tr, dat[cuenta].tr90th);
		else
			fprintf(stdout,"  Response time (min/avg/max/90th): %.3f / %.3f / %.3f / (><)\n", dat[cuenta].min_tr, dat[cuenta].med_tr, dat[cuenta].max_tr);

		/*Special accounting for new-order transaction*/
		if(cuenta==0){
			/*Percentage of "non valid item" transactions */
			porcen=(float)((float)dat[cuenta].roll_cnt/dat[cuenta].cnt)*100;
			fprintf(stdout,"  Percentage of rolled-back transactions: %.3f%% .\n",porcen);

			/*Average number of items per order*/
			porcen=(float)((float)dat[cuenta].sum_ol_cnt/dat[cuenta].cnt);
			fprintf(stdout,"  Average number of items per order: %.3f .\n", porcen);
			if (w>1){
				/*percentage of remote items*/
				porcen=(float)((float)dat[cuenta].art_remoto/dat[cuenta].sum_ol_cnt)*100;
				fprintf(stdout,"  Percentage of remote items: %.3f%% .\n",porcen);
			}/*de if*/
			else fprintf(stdout,"  Percentage of remote items: --- (One warehouse only).\n");
		}/*if*/

		/*Special accounting for paymnent transaction*/
		if(cuenta==1){
			/*Percentage of remote payment transactions*/
			porcen=(float)((float)dat[cuenta].paym_remota/dat[cuenta].cnt)*100;
			if(w==1) fprintf(stdout,"  Percentage of remote transactions: --- (One warehouse only)\n");
			else fprintf(stdout,"  Percentage or remote transactions: %.3f%% .\n",porcen);
			/*Percentage of customers selected by C_ID*/
			porcen=(float)((float)dat[cuenta].sum_c_id/dat[cuenta].cnt)*100;
			fprintf(stdout,"  Percentage of customers selected by C_ID: %.3f%% .\n", porcen);

		}/* if*/

		/*Special accounting for order-status transaction*/
		if(cuenta==2){

			/*Percentage of customers selected by c_id*/
			porcen=(float)((float)dat[cuenta].sum_c_id/dat[cuenta].cnt)*100;
			fprintf(stdout,"  Percentage of customers selected by C_ID: %.3f%% .\n", porcen);
		}/*if*/

		/*Special accounting for delivery transaction */
		if (cuenta == 3){
			dat[cuenta].med_tr_ej = dat[cuenta].sum_tr_ej/dat[cuenta].cnt;
			porcen_90th=(float)((float)dat[cuenta].cnt_90th_ej/dat[cuenta].cnt)*100;
			fprintf(stdout,"  Percentage of execution time < 80s : %.3f%%\n",porcen_90th);
			fprintf(stdout,"  Execution time min/avg/max: %.3f/%.3f/%.3f\n", dat[cuenta].min_tr_ej, dat[cuenta].med_tr_ej, dat[cuenta].max_tr_ej);		
			/*Percentage of skipped districts*/
			porcen=(float)((float)dist_saltado/dat[cuenta].cnt)*100;
			fprintf(stdout,"  No. of skipped districts: %d .\n", dist_saltado);
			fprintf(stdout,"  Percentage of skipped districts: %.3f%%.\n", porcen);
		}/*de if*/


		/* Average think time. Is showed for each transaction type */
		fprintf(stdout,"  Think time (min/avg/max): %.3f / %.3f / %.3f\n\n", dat[cuenta].min_tp, dat[cuenta].med_tp, dat[cuenta].max_tp);

	/* Waiting to enter a <return> after showing the transactions data */
	fprintf(stdout,"Continue...\n");
	getchar();
	} /* for */

	/* Showing executed checkpoints information */
	fprintf(stdout,"\nLongest checkpoints: \n");
	fprintf(stdout, "Start Time\tElapsed time since test start (s)\tExecution time (s)\n");
	for (cuenta = 0; cuenta < 4; cuenta++){
		if (check[cuenta].duracion != 0) {
		
			fprintf(stdout, "%s\t%f\t\t%f\n", check[cuenta].fecha, check[cuenta].transcurrido, check[cuenta].duracion);
		} else cuenta = 4;
	} /*for*/
	fprintf(stdout,"Continue ...\n");
	getchar();
	
	/* Showing executed vacuums information */
	/* only if vacuums have been performed */

	if (int_vacuum == 0) {

		/* No vacuums performed */
		fprintf(stdout,"\nNo vacuums executed.\n");
	} else {

		/* Showing data */
		fprintf(stdout,"\nLongest vacuums: \n");
		fprintf(stdout, "Start Time\tElapsed time since test start (s)\tExecution time (s)\n");
		for (cuenta = 0; cuenta < 4; cuenta++){
			if (vacuum[cuenta].duracion != 0) {
		
				fprintf(stdout, "%s\t%f\t\t%f\n", vacuum[cuenta].fecha, vacuum[cuenta].transcurrido, vacuum[cuenta].duracion);
			} else cuenta = 4;
		} /*for*/
	} /* if */


	/* Checking if response time complies the response time requirement */
	for (cuenta = 0; cuenta < 4; cuenta++){
		
		porcen_90th=(float)((float)dat[cuenta].cnt_90th/dat[cuenta].cnt)*100;
		if (porcen_90th < 90){
			/* Not comply */
			fprintf(stdout, "\n>> TEST FAILED\n");
			fprintf(stdout, "Transactions response time do not follow the standard requirements.\n");
			/* ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ */
			/* fprintf(stdout, "Consulte la sección 'Análisis de Resutados' del manual de usuario.\n"); */
			cuenta = 10;  /* forcing to exit for */
		}
	}

	if (cuenta < 10){
		/* All ok */
		fprintf(stdout, "\n>> TEST PASSED\n");
	}

	} /* if */

	/*Value answerd by the user for storing the results into a file */
	dec = op;

	/* Checking option */
	if (dec=='Y' || dec=='y'){
		/*writing file*/
		strcpy(nombre, filename);
		fich=fopen(nombre,"w");

		/* results writen on stdout are now writen in file */

		fprintf(fich,"Test results accounting performed on %.10s at %.8s using %d warehouses.\n\n",fecha,&fecha[11], w);
fprintf(fich,"Start of measurement interval: %lf m\n", resta_tiempos(&inicio,&com_med)/60);
fprintf(fich,"\nEnd of measurement interval: %lf m\n\n", resta_tiempos(&inicio,&fin_med)/60);
		rendimiento=((float)dat[0].cnt/((float)resta_tiempos(&com_med,&fin_med)/60));
		fprintf(fich,"COMPUTED THROUGHPUT: %.3f tpmC-uva using %d warehouses.\n\n",rendimiento,w);
		fprintf(fich," %d Transactions commited.\n\n", total_tran);

		for (cuenta = 0; cuenta < 5; cuenta++){

		        /* Computing average response and think times */
			dat[cuenta].med_tr = dat[cuenta].sum_tr/dat[cuenta].cnt;
			dat[cuenta].med_tp = dat[cuenta].sum_tp/dat[cuenta].cnt;

			switch (cuenta){
				case 0:
					fprintf(fich,"NEW-ORDER TRANSACTIONS:\n");
					break;
				case 1:
					fprintf(fich,"PAYMENT TRANSACTIONS:\n");
					break;
				case 2:
					fprintf(fich,"ORDER-STATUS TRANSACTIONS:\n");
					break;
				case 3:
					fprintf(fich,"DELIVERY TRANSACTIONS:\n");
					break;
				case 4:
					fprintf(fich,"STOCK-LEVEL TRANSACTIONS:\n");
					break;
				default: break;
			} /*switch*/

                	/* Number of transactions commited */
			fprintf(fich,"  %d Transactions within measurement time (%d Total).\n",dat[cuenta].cnt,dat[cuenta].cnt_total);
			porcen=(float)((float)dat[cuenta].cnt/total_tran)*100;
			fprintf(fich,"  Percentage: %.3f%%\n",porcen);

			/* Response times */
			porcen_90th=(float)((float)dat[cuenta].cnt_90th/dat[cuenta].cnt)*100;
			fprintf(fich,"  Percentage of \"well done\" transactions: %.3f%%\n",porcen_90th);
			if (dat[cuenta].tr90th >= 0) fprintf(fich,"  Response time (min/med/max/90th): %.3f / %.3f / %.3f / %.3f\n", dat[cuenta].min_tr, dat[cuenta].med_tr, dat[cuenta].max_tr, dat[cuenta].tr90th);
			else
				fprintf(fich,"  Response time (min/avg/max/90th): %.3f / %.3f / %.3f / (><)\n", dat[cuenta].min_tr, dat[cuenta].med_tr, dat[cuenta].max_tr);

			/*Special accounting for new-order transaction*/
			if(cuenta==0){
				/*Percentage of "non valid item" transactions */
				porcen=(float)((float)dat[cuenta].roll_cnt/dat[cuenta].cnt)*100;
				fprintf(fich,"  Percentage of rolled-back transactions: %.3f%% .\n",porcen);
				/*Average number of items per order*/
				porcen=(float)((float)dat[cuenta].sum_ol_cnt/dat[cuenta].cnt);
				fprintf(fich,"  Average number of items per order: %.3f .\n", porcen);
				if (w>1){
					/*percentage of remote items*/

					porcen=(float)((float)dat[cuenta].art_remoto/dat[cuenta].sum_ol_cnt)*100;
					fprintf(fich,"  Percentage of remote items: %.3f%% .\n",porcen);
				}/*if*/
				else fprintf(fich,"  Percentage of remote items: --- (One warehouse only).\n",porcen);
			}/*if*/

			/*Special accounting for payment transaction*/
			if(cuenta==1){
				/*Percentage of remote payment transactions*/
				porcen=(float)((float)dat[cuenta].paym_remota/dat[cuenta].cnt)*100;
				if(w==1) fprintf(fich,"  Percentage of remote transactions: --- (One warehouse only)\n");
				else fprintf(fich,"  Percentage of remote transactions: %.3f%% .\n",porcen);
				/*Percentage of customers selected by C_ID*/
				porcen=(float)((float)dat[cuenta].sum_c_id/dat[cuenta].cnt)*100;
				fprintf(fich,"  Percentage of customers selected by C_ID: %.3f%% .\n", porcen);

			}/*if*/

			/*Special accounting for order-status transaction*/
			if(cuenta==2){

				/*Percentage of customers selected by C_ID*/
				porcen=(float)((float)dat[cuenta].sum_c_id/dat[cuenta].cnt)*100;
				fprintf(fich,"  Percentage of clients chosen by C_ID: %.3f%% .\n", porcen);

			}/*if*/

			/*Special accounting for delivery transaction*/
			if (cuenta == 3){
				dat[cuenta].med_tr_ej = dat[cuenta].sum_tr_ej/dat[cuenta].cnt;
				porcen_90th=(float)((float)dat[cuenta].cnt_90th_ej/dat[cuenta].cnt)*100;
				fprintf(fich,"  Percentage of execution time < 80s : %.3f%%\n",porcen_90th);
				fprintf(fich,"  Execution time min/avg/max: %.3f/%.3f/%.3f\n", dat[cuenta].min_tr_ej, dat[cuenta].med_tr_ej, dat[cuenta].max_tr_ej);		
				/*Percentage of skipped districts */
				porcen=(float)((float)dist_saltado/dat[cuenta].cnt)*100;
				fprintf(fich,"  No. of skipped districts: %d .\n", dist_saltado);
				fprintf(fich,"  Percentage of skipped districts: %.3f%%.\n", porcen);
			}/*if*/


			fprintf(fich,"  Think time (min/avg/max): %.3f / %.3f / %.3f\n\n", dat[cuenta].min_tp, dat[cuenta].med_tp, dat[cuenta].max_tp);
		} /* for */
	
		/* Showing executed checkpoints information */
		fprintf(fich,"\nLongest checkpoints: \n");
		fprintf(fich, "Start time\tElapsed time since test start (s)\tExecution time (s)\n");
		for (cuenta = 0; cuenta < 4; cuenta++){
			if (check[cuenta].duracion != 0) {
		
				fprintf(fich, "%s\t%f\t\t%f\n", check[cuenta].fecha, check[cuenta].transcurrido, check[cuenta].duracion);
			} else cuenta = 4;
		} /*for*/
	
		/* Showing executed vacuums information */
		/* only if vacuums have been performed. */
		if (int_vacuum == 0) {

			/* No vacuums executed */
			fprintf(fich, "\nNo vacuums executed.\n");	
		}else {

			/* Imprimir informacion de las limpiezas */
			fprintf(fich,"\nLongest vacuums: \n");
			fprintf(fich, "Start time\tElapsed time since test start (s)\tExecution time (s)\n");
			for (cuenta = 0; cuenta < 4; cuenta++){
				if (vacuum[cuenta].duracion != 0) {
			
					fprintf(fich, "%s\t%f\t\t%f\n", vacuum[cuenta].fecha, vacuum[cuenta].transcurrido, vacuum[cuenta].duracion);
				} else cuenta = 4;
			} /*for*/

		} /* if  */

		/* Checking if response times comply the response time requirement */
		for (cuenta = 0; cuenta < 4; cuenta++){
			porcen_90th=(float)((float)dat[cuenta].cnt_90th/dat[cuenta].cnt)*100;
			// printf("porcent: %d\n", porcen_90th);
			if (porcen_90th < 90){

				/* Not comply */
				fprintf(fich, "\n>> TEST FAILED\n");
				fprintf(fich, "Transactions response time do not follow the standard requirements.\n");
			/* ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ */	
			/*	fprintf(fich, "Consulte la sección 'Análisis de Resutados' del manual de usuario.\n"); */
				cuenta = 10;  /* forcing to exit for */
			}
		}

		if (cuenta < 10){
			/* No ha habido errores */
			fprintf(fich, "\n>> TEST PASSED\n");
		}

		fclose(fich);	
		fprintf(stdout,"   Results have been written on file %s.\n",nombre);

	}/*if*/

/******************* SECOND FILE PARSING ************************/
/***** Generating graph 1 using computed 90% percentile Rt.  ****/
/******* Generating graph 3: no. transactions/think time. *******/ 
/****************************************************************/

/* Opening files */
	if ((g1_0 = fopen("g1_0.aux", "w+")) == NULL){
		fprintf(stderr, "Error opening auxiliar file g1_0\n");
		return(0);
	}

	if ((g1_1 = fopen("g1_1.aux", "w+")) == NULL){
		fprintf(stderr, "Error opening auxiliar file g1_1\n");
		return(0);
	}

	if ((g1_2 = fopen("g1_2.aux", "w+")) == NULL){
		fprintf(stderr, "Error opening auxiliar file g1_2\n");
		return(0);
	}

	if ((g1_3 = fopen("g1_3.aux", "w+")) == NULL){
		fprintf(stderr, "Error opening auxiliar file g1_3\n");
		return(0);
	}

	if ((g1_4 = fopen("g1_4.aux", "w+")) == NULL){
		fprintf(stderr, "Error opening auxiliar file g1_4\n");
		return(0);
	}

	if ((g3 = fopen("g3.aux", "w+")) == NULL){
		fprintf(stderr, "Error opening auxiliar file g3\n");
		return(0);
	}

	/* Calculating number of intervals and initializing files */

	/* file g1_0 */
	if (dat[0].tr90th >= 0){
		intg1_0 = ((double)4*dat[0].tr90th)/NUM_INT_TR;
		for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
			pto.x = cuenta*(intg1_0);
			pto.y = 0;
			fwrite (&pto, sizeof(pto), 1, g1_0);
		}
	} else
		 intg1_0 = -1;
	/* file g1_1 */
	if (dat[1].tr90th >= 0){
		intg1_1 = ((double)4*dat[1].tr90th)/NUM_INT_TR;
		for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
			pto.x = cuenta*(intg1_1);
			pto.y = 0;
			fwrite (&pto, sizeof(pto), 1, g1_1);
		}
	} else
		 intg1_1 = -1;
	/* file g1_2 */
	if (dat[2].tr90th >= 0){
		intg1_2 = ((double)4*dat[2].tr90th)/NUM_INT_TR;
		for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
			pto.x = cuenta*(intg1_2);
			pto.y = 0;
			fwrite (&pto, sizeof(pto), 1, g1_2);
		}
	} else
		 intg1_2 = -1;
	/* file g1_3 */
	if (dat[3].tr90th >= 0){
		if (dat[3].tr90th == 0){
			intg1_3 = ((double)4*TR90thD)/NUM_INT_TR;
			dat[3].tr90th = TR90thD;
		}
		else
			intg1_3 = ((double)4*dat[3].tr90th)/NUM_INT_TR;
		for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
			pto.x = cuenta*(intg1_3);
			pto.y = 0;
			fwrite (&pto, sizeof(pto), 1, g1_3);
		}
	} else
		 intg1_3 = -1;
	/* file g1_4 */
	if (dat[4].tr90th >= 0){
		intg1_4 = ((double)4*dat[4].tr90th)/NUM_INT_TR;
		for (cuenta = 0; cuenta < NUM_INT_TR; cuenta++){
			pto.x = cuenta*(intg1_4);
			pto.y = 0;
			fwrite (&pto, sizeof(pto), 1, g1_4);
		}
	} else
		 intg1_4 = -1;

	/* file g3 */
	intg3 = (dat[0].med_tp*4)/NUM_INT_TP; /* interval length for graph 3*/
	for (cuenta = 0; cuenta < NUM_INT_TP; cuenta++){ 
		pto.x = (cuenta)*intg3;
		pto.y = 0;
		fwrite (&pto, sizeof(pto), 1, g3);
	}

  for(alm=1;alm<=w;alm++){ /*for each almacén */
	for(d=1;d<=term;d++){   /*for each terminal  */
		/*obtaining terminal log file name*/

strcpy(filenameBuffer,VARDIR);
sprintf(n_fichero, "F_%d_%d.log\0", alm,d);
strcat(filenameBuffer,n_fichero);
		/*opening terminal log file name*/
		if ((fich=fopen(filenameBuffer,"r")) == NULL){
			fprintf(stdout, "Error al abrir el fichero: %s\n", filenameBuffer);
			fprintf(stdout, "%s\n", strerror(errno));
		} else {
		/*reading the first line from file*/
		fscanf(fich,"\n*%d> %d %d %d %d %f %d %d %d %d %d %d",&tipo, &seg1,&mseg1, &seg_encl, &mseg_encl, &tr,&tp,&resul,&cont1,&cont2,&seg2,&mseg2);
		sello_leido.time=seg2;
		sello_leido.millitm=mseg2;
		while (!feof(fich)){	

			switch (tipo){
				case NEW_ORDER:
			 	    if(resta_tiempos(&com_med,&sello_leido)>=0){
					 /*only transactions commited into measurement interval are counted*/

					/* graph 1 */
					if ((tr < dat[0].tr90th*4) && (intg1_0 > 0)){
						/* seeking interval */
						fseek(g1_0, sizeof(pto)*(int)double2int(tr/intg1_0), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_0);
						if (ferror(g1_0)){
							fprintf(stdout, "Error reading file g1_0\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}

						/* Updating interval value */
						pto.y++;

						/* Writing the new interval value */
						fseek(g1_0, sizeof(pto)*(int)double2int(tr/intg1_0), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_0);
						if (ferror(g1_0)){
							fprintf(stdout, "Error writing file g1_0\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
					/* graph 3 */
					if (tp < (4*dat[0].med_tp)){

						/* seeking interval */
						fseek(g3, sizeof(pto)*(int)double2int((tp/intg3)), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g3);
						if (ferror(g3)){
							fprintf(stdout, "Error reading file g3\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}

						/* Updating interval value */
						pto.y++;

						/* Writing the new interval value */
						fseek(g3, sizeof(pto)*(int)double2int((tp/intg3)), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g3);
						if (ferror(g3)){
							fprintf(stdout, "Error writing file g3\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
			   	    }
					break;
				case PAYMENT:

			 	    if(resta_tiempos(&com_med,&sello_leido)>=0){
					 /*only transactions commited into measurement interval are counted*/
					/* graph 1 */
					if ((tr < dat[1].tr90th*4) && (intg1_1 > 0)){

						/* seeking interval */
						fseek(g1_1, sizeof(pto)*(int)double2int(tr/intg1_1), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_1);
						if (ferror(g1_1)){
							fprintf(stdout, "Error reading file g1_1\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}

						/* Updating interval value */
						pto.y++;

						/* Writing the new interval value */
						fseek(g1_1, sizeof(pto)*(int)double2int(tr/intg1_1), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_1);
						if (ferror(g1_1)){
							fprintf(stdout, "Error writing file g1_1\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
			   	    }
					break;
				case ORDER_STATUS:
			 	    if(resta_tiempos(&com_med,&sello_leido)>=0){
					 /*only transactions commited into measurement interval are counted*/
					/* graph 1 */
					if ((tr < dat[2].tr90th*4) && (intg1_2 > 0)){

						/* seeking interval */
						fseek(g1_2, sizeof(pto)*(int)double2int(tr/intg1_2), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_2);
						if (ferror(g1_2)){
							fprintf(stdout, "Error reading file g1_2\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}

						/* Updating interval value */
						pto.y++;

						/* Writing the new interval value */
						fseek(g1_2, sizeof(pto)*(int)double2int(tr/intg1_2), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_2);
						if (ferror(g1_2)){
							fprintf(stdout, "Error writing file g1_2\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
			   	    }
					break;
				case DELIVERY:
			 	    if(resta_tiempos(&com_med,&sello_leido)>=0){
					 /*only transactions commited into measurement interval are counted*/
					/* graph 1 */
					if ((tr < dat[3].tr90th*4) && (intg1_3 > 0)){

						/* seeking interval */
						fseek(g1_3, sizeof(pto)*(int)double2int(tr/intg1_3), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_3);
						if (ferror(g1_3)){
							fprintf(stdout, "Error reading file g1_3\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}

						/* Updating interval value */
						pto.y++;

						/* Writing the new interval value */
						fseek(g1_3, sizeof(pto)*(int)double2int(tr/intg1_3), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_3);
						if (ferror(g1_3)){
							fprintf(stdout, "Error writing file g1_3\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
			   	    }
					break;
				case STOCK_LEVEL:
			 	    if(resta_tiempos(&com_med,&sello_leido)>=0){
					 /*only transactions commited into measurement interval are counted*/
					if ((tr < dat[4].tr90th*4) && (intg1_4 > 0)){

						/* seeking interval */
						fseek(g1_4, sizeof(pto)*(int)double2int(tr/intg1_4), SEEK_SET);
						fread(&pto, sizeof(pto), 1, g1_4);
						if (ferror(g1_4)){
							fprintf(stdout, "Error reading file g1_4\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}

						/* Updating interval value */
						pto.y++;

						/* Writing the new interval value */
						fseek(g1_4, sizeof(pto)*(int)double2int(tr/intg1_4), SEEK_SET);
						fwrite(&pto, sizeof(pto), 1, g1_4);
						if (ferror(g1_4)){
							fprintf(stdout, "Error writing file g1_4\n");
							fprintf(stdout, "%s\n", strerror(errno));
						}
					}
			   	    }
					break;
				default:
					break;
			}/*switch*/

			/* reading next line*/
			fscanf(fich,"\n*%d> %d %d %d %d %f %d %d %d %d %d %d",&tipo, &seg1,&mseg1, &seg_encl, &mseg_encl, &tr,&tp,&resul,&cont1,&cont2,&seg2,&mseg2);
			sello_leido.time=seg2;
			sello_leido.millitm=mseg2;
		}/*while*/

		fclose(fich);				
		}/*if */
	}/* for */
	
  }/* for */

/*************************************************************/
/**************** GENERATING OUTPUT FILES ********************/
/*************************************************************/

/* Converting files to gnuplot input file format */

 if ((fich = fopen("g1NewOrder.dat","w")) == NULL){
	fprintf(stdout, "Error opening output file g1NewOrder\n");
	return 0;
 }
 rewind(g1_0);
	fread(&pto,sizeof(pto), 1, g1_0);
	while (!feof(g1_0)){
		if (ferror(g1_0)){
			fprintf(stderr, "Error in file g1_0\n");
			break;
		}
		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fread(&pto, sizeof(pto), 1, g1_0);
	}
 fclose(g1_0);
 remove("g1_0.aux");
 fclose(fich);

 if ((fich = fopen("g1Payment.dat","w")) == NULL){
	fprintf(stdout, "Error opening output file g1Payment\n");
	return 0;
 }
 rewind(g1_1);
	fread(&pto,sizeof(pto), 1, g1_1);
	while (!feof(g1_1)){
		if (ferror(g1_1)){
			fprintf(stderr, "Error in file g1_1\n");
			break;
		}
		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fread(&pto, sizeof(pto), 1, g1_1);
	}
 fclose(g1_1);
 remove("g1_1.aux");
 fclose(fich);

 if ((fich = fopen("g1OrderStatus.dat","w")) == NULL){
	fprintf(stdout, "Error opening output file g1OrderStatus\n");
	return 0;
 }
 rewind(g1_2);
	fread(&pto,sizeof(pto), 1, g1_2);
	while (!feof(g1_2)){
		if (ferror(g1_2)){
			fprintf(stderr, "Error in file g1_2\n");
			break;
		}
		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fread(&pto, sizeof(pto), 1, g1_2);
	}
 fclose(g1_2);
 remove("g1_2.aux");
 fclose(fich);

 if ((fich = fopen("g1Delivery.dat","w")) == NULL){
	fprintf(stdout, "Error opening output file g1Delivery.dat\n");
	return 0;
 }
 rewind(g1_3);
	fread(&pto,sizeof(pto), 1, g1_3);
	while (!feof(g1_3)){
		if (ferror(g1_3)){
			fprintf(stderr, "Error in file g1_3\n");
			break;
		}
		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fread(&pto, sizeof(pto), 1, g1_3);
	}
 fclose(g1_3);
 remove("g1_3.aux");
 fclose(fich);

 if ((fich = fopen("g1StockLevel.dat","w")) == NULL){
	fprintf(stdout, "Error opening output file g1StockLevel\n");
	return 0;
 }
 rewind(g1_4);
	fread(&pto,sizeof(pto), 1, g1_4);
	while (!feof(g1_4)){
		if (ferror(g1_4)){
			fprintf(stderr, "Error in file g1_4\n");
			break;
		}
		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fread(&pto, sizeof(pto), 1, g1_4);
	}
 fclose(g1_4);
 remove("g1_4.aux");
 fclose(fich);

 if ((fich = fopen("g3.dat","w")) == NULL){
	fprintf(stdout, "Error opening output file g3\n");
	return 0;
 }
 rewind(g3);
	fread(&pto,sizeof(pto), 1, g3);
	while (!feof(g3)){
		if (ferror(g3)){
			fprintf(stderr, "Error in file g3\n");
			break;
		}
		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fread(&pto, sizeof(pto), 1, g3);
	}
 fclose(g3);
 remove("g3.aux");
 fclose(fich);


/* Recorrer el fichero auxiliar y calcular los puntos del grafico 4 */
/* Escribir cada punto en el formato de gnuplot.  */
 if ((fich = fopen("g4.dat","w")) == NULL){
	fprintf(stdout, "Error opening file g4\n");
	return 0;
 }
 rewind(g4);
 cuenta = 0;
	fread(&pto,sizeof(pto), 1, g4);
	while (!feof(g4)){
		if (ferror(g4)){
			fprintf(stderr, "Error in file g4\n");
			break;
		}

		/* Calculating current point */
		cuenta = cuenta + pto.y;
		pto.y = (cuenta*60)/(pto.x + INT_REND);

		/* Writing interval */
		fprintf(fich, "%f %f\n", pto.x, pto.y);
		fseek(g4, -sizeof(pto), SEEK_CUR);
		fwrite(&pto, sizeof(pto), 1, g4);
		fread(&pto, sizeof(pto), 1, g4);
	}
 fclose(g4);
 remove("g4.aux");
 fclose(fich);

// strcpy(filenameBuffer,VARDIR);
// strcat(filenameBuffer,"medida.log");
// remove(filenameBuffer);

}/*lectura_bitacora*/


int hay_tablas(int w, int caso){
/* ----------------------------------------------- *\
|* hay_tablas function.                            *|
|* ----------------------------------------------- *|
|* <w> number of warehouse that should be present  *| 
|* into database.                                  *| 
|* <caso> is a flag that indicates if cardinality  *| 
|* check is performed before a test run or before  *| 
|* restoring the database.                         *| 
|* ----------------------------------------------- *|
|* Checks if the number of rows in each table      *|
|* comply the TPC-C standard for the number of     *|
|* warehouses <w>.                                 *|
\* ----------------------------------------------- */

#define CASO_TEST 0 /* Check tables before a test run */
#define CASO_REST 1 /* Check tables before restoring the database */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int ll;
	int ls;
EXEC SQL END DECLARE SECTION;

/* Connecting with database */
EXEC SQL CONNECT TO tpcc;/* USER USERNAME;*/

/* Checking warehouse table */
fprintf(stdout, "\tWarehouse table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(w_id) INTO :ll FROM warehouse;
if (sqlca.sqlcode==-400) return(0);
else if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking warehouse table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */
if (caso == CASO_REST) 
	/* Checking for a test run */
	/*  w = number of warehouses into database */
	w = ll;
else 
	/* Checking for restoring the database */
	/* Checking if into database are <w> warehouses really */
	if (ll < w){
		EXEC SQL DISCONNECT;
		return 0;
	}
fprintf(stdout, "OK\n");

/* Checking stock table */
fprintf(stdout, "\tStock table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(s_quantity) INTO :ll FROM stock;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking stock table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll < (NUM_ART*w)){
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking item table */
fprintf(stdout, "\tItem table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(i_id) INTO :ll FROM item;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking item table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll != NUM_ART){
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking table District */
fprintf(stdout, "\tDistrict table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(d_ytd) INTO :ll FROM district;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking district table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll < (w*10)){
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking customer table */
fprintf(stdout, "\tCustomer table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(c_id) INTO :ll FROM customer;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking customer table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll < (w*10*NUM_CLIENT)){
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking history table */
fprintf(stdout, "\tHistory table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(h_amount) INTO :ll FROM history;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stderr, "Error checking history table\n");
	fprintf(stderr, "TIPO DE ERROR: %d\n", sqlca.sqlcode);
	fprintf(stderr, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll < (w*10*NUM_CLIENT)){
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking order tabler */
fprintf(stdout, "\tOrder table... ");
fflush(stdout);
EXEC SQL SELECT COUNT(o_id) INTO :ll FROM orderr;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking order table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll < (w*10*NUM_ORDER)){
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking table order-line */
fprintf(stdout, "\tOrder-Line table... ");
fflush(stdout);
EXEC SQL SELECT SUM(o_ol_cnt) INTO :ls FROM orderr;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking order-line table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

EXEC SQL SELECT COUNT(ol_quantity) INTO :ll FROM order_line;
if (sqlca.sqlcode==-400) return(0);
else  if (sqlca.sqlcode<0){
	fprintf(stdout, "Error checking order-line table\n");
	fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
	fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
	} /* if */

if (ll != ls){ 
	EXEC SQL DISCONNECT;
	return 0;
}
fprintf(stdout, "OK\n");

/* Checking New-Order table (only for a test run) */
if (caso == CASO_TEST) {
	fprintf(stdout, "\tNew-Order table... ");
	fflush(stdout);
	EXEC SQL SELECT COUNT(no_o_id) INTO :ll FROM new_order;
	if (sqlca.sqlcode==-400) return(0);
	else  if (sqlca.sqlcode<0){
		fprintf(stdout, "Error checking new-order table\n");
		fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
		fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
		} /* if */

	if (ll < 9000*w){ 
		EXEC SQL DISCONNECT;
		return 0;
	}
	fprintf(stdout, "OK\n");
}

EXEC SQL DISCONNECT;
return(1); /*All tables ok*/
}

void estado_base(){
/* ----------------------------------------------- *\
|* estado_base function                            *|
|* ----------------------------------------------- *|
|* Shows on stdout the number of rows of each      *|
|* table in database.                              *|
\* ----------------------------------------------- */

/* SQL variables */
EXEC SQL BEGIN DECLARE SECTION;
	int n_fil;
EXEC SQL END DECLARE SECTION;

/* Connecting with tpcc */
EXEC SQL CONNECT TO tpcc;/* USER USERNAME;*/

fprintf(stdout,"\n\n\n\n\n\n\n		CARDINALITY OF TABLES:\n\n\n");
EXEC SQL SELECT count(*) INTO :n_fil FROM warehouse;
if(n_fil==1)
fprintf(stdout,"		->WAREHOUSE table: %d row.\n",n_fil);
else
fprintf(stdout,"		->WAREHOUSE table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM district;
fprintf(stdout,"		->DISTRICT table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM item;
fprintf(stdout,"		->ITEM table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM stock;
fprintf(stdout,"		->STOCK table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM customer;
fprintf(stdout,"		->CUSTOMER table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM history;
fprintf(stdout,"		->HISTORY table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM orderr;
fprintf(stdout,"		->ORDER table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM order_line;
fprintf(stdout,"		->ORDER-LINE table: %d rows.\n",n_fil);
EXEC SQL SELECT count(*) INTO :n_fil FROM new_order;
fprintf(stdout,"		->NEW-ORDER table: %d rows.\n",n_fil);

EXEC SQL DISCONNECT;
}

int hay_fich(char *punt){
/* ------------------------------------------------- *\
|* hay_fich function                                 *|
|* ------------------------------------------------- *|
|* Checks if exists the file which name is <punt>.   *|
|* ------------------------------------------------- *|
|* Returns 0 if file does not exist and 1 otherwise. *|
|* contrario.                                        *|
\* ------------------------------------------------- */

FILE *prob;

if ((prob=fopen(punt,"r")) == NULL){
	switch(errno){
		case ENOENT:
			return 0;		
			break;
		default:
			fclose(prob);
			return 1;				
			break;
						
	}/*switch*/
}
fclose(prob);
return 1;
} /*hay_fich*/

int main(int argc, char ** argv){
/* ------------------------------------------------ *\
|* Main program of the benchmark controller program *|
\* ------------------------------------------------ */

int command = 0;
int haybd = 0, haylogs = 0;      /* flags that indicates if database and log files exists */
int i, j;
int semid;
int int_ramp, int_med;           /* Variables for storing the rump-up and measurement intervals */
char ch; 
char dec_vacuum,dec_n_vacuum; 
int  int_vacuum, num_vacuum, vacuum;   /* Variables for configuring the vacuums  */ 
char fecha[21];                        /* system date string */

int srv_id;

/* Signals handling functions */
void signal_usr1();                  
void signal_int1();
void signal_int2();
void signal_int3();
void signal_term();
void signal_term_exit();
void signal_alarm();

/* Changing buffer to line buffered mode */
setvbuf(stdout, NULL, _IOLBF, 0);

salir = 0;
/* main program loop */
/* flag <salir> is modified when signal SIGTERM is received */
	if (!salir){
		/* Run the commands asked by clients */
		command = atoi(argv[1]);
		switch (command){
                        case 1: /* Creating the database */
				w = atoi(argv[3]);
				srv_id = atoi(argv[4]);

                                /* catching signal SIGUSR1 */
                                if (signal(SIGUSR1, signal_usr1) == SIG_ERR){
                                        fprintf(stdout, "Error catching signal sigusr1. Create Database (child)\n");
                                        exit(-1);
                                }  /* if */

                                /* forks */
                                if((pidh = fork()) == 0) {
                                        /* populating database */
                                        pidp = getppid();

                                        /* catching signals SIGINT and SIGTERM */
                                        if (signal(SIGINT, signal_int2) == SIG_ERR){
                                                fprintf(stdout, "Error catching signal sigint. Create database\n");
                                                exit(-1);
                                        }
                                        if (signal(SIGTERM, signal_term_exit) == SIG_ERR){
                                                fprintf(stdout, "Error catching signal sigterm_exit. Create Database\n");
                                                exit(-1);
                                        }

                                        /* calling the function for populating */
                                        carga(w);

                                        /* Sending "end of population" signal to control process */
                                        kill(pidp, SIGUSR1);
                                        exit(0);
                                } else {
                                        /* Controling the population process */
                                        /* Catching signal SIGINT */
                                        if (signal(SIGINT, signal_int1) == SIG_ERR){
                                                fprintf(stdout, "Error catching signal sigusr1. Create Database (father)\n");
                                                exit(-1);
                                        }
                                        /* Waiting for the population end */
                                        terminado = 0;
                                        while (terminado == 0);
                                } /* if */
                                /* haybd = 1; */
                                fprintf(stdout,"Finished.\n");
                                break;

                case 2: /* Restoring the database */
			srv_id = atoi(argv[3]);

                        /* Checking database */
                        fprintf(stdout, "Checking database...\n");
                        fflush(stdout);
                        if (!hay_tablas(0,1)) {
                                fprintf(stdout, "\nError: database is incomplete. It can not be restored\n");
                                break; /* exit Case */
                        }
                        fprintf(stdout, "Database OK\n");

                        /* Catching signal SIGUSR1 */
                        if (signal(SIGUSR1, signal_usr1) == SIG_ERR){
                                fprintf(stdout, "Error catching signal sigusr1. Restore database (child)\n");
                                exit(-1);
                        }  /* if */

                        /* forks */
                        if((pidh = fork()) == 0) {
                                /* performing restoration */

                                pidp = getppid();
                                /* catching signal SIGINT */
                                if (signal(SIGINT, signal_int2) == SIG_ERR){
                                        fprintf(stdout, "Error catching signal sigint. Restore database\n");
                                        exit(-1);
                                }
                                /* Catching signal SIGTERM */
                                if (signal(SIGTERM, signal_term_exit) == SIG_ERR){
                                        fprintf(stdout, "Error catching signal sigterm. Restore database\n");
                                        exit(-1);
                                }

                                /* calling the function */
                                restaura();

                                /* Sending "end of restoration" signal to control process */
                                kill(pidp, SIGUSR1);
                                exit(0);
                        } else {
                                /* Restoration control process */

                                /* Catching signal SIGINT */
                                if (signal(SIGINT, signal_int1) == SIG_ERR){
                                        fprintf(stdout, "Error catching signal sigusr1. Restore database (father)\n");
                                        exit(-1);
                                }
                                /* Waiting for the restoration end */
                                terminado = 0;
                                while (terminado == 0);
                        } /* if */
			fprintf(stdout,"Finished.\n");
                        break;


		case 3: /* Run the test */

			/* checking if file "const.dat" exists */		
strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"cons.dat");
			if (!hay_fich(filenameBuffer)){
				fprintf(stdout," ->The terminal programs can not access to file \"const.dat\". Test run aborted.\n");
				fprintf(stdout,"Continue...\n");
				getchar();
				break;
			}
			/* catching signal SIGINT */
			if (signal(SIGINT, signal_int3) == SIG_ERR){
				fprintf(stdout, "Error catching signal sigint. Test run.\n");
				exit(-1);
			} 
			/* catching signal SIGALRM */
			if (signal(SIGALRM, signal_alarm) == SIG_ERR){
				fprintf(stdout, "Error catching signal sigalrm. Test run.\n");
				exit(-1);
			}  
 
			w = atoi(argv[3]);
			term = atoi(argv[4]);
			int_ramp = atoi(argv[5]);
			int_med = atoi(argv[6]);
			vacuum = atoi(argv[7]);
			dec_vacuum = argv[8][0];
			int_vacuum = atoi(argv[9]);
			num_vacuum = atoi(argv[10]);
			srv_id = atoi(argv[11]);

			fprintf(stdout, "Checking the database...\n");
			fflush(stdout);
			if (!hay_tablas(w,0)) { 
				fprintf(stdout,"\nError: the database is not populated correctly.\n");
				fprintf(stdout, "\nExiting.\n");
				break; /* exit case */
			}
			fprintf(stdout, "Database OK\n");

			/* Recording test parameters into test log file */
			ftime(&sellohora1);
			/**Test log file**/

strcpy(filenameBuffer,VARDIR);
strcat(filenameBuffer,"medida.log");
			if ((med=fopen(filenameBuffer,"w")) == NULL){
				fprintf(stderr, "\nError:\n");
				fprintf(stderr, "\topening file medida.log\n");
				fprintf(stderr, "\n\nContinue ...");
				getchar();
				break;
			}else{
				getfechahora(fecha);
				fprintf(med,"TEST PERFORMED ON %s .\n",fecha);
				fprintf(med,"Number of warehouses: %d .\n",w);
				fprintf(med,"Number of terminals for each warehouse: %d .\n",term);
				fprintf(med,"Measurement interval: %d minutes.\n",int_med/60);
				fprintf(med,"Ramp-up interval: %d minutes.\n",int_ramp/60);
				fprintf(med,"Interval between vacuums: %d.\n", int_vacuum);
				fprintf(med,"Maximum number of vacuums: %d .\n", num_vacuum);
				fprintf(med,"Test start timestamp: %d %d .\n",sellohora1.time, sellohora1.millitm);
				fflush(med);
			}


			/* Launching terminals and TM */
			terminado = 0;
			if (lanzador(w, term, srv_id) != 0){
				fprintf(stdout, "\nError: launching terminales and TM\n");
				break;
			} 

			/* Launching the vacuums controller if has been configured */
			if (dec_vacuum=='Y' || dec_vacuum=='y') lanza_vacuum(int_vacuum, num_vacuum);

			/* Waiting ramp-up period */
			if (!terminado) {
				timeout = 0;
				alarm(int_ramp);
			}
			while ((!timeout) && (!terminado))
				sleep(1); 

			/* At this point ramp-up period has expired */
			/* Launching checkponts controller */
			if (!terminado) {
				lanza_check();
				timeout = 0;
			}
			/* Recording measurement interval start */
			ftime(&sellohora2);
			if (med != NULL){
				/* El fichero se pudo abrir */
				fprintf(med,"Measurement interval start timestamp: %d %d .\n",sellohora2.time, sellohora2.millitm);
			}

			alarm(int_med);
			/* waiting measurement interval */
			while ((!timeout) && (!terminado))
				sleep(1);

			/* At this point measurement interval has expired */
			/* Record the measurement interval end */
			ftime(&sellohora1);
			if (med != NULL){
				/* El fichero se pudo abrir */
				fprintf(med,"Measurement interval end timestamp: %d %d .\n",sellohora1.time, sellohora1.millitm);
				fflush(med);
			}

			/* killing processes */
			fprintf(stdout, "bench: Shutting down terminals... \n");
			fflush(stdout);


			// Diego: Original block, too slow.
			// if (pidterm != NULL)
			// 	for (i =0; i < w*10; i+=10) 
			// 	for (j = 0; j < term; j++){
			// 		kill (pidterm[i+j], SIGTERM);
			// 		waitpid(pidterm[i+j], NULL, 0);
			// 	}

			if (pidterm != NULL) {
			   // Diego: New block
			   // First, send all signals
			   for (i =0; i < w; i++) 
			      for (j = 0; j < term; j++)
			         if (kill (pidterm[i*10+j], SIGTERM) != 0) {
				    fprintf(stderr, "bench: kill() failed while killing client %i.\n",i*10+j);
				    perror("bench: error produced");
				 }

			   fprintf(stdout, "bench: Signals sent to terminals. \n");

                           // Then, wait for all threads to finish
			   for (i =0; i < w; i++) 
			      for (j = 0; j < term; j++)
			         waitpid(pidterm[i*10+j], NULL, 0);
                        }

			fprintf(stdout, "bench: Shutting down terminals OK \n");
			fprintf(stdout, "bench: Shutting down TM... \n");
			fflush(stdout);
			if (pidtm != NULL){
				kill (*pidtm, SIGTERM);
				waitpid(*pidtm, NULL, 0);
			}
			fprintf(stdout, "bench: Shutting down TM OK \n");
			fprintf(stdout, "Shutting down Checkpoints Controller ... \n");
			fflush(stdout);
			if (pidcheck != NULL){
				kill (*pidcheck,SIGTERM); 
				waitpid(*pidcheck, NULL, 0);
			}
			fprintf(stdout, "bench: Shutting down Checkpoints Controller OK \n");
			if (pidvacuum != NULL){
				fprintf(stdout, "Shutting down Vacuums Controller ... ");
				fflush(stdout);
				kill (*pidvacuum,SIGTERM); 
				waitpid(*pidvacuum, NULL, 0);
				fprintf(stdout, "OK \n");
			}
			ftime(&sellohora1);
			if (med != NULL){
				fprintf(med,"Test end timestamp: %d %d .\n",sellohora1.time, sellohora1.millitm);
				fclose(med);
			}
			fprintf(stdout, "OK \n");
			fflush(stdout);
			/* haylogs = 1; */

			/* free pid memory*/
			fprintf(stdout, "bench: Freeing memory ... ");
			if (pidtm != NULL)
				free(pidtm);
			if (pidterm != NULL)
				free(pidterm);
			if (pidcheck != NULL)
				free(pidcheck);
			if (pidvacuum != NULL)
				free(pidvacuum);
			fprintf(stdout, "OK\n");

                        // Closing all semaphore sets
			for (i=1;i<=w;i++) {
			   // First, obtain their semid.
			   if ((semid = semget(i, 10, IPC_CREAT | 0600)) == -1){
			      fprintf(stderr,"bench: Problems while identifying semaphore set %i.\n",i);
			      perror("bench: Error produced");
                              continue;
			   } else
			      // Then close all semaphores in set.
			      if (semctl(semid,0,IPC_RMID,0) == -1) {
			         fprintf(stderr,"bench: Problems while closing semaphore set %i.\n",i);
			         perror("bench: Error produced");
                           }
			}

			fprintf(stdout,"Finished.\n");
			break;

                case 4: /*checking consistency*/
			srv_id = atoi(argv[3]);

                        /* Catching signal SIGINT */
                        if (signal(SIGINT, signal_int2) == SIG_ERR){
                                fprintf(stdout, "Error catching signal sigint. \n");
                                exit(-1);
                        }

                        consistencia();
			fprintf(stdout,"Finished.\n");
                        break;

                case 5: /*delete database*/
			srv_id = atoi(argv[3]);

                        /* Catching signal SIGINT */
                        if (signal(SIGINT, signal_int2) == SIG_ERR){
                                fprintf(stdout, "Error catching signal sigint. \n");
                                exit(-1);
                        }

                        /* Connecting with template1 */
                        EXEC SQL CONNECT TO template1;/* USER USERNAME;*/
                        EXEC SQL SET autocommit = ON;

                        /* Delete database */
                        EXEC SQL DROP DATABASE tpcc;
                        if (sqlca.sqlcode<0){
                                fprintf(stdout, "Error deleting database.\n");
                                fprintf(stdout, "Error type: %d\n", sqlca.sqlcode);
                                fprintf(stdout, "      %s\n", sqlca.sqlerrm.sqlerrmc);
                                /* haybd = 1; */
                        }/*else   haybd = 0;*/

                        /* Disconnecting from template1 */
                        EXEC SQL DISCONNECT;
                        fprintf(stdout,"Database deleted.\n\n");
                        break;

                case 6: /* Show results */
			srv_id = atoi(argv[4]);

                        /* Catching signal SIGINT */
                        if (signal(SIGINT, signal_int2) == SIG_ERR){
                                fprintf(stdout, "Error catching signal sigint. \n");
                                exit(-1);
                        }
                        /* accounting results */
                        lectura_bitacora(argv[2][0], argv[3]);
			fprintf(stdout,"Finished.\n");
                        break;

                case 7: /*checking the database state */
			srv_id = atoi(argv[2]);

                        /* Catching sinal sigint */
                        if (signal(SIGINT, signal_int2) == SIG_ERR){
                                fprintf(stdout, "Error catching signal sigint. \n");
                                exit(-1);
                        }
                        /* Checking state */
                        estado_base();
			fprintf(stdout,"Finished.\n");
                        break;
		
		case 10:/*checking if the log files exists -- has_logs */
			srv_id = atoi(argv[2]);

			strcpy(filenameBuffer,VARDIR);
			strcat(filenameBuffer,"medida.log");
                	haylogs=hay_fich(filenameBuffer);
			fprintf(stdout, "%d\n", haylogs);
			break;

		case 11:/* has_db */
			srv_id = atoi(argv[2]);

			/*  Connecting with template1 */
        		EXEC SQL CONNECT TO template1;/* USER USERNAME;*/
        		if (sqlca.sqlcode<0){
                		printf("Error number: %d\n", sqlca.sqlcode);
                		printf("        %s\n", sqlca.sqlerrm.sqlerrmc);
        		}
        		EXEC SQL SET autocommit = ON;

			/* Trying to create database tpcc */
        		EXEC SQL CREATE DATABASE tpcc;
        		if (sqlca.sqlcode<0){
                		if (sqlca.sqlcode == -400){
                        		/* The database already exists */
                        		haybd = 1;
                        	} else {
                                	fprintf(stdout, "Error checking if database exists.\n");
                                	fprintf(stdout, "Error number: %d\n", sqlca.sqlcode);
                                	fprintf(stdout, "       %s\n", sqlca.sqlerrm.sqlerrmc);
                                	EXEC SQL DISCONNECT;
                                	exit(-1);

                        	}
        		} else {
                		/* Database did not exists */
                		/* Deleting database for creating it after use the menu option */
                		EXEC SQL DROP DATABASE tpcc;
                		haybd = 0;
        		}

			/* disconnecting from template1 */
        		EXEC SQL DISCONNECT;

                	if (signal(SIGTERM, signal_term) == SIG_ERR){
                        	fprintf(stdout, "Error catching signal sigterm.\n");
                        	exit(-1);
                	}
			fprintf(stdout, "%d\n", haybd);
			break;


	}/*switch*/
	}/*while*/

return 0;
} /* main */

void signal_int1(){
/* ------------------------------------------------ *\
|* signal_int1 function                             *|
|* ------------------------------------------------ *|
|* This function handles the signal SIGINT into     *|
|* database restoration and database population     *|
|* processes. When signal is received, the process  *|
|* is stopped by sending it the signal SIGSTOP.     *|
|* Then the user is asked to contunue processing or *|
|* to abort the operation. If the user choose to    *|
|* continue, the signal SIGCONT is sent to process. *|
|* Otherwise, signal SIGTERM is sent.               *|
\* ------------------------------------------------ */

char d = 'N';

	/* catching again signal SIGINT */
	if (signal(SIGINT, signal_int1) == SIG_ERR){
		fprintf(stdout, "Error catching signal sigusr1. \n");
		exit(-1);
	} 

	/* Stopping the child process */
	if (kill(pidh, SIGSTOP) == -1) 
		fprintf(stdout, "Error stopping process.\n");
	fprintf(stdout, "Abort the process? (y/n): ");
	d= getchar(); 
	getchar();

	/* Checking input */
	if ((d == 'Y') || (d == 'y')){

		/*Aborting */
		/*Waking-up process */
		if(kill(pidh, SIGCONT) == -1)
			fprintf(stdout, "Error waking up process \n");

		/*killing child process */
		if(kill(pidh, SIGTERM) == -1)
			fprintf(stdout, "%s\n", strerror(errno));
		terminado = 1;
		return;
	}

	/**/
	/*Waking up child process */
	fprintf(stdout, "continuing... \n");
	if(kill(pidh, SIGCONT) == -1)
		fprintf(stdout, "Error waking up process \n");
}

void signal_int2(){
/* ------------------------------------------------ *\
|* signal_int2 function                             *|
|* ------------------------------------------------ *|
|* Skips the signal SIGINT into database            *|
|* population, database restoration, database       *|
|* consistency check, and results accounting        *|
|* processes.                                       *|
\* ------------------------------------------------ */

	if (signal(SIGINT, signal_int2) == SIG_ERR){
		fprintf(stdout, "Error catching signal sigint. \n");
		exit(-1);
	} 
};

void signal_int3(){
/* ------------------------------------------------ *\
|* signal_int3 function                             *|
|* ------------------------------------------------ *|
|* Esta función enmascara a la señal SIGINT en la   *|
|* etapa de control del test. Cuando se recibe esta *|
|* señal pregunta el usuario si desea terminar con  *|
|* el proceso. En caso afirmativo se fuerza al      *|
|* valor '1' el flag global terminado que controla  *|
|* la ejecución del test, provocando la             *|
|* desactivación de los procesos que intervienen en *|
|* él. Si el usuario decide continuar con el test,  *|
|* el valor del flag no se modifica.                *|
\* ------------------------------------------------ */

char d = 'N';

	if (signal(SIGINT, signal_int3) == SIG_ERR){
		fprintf(stdout, "Error catching signal sigint. \n");
		exit(-1);
	} 
	fprintf(stdout, "Abort the test? (y/n) ");
	d= getchar(); 
	getchar();
	if ((d == 'Y') || (d == 'y')){
		/*abort process*/
		terminado = 1;
	}
};

void signal_usr1(){
/* ------------------------------------------------ *\
|* signal_usr1 function                             *|
|* ------------------------------------------------ *|
|* Esta función enmascara a la señal SIGUSR1 en las *|
|* etapa de control de carga y de restauración.     *|
|* Esta señal la envían los procesos encargados de  *|
|* la ejecución de la etapa correspondiente para    *|
|* indicar a la etapa de control que han terminado  *|
|* su ejecucion.                                    *|
|* Cuando se recibe esta señal se fuerza al valor   *|
|* '1' el flag global terminado que controla la     *|
|* ejecución de la etapa.                           *|
\* ------------------------------------------------ */

	if (signal(SIGUSR1, signal_usr1) == SIG_ERR){
		fprintf(stdout, "Error catching signal sigusr1. \n");
		exit(-1);
	}
	terminado = 1;	
}

void signal_alarm(){
/* ------------------------------------------------ *\
|* signal_alarm function                            *|
|* ------------------------------------------------ *|
|* Handles the signal SIGALRM, that is sent by      *|
|* operating system.                                *|
|* When signal is recieved forces the global flag   *|
|* 'timeout' to '1'. Flag 'timeout' controls the    *|
|* waiting loops of test controller process.        *|
\* ------------------------------------------------ */

	if (signal(SIGALRM, signal_alarm) == SIG_ERR){
		fprintf(stdout, "Error catching signal sigalarm. \n");
		exit(-1);
	}
	timeout = 1;
}

void signal_term(){
/* ------------------------------------------------ *\
|* signal_term function                             *|
|* ------------------------------------------------ *|
|* Handles the signal SIGTERM.                      *|
|* When this signal is recieved, the process that   *|
|* are running currently is forced to end, setting  *|
|* the global flags "terminado" and "salir" to      *|
|* value '1'. This flags controls the waiting loops *|
|* and the main loop of the benchmark.              *|
\* ------------------------------------------------ */

	if (signal(SIGTERM, signal_term) == SIG_ERR){
		fprintf(stdout, "Error catching signal sigterm. \n");
		exit(-1);
	}
	salir = 1;
	terminado =1;
}

void signal_term_exit(){
/* ------------------------------------------------ *\
|* signal_term_exit function                        *|
|* ------------------------------------------------ *|
|* Handles the signal SIGTERM when database is      *|
|* being populated or restored. When signal SIGTERM *|
|* is recieved, disconects from database and exits. *|
\* ------------------------------------------------ */

	EXEC SQL DISCONNECT;
	exit(0);
}

