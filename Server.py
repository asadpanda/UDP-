import socket
 
UDP_IP="127.0.0.1"
UDP_PORT=5005
MESSAGE=1
 
print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
 
sock = socket.socket( socket.AF_INET, # Internet
                       socket.SOCK_DGRAM ) # UDP

while True:
  if MESSAGE < 5000:
    sock.sendto( str(MESSAGE), (UDP_IP, UDP_PORT) )
    MESSAGE = MESSAGE + 1