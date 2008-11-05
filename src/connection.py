class connection:
  def __init__(self, sock):
    self.sock = sock
    self.header_size = 32

  def __recv_header(self):
    header = self.sock.recv(self.header_size)
    while len(header) > 0 and len(header) < self.header_size:
        header += self.sock.recv(self.header_size - len(header))
    return header

  def __recv_msg(self, msg_size):
    msg = ""
    if msg_size > 0:
      tmp = self.sock.recv(msg_size)
      msg += tmp
      while len(tmp) > 0 and len(msg) < msg_size:
        tmp = self.sock.recv(msg_size - len(msg))
        msg += tmp

    return msg


  def recv(self):
    msg = ""
    header = self.__recv_header()

    if len(header) == 0: msg_size = 0
    else: msg_size = int(header)

    while msg_size > 0:
      msg += self.__recv_msg(msg_size)

      header = self.__recv_header()
      if len(header) == 0: msg_size = 0
      else: msg_size = int(header)

    return msg

  def recv_part(self):
    header = self.__recv_header()

    if len(header) == 0: msg_size = 0
    else: msg_size = int(header)

    return self.__recv_msg(msg_size)

  def send(self, msg):
    msg_full = "%32d" % len(msg)
    msg_full += msg
    msg_full += "%32d" % 0
    self.sock.sendall(msg_full)

  def send_part(self, msg):
    msg_full = "%32d" % len(msg)
    msg_full += msg
    self.sock.sendall(msg_full)

  def end_part(self):
    msg_full = "%32d" % 0
    self.sock.sendall(msg_full)
