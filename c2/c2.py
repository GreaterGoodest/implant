#!/usr/bin/env python3
import selectors
import socket
import sys
from pathlib import Path

from entities import Agent, Operator

sys.path.append(str(Path(__file__).resolve().parents[1]))

class Controller:
    """Manages connections & communication between operators and agents

    Attributes:
        interface (str): Interface to listen on
        port (int): Port to listen for operator connections on
        backlog (int): Max connection attempts to have in backlog

    """
    def __init__(self, interface='127.1', port=1337, backlog=5):
        self._selector = selectors.DefaultSelector()
        self._interface = interface
        self._port = port
        self._backlog = backlog

        self.agents = {} #reference agent by hashed connection
        self.agent_ids = {} #reference agent by id
        self.operators = {} #reference operator by hashed connection


    def _list_agents(self, conn):
        '''
        Sends the requesting operator a list of the available agent connections

        Parameters:
            conn: operator connection object
        '''
        operator = self.operators[hash(conn)]
        agents = ""
        for hashed, agent in self.agents.items():
            agents += str(agent)
        operator.data_q.append(agents)


    def _attach_sessions(self, conn, data):
        '''
        Connect the requesting operator to the specified agent ID by setting
        the operator's agent to the requested agent, and
        the agent's operator to the requesting operator.

        Parameters:
            conn: operator connection object
            data: operator request data
        '''
        operator = self.operators[hash(conn)]
        agent_id = int(data[1])
        if agent_id in self.agent_ids:
            agent = self.agent_ids[agent_id]
            operator.agent = agent
            agent.operator = operator
            operator.data_q.append(f"Connected to agent {agent_id}\n")

        else:
            operator.data_q.append("No valid agent found")
        
    
    def _controller_comm(self, conn, data):
        '''
        Catch all function for parsing commands that come from an operator
        who doesn't currently have an associated agent session.
        Commands include:
        [listen, agents, attach]

        Parameters:
            conn: operator connection object
            data: operator request data
        '''
        data = data.split(" ")
        operator = self.operators[hash(conn)]
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
            self._selector.register(agent_sock, selectors.EVENT_READ, self._accept_agent)
        elif data[0] == "agents":
            self._list_agents(conn)
        elif data[0] == "attach":
            self._attach_sessions(conn, data)
        else:
            operator.data_q.append("Unknown controller command.\nOptions include: [listen, agents, attach]\n")


    def _data_agent(self, conn, mask):
        '''
        Handles any data received from an agent session or destined to an agent

        Parameters:
            conn: agent connection object
            mask: bitmask allowing determination of connection status (ready for read/write)
        '''
        agent = self.agents[hash(conn)]
        operator = agent.operator

        if mask & selectors.EVENT_READ:
            data = conn.recv(1024).decode()
            if data and operator:
                operator.data_q.append(data)

            else:
                print("closing", conn)
                self._selector.unregister(conn)
                conn.close()
                self.agent_ids.pop(agent.id)
                self.agents.pop(hash(conn))
                operator.agent = None 

        if mask & selectors.EVENT_WRITE and len(agent.data_q):
            data = agent.data_q.popleft()
            conn.send(data.encode())


    def _data_ops(self, conn, mask):
        '''
        Handles any data received from an operator session or destined to an operator 

        Parameters:
            conn: operator connection object
            mask: bitmask allowing determination of connection status (ready for read/write)
        '''       
        operator = self.operators[hash(conn)]
        agent = operator.agent
        if mask & selectors.EVENT_READ:
            data = conn.recv(1024).decode().rstrip()

            if data:
                if not operator.agent:
                    self._controller_comm(conn, data)
                elif data == "exit":
                    print(f"detaching operator from agent {agent.id}")
                    operator.data_q.append(f"Detaching from agent {agent.id}\n")
                    operator.agent = None
                else:
                    data += '\n'
                    agent.data_q.append(data)
                    
            else:
                print("closing", conn)
                self._selector.unregister(conn)
                self.operators.pop(hash(conn))
                conn.close()
        
        if mask & selectors.EVENT_WRITE and len(operator.data_q):
            data = operator.data_q.popleft()
            conn.send(data.encode())     
        
    
    def _accept_agent(self, sock, mask):
        '''
        Receives incoming agent connections and adds them to the selector

        Parameters:
            sock: listening connection object for handling incoming agents on a specific interface/port      
            mask: read/write ready detection. Not used in this function
        '''
        conn, addr = sock.accept()
        conn.setblocking(False)
        print('accepted agent', conn, 'from', addr)        

        agent = Agent(conn, len(self.agents))
        self.agent_ids[len(self.agents)] = agent
        self.agents[hash(conn)] = agent 

        self._selector.register(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self._data_agent)


    def _accept_ops(self, sock, mask):
        '''
        Receives incoming ops connections and adds them to the selector

        Parameters:
            sock: listening connection object for handling incoming operators 
            mask: read/write ready detection. Not used in this function
        '''

        conn, addr = sock.accept()
        conn.setblocking(False)
        print('accepted ops', conn, 'from', addr)        

        operator = Operator(conn)
        self.operators[hash(conn)] = operator 

        self._selector.register(conn, selectors.EVENT_READ | selectors.EVENT_WRITE, self._data_ops)


    def _setup_server(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.setblocking(False)
        server_address = (self._interface, self._port)
        server.bind(server_address)
        server.listen(self._backlog)
        self._selector.register(server, selectors.EVENT_READ, self._accept_ops)


    def run(self):
        '''
        Initiates controller and begins listening on specified port for ops connections
        Handles selector events
        '''
        self._setup_server()
        while True:
            events = self._selector.select()
            for key, mask in events:
                callback = key.data
                callback(key.fileobj, mask)


if __name__ == "__main__":
    controller = Controller()
    controller.run()