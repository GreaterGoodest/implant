import socket
from c2 import Controller, entities

class TestController:
    def setup_method(self):
        self.controller = Controller()
        self.ops_conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.agent_conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.operators = {}
        self.agents = {}


    def test_list_agents(self, monkeypatch, capsys):
        test_agent = entities.Agent(self.agent_conn, len(self.agents))
        test_operator = entities.Operator(self.ops_conn)
        assert len(test_operator.data_q) == 0 #there shouldn't be any messages in q at this point

        monkeypatch.setitem(self.controller.operators, hash(self.ops_conn), test_operator)
        monkeypatch.setitem(self.controller.agents, hash(self.agent_conn), test_agent)
        self.controller._list_agents(self.ops_conn)
        assert len(test_operator.data_q) > 0
        assert test_operator.data_q[0] == 'id:0 - host:0.0.0.0 - port:0\n' #agent list should be ready for operator
        