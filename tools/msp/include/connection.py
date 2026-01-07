import socket


class Connection:
    def __init__(self):
        self.target_ip = "localhost"
        self.target_port = 12345

        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client.settimeout(5.0)
        self.client.connect((self.target_ip, self.target_port))

    def __del__(self):
        self.client.close()

    def transact(self, payload):
        try:
            self.client.sendall(payload)
            response = self.client.recv(4096)
        except Exception as e:
            print(e)
            response = b"0"
        return response
