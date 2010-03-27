<?php

@include_once("keyspace.php");
@include_once("keyspace_client.php");

//keyspace_client::Keyspace_SetTrace(TRUE);
$ks = new Keyspace(array("localhost:7080"));
print($ks->get("hol"));
print($ks->count(array("count" => 10)));
$ks->set("c", 2147483647);
print($ks->add("c", 1) . "\n");
print($ks->result->transportStatus() . "\n");
var_dump($ks->listKeyValues(array("count" => 10)));
var_dump($ks->listKeys(array("count" => 10)));
var_dump($ks->listKeys(array("start_key" => "a", "count" => 10)));

$ks2 = new Keyspace(array("localhost:7080"));
print($ks2->get("hol"));

?>
