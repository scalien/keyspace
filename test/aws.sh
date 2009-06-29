alias up='svn up; make clean debug deb'
alias ntpsync='ntpdate-debian'
alias conf='for i in `seq 0 2`; do test/awsconf.sh $i > test/$i/keyspace.conf; done'
apt-get update
apt-get -y install subversion libdb4.6++-dev db4.6-util joe telnet unzip gdb ntpdate less valgrind
svn checkout http://svn.scalien.com/keyspace/trunk keyspace
cd keyspace
up
echo 1 > /proc/sys/xen/independent_wallclock
ntpdate-debian
echo "*/10 *  * * *   root    ntpdate-debian" >> /etc/crontab
conf

