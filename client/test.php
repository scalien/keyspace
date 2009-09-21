<?php

include_once("keyspace.php");

define(PREFIX, "/test/");
define(TIMEOUT, 0);
define(TRACE, TRUE);
$NODES = array("localhost:7080","localhost:7081");

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

function basicUsersTest($client)
{
		$client->set("user:0", "mtrencseni");
		$client->set("user:1", "agazso");
		$client->set("user:2", "pjozsa");
		$client->set("user:3", "dtorok");
		$client->set("user:4", "pbagi");
		$client->set("user:5", "zsszabo");
		$client->set("user:6", "koszko");
		$client->set("user:7", "itakacs");
		$client->set("user:8", "akutasi");
		$client->set("user:9", "tmatrahegyi");
		$client->set("user:10", "pfarago");
		$client->set("user:11", "glaszlo");
		$client->set("user:11", "abeliczay");

		$client->listkeys("user");
		$client->dirtylistkeys();
}

$client = new KeyspaceClient($NODES, TIMEOUT, TRACE);
pruneTest($client);
// testBasic($client);
basicUsersTest($client);
// stress($client);
// submitTest($client);
// protocolError($client);
// basicListTest($client);
// basicAddTest($client);
// basicRenameTest($client);

print("all OK\n");

?>