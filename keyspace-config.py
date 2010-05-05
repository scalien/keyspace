
import string

txt = "";
txt += "                                  \n"
txt += "  client(s)                       \n"
txt += "                                  \n"
txt += "    ^                             \n"
txt += "    |                             \n"
txt += "    |            node AA          \n"
txt += "    |     +-------------------+   \n"
txt += "    |     +  BBBBBBBBBBBBBBB  |   \n"
txt += "    |     |                   |   \n"
txt += "    |     +  CCCCCCCCCCCCCCC  |   \n"
txt += "    |     |                   |   \n"
txt += "    |     |                   |   \n"
txt += "  DDDDD --+ Keyspace          |   \n"
txt += "    |     |                   |   \n"
txt += "  EEEEE --+ HTTP              |   \n"
txt += "          |                   |   \n"
txt += "          |                   |   \n"
txt += "          |                   |   \n"
txt += "  GGGGG --+ Paxos             |   \n"
txt += "    |     |                   |   \n"
txt += "  HHHHH --+ PaxosLease        |   \n"
txt += "    |     |                   |   \n"
txt += "  IIIII --+ Catchup           |   \n"
txt += "    |     |                   |   \n"
txt += "    |     +-------------------+   \n"
txt += "    |                             \n"
txt += "    |                             \n"
txt += "    |                             \n"
txt += "----+-------- [replication] ------\n"

def node_ascii(id, host, database_dir, keyspace_port, http_port, paxos_port):
	global txt
	rules = []
	rules.append(["AA", id, string.ljust])
	rules.append(["BBBBBBBBBBBBBBB", host, string.center])
	rules.append(["CCCCCCCCCCCCCCC", database_dir, string.center])
	rules.append(["DDDDD", keyspace_port, string.rjust])
	rules.append(["EEEEE", http_port, string.rjust])
	rules.append(["FFFFF", paxos_port, string.rjust])
	rules.append(["GGGGG", str(int(paxos_port)+0), string.rjust])
	rules.append(["HHHHH", str(int(paxos_port)+1), string.rjust])
	rules.append(["IIIII", str(int(paxos_port)+2), string.rjust])
	return replace(txt, rules)

def replace(txt, rules):
	for rule in rules:
		fr = str(rule[0])
		to = str(rule[1])
		padder = rule[2]
		if len(fr) > len(to):
			to = padder(to, len(fr))
		elif len(fr) < len(to):
			to = to[:len(fr)-2] + ".."
		txt = txt.replace(fr, to)
	return txt

def merge(txts):
	parts = []
	for txt in txts:
		p = txt.split("\n")
		parts.append(p)
	num_lines = len(parts[0])
	txt = ""
	for i in xrange(num_lines):
		for j in xrange(len(txts)):
			txt += parts[j][i]
		txt += "\n"
	return txt

def option_input(str, options):
	opts_str = "/".join(options)
	options_lower = []
	for opt in options: options_lower.append(opt.lower())
	inp = ""
	while True:
		inp = raw_input("%s [%s]: " % (str, opts_str)).lower()
		# verify
		if inp in options_lower: return inp
		# handle default
		if inp == "":
			for opt in options:
				if opt == opt.upper(): return opt.lower()
		# no default
		print("Please specify one of [%s]!" % opts_str)

def default_input(str, default, cons_func=None):
	while True:
		inp = raw_input("%s [%s]: " % (str, default))
		# verify
		if cons_func is not None:
			try:
				if cons_func(inp): return inp
			except: pass
		# handle default
		if inp == "": return default
		# no default
		print("Invald input, try again!")
	
def replicated_config():
	num_nodes = int(default_input("How many nodes will the Keyspace cluster consist of?", "3", int))
	same_host = option_input("Will you be running all Keyspace nodes on the *same* host (eg. for testing)?", ["Y", "n"])
	if same_host == "y":
		host = default_input("What is the IP address or domain name of the host?", "127.0.0.1")
		hosts = []
		for i in xrange(num_nodes): hosts.append(host)
		# get database dir
		database_dirs = []
		for i in xrange(num_nodes):
			database_dir = default_input("Where should the %d. node store its database files?" % i, "/tmp/keyspace/%s" % i)
			database_dirs.append(database_dir)
		# get paxos ports
		paxos_ports = []
		for i in xrange(num_nodes):
			port = default_input("Port number for Paxos replication on the %d. node?" % i, str(10000+i*10), int)
			paxos_ports.append(port)
		# get http ports
		http_ports = []
		for i in xrange(num_nodes):
			port = default_input("Port number for HTTP on the %d. node?" % i, str(8080+i), int)
			http_ports.append(port)
		# get keyspace ports
		keyspace_ports = []
		for i in xrange(num_nodes):
			port = default_input("Port number for Keyspace protocol on the %d. node?" % i, str(7080+i), int)
			keyspace_ports.append(port)
	else:
		print("")
		print("-> Assuming identical Keyspace configurations on all %d nodes!" % num_nodes)
		print("")
		hosts = []
		for i in xrange(num_nodes):
			host = raw_input("What is the IP address (or domain name) of the %d. node? " %i)
			hosts.append(host)
		# get database dir
		database_dir = default_input("Where should Keyspace store the database files?", "/var/keyspace")
		database_dirs = []
		for i in xrange(num_nodes): database_dirs.append(database_dir)
		# get paxos port
		port = default_input("Port number for Paxos replication?", str(10000), int)
		paxos_ports = []
		for i in xrange(num_nodes): paxos_ports.append(port)
		# get http ports
		port = default_input("Port number for HTTP?", str(8080), int)
		http_ports = []
		for i in xrange(num_nodes): http_ports.append(port)
		# get keyspace ports
		port = default_input("Port number for Keyspace protocol?", str(7080), int)
		keyspace_ports = []
		for i in xrange(num_nodes): keyspace_ports.append(port)		
	configs = gen_replicated_config(num_nodes, hosts, database_dirs, paxos_ports, http_ports, keyspace_ports)
	print("")
	for i in xrange(num_nodes):
		filename = "keyspace.%d.conf" % i
		print("-> Writing configuration file of the %d. node to %s" % (i, filename))
		write_file(filename, configs[i])
	print("")
	# generate client side connection string
	keyspace_endpoints = []
	for i in xrange(num_nodes):
		keyspace_endpoints.append("%s:%s" % (hosts[i], keyspace_ports[i]))
	keyspace_endpoints = ", ".join(keyspace_endpoints)
	print("-> The connection string on the client side is: %s" % keyspace_endpoints)
	print("")
	ascii_art = option_input("Do you wish to see an informative ascii art describing your configuration?", ["Y", "n"])
	if ascii_art == "y":
		txts = []
		for i in xrange(num_nodes):
			txts.append(node_ascii(i, hosts[i], database_dirs[i], keyspace_ports[i], http_ports[i], paxos_ports[i]))
		txt = merge(txts)
		print(txt)	
		filename = "keyspace_art.txt"
		print("-> Writing ascii art to %s" % filename)
		write_file(filename, txt)

def gen_replicated_config(num_nodes, hosts, database_dirs, paxos_ports, http_ports, keyspace_ports):
	paxos_endpoints = []
	for i in xrange(num_nodes):
		paxos_endpoints.append("%s:%s" % (hosts[i], paxos_ports[i]))
	paxos_endpoints = ", ".join(paxos_endpoints)
	configs = []
	for i in xrange(num_nodes):
		configs.append("")
		configs[i] += "mode = replicated\n\n"
		configs[i] += "paxos.nodeID = %s\n" % i
		configs[i] += "paxos.endpoints = %s\n\n" % paxos_endpoints
		configs[i] += "database.dir = %s\n\n" % database_dirs[i]
		configs[i] += "http.port = %s\n" % http_ports[i]
		configs[i] += "keyspace.port = %s\n\n" % keyspace_ports[i]
		configs[i] += "log.trace = false\nlog.targets = stdout, file\nlog.file = keyspace.log\nlog.timestamping = false\n"
	return configs

def single_config():
	# get database dir
	database_dir = default_input("Where should Keyspace store the database files?", "/var/keyspace")
	http_port = default_input("Port number for HTTP?", str(8080), int)
	keyspace_port = default_input("Port number for Keyspace protocol?", str(7080), int)
	config = gen_single_config(database_dir, http_port, keyspace_port)
	filename = "keyspace.conf"
	print("-> Writing configuration file to %s" % filename)
	write_file(filename, config)

def gen_single_config(database_dir, http_port, keyspace_port):
	config = ""
	config += "mode = single\n\n"
	config += "database.dir = %s\n\n" % database_dir
	config += "http.port = %s\n" % http_port
	config += "keyspace.port = %s\n\n" % keyspace_port
	config += "log.trace = false\nlog.targets = stdout, file\nlog.file = keyspace.log\nlog.timestamping = false\n"
	return config

def write_file(filename, str):
	file = open(filename, 'w')
	file.write(str)
	file.close()

print("")
print("Welcome to the Keyspace configuration file generator!")
print("-----------------------------------------------------")
print("")
print("-> This script will ask you a bunch of questions and then generate the Keyspace configuration files.")
print("-> The files are only generated at the end, you can always start over by hitting CTRL+C.")
print("")
inp = option_input("Is this a replicated or a single configuration?", ["R", "s"])
if inp == "r":
	replicated_config()
else:
	single_config()
print("-> Done.")
print("")
