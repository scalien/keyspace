#!/bin/sh

$DIR=$1

cat $DIR/keyspace_client.php | sed 's/(function_exists($func) /(function_exists($func)) /' >! $DIR/keyspace_client_fixed.php
rm $DIR/keyspace_client.php
mv $DIR/keyspace_client_fixed.php $DIR/keyspace_client.php
