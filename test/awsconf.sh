#!/bin/sh

cat << EOF
paxos.nodeID = $1
paxos.endpoints = 10.226.126.145:10000, 10.226.103.148:10000, 10.226.63.241:10000

io.maxfd = 4096

database.dir = test/$1
database.type = hash # or btree
database.checkpointSize = 100000
database.checkpointInterval = 60
database.pageSize = 4096
database.cacheSize = 4096K
database.logBufferSize = 1024K

asyncDatabase.numReader = 6

http.port = 8080
memcached.port = 11110
keyspace.port = 7080

log.timestamping = false
log.trace = false

timecheck.active = false
EOF