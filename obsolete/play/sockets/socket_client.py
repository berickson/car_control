''' example socket client '''
import sys
import socket
import time
import socket_common as c

def run(args):
    ''' run client '''
    per_second = 0 # 0 means fast as possible
    total_to_send = 100000
    if len(args) >= 2:
        per_second = int(args[1])
    if len(args) >= 2:
        total_to_send = int(args[2])
    connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connection.connect(('localhost', c.port))
    count = 0
    sleep_time = 1. / per_second if per_second > 0 else 0
    while count < total_to_send:
        count += 1
        print("sending: ", count);
        connection.send((str(count)+"\x00").encode())
        recv_string = connection.recv(200).decode("utf-8")
        print(recv_string+".")
        if sleep_time > 0:
            time.sleep(sleep_time)
    connection.close()

if __name__ == '__main__':
    run(sys.argv)
