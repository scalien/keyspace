<?php

define(KEYSPACE_OK, 0);
define(KEYSPACE_NOTMASTER, -1);
define(KEYSPACE_FAILED, -2);
define(KEYSPACE_ERROR, -3);

class KeyspaceException extends Exception 
{
	public $errmsg;
	function __construct($errmsg = "") { $this->errmsg = $errmsg; }
}

class KeyspaceConnectionException extends KeyspaceException 
{                                        
	function __construct($errmsg = "") { parent::__construct($errmsg); }
}

class KeyspaceProtocolException extends KeyspaceException
{
	function __construct($errmsg = "") { parent::__construct($errmsg); }
}

class KeyspaceNotMasterException extends KeyspaceException 
{
	function __construct($errmsg = "") { parent::__construct($errmsg); }
}

class KeyspaceClient
{
	// public interface
	public function __construct(array $nodes, $timeout = 0, $trace = false)
	{
		global $_KEYSPACE_TRACE;
		$_KEYSPACE_TRACE = $trace;

		$this->nodes = $nodes;
		$this->conns = array();
		$this->master = -1;
		$this->masterTime = 0;
		$this->timeout = $timeout;
		$this->reconnectTimeout = $timeout;
		$this->result = new KeyspaceResult();
		$this->eventLoop = new KeyspaceEventLoop();
		$this->id = intval(floor(microtime(true) * 1000));
		$this->safeCommands = array();
		$this->dirtyCommands = array();
		$this->sentCommands = array();

		foreach ($nodes as $node)
			$this->conns[] = new KeyspaceConnection($this, $node, $timeout);		
	}
	
	public function state($node)
	{
		return $this->nodes[$node]->state;
	}
	
	public function get($key, $submit = TRUE)
	{
		$cmd = $this->createCommand(self::GET, $submit, $key);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return NULL;
		return $this->resultValue();
	}
	
	public function dirtyget($key)
	{
		$cmd = $this->createCommand(self::DIRTYGET, $submit, $key);
		$this->dirtyCommands[] = $cmd;
		if (!$submit)
			return NULL;
		return $this->resultValue();
	}
	
	public function listkeys($prefix = "", $startKey = "", $count = 0, $next = 0, $forward = TRUE)
	{                                                                                  
		if ($forward)
			$dir = "f";
		else
			$dir = "b";

		$cmd = $this->createCommand(self::LISTKEYS, FALSE, $prefix, $startKey, $count, $next, $dir);
		$this->safeCommands[] = $cmd;
		$this->result->close();
		$this->loop();
		return $this->result->keys($cmd->id);
	}
	
	public function dirtylistkeys($prefix = "", $startKey = "", $count = 0, $next = 0, $forward = TRUE)
	{                                                                                  
		if ($forward)
			$dir = "f";
		else
			$dir = "b";

		$cmd = $this->createCommand(self::DIRTYLISTKEYS, FALSE, $prefix, $startKey, $count, $next, $dir);
		$this->dirtyCommands[] = $cmd;
		$this->result->close();
		$this->loop();
		return $this->result->keys($cmd->id);
	}

	public function listkeyvalues($prefix = "", $startKey = "", $count = 0, $next = 0, $forward = TRUE)
	{                                                                                  
		if ($forward)
			$dir = "f";
		else
			$dir = "b";

		$cmd = $this->createCommand(self::LISTKEYVALUES, FALSE, $prefix, $startKey, $count, $next, $dir);
		$this->safeCommands[] = $cmd;
		$this->result->close();
		$this->loop();
		return $this->result->keyValues($cmd->id);
	}
	
	public function dirtylistkeyvalues($prefix = "", $startKey = "", $count = 0, $next = 0, $forward = TRUE)
	{                                                                                  
		if ($forward)
			$dir = "f";
		else
			$dir = "b";

		$cmd = $this->createCommand(self::DIRTYLISTKEYVALUES, FALSE, $prefix, $startKey, $count, $next, $dir);
		$this->dirtyCommands[] = $cmd;
		$this->result->close();
		$this->loop();
		return $this->result->keyValues($cmd->id);
	}
	
	public function count($prefix = "", $startKey = "", $count = 0, $next = 0, $forward = TRUE)
	{                                                                                  
		if ($forward)
			$dir = "f";
		else
			$dir = "b";

		$cmd = $this->createCommand(self::COUNT, FALSE, $prefix, $startKey, $count, $next, $dir);
		$this->safeCommands[] = $cmd;
		return $this->resultValue();
	}
	
	public function dirtycount($prefix = "", $startKey = "", $count = 0, $next = 0, $forward = TRUE)
	{                                                                                  
		if ($forward)
			$dir = "f";
		else
			$dir = "b";

		$cmd = $this->createCommand(self::DIRTYCOUNT, FALSE, $prefix, $startKey, $count, $next, $dir);
		$this->dirtyCommands[] = $cmd;
		return $this->resultValue();
	}                       
	
	public function set($key, $value, $submit = TRUE)
	{                               
		$cmd = $this->createCommand(self::SET, $submit, $key, $value);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		   
		$this->result->close();
		$this->loop();
	}
	
	public function testandset($key, $test, $value, $submit = TRUE)
	{
		$cmd = $this->createCommand(self::TESTANDSET, $submit, $key, $value);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		   
		return $this->resultValue();
	}
	
	public function add($key, $num, $submit = TRUE)
	{
		$cmd = $this->createCommand(self::ADD, $submit, $key, $num);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		
		return $this->resultValue();
	}
	
	public function delete($key, $submit = TRUE)
	{
		$cmd = $this->createCommand(self::DELETE, $submit, $key);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		   
		$this->result->close();
		$this->loop();    
	}         
	
	public function remove($key, $submit = TRUE)
	{
		$cmd = $this->createCommand(self::REMOVE, $submit, $key);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		   
		return $this->resultValue();
	}
	
	public function rename($src, $dst, $submit = TRUE)
	{
		$cmd = $this->createCommand(self::RENAME, $submit, $src, $dst);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		
		$this->result->close();
		$this->loop();
	}
	
	public function prune($prefix = "", $submit = TRUE)
	{
		$cmd = $this->createCommand(self::PRUNE, $submit, $prefix);
		$this->safeCommands[] = $cmd;
		if (!$submit)
			return;
		
		$this->result->close();
		$this->loop();
	}
	
	public function begin()
	{
		$this->result->close();
	}                          
	
	public function submit()
	{
		$safeCount = count($this->safeCommands);
		if ($safeCount > 0)
		{
			$cmd = $this->safeCommands[$safeCount - 1];
			$cmd->submit = TRUE;
		}
		
		$this->loop();
	}
	
	public function cancel()
	{
		$this->safeCommands = array();
		$this->dirtyCommands = array();
		$this->result->close();
	}
	
	public function values()
	{
		return $this->result->values();
	}
	
/////////////////////////////////////////////////////////////////////
//
// implementation specific parts
//
/////////////////////////////////////////////////////////////////////

	const GETMASTER = "m";
	const GET = "g";
	const DIRTYGET = "G";
	const LISTKEYS = "l";
	const DIRTYLISTKEYS = "L";
	const LISTKEYVALUES = "p";
	const DIRTYLISTKEYVALUES = "P";
	const SET = "s";
	const TESTANDSET = "t";
	const ADD = "a";
	const DELETE = "d";
	const REMOVE = "r";
	const RENAME = "e";
	const PRUNE = "z";
	const COUNT = "c";
	const DIRTYCOUNT = "C";

	private $nodes;
	private $conns;
	private $master;
	private $timeout;
	private $reconnectTimeout;
	public $result;
	private $id;
	private $safeCommands;
	private $dirtyCommands;
	public $sentCommands;
	public $eventLoop;
	
	private function getNextId()
	{
		return ++$this->id;
	}
	
	private function createString($s)
	{
		return strval(strlen($s)) . ":" . $s;
	}

	public function createCommand($command, $submit)
	{
		$cmd = new KeyspaceCommand();
		$cmd->id = $this->getNextId();
		$cmd->type = $command;
		$cmd->submit = $submit;
		
		$msg = $command . ":" . $cmd->id;
		for ($i = 2; $i < func_num_args(); $i++)
		{
			$msg .= ":" . $this->createString(func_get_arg($i));
		}
		
		$cmd->msg = $this->createString($msg);
		return $cmd;
	}
	
	private function resultValue()
	{
		$this->result->close();
		$this->loop();
		return $this->result->firstValue();
	}                                      
	
	public function setMaster($master)
	{
		if ($master >= count($this->conns))
			$master = -1;
		$this->masterTime = $this->eventLoop->now();
		$this->master = $master;
	}
	
	private function isDone()
	{
		_trace("safeCommands " .count($this->safeCommands));
		_trace("dirtyCommands " .count($this->dirtyCommands));
		_trace("sentCommands " .count($this->sentCommands));
		if (count($this->safeCommands) == 0 &&
			count($this->dirtyCommands) == 0 &&
			count(array_keys($this->sentCommands)) == 0)
		{
			return TRUE;
		}
		return FALSE;
	}
	
	private function loop()
	{
		while (!$this->isDone())
		{
			$this->stateFunc();
			$this->eventLoop->runOnce();
		}
	}
	
	public function stateFunc()
	{
		foreach ($this->conns as $conn)
		{
			if ($conn->state == KeyspaceConnection::DISCONNECTED &&
				$conn->disconnectTime + $this->reconnectTimeout <= $this->eventLoop->now())
			{
				$conn->connect();
			}
		}

		if (count($this->safeCommands) > 0 && $this->master == -1)
		{
			if ($this->masterTime != 0 &&
				$this->masterTime + $this->timeout < $this->eventLoop->now())
			{				
				$this->safeCommands = array();
				$this->result->close();
				return;
			}
			                         
			foreach ($this->conns as $conn)
			{
				if ($conn->state == KeyspaceConnection::CONNECTED &&
					$conn->getMasterPending == FALSE)
				{
					if ($conn->getMasterTime == 0 ||
						$conn->getMasterTime + $this->timeout < $this->eventLoop->now())
					{
						$conn->getMaster();
					}
				}
			}
		}
		
		if (count($this->safeCommands) > 0 && 
			$this->master != -1 &&
			$this->conns[$this->master]->state == KeyspaceConnection::CONNECTED)
		{
			$this->sendCommand($this->conns[$this->master], $this->safeCommands);
		}
		
		if (count($this->dirtyCommands) > 0)
		{
			foreach ($this->conns as $conn)
		    {
				if ($conn->state == KeyspaceConnection::CONNECTED)
				{
					$this->sendCommand($conn, $this->dirtyCommands);
					break;
				}
			}
		}
	}
	
	private function sendCommand(&$conn, &$commands)
	{
		$cmd = array_shift($commands);
		$this->sentCommands[$cmd->id] = $cmd;
		$conn->send($cmd);
	}
}

class KeyspaceConnection
{
	public $client;
	public $eventLoop;
	public $state;
	public $node;
	public $host;
	public $port;
	public $timeout;
	public $disconnectTime;
	public $getMasterPending;
	public $getMasterTime;
	public $readBuffer;
	public $writeBuffer;
	public $writing;
	public $socket;
	
	const DISCONNECTED = 0;
	const CONNECTING = 1;
	const CONNECTED = 2;
	
	public function __construct($client, $node, $timeout)
	{
		$this->client = $client;
		$this->eventLoop = $client->eventLoop;
		$this->state = self::DISCONNECTED;
		$this->node = $node;
		$nodeparts = explode(":", $node);
		$this->host = $nodeparts[0];
		$this->port = $nodeparts[1];
		$this->timeout = $timeout;
		$this->disconnectTime = 0;
		$this->getMasterPending = FALSE;
		$this->getMasterTime = 0;
		$this->readBuffer = "";
		$this->writeBuffer = "";
		$this->writing = FALSE;
	}

	public function getMaster()
	{
		$cmd = $this->client->createCommand(KeyspaceClient::GETMASTER, FALSE);
		$this->client->sentCommands[$cmd->id] = $cmd;
		$this->send($cmd);
		$this->getMasterPending = TRUE;
		$this->getMasterTime = $this->eventLoop->now();
	}
	
	private function method($name)
	{
		return new KeyspaceCallback($this, $name);
	}

	public function connect()
	{
		if ($this->state == self::CONNECTED)
			return;
		
		$this->socket = @socket_create(AF_INET, SOCK_STREAM, getprotobyname("tcp"));
		@socket_set_nonblock($this->socket);
		@socket_connect($this->socket, $this->host, $this->port);
		$this->eventLoop->registerWrite($this->socket, $this->method("onConnect"), $this->method("onClose"));
		$this->eventLoop->registerTimer($this->timeout, $this->method("onConnectTimeout"));
		$this->state = self::CONNECTING;
	}

	public function onConnect()
	{
		_trace("onConnect");
		$this->state = self::CONNECTED;
		$this->eventLoop->registerRead($this->socket, $this->method("onRead"), $this->method("onClose"));
	}

	public function onConnectTimeout()
	{
		// TODO
	}
	
	public function onClose()
	{
		$this->state = self::DISCONNECTED;
		$this->disconnectTime = $this->eventLoop->now();
		$this->client->sentCommands = array();
		@socket_close($this->socket);
		$this->eventLoop->close($this->socket);
		$this->client->stateFunc();
	}
	
	public function onRead()
	{
		$data = "";
		$name = NULL;
		$ret = @socket_recvfrom($this->socket, $data, 4096, 0, $name, $name);
		_trace("ret = $ret, data = $data");
		if ($ret <= 0)
		{
			// TODO
			$this->onClose();
			return;
		}
		
		$this->readBuffer .= $data;
		while (TRUE)
		{
			$msg = $this->readMessage();
			_trace("msg = $msg");
			if ($msg == NULL)
				break;
		    $resp = new KeyspaceResponse();
			if ($resp->read($msg) != FALSE)
			{
				$this->processResponse($resp);
				$this->client->stateFunc();
			}
		}
		$this->eventLoop->registerRead($this->socket, $this->method("onRead"), $this->method("onClose"));
	}
	
	public function send($cmd)
	{
		if ($cmd->submit)
		{
			$this->write($cmd->msg, FALSE);
			$this->write("1:*");
		}
		else
			$this->write($cmd->msg);
	}
	
	public function write($data, $flush = TRUE)
	{
		_trace("data = $data");
		$this->writeBuffer .= $data;
		if ($flush && !$this->writing)
		{
			$this->eventLoop->registerWrite($this->socket, $this->method("onWrite"), $this->method("onClose"));
			$this->writing = TRUE;
		}
	}
	
	
	public function onWrite()
	{
		$nwrite = @socket_write($this->socket, $this->writeBuffer);
		_trace("nwrite = $nwrite");
		$this->writeBuffer = substr($this->writeBuffer, $nwrite);
		if (strlen($this->writeBuffer) == 0)
		{
			$this->writing = FALSE;
			return;
		}                     

		$this->eventLoop->registerWrite($this->socket, $this->method("onWrite"), $this->method("onClose"));		
	}
	 
	protected function removeSentCommand($cmd)
	{
		unset($this->client->sentCommands[$cmd->id]);
	}   

	protected function processResponse($resp)
	{
		if (!array_key_exists($resp->id, $this->client->sentCommands))
			return;

		$cmd = $this->client->sentCommands[$resp->id];
		$cmd->status = $resp->status;
		if ($cmd->type == KeyspaceClient::GETMASTER)
		{
			$this->getMasterPending = FALSE;
			if ($resp->status == KEYSPACE_OK)
				$this->client->setMaster(intval($resp->value));
			else
				$this->client->setMaster(-1);
			$this->removeSentCommand($cmd);
			return;
		}
		
		if ($cmd->type == KeyspaceClient::LISTKEYS ||
			$cmd->type == KeyspaceClient::LISTKEYVALUES ||
			$cmd->type == KeyspaceClient::DIRTYLISTKEYS ||
			$cmd->type == KeyspaceClient::DIRTYLISTKEYVALUES)
		{
			if ($resp->key == NULL)
				$this->removeSentCommand($cmd);
			else
				$this->client->result->appendListResponse($resp);
		}
		else
		{
			$this->removeSentCommand($cmd);
			$this->client->result->appendResponse($resp);
		}
	}
	
	protected function readMessage()
	{
		$colon = strpos($this->readBuffer, ":");
		if ($colon === FALSE)
			return;
		
		$slen = substr($this->readBuffer, 0, $colon);
		$len = strlen($slen) + 1 + intval($slen);
		if (strlen($this->readBuffer) < $len)
			return;
		
		$msg = substr($this->readBuffer, $colon + 1, $len - ($colon + 1));
		$this->readBuffer = substr($this->readBuffer, $len);
		return $msg;
	}
}

class KeyspaceCallback
{
	public $objectinstance;
	public $methodname;
	
	public function __construct($obj, $name)
	{
		$this->objectinstance = $obj;
		$this->methodname = $name;
	}
	
	public function execute()
	{
		$this->objectinstance->{$this->methodname}();
	}
}

class KeyspaceTimer
{
	public $timeout;
	public $callback;
	
	public function __construct($timeout, $callback)
	{
		$this->timeout = $timeout;
		$this->callback = $callback;
	}
}

class KeyspaceEventLoop
{	
	private $rlist;
	private $wlist;
	private $xlist;
	private $timers;
	private $readCallbacks;
	private $writeCallbacks;
	private $exCallbacks;
	private $timeout;
	
	public function __construct()
	{
		$this->rlist = array();
		$this->wlist = array();
		$this->xlist = array();
		$this->timers = array();
		$this->readCallbacks = array();
		$this->writeCallbacks = array();
		$this->exCallbacks = array();
		$this->timeout = 0;
	}
	
	public function registerRead($socket, $onRead, $onClose)
	{
		if (!array_key_exists(strval($socket), $this->readCallbacks))
		{
			$this->readCallbacks[strval($socket)] = $onRead;
			$this->rlist[] = $socket;			
		}
		if (!array_key_exists(strval($socket), $this->exCallbacks))
		{
			$this->exCallbacks[strval($socket)] = $onClose;
			$this->xlist[] = $socket;
		}
	}

	public function registerWrite($socket, $onWrite, $onClose)
	{
		if (!array_key_exists(strval($socket), $this->writeCallbacks))
		{
			$this->writeCallbacks[strval($socket)] = $onWrite;
			$this->wlist[] = $socket;			
		}
		if (!array_key_exists(strval($socket), $this->exCallbacks))
		{
			$this->exCallbacks[strval($socket)] = $onClose;
			$this->xlist[] = $socket;
		}
	}
	
	public function registerTimer($timeout, $onTimeout)
	{
		$timer = new KeyspaceTimer($timeout, $onTimeout);
		$this->timers[] = $timer;
	}
	
	public function close($socket)
	{
		$callbacks = array($this->readCallbacks, $this->writeCallbacks, $this->exCallbacks);
		foreach($callbacks as $callback)
		{
			if (array_key_exists(strval($socket), $callback))
				unset($callback[strval($socket)]);
		}
		
		$socklist = array($socket);
		$this->rlist = array_diff($this->rlist, $socklist);
		$this->wlist = array_diff($this->wlist, $socklist);
		$this->xlist = array_diff($this->xlist, $socklist);
	}
	
	public function now()
	{
		return microtime();
	}
	
	public function runOnce()
	{
		$this->poll(1000);
	}

	private function arrayCopy($array)
	{
		$new = array();
		foreach ($array as $e)
			$new[] = $e;
		
		return $new;
	}
	
	public function compareTimer($a, $b)
	{
		if ($a->timeout < $b->timeout)
			return -1;
		if ($b->timeout < $a->timeout)
			return 1;
	   	return 0;
	}
	
	public function poll($timeout)
	{
		usort($this->timers, array($this, 'compareTimer'));
		if (count($this->timers) > 0)
			$timeout = $this->timers[0]->timeout;
		
		$startTime = $this->now();
		
		// copy the arrays manually as php does not provide array_copy...
		$read = $this->arrayCopy($this->rlist);
		$write = $this->arrayCopy($this->wlist);
		$except = $this->arrayCopy($this->xlist);
		
		$ret = @socket_select($read, $write, $except, intval($timeout / 1000), intval($timeout % 1000));
		// error or timeout
		_trace("ret = $ret");
		if ($ret === FALSE || $ret == 0)
			return;
		
		$endTime = $this->now();

		// remove those sockets which were active
		$this->rlist = array_diff($this->rlist, $read);
		$this->wlist = array_diff($this->wlist, $write);
		$this->xlist = array_diff($this->xlist, $except);

		foreach ($read as $r)
		{
			$callback = $this->readCallbacks[strval($r)];
			unset($this->readCallbacks[strval($r)]);
			$callback->execute();
		}
		
		foreach ($write as $w)
		{
			$callback = $this->writeCallbacks[strval($w)];
			unset($this->writeCallbacks[strval($w)]);
			$callback->execute();
		}
		
		foreach ($except as $x)
		{
			$callback = $this->exCallbacks[strval($x)];
			unset($this->exCallbacks[strval($x)]);
			$callback->execute();
		}
		
		$timers = array();
		foreach ($this->timers as $timer)
		{
			if ($startTime + $timer->timeout < $endTime)
				$timer->callback->execute();
			else
				$timers[] = $timer;
		}
		$this->timers = $timers;
	}
}

class KeyspaceResponse
{
	const OK = "o";
    const FAILED = "f";
	const NOTMASTER = "n";
	const LIST_ITEM = "i";
	const LISTP_ITEM = "j";
	const LIST_END = ".";
	const SEPARATOR = ":";

	public $key;
	public $value;
	public $status;
	public $id;
	public $data;
	public $pos;
	
	public function __construct()
	{
		$this->key = "";
		$this->value = "";
		$this->status = OK;
		$this->id = 0;
		$this->data = "";
		$this->pos = 0;
	}
	
	public function read($data)
	{
		$this->key = "";
		$this->value = "";
		$this->status = KEYSPACE_ERROR;
		$this->data = $data;
		$this->pos = 0;
		try
		{
			$cmd = $this->readChar();
			$this->readSeparator();
			$this->id = $this->readNumber();
			
			_trace("cmd = $cmd, id = $this->id");
			switch ($cmd)
			{
			case self::NOTMASTER:
				$this->status = KEYSPACE_NOTMASTER;
				return $this->validateLength();
			case self::FAILED:
				$this->status = KEYSPACE_FAILED;
				return $this->validateLength();
			case self::LIST_END:
				$this->status = KEYSPACE_OK;
				return $this->validateLength();
			case self::OK:
				$this->status = KEYSPACE_OK;
				if ($this->pos < strlen($this->data))
				{
					$msg = $this->readMessage();
					_trace("msg = $msg");
					_trace("pos = $this->pos, len = " . strlen($this->data));
					if ($msg !== NULL)
						$this->value = $msg;
				}
				return $this->validateLength();
			case self::LIST_ITEM:
				$this->status = KEYSPACE_OK;
				$msg = $this->readMessage();
				if ($msg !== NULL)
					$this->key = $msg;
				return $this->validateLength();
			case self::LISTP_ITEM:
				$this->status = KEYSPACE_OK;
				$msg = $this->readMessage();
				if ($msg !== NULL)
				{
					$this->key = $msg;
					$msg = $this->readMessage();
					if ($msg !== NULL)
						$this->value = $msg;
				}
				return $this->validateLength();
			default:
				throw new KeyspaceProtocolException("Unknown response");
			}
		}
		catch (KeyspaceProtocolException $e)
	    {
			_trace("$e->errmsg");
			return FALSE;
		}
	}
	
	public function checkOverflow()
	{
		if ($this->pos >= strlen($this->data))
			throw new KeyspaceProtocolException("overflow");
	}

	public function validateLength()
	{
		if ($this->pos != strlen($this->data))
			throw new KeyspaceProtocolException("length");
		return TRUE;
	}

	public function readChar()
	{
		$this->checkOverflow();
		$c = $this->data[$this->pos];
		$this->pos++;
		return $c;
	}
	
	public function readSeparator()
	{
		$this->checkOverflow();
		if ($this->data[$this->pos] != ":")
			throw new KeyspaceProtocolException("separator");
		$this->pos++;
	}
	
	public function readNumber()
	{
		$this->checkOverflow();
		$num = 0;
		$rem = strlen($this->data) - $this->pos;
		if ($rem <= 0)
			throw new KeyspaceProtocolException("number");
		
		while ($rem > 0 && is_numeric($this->data[$this->pos]))
		{
			$num *= 10;
			$num += intval($this->data[$this->pos]);
			$rem--;
			$this->pos++;
		}
		if ($rem > 0 && $this->data[$this->pos] != ":")
			throw new KeyspaceProtocolException("number");
		
		return $num;
	}                
	
	public function readData($len)
	{
		if ($this->pos - $len > strlen($this->data))
			throw new KeyspaceProtocolException("data");
		$data = substr($this->data, $this->pos, $len);
		$this->pos += $len;
		return $data;
	}
	
	public function readMessage()
	{
		$this->readSeparator();
		$num = $this->readNumber();
		$this->readSeparator();
		return $this->readData($num);		
	}
}

class KeyspaceResult
{
	public $responses;
	public $startId;
	
	public function __construct()
	{
		$this->responses = array();
		$this->startId = 0;
	}
	
	public function keys($id)
	{
		$keys = array();
		_trace($id);
		if (!array_key_exists($id, $this->responses))
			return $keys;
		
		$resp = $this->responses[$id];
		if (is_array($resp))
		{
			foreach ($resp as $r)
				$keys[] = $r->key;
		}
		else
			$keys[] = $resp;
		return $keys;
	}
	
	public function keyValues($id)
	{
		$keyValues = array();
		if (!array_key_exists($id, $this->responses))
			return $keyValues;
		
		$resp = $this->responses[$id];
		if (is_array($resp))
		{
			foreach($resp as $r)
				$keyValues[$r->key] = $r->value;
			return $keyValues;
		}
		
		$keyValues[$resp->key] = $resp->value;
		return $keyValues;
	}
	
	public function firstValue()
	{
		if (!array_key_exists($this->startId, $this->responses))
			return;
		
		$resp = $this->responses[$this->startId];
		return $resp->value;
	}
	
	public function close()
	{
		_trace("");
		$this->startId = 0;
		$this->responses = array();
	}
	
	public function appendResponse($resp)
	{
		if ($this->startId == 0)
			$this->startId = $resp->id;
		$this->responses[$resp->id] = $resp;
	}
	
	public function appendListResponse($resp)
	{
		if ($this->startId == 0)
			$this->startId = $resp->id;

		if (!array_key_exists($resp->id, $this->responses))
		{
			$resps = array();
			$this->responses[$resp->id] = &$resps;
		}
		else
		{
			$resps = &$this->responses[$resp->id];
		}
		
		$resps[] = $resp;
	}
	
	public function values()
	{
		$values = array();
		foreach ($this->responses as $resp)
		{
			if (is_array($resp))
			{
				foreach ($resp as $r)
					$values[] = $r;
			}
			else
				$values[] = $resp;
		}
		return $values;
	}
}

class KeyspaceCommand
{
	public $type;
	public $nodeId;
	public $status;
	public $id;
	public $submit;
	
	function __construct()
	{
		$this->type = 0;
		$this->nodeId = -1;
		$this->status = 0;
		$this->id = 0;
		$this->submit = FALSE;
	}
}

$_KEYSPACE_TRACE = FALSE;
function _trace($msg)
{
	global $_KEYSPACE_TRACE;
	if ($_KEYSPACE_TRACE)
	{
		$bt = debug_backtrace();
		$caller = $bt[1];
		$cstr = "";
		
    	if (isset($bt[0]["file"])) 
		{
			$cstr .= $bt[0]["file"];
			$pos = strrpos($cstr, "/");
			if ($pos !== FALSE)
				$cstr = substr($cstr, $pos + 1);

			if (isset($bt[0]["line"])) $cstr .= ":" . $bt[0]["line"] . " ";
		}
		if (isset($caller["class"])) $cstr .= $caller["class"] . "::";
		if (isset($caller["function"])) $cstr .= $caller["function"];
		if (is_string($msg))
			echo($cstr . ": " . $msg . "\n");
		else
		{
			echo($cstr . " ");
			var_dump($msg);
		}
		if (isset($_SERVER["HTTP_HOST"]))
			echo("<br/>");
	}
}


?>
