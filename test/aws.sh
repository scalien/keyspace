#!/bin/sh

apt-get update
apt-get -y install subversion libdb4.6++-dev joe telnet unzip gdb
svn checkout http://svn.scalien.com/keyspace/trunk keyspace
cd keyspace
make
make clientlib
g++ -Isrc -o bin/clienttest src/Test/KeyspaceClientTest.cpp bin/clientlib.a
