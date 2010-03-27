#!/bin/sh

DIR=$1

mv $DIR/keyspace_client.php $DIR/keyspace_client_swig.php
cat $DIR/keyspace_client_swig.php | sed 's/(function_exists($func) /(function_exists($func)) /' > $DIR/keyspace_client.php
rm $DIR/keyspace_client_swig.php
