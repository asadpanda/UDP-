import UDP_Plus

conn = UDP_Plus.UDP_Plus()
conn.bind()

# wait for a client
conn.listen()
