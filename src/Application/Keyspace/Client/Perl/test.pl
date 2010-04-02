use Keyspace;

$client = new Keyspace(("localhost:7080"));
print($client->get("hol") . "\n");
$client->set("hol", "budapest");
print($client->get("hol") . "\n");
print($client->count('count' => 5) . "\n");

print($client->add("c", 1) . "\n");

@keys = $client->list_keys();
for my $key (@keys) {
  print($key . "\n");
}

%kv = $client->list_key_values();
for my $key (keys %kv) {
  print($key . " => " . $kv{$key} . "\n");
}

print(Keyspace::status_string($client->{result}->transport_status()) . "\n");
