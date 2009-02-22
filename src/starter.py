#!/usr/bin/python
from socket import SocketType
from time import sleep
import socket, sys, signal, os
from connection import connection, ConnectionClosedException
from threading import Thread, Lock
from Queue import Queue

class DatabaseStatusError(RuntimeError):
  pass

def get_raw_input(message):
  ans = None
  while ans == None:
    try:
      ans = raw_input(message)
    except EOFError:
      ans = None

  return ans

# Obtaining test parameters
def runtest_menu(NUM_MAX_ALM):
  ch = 'n'
  while (ch == 'n'):
    w = input("->Enter number of warehouses ")
    term = input("->Enter number of terminals per warehouse (max. 10) ")
    int_ramp = input("->Enter ramp-up period (minutes) ")
    int_ramp *= 60
    int_med = input("->Enter measurement interval (minutes) ")
    int_med *= 60
    
    # Checking entered data
    if ((w <= 0) or (w > NUM_MAX_ALM) or (int_ramp <= 0) or (term <= 0) or (term > 10) or (int_med <= 0)):
      print "Error:-> wrong parameters"
      ch = 'n'
    else:
      ch = 'y'

  # Asking for continue
  ch = raw_input("Continue? (y/n): ")
  if ((ch != 'y') and (ch != 'Y')): return ()

  # Vacuums controller configuration
  vacuum=0
  dec_vacuum = raw_input("->Do you want to perform vacuums during the test? (y/n): ")

  if (dec_vacuum=='Y' or dec_vacuum=='y'):
    int_vacuum = 0
    while (int_vacuum <= 0):
      int_vacuum = input("        ->Enter interval between vacuums (minutes): ")
      if (int_vacuum <= 0): print "             ->Interval not valid."

    dec_n_vacuum = raw_input("->Do you want to specify the maximum number of vacuums? (y/n): ")
    if (dec_n_vacuum=='Y' or dec_n_vacuum=='y'):
      num_vacuum = -1
      while (num_vacuum < 0):
        num_vacuum = input("        ->Enter the maximum number of vacuums: ")
	if num_vacuum < 0: print "              ->Value not valid."
    else: num_vacuum = 0

    if (dec_n_vacuum=='Y' or dec_n_vacuum=='y'):
    	str_tmp = ", to reach a maximum number of %d." % num_vacuum
    else: str_tmp = "."

    print "->Performing vacuums every %d minutes%s" % (int_vacuum, str_tmp)
    
  else:
    int_vacuum = 0
    num_vacuum = 0


  # Asking for continue
  ch = raw_input("Start the test? (y/n): ")
  return (ch, w, term, int_ramp, int_med, vacuum, dec_vacuum, int_vacuum, num_vacuum)

def createdb_menu(NUM_MAX_ALM):
  # Asking for the number of warehouses
  w = input("Enter the number of warehouses: ")

  # confirming the input
  ch = raw_input("Continue? (y/n): ")
  if ((ch != 'y') and (ch != 'Y')): return ()

  # Checking the number of warehouses entered
  if ((w > NUM_MAX_ALM) or (w <= 0)):
    print "The number of warehouses must be between 1 and %d" % NUM_MAX_ALM

  return (ch,w)

def consistency_menu():
  ch = raw_input("Continue? (y/n): ")
  if ((ch != 'y') and (ch != 'Y')): return ()
  return (ch,)

def restore_menu():
  # confirming operation
  ch = raw_input("Continue? (y/n): ")
  if ((ch != 'y') and (ch != 'Y')): return ()
  return (ch,)

def delete_menu():
  ch = raw_input("Delete database? (y/n): ")
  if ((ch != 'y') and (ch != 'Y')):
    print "Database not modified."
    return ()
  return (ch,)

def result_menu():
  ch = raw_input("Do yo want to store the results into a file? (y/n): ")
  if (ch == 'y' or ch == 'Y'):
    print "   Enter file name (prefereably with .tpc extension)"
    name = raw_input("   filename: ")
    return (ch, name)
  return ()

def check_data(servers):
  # Find out has_db and has_logs values
  msg_list = servers.execute_all(10)
  #print msg_list
  try:
    if len(msg_list) > 1:
      for msg in msg_list[1:]:
        if msg != msg_list[0]: raise DatabaseStatusError
    else: msg = msg_list[0]

    has_logs = int(msg)
  except RuntimeError:
    print "msg should be an integer but it isn't"
    sys.exit(-1)
  except DatabaseStatusError:
    print "Log files are different: %s" % msg_list
    sys.exit(-1)
  except ValueError:
    print "Datatabase unreachable! Is it started?"
    sys.exit(-1)

  msg_list = servers.execute_all(11)
  #print msg_list
  try:
    if len(msg_list) > 1:
      for msg in msg_list[1:]:
        if msg != msg_list[0]: raise DatabaseStatusError
    else: msg = msg_list[0]
 
    has_db = int(msg)
  except RuntimeError:
    print msg
    sys.exit(-1)
  except DatabaseStatusError:
    print "Some servers doesn't have a database: %s" % msg_list
    sys.exit(-1)
  except ValueError:
    print "Datatabase unreachable! Is it started?"
    sys.exit(-1)

  return (has_db, has_logs)

def main_menu(has_bd, has_logs):
# ------------------------------------------------ #
# menu function.                                   #
# ------------------------------------------------ #
# <has_db> if has value 1 indicates that the       #
# database exists. If has value 0  the database    #
# not exists.                                      #
# <has_logs> if has value 1 indicates that the     #
# log files has been created. If has value 0 they  #
# has not been created.                            #
# ------------------------------------------------ #
# Shows on stdout a menu screen with the allowed   #
# options depending of the <has_db> and <has_logs> #
# values. In addition, gets from the user the      #
# selected option and returns its identificator.   #
# ------------------------------------------------ #

	print "\n\n\n\n    +---------------------------------------------------+"
	print "    | BENCHMARK TPCC  --- UNIVERSIDAD DE VALLADOLID --- |"
	print "    +---------------------------------------------------+\n\n\n\n"
	if (has_db):
		print "	2.- RESTORE EXISTING DATABASE."
		print "	3.- RUN THE TEST."
		print "	4.- CHECK DATABASE CONSISTENCY."
		print "	5.- DELETE DATABASE."
		if (has_logs):
			print "	6.- SHOW AND STORE TEST RESULTS"
		else:
			print
		print "	7.- CHECK DATABASE STATE."
		
	else:
		print "	1.- CREATE A NEW TEST DATABASE*"
		print "\n\n\n"
		if (has_logs):
			print "	6.- SHOW AND STORE TEST RESULTS*"
		else:
			print
	
	print "\n\n	8.- Quit\n\n\n\n\n"
	print "* - Not working"
	while True:
		try:
			option = get_raw_input("SELECT OPTION: ")
		except EOFError:
			option = None
		
		if option.isdigit():
			option = int(option)
			if option == 1 and not has_db:
				return option
			elif option >= 2  and option <= 5 and has_db:
				return option
			elif option == 6 and has_logs:
				return option
			elif option ==7 and has_db:
				return option
			elif option == 8:
				return option

def send_command(data_str, sock):
  data_size = len(data_str)
  if data_size > 999: raise Exception
  sock.sendall("%3d" % data_size + data_str)

# Returns a list of tuples.
# Comments in config file are ignored.
# [("<ip_addr>", <port_num>), ...]
def read_config():
  nodes = []
  config_file = open("nodes.conf", "r")
  for line in config_file.readlines():
    if line.strip()[0] != "#":
      tmp = line.split()
      nodes.append((tmp[0], int(tmp[1])))

  config_file.close()
  return nodes

class server(Thread):
  def __init__(self, addr, id):
    self.__lock = Lock()
    self.sock = SocketType()
    self.addr = addr
    self.connected = False
    self.id = id
    self.ans_queue = Queue()

    # Says if we should print output of executed command on the screen
    self.show_output = True

    # Says if we have to put the answer of executed command in self.ans_queue
    self.return_output = False

    Thread.__init__(self)

  # TODO: fix mutex
  def execute(self, cmd, data, show_output, return_output):
    self.__lock.acquire()
    self.show_output = show_output
    self.return_output = return_output
    #msg = str((cmd,) + data)
    msg = str((self.return_output, cmd) + data + (self.id,)) # Append server id in the end
    self.conn.send(msg)

  def get_answer(self):
    buffer = self.ans_queue.get()
    return buffer

  def connect(self):
    try:
      self.sock.connect(self.addr)
      self.conn = connection(self.sock)
      print "Connected to %s:%d." % self.addr
      self.connected = True
      self.start()
    except:
      print "Couldn't connect to %s:%d." % self.addr

  def disconnect(self):
    self.sock.shutdown(socket.SHUT_RDWR)
    self.sock.close()
    self.__lock.acquire()
    self.connected = False

  # TODO: fix mutex
  def run(self):
    try:
      while self.connected:
        tmp = self.conn.recv_part()
        buffer = ""
        while tmp != "":
          if self.show_output: print tmp[:-1]
          if self.return_output: buffer += tmp
          tmp = self.conn.recv_part()

        if self.return_output: self.ans_queue.put(buffer)
        self.__lock.release()
    except ConnectionClosedException:
      print "\nConnection closed by server. Shuting down connection %d %s..." % (self.id, self.addr)
      self.disconnect()
      os.kill(os.getpid(), signal.SIGUSR1)

class server_grp:
  def __init__(self, nodes):
    self.nodes = nodes # List of addresses tuples
    self.servers = [] # List of server objects
    id = 0
    for node in nodes:
      self.servers.append(server(node, id))
      id += 1


  def connect(self):
    success = True
    for s in self.servers:
      if not s.connected:
        s.connect()
        if not s.connected: success = False

    return success

  def disconnect(self):
    for s in self.servers:
      if s.connected:
        s.disconnect()
  
  def execute_all(self, cmd, data=(), show_output=False, return_output=True):
    executed = False
    ret = []

    for s in self.servers:
      if s.connected:
        s.execute(cmd, data, show_output, return_output)
	executed = True
	print "Executing command in server %d" % s.id

    # If no server have executed the command, let's finish the program.
    if not executed: sys.exit(0)

    for s in self.servers:
      if s.connected and return_output:
        tmp = s.get_answer()
	ret.append(tmp)

    if return_output:
      return ret

  def __len__(self):
    return len(self.servers)

def connection_closed(signum, frame):
  some_connected = False
  for s in servers.servers:
    if s.connected == True:
      some_connected = True
      break
  if not some_connected:
    print "There is no server available anymore. Exiting..."
    servers.disconnect()
    sys.exit(0)


try:
  nodes = read_config()
except IOError:
  print "Problem reading config file: nodes.conf"
  sys.exit(-1)

servers = server_grp(nodes)
tries = 1
signal.signal(signal.SIGUSR1, connection_closed)
try:
  while not servers.connect():
    if tries > 3:
      print "It's not possible to connect to all the servers.\nExiting."
      raise RuntimeException
    sleep(3)
    tries += 1


  NUM_MAX_ALM = 10
  has_db = 1
  has_logs = 1

  (has_db, has_logs) = check_data(servers)

  cmd = main_menu(has_db, has_logs)
# *** Parameters passed ***
#  1: (ch,w)
#  2: (ch,)
#  3: (ch, w, term, int_ramp, int_med, vacuum, dec_vacuum, int_vacuum, num_vacuum)
#  4: (ch,)
#  5: (ch,)
#  6: (ch, name)
#  7: ()
# 10: ()
# 11: ()
# ############################
  while(cmd != 8):
    run = False
    if cmd == 1:
      params = createdb_menu(NUM_MAX_ALM)
      if params != () and (params[0] == 'y' or params[0] == 'Y'):
        # Connect to server and make them to create the database
        run = True

    elif cmd == 2:
      params = restore_menu()
      if params != (): run = True

    elif cmd == 3:
      params = runtest_menu(NUM_MAX_ALM)
      if params != ():
        run = True
        if params[1] > len(servers):
          print "\nNumber of warehouses (%d) bigger than servers available (%d)!\nABORTING!!!\n\n" % (params[1], len(servers))
          run = False
  
    elif cmd == 4:
      params = consistency_menu()
      if params != (): run = True
  
    elif cmd == 5:
      params = delete_menu()
      if params != (): run = True

    elif cmd == 6:
      params = result_menu()
      if params != (): run = True
  
    elif cmd == 7:
      # Just run without asking anything
      params = ()
      run = True
  
    if run: servers.execute_all(cmd, params, False, False)
  
    (has_db, has_logs) = check_data(servers)
    cmd = main_menu(has_db, has_logs)

except ConnectionClosedException:
  pass

servers.disconnect()
