import UDP_Plus, bitstring

conn = UDP_Plus.UDP_Plus()
conn.bind()

# wait for a client
conn.listen()
