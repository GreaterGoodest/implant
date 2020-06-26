from collections import deque
class Agent:
    def __init__(self, conn, id_no):
        self.id = id_no
        self.conn = conn 
        self.data_q = deque()
        self.operator = None

    def __str__(self):
        return f"id:{self.id} - host:{self.conn.getsockname()[0]} - port:{self.conn.getsockname()[1]}"

class Operator:
    def __init__(self, conn):
        self.conn = conn
        self.data_q = deque() #data for operator
        self.agent = None #current agent context