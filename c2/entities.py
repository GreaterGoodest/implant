from collections import deque
class Agent:
    def __init__(self, conn, id_no):
        self.id = id_no
        self.conn = conn 
        self.data_q = deque()

class Operator:
    def __init__(self, conn):
        self.conn = conn
        self.data_q = deque() #data for operator
        self.agent = None #current agent context