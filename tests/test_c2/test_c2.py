import pytest
import socket
from c2 import Controller, entities

class TestController:
    def setup_method(self, monkeypatch):
        self.controller = Controller()
        self.ops_conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.agent_conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.operators = {}
        self.agents = {}


    def test_list_agents(self, monkeypatch):
        self.agents = {hash(self.agent_conn): entities.Agent(self.agent_conn, len(self.agents))}
        self.controller.operators = {hash(self.ops_conn): entities.Operator(self.ops_conn)}
        self.controller._list_agents(self.ops_conn)
        