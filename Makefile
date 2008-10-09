
# Configuration variables 
# =======================

# If you change anything of these, please run "make; make install" to 
# apply the changes to the code.

# VARDIR stores where the auxiliary files will be stored during the
# benchmark execution. Do not forget the last slash!
VARDIR = $(HOME)/tpcc-uva/var/tpcc/

# EXECDIR stores where the executable files will be stored during the
# installation. Do not forget the last slash!
EXECDIR = $(HOME)/tpcc-uva/bin/

# INCLUDEPATH stores the location of PostgreSQL include files.
export INCLUDEPATH = $(HOME)/tpcc-uva/pgsql/include/

# LIBSPATH stores the location of PostgreSQL library files.
export LIBSPATH = $(HOME)/tpcc-uva/pgsql/lib/

# USERNAME stores the user name under which PostgreSQL will run. The
# most common options are to run PostgreSQL in our user account or to 
# run PostgreSQL under user "postgres". By default we choose the first
# option: modify USERNAME if you choose another one.
export BENCH_USERNAME = $(USER)

# Do not touch anything after this point.
# ---------------------------------------------------------------------

EXECS = bench clien tm check vacuum

all: Makefile 
	cd include; ./configure-variables $(VARDIR) $(EXECDIR) 
	cd src; make

install: all
	install -d $(VARDIR)
	install -d $(EXECDIR)
	cd bin; install $(EXECS) $(EXECDIR)


uninstall:
	rm -rf $(EXECDIR)check $(EXECDIR)tm $(EXECDIR)/clien 
	rm -rf $(EXECDIR)bench $(EXECDIR)vacuum $(VARDIR) 

clean:
	rm -f include/tpcc.h
	cd src; make clean
	cd bin; rm -rf $(EXECS) 

