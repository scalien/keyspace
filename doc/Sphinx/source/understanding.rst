.. _understanding:


**********************
Understanding Keyspace
**********************

Replication
===========

In Keyspace all write commands (``set``, ``test_and_set``, ``add``, ``rename``, ``delete``, ``remove``, ``prune``) are appended into a *replicated log*. To guarantee that all nodes end up with the same database, all nodes have to execute the commands in the same order. In other words, the local copies of the replicated log have to be identical. This is called **consistent replication**.

The replicated log is made up of replication rounds. Each round is no more than 500KB in size and can contain several write commands. Keyspace uses a distributed algorithm called *Paxos* in each replication round. For increased performance, several commands, possibly from different clients are batched and replicated together.

Since Paxos is majority vote algorithm, in Keyspace a majority of the nodes have to be alive for the cluster to be able to take write commands. The Paxos algorithm is run over TCP, and uses the port specified in the confuguration file's ``paxos.endpoints`` line.

::

  paxos.endpoints = 192.168.1.50:10000, 192.168.1.51:10000, 192.168.1.52:10000

In this example replication is performed over port 10000.

In order to see replication in action, perform::

  http://<ip-address-of-master-node>:8080/set?testkey,testvalue

repeatedly and check the master's HTTP status at::

  http://<ip-address-of-master-node>:8080

You will see the ``I am in replication round x`` message change to ``x+1`` and so on.

Master leases
=============

Keyspace is a dynamic master-based replicated database. This means that the nodes elect a master, and that master manages client's write commands. Only the master serves so-called safe read commands, while all nodes serve dirty read commands.

In order to become master, a node much acquire the *master lease*. In Keyspace, the master lease lasts for 7 seconds. The master then extends its lease before it expires; this way a master can hold on to the lease for long periods of time.

Keyspace uses the *PaxosLease* algorithm for master leases. Like Paxos, PaxosLease is a majority vote based algorithm, hence a majority of the nodes have to be up for there to be a master. The PaxosLease algorithm is run over TCP, and uses the port specified in the configuration file's ``paxos.endpoints`` line *plus one*. In the example above, PaxosLease runs over port 10001.

When the master node goes down or becomes disconnected from the rest, its lease expires and another node will be elected master. If the old master rejoins the cluster, its lease will have expired and it will participate as a non-master node.

A node that is lagging behind in the replication rounds can never become the master. This is not a limitation but a feature, because if a majority of nodes are available at least on is always up-to-date!

To see master election, find the master node and shut it down using ``Control+C``. While doing so, keep refreshing another node's HTTP status. You wil see the ``Master is node x`` message change to ``Master is node -1 (unknown)`` and then a few tenth of a second later to ``Master is node y`` after the new node is elected.

Catchup
=======

Keyspace stores 100.000 replication rounds on disk by default. This can be changed using the ``rlog.cacheSize`` line of the configuration file. This cache is used to help lagging nodes catch up. For example, if a node goes down at round 1000, the clients issue some write commands and the cluster proceeds to replication round 2000, and then the node rejoins the cluster, the master node will send it rounds 1000-2000 and the node will be up-to-date again. This is called *Paxos-based catchup*.

If a node is out for a longer period of time, and the rest of the cluster performs more than 100.000 replication rounds in the meantime, Paxos-based catchup will not work. In this case, the lagging node will delete its local database and copy over the entire, up-to-date database from the master node. This is called *database-catchup*. When this happens, the node will print::

  Catchup started from node <id-of-master-node>

to its log and::

  Catchup complete

once catchup is complete. If the master node goes down or loses its mastership while a node is catching up the process will fail with the message::

  Catchup failed

You should configure ``rlog.cacheSize`` to be large enough (depending on your workload) so database-catchup very rarely if ever happens, as it involves copying over the entire database!

Dirty read commands
===================

Keyspace differentiates *safe* and *dirty* read commands. Safe commands are only served by the master, while all nodes will always serve dirty read commands. This is because only the master node can guarantee that is has seen all previous write operations, hence only the master can guarantee that the data returned by the read command is not stale. There are no guarantees regarding dirty reads, and since all nodes serve dirty reads the client library can spread them out over the entire cluster, thus resulting in linear speedup.

Additional reading
==================

For more details see the Keyspace and PaxosLease whitepapers available at http://scalien.com/whitepapers.

Paxos is explained in Leslie Lamport's paper `Paxos made simple <http://www.google.com/search?client=opera&rls=en&q=paxos+made+simple&sourceid=opera&ie=utf-8&oe=utf-8>`_.
