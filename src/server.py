#!/usr/bin/python
from SocketServer import ThreadingTCPServer, BaseRequestHandler
from connection import connection, ConnectionClosedException
import socket, os, subprocess, sys

class Handler(BaseRequestHandler):
  def setup(self):
    print self.client_address, 'connected!'

  def handle(self):
    conn = connection(self.request)

    try:
      msg = conn.recv()

      while len(msg) > 0:
        data_t = eval(msg)
        command = "./bench_srv"
        return_output = data_t[0] # First parameter says if the output of execution need to be sent back
        for i in data_t[1:]: # Excludes the first parameter
          command += " " + str(i)
          
        print "Running %s..." % command
        pipe_obj = subprocess.Popen(args=command, shell=True,
                                    stdout=subprocess.PIPE)
        bench_srv_out = pipe_obj.stdout

        ans = ""
        line = bench_srv_out.readline()
        while line:
          print line[:-1]
	  if return_output: ans += line
          #conn.send_part(line)
          line = bench_srv_out.readline()

        #conn.end_part()
        if return_output: conn.send(ans)
        else: conn.end_part()

        msg = conn.recv()
    except ConnectionClosedException:
      pass

    self.request.shutdown(socket.SHUT_WR)
      
  def finish(self):
    print self.client_address, 'disconnected!'

def getMyIP():
    import socket
    hostaddr = "127.0.0.1"
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect(("google.com",80))
        hostaddr = s.getsockname()[0]
	s.close()
    except:
    	print "google.com is unreachable. Are you connectec to internet?"
	print "Using %s (localhost)" % hostaddr
    return hostaddr


server_addr = (getMyIP(), int(sys.argv[1])) # The first parameter is the port
print "Listening to connections on " + str(server_addr) + " address."
s = ThreadingTCPServer(server_addr, Handler)
try:
  s.serve_forever()
except KeyboardInterrupt:
  import sys
  print "Exiting..."
  s.server_close()
  sys.exit(0)
