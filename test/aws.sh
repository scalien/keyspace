#!/bin/sh

alias up='svn up; make clean debug clienttest'
alias ntpsync='ntpdate-debian'
apt-get update
apt-get -y install subversion libdb4.6++-dev db4.6-util joe telnet unzip gdb ntpdate less
svn checkout http://svn.scalien.com/keyspace/trunk keyspace
cd keyspace
up
echo 1 > /proc/sys/xen/independent_wallclock
ntpdate-debian
echo "*/10 *  * * *   root    ntpdate-debian" >> /etc/crontab


