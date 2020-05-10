#!/bin/python3.7
import socket
from pathlib import Path

import sys
sys.path.append(str(Path(__file__).resolve().parents[1]))

from protocol.command_pb2 import Command
from agent import Agent


class Controller:
    def __init__(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setblocking(0)
        server_address = ('0', 9999)
        server.bind(server_address)
        server.listen(5)

    def send_command(self, agent, command):
        pass 


'''For testing'''
command = None
with open('placeholder', 'rb') as f:
    command = Command.FromString(f.read())
'''For testing'''

controller = Controller()
agent = Agent('127.1', '1337')

