#ifndef CONFIG_H
#define CONFIG_H

class Config
{
public:
	static bool			Init(const char* filename);
	
	static int			GetIntValue(const char *name, int defval);
	static const char*	GetValue(const char* name, const char *defval);
	
	static int			GetListNum(const char* name);
	static const char*	GetListValue(const char* name, int num, const char* defval);
};

#endif
/*

paxos.nodeID = 0
paxos.endpoints = 127.0.0.1:10000, 127.0.0.1:10010, 127.0.0.1:10020

database.dir = test/0/
database.type = hash # or btree

http.port = 8080
memcached.port = 11111

====================
C++ interface usage:
====================
Config::Init(configfile);

PaxosConfig::nodeID = Config::GetIntValue("paxos.nodeID", 0);
PaxosConfig::numNodes = Config::GetListNum("paxos.endpoints");
for (int i = 0; i < PaxosConfig::numNodes; i++)
{
	PaxosConfig::endpoints[i].Set(Config::GetListValue("paxos.endpoints", i, NULL));
}

database.Init(Config::GetValue("database.dir"));

*/
