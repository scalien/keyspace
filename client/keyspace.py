# Keyspace database client library
#
# http://scalien.com/keyspace
#
import socket
import random
import time
import select

RECONNECT_TIMEOUT=1

OK = 0
NOTMASTER = -1
FAILED = -2
ERROR = -3

TRACE = False
def trace(msg = ""):
	if not TRACE:
		return
	caller = ""
	import sys
	frame = sys._getframe(1)
	if frame.f_locals.has_key("self"):
		caller = str(frame.f_locals["self"].__class__.__name__) + "."
	caller += frame.f_code.co_name
	ts = time.strftime("%Y-%m-%d %H:%M:%S")
	logstr = ts + ": " + unicode(caller + ": ") + unicode(msg)
	print logstr

def closure(func, *args):
	return lambda: func(*args)


class KeyspaceException(Exception): pass
class ConnectionException(KeyspaceException): pass
class ProtocolException(KeyspaceException): pass
class NotMasterException(KeyspaceException): pass


class Client:
	GETMASTER = "m"
	GET = "g"
	DIRTYGET = "G"
	COUNT = "c"
	DIRTYCOUNT = "C"
	LISTKEYS = "l"
	DIRTYLISTKEYS = "L"
	LISTKEYVALUES = "p"
	DIRTYLISTKEYVALUES = "P"
	SET = "s"
	TESTANDSET = "t"
	ADD = "a"
	DELETE = "d"
	REMOVE = "r"
	RENAME = "e"
	PRUNE = "z"


	def __init__(self, nodes, timeout = 60, trace_ = False):
		global TRACE
		TRACE = trace_
		self.nodes = nodes
		self.conns = []
		self.master = -1
		self.masterTime = 0
		self.timeout = timeout
		self.reconnectTimeout = timeout
		self.result = Client.Result()
		self.eventLoop = Client.EventLoop()
		self.id = int(time.time() * 1000)
		self.sentCommands = {}
		self.safeCommands = []
		self.dirtyCommands = []
		for i in xrange(len(self.nodes)):
			self.conns.append(Client.Connection(self, self.nodes[i], timeout))
		trace("running, id = %d" % self.id)

#####################################################################
#
# Keyspace client interface
# 
#####################################################################

	def state(node):
		return conns[node].state

	def get(self, key, submit = True):
		cmd = self.createCommand(Client.GET, submit, key)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		return self.resultValue()		

	def dirtyget(self, key, submit = True):
		cmd = self.createCommand(Client.DIRTYGET, submit, key)
		self.safeCommands.append(cmd)
		if not submit:
			return None;
		return self.resultValue()		

	def count(self, prefix = "", startKey = "", count = 0, next = 0, backward = False):
		if backward:
			dir = "b"
		else:
			dir = "f"
		cmd = self.createCommand(Client.COUNT, False, prefix, startKey, count, next, dir)
		self.safeCommands.append(cmd)
		return self.resultValue()
	
	def dirtyCount(self, prefix = "", startKey = "", count = 0, next = 0, backward = False):
		if backward:
			dir = "b"
		else:
			dir = "f"
		cmd = self.createCommand(Client.DIRTYCOUNT, False, prefix, startKey, count, next, dir)
		self.dirtyCommands.append(cmd)
		return self.resultValue()

	def listkeys(self, prefix = "", startKey = "", count = 0, next = 0, backward = False):
		if backward:
			dir = "b"
		else:
			dir = "f"
		cmd = self.createCommand(Client.LISTKEYS, False, prefix, startKey, count, next, dir)
		self.safeCommands.append(cmd)
		self.result.close()
		self.loop()
		return self.result.keys(cmd.id)

	def dirtylistkeys(self, prefix = "", startKey = "", count = 0, next = 0, backward = False):
		if backward:
			dir = "b"
		else:
			dir = "f"
		cmd = self.createCommand(Client.DIRTYLISTKEYS, False, prefix, startKey, count, next, dir)
		self.dirtyCommands.append(cmd)
		self.result.close()
		self.loop()
		return self.result.keys(cmd.id)

	def listkeyvalues(self, prefix = "", startKey = "", count = 0, next = 0, backward = False):
		if backward:
			dir = "b"
		else:
			dir = "f"
		cmd = self.createCommand(Client.LISTKEYVALUES, False, prefix, startKey, count, next, dir)
		self.safeCommands.append(cmd)
		self.result.close()
		self.loop()
		return self.result.keyValues(cmd.id)
		
	def dirtylistkeyvalues(self, prefix = "", startKey = "", count = 0, next = 0, backward = False):
		if backward:
			dir = "b"
		else:
			dir = "f"
		cmd = self.createCommand(Client.DIRTYLISTKEYVALUES, False, prefix, startKey, count, next, dir)
		self.dirtyCommands.append(cmd)
		self.result.close()
		self.loop()
		return self.result.keyValues(cmd.id)

	def set(self, key, value = "", submit = True):
		cmd = self.createCommand(Client.SET, submit, key, value)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		return None

	def testandset(self, key, test, value, submit = True):
		cmd = self.createCommand(Client.TESTANDSET, submit, key, test, value)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		return self.result.firstValue()

	def add(self, key, num, submit = True):
		cmd = self.createCommand(Client.ADD, submit, key, num)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		value = self.result.firstValue()
		trace(str(value))
		if value == None or value == "":
			return None
		return int(value)

	def delete(self, key, submit = True):
		cmd = self.createCommand(Client.DELETE, submit, key)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		return None
	
	def remove(self, key, submit = True):
		cmd = self.createCommand(Client.REMOVE, submit, key)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		return self.result.firstValue()
	
	def rename(self, src, dst, submit = True):
		cmd = self.createCommand(Client.RENAME, submit, src, dst)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		return self.result.firstStatus()
		
		
	def prune(self, prefix, submit = True):
		cmd = self.createCommand(Client.PRUNE, submit, prefix)
		self.safeCommands.append(cmd)
		if not submit:
			return None
		
		self.result.close()
		self.loop()
		return None
	
	def begin(self):
		trace()
		if not self.isDone():
			raise ProtocolException

		self.result.close()

		
	def submit(self):
		trace()
		if len(self.safeCommands) > 0:
			cmd = self.safeCommands[-1]
			cmd.submit = True
		else:
			if len(self.dirtyCommands) == 0:
				raise ProtocolException
		
		self.loop()

	def cancel(self):
		self.safeCommands = []
		self.dirtyCommands = []
		self.result.close()
	
	def values(self):
		return self.result.values()

#####################################################################
#
# Private functions
# 
#####################################################################
	
	class Command:
		def __init__(self):
			self.type = 0
			self.nodeId = -1
			self.status = 0
			self.id = 0
			self.submit = False

	class Connection:
		DISCONNECTED = 0
		CONNECTING = 1
		CONNECTED = 2

		def __init__(self, client, node, timeout):
			self.client = client
			self.eventLoop = client.eventLoop
			self.state = Client.Connection.DISCONNECTED
			self.sock = None
			self.node = node
			nodeparts = node.split(":")
			self.host = nodeparts[0]
			self.port = int(nodeparts[1])
			self.timeout = timeout
			self.disconnectTime = 0
			self.getMasterPending = False
			self.getMasterTime = 0
			self.readBuffer = ""
			self.writeBuffer = ""
			self.writing = False
		
		def getMaster(self):
			cmd = self.client.createCommand(Client.GETMASTER, False)
			self.client.sentCommands[cmd.id] = cmd
			self.send(cmd)
			self.getMasterPending = True
			self.getMasterTime = self.eventLoop.now()
		
		def connect(self):
			trace("%s:%d" % (self.host, self.port))
			if self.state == Client.Connection.CONNECTED:
				return
			self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			self.sock.setblocking(0)
			self.sock.connect_ex((self.host, self.port))
			self.eventLoop.registerWrite(self.sock, closure(self.onConnect), closure(self.onClose))
			self.eventLoop.registerTimer(self.timeout, closure(self.onConnectTimeout))
			self.state = Client.Connection.CONNECTING
		
		def onConnect(self):
			try:
				self.sock.getpeername()
			except socket.error, e:
				self.onClose()
				return
			self.state = Client.Connection.CONNECTED
			trace("node = %s, state = %s" % (self.node, str(self.state)))
			self.eventLoop.registerRead(self.sock, closure(self.onRead), closure(self.onClose))

		def onConnectTimeout(self):
			trace()
			

		def onClose(self):
			trace("node = %s" % (self.node))
			self.state = Client.Connection.DISCONNECTED
			self.disconnectTime = self.eventLoop.now()
			# self.client.sentCommands.clear()
			self.sock.close()
			self.eventLoop.close(self.sock)
			self.client.stateFunc()
			
		def onRead(self):
			try:
				data = self.sock.recv(4096)
				trace("node = %s, len(data) = %d" % (self.node, len(data)))
				if len(data) == 0:
					trace("before onClose")
					self.onClose()
					trace("after onClose")
					return

				self.readBuffer += data
				trace(self.readBuffer)
				
				while True:
					msg = self.readMessage()
					trace("node = %s, msg = %s" % (self.node, str(msg)))
					if msg == None:
						break;
					resp = Client.Response()
					if resp.read(msg) != False:
						self.processResponse(resp)
						self.client.stateFunc()
				
				self.eventLoop.registerRead(self.sock, closure(self.onRead), closure(self.onClose))
			except socket.error, e:
				trace(str(e))
			
		def send(self, cmd):
			if cmd.submit:
				self.write(cmd.msg, False)
				self.write("1:*")
			else:
				self.write(cmd.msg)
		
		def write(self, data, flush = True):
			self.writeBuffer += data
			if flush and not self.writing:
				self.eventLoop.registerWrite(self.sock, closure(self.onWrite), closure(self.onClose))
				self.writing = True
			
		def onWrite(self):
			trace("node = %s, writeBuffer = %s" % (self.node, self.writeBuffer))
			nwrite = self.sock.send(self.writeBuffer)
			trace("node = %s, nwrite = %d" % (self.node, nwrite))
			self.writeBuffer = self.writeBuffer[nwrite:]
			if len(self.writeBuffer) == 0:
				self.writing = False
				return
			self.eventLoop.registerWrite(self.sock, closure(self.onWrite), closure(self.onClose))
			
		def removeSentCommand(self, cmd):
			del self.client.sentCommands[cmd.id]
			# TODO read timeout
		
		def processResponse(self, resp):
			if resp.id not in self.client.sentCommands:
				return
			
			cmd = self.client.sentCommands[resp.id]
			cmd.status = resp.status
			if cmd.type == Client.GETMASTER:
				self.getMasterPending = False
				if resp.status == OK:
					trace(resp.value)
					self.client.setMaster(int(resp.value))
				else:
					self.client.setMaster(-1)
				self.removeSentCommand(cmd)
				return 
		
			# with LIST style commands there can be more than one response with
			# the same id
			
			if cmd.type == Client.LISTKEYS or \
			cmd.type == Client.LISTKEYVALUES or \
			cmd.type == Client.DIRTYLISTKEYS or \
			cmd.type == Client.DIRTYLISTKEYVALUES:
				# key.length == 0 means end of the list response
				if resp.key == None:
					self.removeSentCommand(cmd)
					#self.client.result.setStatus(resp.status)
				else:
					self.client.result.appendListResponse(resp)
			else:
				self.removeSentCommand(cmd)
				self.client.result.appendResponse(resp)
	
		def readMessage(self):
			colon = self.readBuffer.find(":")
			if colon < 0:
				return None
			slength = self.readBuffer[:colon]
			length = len(slength) + 1 + int(slength)
			if len(self.readBuffer) < length:
				return None
			# self.trace("%2s" % self.readbuf)
			msg = self.readBuffer[colon+1:length]
			self.readBuffer = self.readBuffer[length:]
			return msg

	class Timer:
		def __init__(self, timeout, callback):
			self.timeout = timeout
			self.callback = callback

	class EventLoop:
		def __init__(self):
			self.rlist = []
			self.wlist = []
			self.xlist = []
			self.timers = []
			self.readCallbacks = {}
			self.writeCallbacks = {}
			self.exCallbacks = {}
			self.timeout = 0

		def registerRead(self, sock, onRead, onClose):
			if not sock in self.readCallbacks:
				self.readCallbacks[sock] = onRead
				self.rlist.append(sock)
			if not sock in self.exCallbacks:
				self.exCallbacks[sock] = onClose
				self.xlist.append(sock)
		
		def registerWrite(self, sock, onRead, onClose):
			if not sock in self.writeCallbacks:
				self.writeCallbacks[sock] = onRead
				self.wlist.append(sock)
			if not sock in self.exCallbacks:
				self.exCallbacks[sock] = onClose
				self.xlist.append(sock)
		
		def registerTimer(self, timeout, onTimeout):
			timer = Client.Timer(timeout, onTimeout)
			self.timers.append(timer)
		
		def close(self, sock):
			for callbacks in [self.readCallbacks, self.writeCallbacks, self.exCallbacks]:
				if sock in callbacks:
					del callbacks[sock]
			for l in [self.rlist, self.wlist, self.xlist]:
				if sock in l:
					l.remove(sock)
		
		def now(self):
			return time.time() * 1000
		
		def compareTimer(self, a, b):
			if a.timeout < b.timeout:
				return -1
			if b.timeout < a.timeout:
				return 1
			return 0
		
		def runOnce(self):
			self.poll(1)
		
		def poll(self, timeout):
			self.timers.sort(self.compareTimer)
			if len(self.timers) > 0:
				timeout = self.timers[0].timeout
				
			startTime = self.now()
			trace("rlist = %d, wlist = %d, xlist = %d" % (len(self.rlist), len(self.wlist), len(self.xlist)))
			rc, wc, ec = select.select(self.rlist, self.wlist, self.xlist, timeout)
			trace("rc = %d, wc = %d, ec = %d" % (len(rc), len(wc), len(ec)))
			endTime = self.now()
			if len(rc) == 0 and len(wc) == 0 and len(ec) == 0:
				# timeout
				return
						
			for sock in rc:
				callback = self.readCallbacks[sock]
				del self.readCallbacks[sock]
				self.rlist.remove(sock)
				callback()
			
			for sock in wc:
				callback = self.writeCallbacks[sock]
				del self.writeCallbacks[sock]
				self.wlist.remove(sock)
				callback()

			for sock in ec:
				callback = self.exCallbacks[sock]
				del self.exCallbacks[sock]
				self.xlist.remove(sock)
				callback()
				
			timers = []
			for timer in self.timers:
				if startTime + timer.timeout < endTime:
					timer.callback()
				else:
					timers.append(timer)
			self.timers = timers
	
	class Response:
		OK = "o"
		FAILED = "f"
		NOTMASTER = "n"
		LIST_ITEM = "i"
		LISTP_ITEM = "j"
		LIST_END = "."
		SEPARATOR = ":"
		
		def __init__(self):
			self.key = ""
			self.value = ""
			self.status = OK
			self.id = 0
			self.data = ""
			self.pos = 0
		
		def read(self, data):
			self.data = data
			self.pos = 0;
			self.status = ERROR
			self.key = None
			self.value = None
			try:
				cmd = self.readChar()
				self.readSeparator()
				self.id = self.readNumber()
				
				if cmd == Client.Response.NOTMASTER:
					self.status = NOTMASTER
					return self.validateLength()
				elif cmd == Client.Response.FAILED:
					self.status = FAILED
					return self.validateLength()
				elif cmd == Client.Response.LIST_END:
					self.status = OK
					return self.validateLength()
				elif cmd == Client.Response.OK:
					self.status = OK
					if self.pos < len(self.data):
						msg = self.readMessage()
						trace(msg)
						if msg != None:
							self.value = msg
					return self.validateLength()
				elif cmd == Client.Response.LIST_ITEM:
					self.status = OK
					msg = self.readMessage()
					if msg != None:
						self.key = msg
					return self.validateLength()
				elif cmd == Client.Response.LISTP_ITEM:
					self.status = OK
					msg = self.readMessage()
					if msg != None:
						self.key = msg
						msg = self.readMessage()
						if msg != None:
							self.value = msg
					return self.validateLength()
					
			except ProtocolException, e:
				# TODO
				print("protocol exception")
				trace(str(e))
				return False
			
		def checkOverflow(self):
			if self.pos >= len(self.data):
				raise ProtocolException
		
		def validateLength(self):
			if self.pos != len(self.data):
				raise ProtocolException
		
		def readChar(self):
			self.checkOverflow()
			c = self.data[self.pos]
			self.pos += 1
			return c
		
		def readSeparator(self):
			self.checkOverflow()
			if self.data[self.pos] != ":":
				raise ProtocolException
			self.pos += 1
		
		def readNumber(self):
			self.checkOverflow()
			num = 0
			rem = len(self.data) - self.pos
			if rem <= 0:
				raise ProtocolException
			while rem > 0 and self.data[self.pos].isdigit():
				num *= 10
				num += int(self.data[self.pos])
				rem -= 1
				self.pos += 1
			if rem > 0 and self.data[self.pos] != ":":
				raise ProtocolException
			return num
		
		def readData(self, num):
			if self.pos - num > len(self.data):
				raise ProtocolException
			
			data = self.data[self.pos:self.pos + num]
			self.pos += num
			return data

		def readMessage(self):
			self.readSeparator()
			num = self.readNumber()
			self.readSeparator()
			return self.readData(num)
		
	# end of inner class Response

	class Result:
		def __init__(self):
			self.responses = {}
			self.startId = 0
		
		def keys(self, id):
			keys = []
			if id not in self.responses:
				return keys
			resp = self.responses[id]
			if isinstance(resp, list):
				for r in resp:
					keys.append(r.key)
			else:
				keys.append(resp.key)
			return keys
		
		def keyValues(self, id):
			keyValues = {}
			if id not in self.responses:
				return keyValues
			resp = self.responses[id]
			if isinstance(resp, list):
				for r in resp:
					keyValues[r.key] = r.value
			else:
				keyValues[resp.key] = resp.value
			return keyValues
		
		def firstValue(self):
			if self.startId not in self.responses:
				return None
			resp = self.responses[self.startId]
			return resp.value
		
		def firstStatus(self):
			if self.startId not in self.responses:
				return None
			resp = self.responses[self.startId]
			return resp.status
		
		def appendResponse(self, resp):
			if self.startId == 0:
				self.startId = resp.id
			self.responses[resp.id] = resp
		
		def appendListResponse(self, resp):
			if self.startId == 0:
				self.startId = resp.id
			if not resp.id in self.responses:
				resps = []
				self.responses[resp.id] = resps
			else:
				resps = self.responses[resp.id]
			resps.append(resp)

		def values(self):
			values = []
			for resp in self.responses.itervalues():
				if isinstance(resp, list):
					for r in resp:
						values.append(r.value)
				else:
					values.append(resp.value)
			return values

		def close(self):
			self.startId = 0
			self.responses.clear()
		
	# end of inner class Result

	def resultValue(self):
		self.result.close()
		self.loop()
		return self.result.firstValue()

	def setMaster(self, master):
		if master >= len(self.conns):
			master = -1
		self.masterTime = self.eventLoop.now()
		self.master = master

	def isDone(self):
		if len(self.safeCommands) == 0 and \
			len(self.dirtyCommands) == 0 and \
			len(self.sentCommands.keys()) == 0:
			return True
		return False

	def loop(self):
		while not self.isDone():
			self.stateFunc()
			self.eventLoop.runOnce()
	
	def stateFunc(self):
		trace("safeCommands: " + str(len(self.safeCommands)))
		trace("dirtyCommands: " + str(len(self.dirtyCommands)))
		trace("sentCommands: " + str(len(self.sentCommands)))
		for conn in self.conns:
			if conn.state == Client.Connection.DISCONNECTED and \
			conn.disconnectTime + self.reconnectTimeout <= self.eventLoop.now():
				conn.connect()
	
		if len(self.safeCommands) > 0 and self.master == -1:
			# TODO in case of master timeout clear safeCommands
			if self.masterTime != 0 and self.masterTime + self.timeout < self.eventLoop.now():
				del self.safeCommands[:]
				self.result.close()
				return

			for conn in self.conns:
				if conn.state == Client.Connection.CONNECTED and \
				not conn.getMasterPending:
					if conn.getMasterTime == 0 or conn.getMasterTime + self.timeout < self.eventLoop.now():
						conn.getMaster()

		if len(self.safeCommands) > 0 and self.master != -1 and \
		self.conns[self.master].state == Client.Connection.CONNECTED:
			self.sendCommand(self.conns[self.master], self.safeCommands)
		
		if len(self.dirtyCommands) > 0:
			# TODO distribute dirty commands
			for conn in self.conns:
				if conn.state == Client.Connection.CONNECTED:
					self.sendCommand(conn, self.dirtyCommands)
					break
		
	def sendCommand(self, conn, commandList):
		cmd = commandList.pop(0)
		self.sentCommands[cmd.id] = cmd
		conn.send(cmd)
				
	def getNextId(self):
		self.id += 1
		return self.id
	
	def createString(self, us):
		bs = us.encode("utf-8")
		return ":".join((str(len(bs)), bs))

	def createCommand(self, command, submit, *args):
		cmd = Client.Command()
		cmd.id = self.getNextId()
		cmd.type = command
		cmd.submit = submit
	
		msg = command + ":" + str(cmd.id)
		for arg in args:
			if isinstance(arg, str):
				us = unicode(arg, "utf-8")
			else:
				us = unicode(arg)
			msg = "".join((msg, ":", self.createString(us)))
		cmd.msg = self.createString(unicode(msg, "utf-8"))
		return cmd
