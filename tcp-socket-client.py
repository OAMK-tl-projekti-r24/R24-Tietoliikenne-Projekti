import socket

ip = "172.20.241.9"
port = 20000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.connect((ip,port))
print("yhdistetty")
s.sendall(b'24\n')

text = []

while True:
    data = s.recv(1024)
    if len(data) == 0:
        break
    text.append(data)

dataf = b"".join(text).decode("utf-8")

print(dataf)

with open("tct_sc_data", "w", encoding="utf-8") as f:
    f.write(dataf)



