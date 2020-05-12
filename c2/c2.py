#!/bin/python3.7
import selectors
import socket
from pathlib import Path

import sys
sys.path.append(str(Path(__file__).resolve().parents[1]))

from protocol.command_pb2 import Command
from agent import Agent


class Controller:
    def __init__(self, interface='0', port=9999):
        self.selector = selectors.DefaultSelector()

        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.setblocking(False)
        self.server_address = (interface, port)
        self.server.bind(self.server_address)
        self.server.listen(5)
        self.selector.register(self.server, selectors.EVENT_READ, self.accept)

    
    def type_determination(self, conn, mask):
        '''Determine if operator/agent'''
        try:
            identity = conn.recv(1024).decode().rstrip()
        except BlockingIOError:
            return

        if identity == "agent":
            print('agent')
            self.selector.modify(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self.data_agent)
            
        elif identity == "operator":
            print('ops')
            self.selector.modify(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self.data_ops)

        else:
            print("invalid:", identity)
            self.selector.unregister(conn)
            conn.close()


    def data_agent(self, conn, mask):
        try:
            data = conn.recv(1024)
        except BlockingIOError:
            return

        if data:
            print("agent received", data,"from", conn)
        else:
            print("closing", conn)
            self.selector.unregister(conn)
            conn.close()


    def data_ops(self, conn, mask):
        try:
            data = conn.recv(1024)
        except BlockingIOError:
            return

        if data:
            print("ops received", data,"from", conn)
        else:
            print("closing", conn)
            self.selector.unregister(conn)
            conn.close()


    def accept(self, sock, mask):
        conn, addr = sock.accept()
        print('accepted', conn, 'from', addr)        
        conn.setblocking(False)
        self.selector.register(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self.type_determination)


    def handle_agents(self):
        while True:
            events = self.selector.select()
            for key, mask in events:
                callback = key.data
                callback(key.fileobj, mask)


    def send_command(self, agent, command):
        pass 


'''For testing'''
command = None
with open('placeholder', 'rb') as f:
    command = Command.FromString(f.read())
'''For testing'''

controller = Controller()
controller.handle_agents()
agent = Agent('127.1', '1337')

