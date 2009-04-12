/*
 * TPCC-UVa Project: Open-source implementation of TPCC-v5
 *
 * check.c: Checkpoint Controller program
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
|*               CHECKPOINT CONTROLLER PROGRAM                      *|
|* -----------------------------------------------------------------*|
|* Forces a checkpoint each 30 minutes in the database used by the  *|
|* benchmark. Checkpoints start and end timestamps are logged into  *|
|* file: VARDIR/check.log.                                          *|
|* ---------------------------------------------------------------- *|
|* Checkpoints are performed connecting to database 'tpcc' and      *|
|* executing the postgreSQL command: CHECKPOINT                     *|
\********************************************************************/

/********************************************************************\
|* Header files.	                                            *|
\* ---------------------------------------------------------------- */
#include "../include/tpcc.h"
#include <stdio.h>
/********************************************************************/

/********************************************************************\
|* Global variables.                                                *|
\* ---------------------------------------------------------------- */
FILE *check;			/* Pointer to checkpoints log file */

struct timeb sello1;		/* Checkpoint start and end  */
struct timeb sello2;		/* timestamps.                */

/* Flow Control flags */
int alarma;			/* modified by "main" and "sig_alarma" functions */
int no_term;			/* modified by "main" and "sig_term" functions */
/********************************************************************/

int
main ()
{
/* ---------------------------------------------- *\
|* Program's Main function                        *|
\* ---------------------------------------------- */

  void sig_alarma ();		/* SIGNALRM handler function */
  void sig_int ();		/* SIGINT handler function */
  void sig_term ();		/* SIGTERM handler function */

/* Connect to database */
  EXEC SQL CONNECT TO tpcc;/* USER USERNAME;*/
/* Set autocommit mode */
  EXEC SQL SET autocommit = ON;

/* Catching signal SIGALRM */
  if (signal (SIGALRM, sig_alarma) == SIG_ERR)
     {
       fprintf (stdout, "Error catching signal SIGALRM\n");
       exit (-1);
     }

/* Catching signal SIGINT */
  if (signal (SIGINT, sig_int) == SIG_ERR)
     {
       fprintf (stdout, "Error catching signal SIGINT.\n");
       exit (-1);
     }

/* Catching signal SIGTERM */
  if (signal (SIGTERM, sig_term) == SIG_ERR)
     {
       fprintf (stdout, "Error catching signal SIGTERM.\n");
       exit (-1);
     }

/* Opening output file */
  strcpy (filenameBuffer, VARDIR);
  strcat (filenameBuffer, "check.log");
  if ((check = fopen (filenameBuffer, "w")) == NULL)
     {
       /* Can't open file */

       fprintf (stdout, "Error has occured while opening output \n");
       fprintf (stdout, "file %s \n", filenameBuffer);
       fprintf (stdout, "%s\n", strerror (errno));
       exit (-1);
     }

  no_term = 1;
  while (no_term)
     {

       /* Doing checkpoint */
       ftime (&sello1);
       EXEC SQL CHECKPOINT;
       ftime (&sello2);

       /* Writing timestamp */
       fprintf (check, "1º sello %d %d 2º sello %d %d \n", sello1.time,
		sello1.millitm, sello2.time, sello2.millitm);
       fflush (check);

       alarm (INT_CHECKPOINT);
       alarma = 0;
       /* Waiting checkpoint interval */
       while (!alarma && no_term)
       	sleep(1);

     }				/* While */

  EXEC SQL DISCONNECT;
  fclose (check);

}

void
sig_alarma ()
{
/* ---------------------------------------------- *\
|* Sig_alarma function.                           *|
|* ---------------------------------------------- *|
|* Function for signal SIGALRM handling.          *|
|* Forces the flag 'alarma' to value '1' and      *|
|* catches again the signal SIGALRM.              *|
\* ---------------------------------------------- */

  if (signal (SIGALRM, sig_alarma) == SIG_ERR)
     {
       fprintf (stdout, "ERROR CATCHING SIGALRM\n");
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
|* Function for signal SIGINT handling.           *|
|* Catches again the signal SIGINT. Skips the     *|
|* keyboard interruption <ctrl-C>.                *|
\* ---------------------------------------------- */

  if (signal (SIGINT, sig_int) == SIG_ERR)
     {
       fprintf (stdout, "ERROR CATCHING SIGINT\n");
       exit (-1);
     }
}

void
sig_term ()
{
/* ---------------------------------------------- *\
|* Función sig_term                               *|
|* ---------------------------------------------- *|
|* Function for signal SIGTERM handling.          *|
|* Forces the flag 'no_term' to value '0' and     *|
|* catches againt the signal SIGTERM.             *|
\* ---------------------------------------------- */

  if (signal (SIGTERM, sig_int) == SIG_ERR)
     {
       fprintf (stdout, "ERROR CATCHING SIGTERM\n");
       exit (-1);
     }
  no_term = 0;
}
