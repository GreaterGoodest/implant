from collections import deque
class Agent:
    def __init__(self, conn, id_no):
        self.id = id_no
        self.conn = conn 
        self.data_q = deque()