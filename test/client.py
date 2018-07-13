import socket
import os
import time

# Made by Leni
# Latest Update: Monday. July. 02nd. 2018

HOST = '61.39.114.202'
RECORDER_PORT = 5555

RECODER_ADDR = (HOST, RECORDER_PORT)

FILE_HEADER_SIZE = 44
FILE_READ_SIZE = 8192
BUF_SIZE = 1024


file_name = ''


def make_user_dir(client_port):
    if not os.path.exists('__user_audio/{}'.format(client_port)):
        os.system('mkdir __user_audio/{}'.format(client_port))


if __name__ == "__main__":

    while True:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        print('\n 1. 안녕하세요')
        print(' 2. 병원 놀이 하자')
        print(' 3. 그만 할래')
        choice = input(' >> ')

        if choice == '1':
            file_name = 'hi.raw'
        elif choice == '2':
            file_name = 'play.raw'
        elif choice == '3':
            file_name = 'stop.raw'
        else:
            file_name = 'hi.raw'

        client.connect(RECODER_ADDR)
        client_port = client.getsockname()[1]
        print('I am "{}"'.format(client_port))
        make_user_dir(client_port)

        f = open('__file/{}'.format(file_name), 'rb')
        client.send('rec'.encode())

        # Send recording pcm data
        while True:
            data = f.readline()
            # print("data type >> {}".format(data))

            if len(data) == 0:
                client.send('end'.encode())
                break
            client.send(data)
        print("Sending End")
        f.close()

        time.sleep(2)

        client.send('tel'.encode())

        file_size = client.recv(BUF_SIZE)
        print("file size >> {}".format(file_size))

        client.send('spk'.encode())

        f = open('__user_audio/{}/in.wav'.format(client_port), 'wb')
        header = client.recv(BUF_SIZE)
        f.write(header)

        client.send('ack'.encode())

        while True:
            data = client.recv(FILE_READ_SIZE)
            # print("data size >> {}".format(len(data)))
            # print("data >> {}".format(data))
            if data[-3:] == b'end':
                # print('close')
                f.write(data[:-3])
                f.close()
                break
            f.write(data)
            client.send('ack'.encode())

        client.close()






