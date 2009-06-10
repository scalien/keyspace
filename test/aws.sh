#!/bin/sh

alias up='svn up; make clean debug'
apt-get update
apt-get -y install subversion libdb4.6++-dev db4.6-util joe telnet unzip gdb ntpdate less
svn checkout http://svn.scalien.com/keyspace/trunk keyspace
cd keyspace
make
make clientlib
g++ -Isrc -o bin/clienttest src/Test/KeyspaceClientTest.cpp bin/clientlib.a
echo 1 > /proc/sys/xen/independent_wallclock
ntpdate-debian


