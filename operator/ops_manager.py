#!/usr/bin/env python3
import argparse
import cmd2
import sys

class OpsManager(cmd2.Cmd):

    connection_args = argparse.ArgumentParser(description="Connect to C2 Server")
    connection_args.add_argument('host', help='hostname/ip of C2 server to connect to')
    connection_args.add_argument('port', help="port C2 is listening on")
    
    @cmd2.with_argparser(connection_args)
    def do_connect(self, args):
        pass


if __name__ == "__main__":
    app = OpsManager()
    sys.exit(app.cmdloop())
        