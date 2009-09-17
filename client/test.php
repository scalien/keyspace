<?php

include_once("keyspace.php");

define(PREFIX, "/test/");
define(TIMEOUT, 0);
define(TRACE, FALSE);
$NODES = array("localhost:7080");

function testBasic($client)
{
	$client->set(PREFIX . "key", "value");  
}

function genString($len)
{
	// there must be a more effective way of doing this
	$s = "";
	for ($i = 0; $i < $len; $i++)
		$s .= "$";
		
	return $s;
}

function pruneTest($client)
{
	$client->prune(PREFIX);
}

function stress($client)
{
	$msg = genString(1000 * 1000);
	for ($i = 0; $i < 100; $i++)
		$client->set(PREFIX . "big$i", $msg);
}

function submitTest($client, $prefix = "test", $num = 10000)
{
	$client->begin();
	for ($i = 0; $i < $num; $i++)
		$client->set(PREFIX . "$prefix$i", $i, FALSE);
	$client->submit();
}

function protocolError($client)
{
	$client->set(PREFIX . "key", genString(3000*1000+1));
}

function basicListTest($client)
{
	$count = 100;
	
	submitTest($client, "list", $count);
	$list = $client->listkeys(PREFIX . "list");
	if (count($list) != $count)
		throw new KeyspaceException("basicListTest");

	$list = $client->listkeyvalues(PREFIX . "list");
	if (count($list) != $count)
		throw new KeyspaceException("basicListTest");	
	
	$c = $client->count(PREFIX . "list");
	if ($c != $count)
		throw new KeyspaceException("basicListTest");
}

function basicAddTest($client)
{
	$client->set(PREFIX . "counter", 0);
	$value = $client->add(PREFIX . "counter", 1);
	if ($value != 1)
		throw new KeyspaceException("basicAddTest");
	
	$value = $client->get(PREFIX . "counter");
	if ($value != 1)
		throw new KeyspaceException("basicAddTest");
		
    $value = $client->remove(PREFIX . "counter");
	if ($value != 1)
		throw new KeyspaceException("basicAddTest");		
}

function basicRenameTest($client)
{
	$client->set(PREFIX . "a", "abc");
	$client->rename(PREFIX . "a", PREFIX . "b");
	$abc = $client->get(PREFIX . "b");
	if ($abc != "abc")
		throw new KeyspaceException("basicRenameTest");

	$client->delete(PREFIX . "b");
}

$client = new KeyspaceClient($NODES, TIMEOUT, TRACE);
pruneTest($client);
testBasic($client);
// stress($client);
// submitTest($client);
// protocolError($client);
basicListTest($client);
basicAddTest($client);
basicRenameTest($client);

print("all OK\n");

?>