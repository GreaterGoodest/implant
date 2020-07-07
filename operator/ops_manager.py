#!/usr/bin/env python3
import argparse
import cmd2
import socket
import sys

class OpsManager(cmd2.Cmd):

    connection_args = argparse.ArgumentParser(description="Connect to C2 Server")
    connection_args.add_argument('host', help='hostname/ip of C2 server to connect to')
    connection_args.add_argument('port', help="port C2 is listening on")

    @cmd2.with_argparser(connection_args)
    def do_connect(self, args):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = args.host
        server_port = int(args.port)

        try:
            server_socket.connect((server_address, server_port))
        except:
            self.perror("Failed to connect")
            return

        ops_buffer = ""
        c2_buffer = ""
        while ops_buffer != "disconnect\n"


if __name__ == "__main__":
    app = OpsManager()
    sys.exit(app.cmdloop())
        