#!/bin/python3.8
import selectors
import socket
from pathlib import Path

import sys
sys.path.append(str(Path(__file__).resolve().parents[1]))

from protocol.command_pb2 import Command
from entities import Agent, Operator


class Controller:
    def __init__(self, interface='127.1', port=1337, backlog=5):
        self.selector = selectors.DefaultSelector()

        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.setblocking(False)
        self.server_address = (interface, port)
        self.server.bind(self.server_address)
        self.server.listen(backlog)
        self.selector.register(self.server, selectors.EVENT_READ, self.__accept_ops)

        self.agents = {}
        self.operators = {}


    def __list_agents(self, conn):
        operator = self.operators[hash(conn)]
        agents = ""
        for hashed, agent in self.agents.items():
            agent_bytes = f"{agent.id}, {agent.conn.getpeername()[0]}, {agent.conn.getpeername()[1]}\n"
            agents += agent_bytes
        operator.data_q.append(agents)
        
    
    def __controller_comm(self, conn, data):
        data = data.split(" ")
        if data[0] == "listen" and len(data) == 3:
            ip = data[1]
            port = int(data[2])
            print(f"listening on {ip} {port}")
            agent_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            agent_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            agent_sock.setblocking(False)
            agent_address = (ip, port)
            agent_sock.bind(agent_address)
            agent_sock.listen(1)
            self.selector.register(agent_sock, selectors.EVENT_READ, self.__accept_agent)
        elif data[0] == "agents":
            self.__list_agents(conn)


    def __data_agent(self, conn, mask):
        try:
            data = conn.recv(1024).decode().rstrip()
        except BlockingIOError:
            return

        if data:
            print("agent received", data, "from", conn)
        else:
            print("closing", conn)
            self.selector.unregister(conn)
            conn.close()


    def __data_ops(self, conn, mask):
        
        operator = self.operators[hash(conn)]
        if mask & selectors.EVENT_READ:
            data = conn.recv(1024).decode().rstrip()

            if data:
                print("ops received", data,"from", conn)
                if not operator.agent:
                    self.__controller_comm(conn, data)
                else:
                    self.ops_to_agent(conn, data)
            else:
                print("closing", conn)
                self.selector.unregister(conn)
                conn.close()
        
        elif mask & selectors.EVENT_WRITE and len(operator.data_q):
            data = operator.data_q.popleft()
            conn.send(data.encode())     
        

    
    def __accept_agent(self, sock, mask):
        conn, addr = sock.accept()
        print('accepted agent', conn, 'from', addr)        
        agent = Agent(conn, len(self.agents))
        self.agents[hash(conn)] = agent 
        conn.setblocking(False)
        self.selector.register(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self.__data_agent)


    def __accept_ops(self, sock, mask):
        conn, addr = sock.accept()
        print('accepted ops', conn, 'from', addr)        

        operator = Operator(conn)
        self.operators[hash(conn)] = operator 
        conn.setblocking(False)
        self.selector.register(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self.__data_ops)


    def handle_agents(self):
        while True:
            events = self.selector.select()
            for key, mask in events:
                callback = key.data
                callback(key.fileobj, mask)


    def __ops_to_agent(self, agent, command):
        pass 


    def __receive_data(self, agent):
        pass


'''For testing'''
command = None
with open('placeholder', 'rb') as f:
    command = Command.FromString(f.read())
'''For testing'''

controller = Controller()
controller.handle_agents()

