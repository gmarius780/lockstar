from twisted.internet import protocol, reactor, task

class LockStarNameServer(protocol.Protocol):
    
    def __init__(self, name):
        self.name = name
    
    def connectionMade(self):
        self.data = ""
    
    def dataReceived(self, data):
        self.data += str(data)
        
        if "GETNAME" in self.data:
            self.transport.write(self.name.encode('utf-8'))
            self.transport.loseConnection()
            
        
        
class ControlServerFactory(protocol.ServerFactory):
    
    protocol = LockStarNameServer
    
    def __init__(self):
        self.name = "Lockstar"
        
    def buildProtocol(self, addr):
        c = LockStarNameServer(self.name)
        c.factory = self
        return c
    

def GetName(factory):
    fil = open("config.txt", 'r') 
    lines = fil.readlines()
    fil.close()
    factory.name = lines[0].rstrip()
    

    
CSF = ControlServerFactory()
reactor.listenTCP(10785, CSF)

lc = task.LoopingCall(lambda: GetName(CSF))
lc.start(5)

reactor.run()