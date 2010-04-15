import socket, random

class UDP_Plus():
    
    # contstructor initializes data members
    def __init__(self):
      self.HOST = "127.0.0.1"
      self.PORT = 30000
      self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
      self.data = ""
      self.addr = ""
            
    # binds IP / PORT to receive data
    def bind(self):
      self.sock.bind( (self.HOST, self.PORT) )
      return
    
    # send a message to ip:port defined by class members  
    def send(self, message):
      self.sock.sendto( str(message), (self.HOST, self.PORT) )
      return
    
    # recieve incoming data
    def rcv(self):
      self.data, self.addr = self.sock.recvfrom( 1024 )
      return self.data
    
    # reads 32 bit header data and extracts flags / seq num
    # head is a binary string
    def read_h(self, head):
      ACK = head[0]
      SYN = head[1]
      FIN = head[2]
      OPT = head[3]
      # ignore RESERVED bits for now
      SEQ = int(head[8:], 2)
      return (ACK, SYN, FIN, OPT, SEQ)
      
    # simple method to convert a decimal number to a binary string
    # code from : http://www.daniweb.com/code/snippet216539.html#
    def binary(self, n, digits=16):
      rep = bin(n)[2:]
      return ('0' * (digits - len(rep))) + rep
      
    # connect to listening server
    def connect(self):
      # seed the sequence number with a random
      # number between 0 and 2^16
      seq = self.binary(random.randrange(0,65536))
      self.send("0100000000000000" + seq)
      # wait for syn-ack from server
      # ...
    
    # idle until a syn message is sent from a client
    def listen(self):
      idle = True
      while(idle):
        pckt = self.rcv()
        ack, syn, fin, opt, seq = self.read_h(pckt)
        # respond with syn-ack
        # set seq to new random number
        if syn == '1':
          print "Incoming connection..."
          idle = False
          seq_r = self.binary(random.randrange(0,65536))
          self.send("1100000000000000" + seq_r)
