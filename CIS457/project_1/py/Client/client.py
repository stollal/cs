import socket
import sys
from IPy import IP


class Client():
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = self.new_socket()

    def new_socket(self):
        return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def run(self, filename):
        print('Connecting to server: ', self.host, self.port)
        print('\nSending filename:   ', filename)

        self.send_filename(filename)
        with open(filename, 'wb') as fp:
            print('Checking for response...')

            while True:
                try:
                    packet = self.sock.recv(1024)
                    if packet:
                        num, data = self.get_data(packet)
                        print("Got packet ", num)

                        # Send acknlowedgement to the sender
                        fp.write(data)

                except socket.error as err:
                    pass

            self.sock.close()
            fp.close()

    def send_filename(self, filename):
        print(filename.encode())
        return self.sock.sendto(filename.encode(), (self.host, self.port))

    def get_data(self, packet):
        num = int.from_bytes(packet[0:4], byteorder='little', signed=True)
        return num, packet[4:]


if __name__ == '__main__':
    host = sys.argv[1]

    try:
        IP(host)
    except Exception as e:
        print('Invalid host, exiting')
        sys.exit()

    port = sys.argv[2]

    if len(port) < 4:
        print("Error! Port must be <= 65535 and >= 1024")
        sys.exit()

    filename = input('What file would you like to get from the server?\n')
    if filename is None:
        print("Error, please enter a valid filename")
        sys.exit()

    client = Client(host, int(port))
    client.run(filename)

