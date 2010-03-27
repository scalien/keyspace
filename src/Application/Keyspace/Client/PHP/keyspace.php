<?php

include("keyspace_client.php");
//dl("/Users/attila/Projects/Scalien/Keyspace.trunk/src/Application/Keyspace/Client/PHP/keyspace_client.so");

class Result {
	private $cPtr;
	
	public function __construct() {
		$this->cPtr = NULL;
	}
	
	public function __destruct() {
		$this->close();
	}
	
	public function close() {
		keyspace_client::Keyspace_ResultClose($this->cPtr);
		$this->cPtr = NULL;
	}
	
	public function set($cPtr) {
		$this->close();
		$this->cPtr = $cPtr;
	}
	
	public function key() {
		return keyspace_client::Keyspace_ResultKey($this->cPtr);
	}
	
	public function value() {
		return keyspace_client::Keyspace_ResultValue($this->cPtr);
	}
	
	public function begin() {
		return keyspace_client::Keyspace_ResultBegin($this->cPtr);
	}
	
	public function isEnd() {
		return keyspace_client::Keyspace_ResultIsEnd($this->cPtr);
	}
	
	public function next() {
		return keyspace_client::Keyspace_ResultNext($this->cPtr);
	}

	public function commandStatus() {
		return keyspace_client::Keyspace_ResultCommandStatus($this->cPtr);
	}

	public function transportStatus() {
		return keyspace_client::Keyspace_ResultTransportStatus($this->cPtr);
	}
	
	public function connectivityStatus() {
		return keyspace_client::Keyspace_ResultConnectivityStatus($this->cPtr);
	}
	
	public function timeoutStatus() {
		return keyspace_client::Keyspace_ResultTimeoutStatus($this->cPtr);
	}

	public function keys() {
		$keys = array();
		$this->begin();
		while (! $this->isEnd()) {
			$keys[] = $this->key();
		}

		return $keys;
	}
	
	public function keyValues() {
		$kv = array();
		$this->begin();
		while (! $this->isEnd()) {
			$kv[$this->key()] = $this->value();
			$this->next();
		}

		return $kv;
	}
}

class Keyspace {	
	private $co = NULL;
	public $result = NULL;
	
	public function __construct($nodes) {
		$this->co = keyspace_client::Keyspace_Create();
		$this->result = new Result();
		$nodeParams = new Keyspace_NodeParams(count($nodes));
		foreach ($nodes as $node) {
			$nodeParams->AddNode($node);
		}
		keyspace_client::Keyspace_Init($this->co, $nodeParams);
		$nodeParams->Close();
	}
	
	public function __destruct() {
		keyspace_client::Keyspace_Destroy($this->co);
	}
	
	public function setGlobalTimeout($timeout) {
		keyspace_client::Keyspace_SetGlobalTimeout($this->co, $timeout);
	}
	
	public function getGlobalTimeout() {
		return keyspace_client::Keyspace_GetGlobalTimeout($this->co);
	}
	
	public function setMasterTimeout($timeout) {
		keyspace_client::Keyspace_SetMasterTimeout($this->co, $timeout);
	}
	
	public function getMasterTimeout() {
		return keyspace_client::Keyspace_GetMasterTimeout($this->co);
	}
	
	public function getMaster() {
		return keyspace_client::Keyspace_GetMaster($this->co);
	}
	
	public function getState() {
		return keyspace_client::Keyspace_GetState($this->co);
	}
	
	public function distributeDirty($dist) {
		return keyspace_client::Keyspace_DistributeDirty($this->co, $dist);
	}
	
	public function get($key) {
		$status = keyspace_client::Keyspace_Get($this->co, $key);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		return $this->result->value();
	}

	public function dirtyGet($key) {
		$status = keyspace_client::Keyspace_DirtyGet($this->co, $key);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		return $this->result->value();
	}

	public function count_($prefix = "", $start_key = "", $count = 0, $skip = FALSE, $forward = TRUE) {
		$status = keyspace_client::Keyspace_CountStr($this->co, $prefix, $start_key, $count, $skip, $forward);
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		if ($status < 0)
			return NULL;
		return $this->result->value();
	}
	
	public function count($params) {
		$prefix = $this->listArg($params, "prefix");
		$start_key = $this->listArg($params, "start_key");
		$count = $this->listArg($params, "count");
		$skip = $this->listArg($params, "skip");
		$forward = $this->listArg($params, "forward");
		return $this->count_($prefix, $start_key, $count, $skip, $forward);
	}
	
	public function dirtyCount_($prefix = "", $start_key = "", $count = 0, $skip = FALSE, $forward = TRUE) {
		$status = keyspace_client::Keyspace_DirtyCountStr($this->co, $prefix, $start_key, $count, $skip, $forward);
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		if ($status < 0)
			return NULL;
		return $this->result->value();
	}
	
	public function dirtyCount($params) {
		$prefix = $this->listArg($params, "prefix");
		$start_key = $this->listArg($params, "start_key");
		$count = $this->listArg($params, "count");
		$skip = $this->listArg($params, "skip");
		$forward = $this->listArg($params, "forward");
		return $this->dirtyCount_($prefix, $start_key, $count, $skip, $forward);
	}

	public function listKeys_($prefix = "", $start_key = "", $count = 0, $skip = FALSE, $forward = TRUE) {
		$status = keyspace_client::Keyspace_ListKeysStr($this->co, $prefix, $start_key, $count, $skip, $forward);
                $this->result->set(keyspace_client::Keyspace_GetResult($this->co));
                if ($status < 0)
                	return NULL;
		return $this->result->keys();
	}

	public function listKeys($params) {
		$prefix = $this->listArg($params, "prefix");
		$start_key = $this->listArg($params, "start_key");
		$count = $this->listArg($params, "count");
		$skip = $this->listArg($params, "skip");
		$forward = $this->listArg($params, "forward");
		return $this->listKeys_($prefix, $start_key, $count, $skip, $forward);
	}

	public function dirtyListKeys_($prefix = "", $start_key = "", $count = 0, $skip = FALSE, $forward = TRUE) {
		$status = keyspace_client::Keyspace_DirtyListKeysStr($this->co, $prefix, $start_key, $count, $skip, $forward);
                $this->result->set(keyspace_client::Keyspace_GetResult($this->co));
                if ($status < 0)
                	return NULL;
		return $this->result->keys();
	}
	
	public function dirtyListKeys($params) {
		$prefix = $this->listArg($params, "prefix");
		$start_key = $this->listArg($params, "start_key");
		$count = $this->listArg($params, "count");
		$skip = $this->listArg($params, "skip");
		$forward = $this->listArg($params, "forward");
		return $this->dirtyListKeys_($prefix, $start_key, $count, $skip, $forward);
	}

	public function listKeyValues_($prefix = "", $start_key = "", $count = 0, $skip = FALSE, $forward = TRUE) {
		$status = keyspace_client::Keyspace_ListKeyValuesStr($this->co, $prefix, $start_key, $count, $skip, $forward);
                $this->result->set(keyspace_client::Keyspace_GetResult($this->co));
                if ($status < 0)
                	return NULL;
		return $this->result->keyValues();
	}
	
	public function listKeyValues($params) {
		$prefix = $this->listArg($params, "prefix");
		$start_key = $this->listArg($params, "start_key");
		$count = $this->listArg($params, "count");
		$skip = $this->listArg($params, "skip");
		$forward = $this->listArg($params, "forward");
		return $this->listKeyValues_($prefix, $start_key, $count, $skip, $forward);
	}
	
	public function dirtyListKeyValues_($prefix = "", $start_key = "", $count = 0, $skip = FALSE, $forward = TRUE) {
		$status = keyspace_client::Keyspace_DirtyListKeyValues($this->co, $prefix, $start_key, $count, $skip, $forward);
                $this->result->set(keyspace_client::Keyspace_GetResult($this->co));
                if ($status < 0)
                	return NULL;
		return $this->result->keyValues();
	}
	
	public function dirtyListKeyValues($params) {
		$prefix = $this->listArg($params, "prefix");
		$start_key = $this->listArg($params, "start_key");
		$count = $this->listArg($params, "count");
		$skip = $this->listArg($params, "skip");
		$forward = $this->listArg($params, "forward");
		return $this->dirtyListKeyValues_($prefix, $start_key, $count, $skip, $forward);
	}
	
	public function set($key, $value) {
		$status = keyspace_client::Keyspace_Set($this->co, $key, $value);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
	}

	public function testAndSet($key, $test, $value) {
		$status = keyspace_client::Keyspace_TestAndSet($this->co, $key, $test, $value);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		return $this->result->value();
	}

	public function add($key, $num) {
		$status = keyspace_client::Keyspace_AddStr($this->co, $key, $num);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		return $this->result->value();
	}
	
	public function delete($key) {
		$status = keyspace_client::Keyspace_Delete($this->co, $key);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
	}
	
	public function remove($key) {
		$status = keyspace_client::Keyspace_Remove($this->co, $key);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		return $this->result->value();
	}
	
	public function rename($src, $dst) {
		$status = keyspace_client::Keyspace_Rename($this->co, $src, $dst);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
	}

	public function prune($prefix) {
		$status = keyspace_client::Keyspace_Prefix($this->co, $prefix);
		if ($status < 0) {
			$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
			return NULL;
		}
		if ($this->isBatched())
			return NULL;
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
	}	

	public function begin() {
		return keyspace_client::Keyspace_Begin($this->co);
	}
	
	public function submit() {
		$status = keyspace_client::Keyspace_Submit($this->co);
		$this->result->set(keyspace_client::Keyspace_GetResult($this->co));
		return $status;
	}
	
	public function cancel() {
		return keyspace_client::Keyspace_Cancel($this->co);
	}
	
	public function isBatched() {
		return keyspace_client::Keyspace_IsBatched($this->co);
	}
	
	private function listArg($params, $name) {
		if (isset($params[$name]))
			return $params[$name];
			
		switch ($name) {
		case "prefix": return "";
		case "start_key": return "";
		case "count": return 0;
		case "skip": return FALSE;
		case "forward": return TRUE;
		}
	}
}

keyspace_client::Keyspace_SetTrace(TRUE);
$ks = new Keyspace(array("localhost:7080"));
print($ks->get("hol"));
print($ks->count(array("count" => 10)));
$ks->set("c", 2147483647);
print($ks->add("c", 1) . "\n");
print($ks->result->transportStatus() . "\n");
?>
