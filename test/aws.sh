# as root
alias up='svn up; make clean debug'
alias ntpsync='ntpdate-debian'
alias conf='for i in `seq 0 2`; do test/awsconf.sh $i > test/$i/keyspace.conf; done'
apt-get update
apt-get -y install subversion libdb4.6++-dev db4.6-util joe telnet unzip gdb ntpdate less valgrind fakeroot iotop curl make g++

# as user
#svn checkout http://svn.scalien.com/keyspace/trunk keyspace
VERSION=`curl http://scalien.com/downloads/latest.txt`
KEYSPACE_VERSION=keyspace-$VERSION
KEYSPACE_TGZ=$KEYSPACE_VERSION.tgz
curl http://scalien.com/releases/keyspace/$KEYSPACE_TGZ -o $KEYSPACE_TGZ
tar xzvf $KEYSPACE_TGZ
cd $KEYSPACE_VERSION
make

up
echo 1 > /proc/sys/xen/independent_wallclock
ntpdate-debian
echo "*/10 *  * * *   root    ntpdate-debian" >> /etc/crontab
conf

