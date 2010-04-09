import socket

class UDP_Plus():
    
    # contstructor initializes data members
    def __init__(self):
      self.HOST = "127.0.0.1"
      self.PORT = 30000
      self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
      self.data = ""
      self.addr = ""
    
    # send a message to ip:port defined by class members
    # messages are always sent as strings  
    def send(self, message):
      self.sock.sendto( str(message), (self.HOST, self.PORT) )
    
    # recieve incoming data
    def rcv(self):
      self.data, self.addr = self.sock.recvfrom( 1024 )


def main():
  conn = UDP_Plus()

if __name__ == "__main__":
  main()
