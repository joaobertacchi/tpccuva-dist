/********************************************************************\
|*               BENCHMARK TPCC-UVA HEADER FILE                     *|
|* -----------------------------------------------------------------*|
|* En esta librer�a se definen las funciones utilizadas para la     *|
|* generaci�n de datos de base de datos y de transacciones          *|
|* ---------------------------------------------------------------- *|
|* Programmed by:                                                   *|
|*	Julio Alberto Hern�ndez Gonzalo.                            *|
|*	Eduardo Hern�ndez Perdiguero.                               *|
|*	Diego R. Llanos                                             *|
\********************************************************************/

#include<stdio.h>
#include<math.h>

#include "../include/tpcc.h"

/********************************************************************\
|*                            FUNCIONES                             *|
\* ---------------------------------------------------------------- */

int
double2int (double f)
{
/* ---------------------------------------------- *\
|* Funci�n que trunca un double convirtiendolo en *|
|* un int                                        *|
|* ---------------------------------------------- *|
|* Par�metro f: n�mero a truncar                  *|
|* ---------------------------------------------- *|
|* Retorna el n�mero truncado tipo int           *|
\* ---------------------------------------------- */

  return (int) floor (f);
}

int
decimal (double f, int pos)
{
/* ---------------------------------------------- *\
|* Funci�n que retorna las parte decimal de un    *|
|* double como un m�mero tipo int.               *|
|* ---------------------------------------------- *|
|* Par�metro f: n�mero del que se desea obtener la*|
|* parte decimal.                                 *|
|* Par�metro pos: posiciones decimales que se     *|
|* desean obtener                                 *|
|* ---------------------------------------------- *|
|* Retorna el int que contiene la parte decilmal *|
\* ---------------------------------------------- */

  return (int) floor ((f - floor (f)) * pow (10, pos));
}

double
aleat_dbl (double inicio, double fin)
{
/* ---------------------------------------------- *\
|* Funci�n que retorna las parte decimal de un    *|
|* double como un m�mero tipo int.               *|
|* ---------------------------------------------- *|
|* Par�metro f: n�mero del que se desea obtener la*|
|* parte decimal.                                 *|
|* Par�metro pos: posiciones decimales que se     *|
|* desean obtener                                 *|
|* ---------------------------------------------- *|
|* Retorna el int que contiene la parte decilmal *|
\* ---------------------------------------------- */

  return inicio + (fin - inicio) * (random () / (double) RAND_MAX);
}

int
aleat_int (int inicio, int fin)
{
/* ---------------------------------------------- *\
|* Funci�n que genera un entero aleat�rio         *|
|* comprendido entre 'inicio' y 'fin' ambos       *|
|* incluidos                                      *|
|* ---------------------------------------------- *|
|* Par�metro inicio: inicio del itervalo de       *|
|* selecci�n                                      *|
|* Par�metro fin: fin del intervalo de selecci�n  *|
|* ---------------------------------------------- *|
|* Retorna el n�mero entero generado              *|
\* ---------------------------------------------- */

  return (inicio +
	  double2int ((double) (fin - inicio) *
		      (random () / (double) RAND_MAX)));
}


char *
getfechahora (char *cad)
{
/* ---------------------------------------------- *\
|* Funci�n que obtiene la fecha y la hora del     *|
|* sistema y la almacena en una cadena de         *|
|* caracteres                                     *|
|* ---------------------------------------------- *|
|* Par�metro cad: cadena de caracteres donde se   *|
|* almacena la la fecha y la hora del sistema.    *|
|* ---------------------------------------------- *|
|* Retorna un puntero a la cadena donde se ha     *|
|* almacenado la fecha y la hora                  *|
\* ---------------------------------------------- */

  time_t t;
  struct tm *tm;

  time (&t);			/*se obtiene la fecha del sistema */
  tm = localtime (&t);
  sprintf (cad, "%02d-%02d-%02d %02d:%02d:%02d\0", 1900 + tm->tm_year,
	   1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  return cad;
}


char *
ctime_sin_salto (time_t * tiempo)
{
/* ---------------------------------------------- *\
|* Funci�n que elimina el salto de l�nea de la    *|
|* salida de c_time.                              *|
\* ---------------------------------------------- */
/* Elimina el salto de linea de la salida ctime */

  char *cad;
  int i;
  cad = ctime (tiempo);
  for (i = 0; cad[i] != '\n'; i++);
  cad[i] = '\0';
  return cad;
}

int
cad_alfa_num (int ini, int fin, char *cadena, char *estado1, char *estado2)
{
/* ------------------------------------------------------ *\
|* Funci�n que genera una cadena de caracteres            *|
|* alfanum�ricos de tama�o comfrendido entre              *|
|* 'ini' y 'fin'                                          *|
|* ------------------------------------------------------ *|
|* Par�metro ini: inicio del entervalo de selecci�n del   *|
|* tama�o                                                 *|
|* Par�metro fin: fin del entervalo de selecci�n del tama�o*|
|* Par�metro cadena: cadena donde se almacenan los        *|
|* caractero aleatorios                                   *|
|* Par�metro estado1: estado para la generaci�n del       *|
|* tama�o de la cadena aleatoria                          *|
|* Par�metro estado2: estado para la generaci�n del       *|
|* de los caracteres aleatorios                           *|
|* ------------------------------------------------------ *|
|* Retorna el tama�o de la cadena generada                *|
\* ------------------------------------------------------ */
  int tam, i;
  char ch;

  /* utilizamos estado1 para el tama�o de la cadena */
  setstate (estado1);
  tam = (int) aleat_int (ini, fin);
  setstate (estado2);
  /* utilizamos estado2 para los caracteres aleatorios (e_global) */

  /*Se generan cada uno de los caracteres hasta completar el */
  /*el tama�o generado                                      */
  for (i = 0; i < tam; i++)
     {
       ch = (char) aleat_int (32, 125);	/*generamos aleatoriamente caracteres. */
       if ((ch >= 32) && (ch <= 125))
	  {			/*s�lo se contemplas caracteres alfanum�ricos   */
	    /*ATENCI�N: si el caracter generado en el 39 (comilla simple ') se escoje el */
	    /*caracter 40. Esto se hace para evitar el error produce postgreSQL al       */
	    /*introducir una comilla simple en un atributo.                              */
	    if (ch == 39)
	      ch++;
	    cadena[i] = ch;
	  }
       else
	  {
	    printf ("ERROR EN LA FUNCION CAD_ALFA_NUM\n");
	    exit (0);
	  }
     }				/* de for */
  cadena[i] = '\0';		/*se termina la cadena con el fin de cadena */
  return tam;			/*se retorna el tama�o de la cadena generada */
}

int
cad_num (int tam, char *cadena, char *estado)
{
/* ------------------------------------------------------- *\
|* Funci�n que genera una cadena de caracteres             *|
|* num�ricos de tama�o 'tam'                               *|
|* ------------------------------------------------------- *|
|* Par�metro tam: tama�o de la cadena de caracteres a      *|
|* generar                                                 *|
|* Par�metro cadena: cadena donde se almacenan los         *|
|* n�meros aleatorios                                      *|
|* Par�metro estado: estado para la generaci�n del tama�o  *|
|* de la cadena aleatoria.                                 *|
|* ------------------------------------------------------- *|
|* Retorna el tama�o de la cadena generada.                *|
\* ------------------------------------------------------- */

  int i;

/* obtener el estado  para generar los caracteres */

  setstate (estado);

  for (i = 0; i < tam; i++)
     {

       cadena[i] = (char) aleat_int (48, 57);	/*generamos aleatoriamente carac. */
     }				/*de for */
  cadena[i] = '\0';
  return tam;
}


char *
crea_clast (int num, char *cadena)
{
/* ------------------------------------------------------- *\
|* Funci�n que genera el campo c_last seg�n la cl�usula    *|
|* 4.3.2.3 del TPC-C. Se concatenan tres cadenas de        *|
|* determinadas por las tres cifras del n�mero 'nnum'      *|
|* ------------------------------------------------------- *|
|* Par�metro num: entero comprendido entre 000 y 999 que   *|
|* cuyas cifras determinan las cadenas a concatenar        *|
|* Par�metro cadena: cadena donde se almacenan las cadenas *|
|* concatenadas.                                           *|
|* ------------------------------------------------------- *|
|* Retorna un puntero a la cadena generada                 *|
\* ------------------------------------------------------- */

  int i, j;
  int v[3];

/**********CADENAS A CONCATENAR*************/
  char bar[] = { 'B', 'A', 'R', '\0' };
  char ougth[] = { 'O', 'U', 'G', 'T', 'H', '\0' };
  char able[] = { 'A', 'B', 'L', 'E', '\0' };
  char pri[] = { 'P', 'R', 'I', '\0' };
  char pres[] = { 'P', 'R', 'E', 'S', '\0' };
  char ese[] = { 'E', 'S', 'E', '\0' };
  char anti[] = { 'A', 'N', 'T', 'I', '\0' };
  char cally[] = { 'C', 'A', 'L', 'L', 'I', '\0' };
  char ation[] = { 'A', 'T', 'I', 'O', 'N', '\0' };
  char eing[] = { 'E', 'I', 'N', 'G', '\0' };
/*Posiciones de las cadenas a concatenar*/
  char *cads[] =
    { bar, ougth, able, pri, pres, ese, anti, cally, ation, eing };

/*a partir de 'num' se obtienen las tres cifras que determinan las cadenas */
/*a concatenar                                                             */
  v[0] = num % 10;
  num = num / 10;
  v[1] = num % 10;
  num = num / 10;
  v[2] = num % 10;
  num = num / 10;

/*Se concatenan las tres cadenas*/
  j = 0;
/*Primera cadena*/
  for (i = 0; cads[v[2]][i] != '\0'; i++)
     {
       cadena[j] = cads[v[2]][i];
       j++;
     }
/*Segunda cadena*/
  for (i = 0; cads[v[1]][i] != '\0'; i++)
     {
       cadena[j] = cads[v[1]][i];
       j++;
     }
/*Tercera cadena*/
  for (i = 0; cads[v[0]][i] != '\0'; i++)
     {
       cadena[j] = cads[v[0]][i];
       j++;
     }
  cadena[j] = '\0';
  return cadena;
}

int
nurand (int A, int x, int y, int C, char *estado1, char *estado2)
{
/* ------------------------------------------------------- *\
|* Funci�n que genera un n�mero aleat�rio con distribuci�n *|
|* no uniforme, seg�n el m�todo definifido en la cl�usula  *|
|* 2.1.6 del TPC-C.                                        *|
|* ------------------------------------------------------- *|
|* Par�metro A: entero que determina el intervalo [0, A]   *|
|* para determinar un n�ero aleat�rio neces�rio la         *|
|* generaci�n del n�emro buscado.                          *|
|* Par�metro x: inicio del itervalo de selecci�n.          *|
|* Par�metro y: fin del itervalo de selecci�n.             *|
|* Par�metro C: constante seleccionada para cada atributo. *|
|* Par�metro estado1: estado para la generaci�n de un      *|
|* n�mero aleatorio seleccionado entre x e y               *|
|* Par�metro estado2: estado para la generaci�n de un      *|
|* n�mero aleatorio seleccionado 0 y A                     *|
|* ------------------------------------------------------- *|
|* Retorna el n�mero generado.                             *|
\* ------------------------------------------------------- */

  int lon1, lon2;


  setstate (estado1);
  lon1 = aleat_int (x, y);
  setstate (estado2);
  lon2 = aleat_int (0, A);

  /*Se obtienen el n�mero seg�n la cl�usula 2.1.6 */
  return (((lon2 | lon1) + C) % (y - x + 1)) + x;
};


void
permutacion_int (int *v, int tam, char *estado)
{
/* ------------------------------------------------------- *\
|* Funci�n que genera una permutaci�n de n�meros aleat�rios*|
|* no uniforme, seg�n el m�todo definifido en la cl�usula  *|
|* 2.1.6 del TPC-C.                                        *|
|* ------------------------------------------------------- *|
|* Par�metro v: puntero a vector de enteros donde se       *|
|* almacenan los caracteres de la premutaci�n.             *|
|* Par�metro tam: tama�o de la permutaci�n                 *|
|* Par�metro estado: estado para la generaci�n de          *|
|* de posiciones aleat�rias en el vector                   *|
\* ------------------------------------------------------- */
  int i, j, z;

  setstate (estado);
  /*las cifras desde 0 hasta 'tam' se colocan en una posici�n aleat�ria del vector */
  for (i = 0; i < tam; i++)
    v[i] = -1;			/*inicializamos el vector */
  for (i = tam; i > 0; i--)
     {
       j = (int) aleat_int (1, i);	/*se selecciona la posici�n aleat�ria */
       z = 0;
       /*reccorremos el vector hasta la posici�n seleccionada para colocar el */
       /*la cifra que toca. */
       while (j > 0)
	  {
	    /*Si la posici�n seleccionada est� ocupada se pasa a la siguiente */
	    if (v[z] == -1)
	      j--;
	    z++;
	  }
       v[z - 1] = i;

     }
}


void
posicion_cartas (int *v, int tam, char *estado)
{

/* ------------------------------------------------------- *\
|* Funci�n que baraja la cartas para la mezcla de          *|
|* transacciones.                                          *|
|* ------------------------------------------------------- *|
|* Par�metro v: puntero a vector de enteros donde se       *|
|* almacenan los identificadores de transacci�n            *|
|* Par�metro tam: tama�o de la baraja de transacci�n       *|
|* Par�metro estado: estado para la generaci�n de          *|
|* de posiciones aleat�rias en el vector                   *|
\* ------------------------------------------------------- */

  int i, j, z;

/*Se genera una permutaci�n de n�mero comprendidos entre 0 y 'tam'*/
  setstate (estado);
/*las cifras desde 0 hasta 'tam' se colocan en una posici�n aleat�ria del vector*/
  for (i = 0; i < tam; i++)
    v[i] = -1;			/*inicializamos el vector */
  for (i = tam - 1; i >= 0; i--)
     {
       j = (int) aleat_int (0, i);
       z = 0;
       /*reccorremos el vector hasta la posici�n seleccionada para colocar el */
       /*la cifra que toca. */
       while (j >= 0)
	  {
	    /*Si la posici�n seleccionada est� ocupada se pasa a la siguiente */
	    if (v[z] == -1)
	      j--;
	    z++;
	  }
       v[z - 1] = i;

     }
}

int
buscar_n_vector (int *v, int tam, int num)
{

/* ------------------------------------------------------- *\
|* Funci�n de b�squeda en un vector ordenado.              *|
|* ------------------------------------------------------- *|
|* Busca el entero 'num' en un vector ordenado de 'tam'    *|
|* posiciones, apuntado por '*v'.                          *|
|* Utiliza el algoritmo de b�squeda bin�ria.               *|
|* ------------------------------------------------------- *|
|* Se retorna la posici�n en la que se ha encontrado o '-1'*|
|* en caso de que no se haya podido encontrar.             *|
\* ------------------------------------------------------- */

  int ini;			/* Inicio y del segmento */
  int fin;			/* del vector considerado */
  int npos;			/* Posicion central del segmento */
  int i;


  ini = 0;
  fin = tam - 1;
  i = 0;

  npos = (ini + fin) / 2;
  while (i < tam)
     {
       if (v[npos] > num)
	  {
	    if (fin != npos)
	       {
		 fin = npos;
		 if (fin == ini + 1)
		    {
		      if (v[ini] == num)
			return ini;
		      else
			 {
			   if (v[fin] == num)
			     return fin;
			   return -1;
			 }
		    }
		 else
		    {
		      npos = (fin + ini) / 2;
		    }		/* if */
	       }
	    else
	       {
		 break;
	       }		/* if */
	  }
       else
	  {
	    if (v[npos] == num)
	       {
		 return npos;
	       }
	    else
	       {
		 if (v[npos] < num)
		    {
		      if (ini != npos)
			 {
			   ini = npos;
			   if (fin == ini + 1)
			      {
				if (v[ini] == num)
				  return ini;
				else
				   {
				     if (v[fin] == num)
				       return fin;
				     return -1;
				   }
			      }
			   else
			      {
				npos = (fin + ini) / 2;
			      }	/* if */
			 }
		      else
			 {
			   break;
			 }	/* if */
		    }		/* if */
	       }		/* if */
	  }			/* if */
       i++;
     }				/* while */
  return -1;
}


void
insert_ord (int *v, int tam, int num)
{
/* ------------------------------------------------------- *\
|* Funci�n de inserci�n en orden.                          *|
|* ------------------------------------------------------- *|
|* Inserta en la posici�n correspondiente el entero 'num'  *|
|* en un vector ordenado de 'tam' posiciones, apuntado     *|
|* por '*v'.                                               *|
\* ------------------------------------------------------- */

  int pos = 0, cont;

  if (tam != 1)
    while (pos < tam)
       {
	 if (v[pos] > num)
	   break;
	 pos++;
       }
  else
    pos = 0;

  if (pos == tam)
     {
       v[tam - 1] = num;
     }
  else
     {
       for (cont = tam - 1; cont > pos; cont--)
	  {
	    v[cont] = v[cont - 1];
	  }
       v[pos] = num;
     }
}


void
aleat_vect (int *v, int tam, int i, int f)
{
/* ------------------------------------------------------- *\
|* Funci�n de generaci�n de n�meros aleat�rios.            *|
|* ------------------------------------------------------- *|
|* Genera un vector de n�mero aleatorios no repetidos      *|
|* comprendidos entre 'i' y 'fin', de 'tam' posiciones,    *|
|*  apuntado por '*v'.                                     *|
\* ------------------------------------------------------- */
  int cont, rnd;

  /*Se genera cada uno de los n�eros de las 'tam' posiciones del vector */
  for (cont = 0; cont < tam; cont++)
     {
       rnd = aleat_int (i, f);	/*Se genera el n�mero correspondiente */
       /*Se mira si el n�mero generado est� ya en el vector */
       if (buscar_n_vector (v, cont + 1, rnd) != -1)
	  {
	    do
	       {		/*se incrementa el n�mero generado hasta encontrar uno que no est� */
		 rnd++;
	       }
	    while (buscar_n_vector (v, cont + 1, rnd) != -1);
	  }
       insert_ord (v, cont + 1, rnd);	/*Se inserta el n�mero encontrado */
     }				/* for */
}

double
resta_tiempos (struct timeb *tAnt, struct timeb *tPost)
{
/* ------------------------------------------------------- *\
|* Funci�n que calcula en segundos la diferencia enrtre    *|
|* dos sellos de hora en                                   *|
|* ------------------------------------------------------- *|
|* Par�metro tAnt: primer sello de hora.                   *|
|* Par�metro tPost: segundo sello de hora                  *|
|* --------------------------------------------------------*|
|* Retorna la diferencia entre los sellos de hora          *|
\* ------------------------------------------------------- */
  return ((double) tPost->time + (double) tPost->millitm * 0.001) -
    ((double) tAnt->time + 0.001 * (double) tAnt->millitm);
}

int
es_entero (char *s)
{
/* ------------------------------------------------------- *\
|* Funci�n que comprueba si la cadena representada por 's' *|
|* representa un n�mero entero. Si lo es retorna 1 y si no *|
|* 0.                                                      *|
\* ------------------------------------------------------- */
/*Se rrecorre la cadena comprobando cada caracter y cuando se encuenta uno */
/*que no es entero se retorna 0.                                           */
  while ((*s != '\0') && (*s != '\n'))
     {
       if (!isdigit (*s))
	 return 0;
       s++;
     }
  return 1;
}

int
es_real (char *s)
{
/* ------------------------------------------------------- *\
|* Funci�n que comprueba si la cadena representada por 's' *|
|* representa un n�mero real. Si lo es retorna 1 y si no 0 *|
\* ------------------------------------------------------- */
/*Se rrecorre la cadena comprobando cada caracter y cuando se encuenta uno */
/*que no es entero ni punto se retorna 0.                                  */
  while ((*s != '\0') && (*s != '\n'))
     {
       if (!isdigit (*s))
	 if (*s != '.')
	   return 0;
       s++;
     }
  return 1;
}

int
es_alfa (char *s)
{
/* ------------------------------------------------------- *\
|* Funci�n que comprueba si la cadena representada por 's' *|
|* es alfanum�rica. Si lo es retorna 1 y si no 0           *|
\* ------------------------------------------------------- */
/*Se rrecorre la cadena comprobando cada caracter y cuando se encuenta uno */
/*que no es alfanum�rico se retorna 0.                                     */
  while ((*s != '\0') && (*s != '\n'))
     {
       if (!isgraph (*s))
	 if (*s != '.')
	   return 0;
       s++;
     }
  return 1;
}
