from collections import deque

class Entity:
    def __init__(self, conn):
        self.conn = conn
        self.data_q = deque()
        

class Agent(Entity):
    def __init__(self, conn, id_no):
        super().__init__(conn)
        self.id = id_no
        self.operator = None #current controlling operator 


    def __str__(self):
        return f"id:{self.id} - host:{self.conn.getsockname()[0]} - port:{self.conn.getsockname()[1]}\n"


class Operator(Entity):
    def __init__(self, conn):
        super().__init__(conn)
        self.agent = None #current agent context