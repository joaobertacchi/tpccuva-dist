
/*
 * TPCC-UVa Project: Open-source implementation of TPCC-v5
 *
 * vacuum.c: Vacuums controller
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
|*                  VACUUMS CONTROLLER PROGRAM                      *|
|* -----------------------------------------------------------------*|
|* Cleans and analizes the benchmark database, periodically.        *| 
|* Vacuums start and end timestamps are logged into file            *|
|* VARDIR/vacuum.log.                                               *|
|* ---------------------------------------------------------------- *|
|* Vacuums are performed connecting to the database 'tpcc' and      *|
|* executing the postgreSQL command: VACUUM ANALYZE                 *|
\********************************************************************/

/********************************************************************\
|* Header Files.                                                    *|
\* ---------------------------------------------------------------- */
#include "../include/tpcc.h"
#include <stdio.h>
/********************************************************************/

/********************************************************************\
|* Global variables.                                                *|
\* ---------------------------------------------------------------- */
FILE *vacu;			/* Vacuums log file */

struct timeb sello1;		/* Vaccums start and end */
struct timeb sello2;		/* timestamps            */

/* Control flags */
int alarma;			/* modified by "main" and "sig_alarma" functions */
int no_term;			/* modified by "main" and "sig_term" funcionts   */
/********************************************************************/

void
leyenda ()
{
/* ---------------------------------------------- *\
|* Leyenda Function                               *|
|* ---------------------------------------------- *|
|* Shows the correct sintax for calling the       *|
|* program.                                       *|
\* ---------------------------------------------- */

  fprintf (stdout, "\nBad call. Use: \n");
  fprintf (stdout, " $ vacuum <ARG1> <ARG2> \n\n");
  fprintf (stdout, "\t<ARG1>: interval between two vacuums (minutes)\n");
  fprintf (stdout, "\t<ARG2>: total number of vaccums \n\n");
}

int
main (int argc, char *argv[])
{
/* ----------------------------------------------- *\
|* Program's main function.                        *|
|* ----------------------------------------------- *|
|* Argumment 1= interval between vacuums (minutes) *|
|* Argumment 2= total number of vacuums.           *|
\* ----------------------------------------------- */

  int int_vacuum;		/* Interval between vacuums */
  int num_vacuum;		/* Total number of vacuums */

  void sig_alarma ();		/* Function for SIGALRM handling */
  void sig_int ();		/* Function for SIGINT handling  */
  void sig_term ();		/* Function for SIGTERM handling */

  if (argc != 3)
     {
       /*  */

       leyenda ();
       exit (-1);
     }
  else if (!es_entero (argv[1]) || !es_entero (argv[2]))
     {
       /* El formato de los parametros no es correcto */

       leyenda ();
       exit (-1);
     }

  int_vacuum = atoi (argv[1]);
  num_vacuum = atoi (argv[2]);

/* Connect to database */
  EXEC SQL CONNECT TO tpcc USER USERNAME;
/* Autocommit mode */
  EXEC SQL SET autocommit = ON;

/* Catching signal SIGALRM */
  if (signal (SIGALRM, sig_alarma) == SIG_ERR)
     {
       printf ("Error catching SIGALRM\n");
       exit (-1);
     }

/* Catching signal SIGINT */
  if (signal (SIGINT, sig_int) == SIG_ERR)
     {
       fprintf (stdout, "Error catching SIGINT.\n");
       exit (-1);
     }

/* Catching signal SIGTERM */
  if (signal (SIGTERM, sig_term) == SIG_ERR)
     {
       fprintf (stdout, "Error catching SIGTERM.\n");
       exit (-1);
     }

/* Openning output file */
  strcpy (filenameBuffer, VARDIR);
  strcat (filenameBuffer, "vacuum.log");
  if ((vacu = fopen (filenameBuffer, "w")) == NULL)
     {
       /* Error opening file */

       fprintf (stdout, "Error has occured while opening output \n");
       fprintf (stdout, "file %s \n", filenameBuffer);
       fprintf (stdout, "%s\n", strerror (errno));
       exit (-1);
     }

  int_vacuum = int_vacuum * 60;	/* converting interval into seconds */
  no_term = 1;
  while (no_term)
     {
       if (num_vacuum >= 0)
	  {
	    /* There is still vacuums to do */
	    alarm (int_vacuum);
	    alarma = 0;

	    /* Waiting interval */
	    while (!alarma && no_term);

	    if (no_term)
	       {
		 /* Executing vacuum */
		 ftime (&sello1);
		 EXEC SQL VACUUM ANALYZE;
		 ftime (&sello2);

		 /* Writing timestamps */
		 fprintf (vacu, "1º sello %d %d 2º sello %d %d \n",
			  sello1.time, sello1.millitm, sello2.time,
			  sello2.millitm);

		 fflush (vacu);
	       }		/* if */

	    if (num_vacuum > 0)
	       {
		 num_vacuum--;
		 if (num_vacuum == 0)
		   /* No more vacuums to do */
		   break;
	       }		/* if */
	  }			/* if */
     }				/* while */

  /* Waiting for signal SIGTERM */
  while (no_term);
  EXEC SQL DISCONNECT;
  fclose (vacu);
}				/* main */

void
sig_alarma ()
{
/* ---------------------------------------------- *\
|* Sig_alarma function                            *|
|* ---------------------------------------------- *|
|* Function for the signal SIGARLM handling.      *|
|* Forces the flag 'alarma' to value '1' and      *|
|* catches again the signal SIGALRM.              *|
\* ---------------------------------------------- */

  if (signal (SIGALRM, sig_alarma) == SIG_ERR)
     {
       printf ("ERROR CATCHING SIGALRM\n");
       exit (-1);
     }
  alarma = 1;
}

void
sig_int ()
{
/* ---------------------------------------------- *\
|* Sig_int function.                              *|
|* ---------------------------------------------- *|
|* Function for the signal SIGINT handling.       *|
|* Catches the signal SIGINT. Skips the keyboard  *|
|* interruption <ctrl-C>.                         *|
\* ---------------------------------------------- */

  if (signal (SIGINT, sig_int) == SIG_ERR)
     {
       printf ("ERROR CATCHING SIGINT\n");
       exit (-1);
     }
}

void
sig_term ()
{
/* ---------------------------------------------- *\
|* Sig_term function.                             *|
|* ---------------------------------------------- *|
|* Function for the signal SIGTERM handling.      *|
|* Forces the flag 'no_term' to value '0' and     *|
|* catches again the signal SIGTERM.              *|
\* ---------------------------------------------- */

  if (signal (SIGTERM, sig_term) == SIG_ERR)
     {
       printf ("ERROR CATCHING SIGTERM\n");
       exit (-1);
     }
  no_term = 0;
}
