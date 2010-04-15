import UDP_Plus, bitstring

conn = UDP_Plus.UDP_Plus()

# send sync request to server
conn.connect()

# test bitstring
b = bitstring.Bits(bin='001001111')
print b.uint