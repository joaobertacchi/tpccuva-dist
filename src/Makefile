
CLIBS = -lecpg -lpq -lm
EXECS = clien tm bench check vacuum

all: $(EXECS) Makefile
	cp $(EXECS) ../bin

# Building the client --------------------------------------------------------

clien: clitran.o extra.o 
	gcc -o clien clitran.o extra.o -lm

clitran.o: clitran.c ../include/tpcc.h
	gcc -c clitran.c 

# Building the transaction monitor -------------------------------------------

tm: tm.o
	gcc -o tm tm.o extra.o -lm -I $(INCLUDEPATH) -L $(LIBSPATH) $(CLIBS)

tm.o: tm.c ../include/tpcc.h
	ecpg tm.c -o tt.c -D USERNAME=$(BENCH_USERNAME) 
	gcc -c tt.c -I $(INCLUDEPATH) -L $(LIBSPATH) 
	mv tt.o tm.o

# Building the benchmark main program ----------------------------------------

bench: bench.o
	gcc -o bench bench.o extra.o -lm -I $(INCLUDEPATH) -L $(LIBSPATH) $(CLIBS)

bench.o: bench.c ../include/tpcc.h
	ecpg bench.c -o tt.c -D USERNAME=$(BENCH_USERNAME) 
	gcc -c  tt.c -I $(INCLUDEPATH) -L $(LIBSPATH) 
	mv tt.o bench.o

# Building the check ---------------------------------------------------------

check: check.o
	gcc -o check check.o extra.o -lm -I $(INCLUDEPATH) -L $(LIBSPATH) $(CLIBS)

check.o: check.c ../include/tpcc.h
	ecpg check.c -o tt.c -D USERNAME=$(BENCH_USERNAME) 
	gcc -c tt.c -I $(INCLUDEPATH) -L $(LIBSPATH) 
	mv tt.o check.o

# Building the vacuum controller ----------------------------------------------

vacuum: vacuum.o
	gcc -o vacuum vacuum.o extra.o -lm -I $(INCLUDEPATH) -L $(LIBSPATH) $(CLIBS)

vacuum.o: vacuum.c ../include/tpcc.h
	ecpg vacuum.c -o tt.c -D USERNAME=$(BENCH_USERNAME) 
	gcc -c tt.c -I $(INCLUDEPATH) -L $(LIBSPATH) 
	mv tt.o vacuum.o


# Extra functions ------------------------------------------------------------

extra.o: extra.c ../include/tpcc.h
	gcc -c extra.c -I $(INCLUDEPATH) -L $(LIBSPATH) 


# Clean ----------------------------------------------------------------------

clean:
	- rm -rf *.o tt.c
	- rm -rf $(EXECS)
	- cd ../bin; rm -rf $(EXECS)

