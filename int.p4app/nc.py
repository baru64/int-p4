# simple netcat in python2

import sys
import socket

def netcat(host, port, is_server):
    if is_server:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((host, port))
        s.listen(1)
        conn, _ = s.accept()
        while True:
            data = conn.recv(1024)
            if not data: break
            print data
        conn.close()
    else:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        while True:
            data = raw_input()
            if not data: break
            s.sendall(data)
        s.close()

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print """usage\t\t nc.py TYPE HOST PORT
                 TYPE - 'c' for client, 'l' for server"""
    else:
        if sys.argv[1] == 'c':
            netcat(sys.argv[2], int(sys.argv[3]), False)
        elif sys.argv[1] == 'l':
            netcat(sys.argv[2], int(sys.argv[3]), True)
        else:
            print "Bad option. Use 'l' or 'c'."
