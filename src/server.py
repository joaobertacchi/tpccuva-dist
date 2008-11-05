#!/usr/bin/python
from SocketServer import ThreadingTCPServer, BaseRequestHandler
from connection import connection
import socket, os, subprocess

class Handler(BaseRequestHandler):
  def setup(self):
    print self.client_address, 'connected!'

  def handle(self):
    conn = connection(self.request)
    msg = conn.recv()


    while len(msg) > 0:
      data_t = eval(msg)
      command = "./bench_srv"
      for i in data_t:
        command += " " + str(i)
        
      print "Running %s..." % command
      pipe_obj = subprocess.Popen(args=command, shell=True,
                                  stdout=subprocess.PIPE)
      bench_srv_out = pipe_obj.stdout

      line = bench_srv_out.readline()
      #msg_snd = ""
      while line:
        print line[:-1]
        conn.send_part(line)
        #msg_snd += line
        line = bench_srv_out.readline()

      conn.end_part()
      #conn.send(msg_snd)

      msg = conn.recv()
    self.request.shutdown(socket.SHUT_WR)
      
  def finish(self):
    print self.client_address, 'disconnected!'


server_addr = (socket.gethostname(), 10200)
s = ThreadingTCPServer(server_addr, Handler)
s.serve_forever()
