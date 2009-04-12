#!/usr/bin/env python

import socket
import random
import time

RECONNECT_TIMEOUT=1

class KeyspaceException(Exception): pass
class ConnectionException(KeyspaceException): pass
class ProtocolException(KeyspaceException): pass

class KeyspaceClient:
	def __init__(self, nodes, timeout = None, trace = False):
		self.nodes = nodes
		self.timeout = timeout
		self.node = None
		self.multi = False
		self.readbuf = ""
		self.id = int(time.time() * 1000)
		self.writeQueue = []
		self.trace = trace
		self._trace("running, id = %d" % self.id)
		self._reconnect()

#####################################################################
#
# Keyspace client interface
# 
#####################################################################

	def getmaster(self):
		msg = self._createMessage("m")
		self._send(msg)
		return self._getValueResponse()

	def get(self, key):
		msg = self._createMessage("g", key)
		self._send(msg)
		return self._getValueResponse()

	def dirtyget(self, key):
		msg = self._createMessage("G", key)
		self._send(msg)
		return self._getValueResponse()

	def list(self, prefix, count):
		msg = self._createMessage("l", prefix, count)
		self._send(msg)
		return self._getListResponse()

	def dirtylist(self, prefix, count):
		msg = self._createMessage("L", prefix, count)
		self._send(msg)
		return self._getListResponse()

	def set(self, key, value):
		msg = self._createMultiMessage("s", key, value)
		self._send(msg)
		return self._getStatusResponse()

	def testandset(self, key, test, value):
		msg = self._createMessage("t", key, test, value)
		self._send(msg)
		return self._getStatusResponse()

	def add(self, key, num):
		msg = self._createMessage("a", key, num)
		self._send(msg)
		return self._getValueResponse()

	def delete(self, key):
		msg = self._createMessage("d", key)
		self._send(msg)
		return self._getStatusResponse()
	
	def submit(self):
		pass
	
	def connectMaster(self):
		while True:
			master = None
			while master == None:
				try:
					master = int(self.getmaster())
					self._trace("master = %s" % str(master))
				except ProtocolException, e:
					self._trace("exception = %s" % str(e))
					master = None
			self._trace("connected to %s, master is %s" % (self.nodes[master], str(master)))
			if self.node == self.nodes[master]:
				self._trace("connected to master, %s" % self.nodes[master])
				return
			
			try:
				self._disconnect()
				self._connectNode(self.nodes[master])
			except socket.error, e:
				self._trace("connection to master failed, master = %d" % master)

#####################################################################
#
# Private functions
# 
#####################################################################

	def _connect(self):
		node = random.choice(self.nodes)
		self._connectNode(node)
	
	def _disconnect(self):
		self.sock.close()
		self.node = None
	
	def _connectNode(self, nodename):
		node = nodename.split(":")
		host = node[0]
		port = int(node[1])
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		if self.timeout:
			self.sock.settimeout(self.timeout)
		self.sock.connect((host, port))
		self.node = nodename
	
	def _reconnect(self):
		while True:
			try:
				self._connect()
				self._trace("connected to %s" % self.node)
				return
			except socket.error, e:
				self._trace("connect failed to %s" % self.node)
				time.sleep(RECONNECT_TIMEOUT)
	
	def _createString(self, s):
		return ":".join((str(len(s)), s))

	def _getNextId(self):
		self.id += 1
		return self.id
	
	def _createMultiMessage(self, command, *args):
		msg = command + ":" + str(self._getNextId())
		for arg in args:
			s = str(arg)
			msg = "".join((msg, ":", self._createString(s)))
		return msg

	def _createMessage(self, command, *args):
		msg = command + ":" + str(self._getNextId())
		for arg in args:
			s = str(arg)
			msg = "".join((msg, ":", self._createString(s)))
		return msg
	
	def _send(self, msg):
		msg = self._createString(msg) + "1:*"
		while True:
			try:
				self._trace("sending: " + msg)
				self.sock.sendall(msg)
				return
			except socket.error, e:
				self._disconnect()
				self._reconnect()
	
	def _readMessage(self):
		colon = self.readbuf.find(":")
		if colon < 0:
			return None
		slength = self.readbuf[:colon]
		length = len(slength) + 1 + int(slength)
		if len(self.readbuf) < length:
			return None
		self._trace("%s" % self.readbuf)
		msg = self.readbuf[:length]
		self.readbuf = self.readbuf[length:]
		return msg
	
	def _read(self):
		try:
			msg = self._readMessage()
			while msg == None:
				data = self.sock.recv(4096)
				self.readbuf += data
				try:
					msg = self._readMessage()
				except Exception, e:
					raise ProtocolException(e.args)
			return msg 
		except Exception, e:
			raise ConnectionException(e.args)


	def _getToken(self, s, pos = 0):
		colon = s.find(":", pos)
		if colon < 0:
			raise ProtocolException("")
		return s[pos:colon], colon + 1


	def _getTokenNum(self, s, pos = 0):
		num, colon = self._getToken(s, pos)
		return int(num), colon
	

	def _getValueResponse(self):
		"""
		MSG_OK 			"o"
		MSG_NOTFOUND	"n"
		MSG_FAILED		"f"
		MSG_LISTEND		"."
		"""
		try:
			msg = self._read()
			self._trace("response = %s" % msg)
			
			token, next = self._getToken(msg)
			token, next = self._getToken(msg, next)

			if token == "n":
				return None
			elif token == "f":
				raise ProtocolException("Failed")
			elif token != 'o':
				raise ProtocolException("Unexpected response")

			id, next = self._getTokenNum(msg, next)
			if int(id) != self.id:
				raise ProtocolException("Invalid id")
			vallen, next = self._getTokenNum(msg, next)
			value = msg[next:]
		
			return value
		except Exception, e:
			raise ProtocolException("Invalid response: %s" % e.args)
	
	
	def _getStatusResponse(self):
		try:
			msg = self._read()
			self._trace("response = %s" % msg)
			token, next = self._getToken(msg)
			token, next = self._getToken(msg, next)
			
			if token == "n":
				return None
			elif token == "f":
				raise ProtocolException("Failed")
			elif token != 'o':
				raise ProtocolException("Unexpected response")

			id = msg[next:]
			if int(id) != self.id:
				raise ProtocolException("Invalid id")
			
			return "Ok"
		except Exception, e:
			raise ProtocolException(e.args)

				
	def _getListResponse(self):
		self.listResponse = []
		try:
			while True:
				msg = self._read()
				token, next = self._getToken(msg)
				token, next = self._getToken(msg, next)

				if token == '.':
					return self.listResponse
			
				if token != 'i':
					raise ProtocolException("expected 'i', got %s" % token)

				id, next = self._getTokenNum(msg, next)
				if int(id) != self.id:
					raise ProtocolException("Invalid id")
				keylen, next = self._getTokenNum(msg, next)
				key = msg[next:]
				self.listResponse.append(key)			
		except Exception, e:
			raise ProtocolException("Invalid response: %s" % e.args)
	

	def _trace(self, msg):
		if not self.trace:
			return
		caller = ""
		import sys
		frame = sys._getframe(1)
		if frame.f_locals.has_key("self"):
			caller = str(frame.f_locals["self"].__class__.__name__) + "."
		caller += frame.f_code.co_name
		logstr = caller + ": " + msg
		print logstr
