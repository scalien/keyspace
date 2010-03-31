require 'keyspace'

ks = Keyspace.new(["localhost:7080"])
ks.get("hol")
print(ks.result.value)
print(ks.count() + "\n")

keys = ks.list_keys()
keys.each do |key|
	print(key + "\n")
end

kv = ks.list_key_values()
kv.each_pair do |k, v|
	print(k + " => " + v + "\n")
end