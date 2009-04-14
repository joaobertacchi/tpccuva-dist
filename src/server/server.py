from threading import Thread, Lock
from Queue import Queue
from connection import connection, ConnectionClosedException
import socket, os, signal

class server(Thread):
  def __init__(self, addr, id):
    self.__lock = Lock()
    self.sock = socket.SocketType()
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
      print "Shuting down connection %d %s..." % (self.id, self.addr)
      if self.connected:
        self.disconnect()
        os.kill(os.getpid(), signal.SIGUSR1)

class server_grp:
  def __init__(self, nodes):
    self.nodes = nodes # List of addresses tuples
    self.servers = [] # List of server objects
    id = 0 # Server id starts from zero
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
    if not executed:
      print "No server available. Exiting..."
      sys.exit(0)

    for s in self.servers:
      if s.connected and return_output:
        tmp = s.get_answer()
	ret.append(tmp)

    if return_output:
      return ret

  def __len__(self):
    return len(self.servers)
