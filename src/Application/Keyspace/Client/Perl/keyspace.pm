package Keyspace;

use warnings;
use strict;

use keyspace_client;

use constant KEYSPACE_SUCCESS => 0;
use constant KEYSPACE_API_ERROR => -1;

use constant KEYSPACE_PARTIAL => -101;
use constant KEYSPACE_FAILURE => -102;

use constant KEYSPACE_NOMASTER => -201;
use constant KEYSPACE_NOCONNECTION => -202;

use constant KEYSPACE_MASTER_TIMEOUT => -301;
use constant KEYSPACE_GLOBAL_TIMEOUT => -302;

use constant KEYSPACE_NOSERVICE => -401;
use constant KEYSPACE_FAILED => -402;


package Keyspace::Result;

sub new {
	my $class = shift;
	my $self = {};
	$self->{cptr} = $_[0];
	bless $self;
	return $self;
}

sub DESTROY {
	my $self = $_[0];
	keyspace_client::Keyspace_ResultClose($self->{cptr});
}

sub key {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultKey($self->{cptr});
}

sub value {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultValue($self->{cptr});
}

sub begin {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultBegin($self->{cptr});
}

sub is_end {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultIsEnd($self->{cptr});
}

sub next {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultNext($self->{cptr});
}

sub command_status {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultCommandStatus($self->{cptr});
}

sub transport_status {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultTransportStatus($self->{cptr});
}

sub connectivity_status {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultConnectivityStatus($self->{cptr});
}

sub timeout_status {
	my $self = $_[0];
	return keyspace_client::Keyspace_ResultTimeoutStatus($self->{cptr});
}

sub keys {
	my $self = $_[0];
	my @keys = ();
	$self->begin();
	while (!$self->is_end()) {
		push(@keys, $self->key());
		$self->next();
	}
	return @keys;
}

sub key_values {
	my $self = $_[0];
	my %kv = ();
	$self->begin();
	while (!$self->is_end()) {
		$kv{$self->key()} = $self->value();
		$self->next();
	}
	return %kv;
}


package Keyspace;

sub new {
	my $class = shift;
	my @args = @_;
	my $self = {};
	$self->{cptr} = keyspace_client::Keyspace_Create();
	$self->{result} = undef;

	my @nodes = $args[0];
	my $num_nodes = @nodes;
	my $node_params = new keyspace_client::Keyspace_NodeParams($num_nodes);
	for my $node (@nodes) {
		keyspace_client::Keyspace_NodeParams::AddNode($node_params, $node);
	}

	keyspace_client::Keyspace_Init($self->{cptr}, $node_params);
	keyspace_client::Keyspace_NodeParams::Close($node_params);
	bless $self;
	return $self;
}

sub DESTROY {
	my $self = $_[0];
	keyspace_client::Keyspace_Destroy($self->{cptr});
}

sub set_global_timeout {
	my $self = $_[0];
	my $timeout = $_[1];
	keyspace_client::Keyspace_SetGlobalTimeout($self->{cptr});
}

sub get_global_timeout {
	my $self = $_[0];
	return keyspace_client::Keyspace_GetGlobalTimeout($self->{cptr});
}

sub set_master_timeout {
	my $self = $_[0];
	keyspace_client::Keyspace_SetMasterTimeout($self->{cptr});
}

sub get_master_timeout {
	my $self = $_[0];
	return keyspace_client::Keyspace_GetMasterTimeout($self->{cptr});
}

sub get_master {
	my $self = $_[0];
	return keyspace_client::Keyspace_GetMaster($self->{cptr});
}

sub get_state {
	my $self = $_[0];
	return keyspace_client::Keyspace_GetState($self->{cptr});
}

sub distribute_diry {
	my $self = $_[0];
	my $dd = $_[1];
	return keyspace_client::Keyspace_DistributeDirty($self->{cptr}, $dd);
}

sub get {
	my $self = $_[0];
	my $key = $_[1];
	my $status = keyspace_client::Keyspace_Get($self->{cptr}, $key);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	return $self->{result}->value();
}

sub dirty_get {
	my $self = $_[0];
	my $key = $_[1];
	my $status = keyspace_client::Keyspace_DirtyGet($self->{cptr}, $key);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	return $self->{result}->value();
}

sub count {
	my $self = shift;
	my $start_key;
	my $prefix;
	my $count;
	my $skip;
	my $forward;
	($start_key, $prefix, $count, $skip, $forward) = _list_args(@_);
	my $status = keyspace_client::Keyspace_Count($self->{cptr}, $prefix, $start_key, $count, $skip, $forward);
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	if ($status < 0) {
		return undef;
	}
	return $self->{result}->value();
}

sub dirty_count {
	my $self = shift;
	my $start_key;
	my $prefix;
	my $count;
	my $skip;
	my $forward;
	($start_key, $prefix, $count, $skip, $forward) = _list_args(@_);
	my $status = keyspace_client::Keyspace_DirtyCount($self->{cptr}, $prefix, $start_key, $count, $skip, $forward);
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	if ($status < 0) {
		return undef;
	}
	return $self->{result}->value();
}

sub list_keys {
	my $self = shift;
	my $start_key;
	my $prefix;
	my $count;
	my $skip;
	my $forward;
	($start_key, $prefix, $count, $skip, $forward) = _list_args(@_);
	my $status = keyspace_client::Keyspace_ListKeys($self->{cptr}, $prefix, $start_key, $count, $skip, $forward);
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	if ($status < 0) {
		return undef;
	}
	return $self->{result}->keys();
}

sub dirty_list_keys {
	my $self = shift;
	my $start_key;
	my $prefix;
	my $count;
	my $skip;
	my $forward;
	($start_key, $prefix, $count, $skip, $forward) = _list_args(@_);
	my $status = keyspace_client::Keyspace_DirtyListKeys($self->{cptr}, $prefix, $start_key, $count, $skip, $forward);
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	if ($status < 0) {
		return undef;
	}
	return $self->{result}->keys();
}

sub list_key_values {
	my $self = shift;
	my $start_key;
	my $prefix;
	my $count;
	my $skip;
	my $forward;
	($start_key, $prefix, $count, $skip, $forward) = _list_args(@_);
	my $status = keyspace_client::Keyspace_ListKeyValues($self->{cptr}, $prefix, $start_key, $count, $skip, $forward);
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	if ($status < 0) {
		return undef;
	}
	return $self->{result}->key_values();
}

sub dirty_list_key_values {
	my $self = shift;
	my $start_key;
	my $prefix;
	my $count;
	my $skip;
	my $forward;
	($start_key, $prefix, $count, $skip, $forward) = _list_args(@_);
	my $status = keyspace_client::Keyspace_DirtyListKeyValues($self->{cptr}, $prefix, $start_key, $count, $skip, $forward);
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	if ($status < 0) {
		return undef;
	}
	return $self->{result}->key_values();
}



sub set {
	my $self = $_[0];
	my $key = $_[1];
	my $value = $_[2];
	my $status = keyspace_client::Keyspace_Set($self->{cptr}, $key, $value);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
}

sub test_and_set {
	my $self = $_[0];
	my $key = $_[1];
	my $test = $_[2];
	my $value = $_[3];
	my $status = keyspace_client::Keyspace_TestAndSet($self->{cptr}, $key, $test, $value);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	return $self->{result}->value();
}

sub add {
	my $self = $_[0];
	my $key = $_[1];
	my $num = $_[2];
	my $status = keyspace_client::Keyspace_Add($self->{cptr}, $key, $num);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	return $self->{result}->value();
}

sub delete {
	my $self = $_[0];
	my $key = $_[1];
	my $status = keyspace_client::Keyspace_Delete($self->{cptr}, $key);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
}

sub remove {
	my $self = $_[0];
	my $key = $_[1];
	my $status = keyspace_client::Keyspace_Remove($self->{cptr}, $key);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	return $self->{result}->value();
}

sub rename {
	my $self = $_[0];
	my $src = $_[1];
	my $dst = $_[2];
	my $status = keyspace_client::Keyspace_Rename($self->{cptr}, $src, $dst);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
}

sub prune {
	my $self = $_[0];
	my $prefix = $_[1];
	my $status = keyspace_client::Keyspace_Prune($self->{cptr}, $prefix);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
}

sub set_expiry {
	my $self = $_[0];
	my $key = $_[1];
	my $exptime = $_[2];
	my $status = keyspace_client::Keyspace_SetExpiry($self->{cptr}, $key, $exptime);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
}

sub remove_expiry {
	my $self = $_[0];
	my $key = $_[1];
	my $status = keyspace_client::Keyspace_RemoveExpiry($self->{cptr}, $key);
	if ($status < 0) {
		$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
		return undef;
	}
	if (keyspace_client::Keyspace_IsBatched($self->{cptr})) {
		return undef;
	}
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
}

sub begin {
	my $self = $_[0];
	return keyspace_client::Keyspace_Begin($self->{cptr});
}

sub submit {
	my $self = $_[0];
	my $status = keyspace_client::Keyspace_Submit($self->{cptr});
	$self->{result} = new Keyspace::Result(keyspace_client::Keyspace_GetResult($self->{cptr}));
	return $status;
}

sub cancel {
	my $self = $_[0];
	return keyspace_client::Keyspace_Cancel($self->{cptr});
}

sub status_string {
	my $status = $_[0];
	if ($status == KEYSPACE_SUCCESS) { return "KEYSPACE_SUCCESS"; }
	elsif ($status == KEYSPACE_API_ERROR) { return "KEYSPACE_API_ERROR"; }
	elsif ($status == KEYSPACE_PARTIAL) { return "KEYSPACE_PARTIAL"; }
	elsif ($status == KEYSPACE_FAILURE) { return "KEYSPACE_FAILURE"; }
	elsif ($status == KEYSPACE_NOMASTER) { return "KEYSPACE_NOMASTER"; }
	elsif ($status == KEYSPACE_NOCONNECTION) { return "KEYSPACE_NOCONNECTION"; }
	elsif ($status == KEYSPACE_MASTER_TIMEOUT) { return "KEYSPACE_MASTER_TIMEOUT"; }
	elsif ($status == KEYSPACE_GLOBAL_TIMEOUT) { return "KEYSPACE_GLOBAL_TIMEOUT"; } 
	elsif ($status == KEYSPACE_NOSERVICE) { return "KEYSPACE_NOSERVICE"; }
	elsif ($status == KEYSPACE_FAILED) { return "KEYSPACE_FAILED"; } 
	else { return "<UNKNOWN>"; }
}

sub _list_args {
	my $args = {@_};
	my $prefix = $args->{prefix} || "";
	my $start_key = $args->{start_key} || "";
	my $count = $args->{count} || 0;
	my $skip = $args->{skip} || 0;
	my $forward = $args->{forward} || 1;
	return ($prefix, $start_key, $count, $skip, $forward);
}


1;
