.. _installation:


************
Installation
************

Installing on Debian Linux
==========================

Download the latest Debian package of Keyspace from http://scalien.com/downloads or directly from the command line (replace XXX with the latest version of Keyspace)::

  $ wget http://debian.scalien.com/pool/main/k/keyspace-server/keyspace-server_XXX_i386.deb 

Once you have downloaded the ``.deb`` file, issue the following command to install Keyspace::

  $ dpkg -i keyspace-server_1.5.1_i386.deb

This will install the server and the proper ``init.d`` scripts. Launch Keyspace using::

  $ /etc/init.d/keyspace start

The usual ``start``, ``stop``, ``restart`` commands are available.

Installing from source on Linux and other UNIX platforms
========================================================

Download the latest tarbal of Keyspace from http://scalien.com/downloads or directly from the commnad line (replace XXX with the latest version of Keyspace)::

  $ wget http://scalien.com/releases/keyspace/keyspace-XXX.tgz

Once you have downloaded the ``tgz`` file, issue the following command to extract it::

  $ tar xvf keyspace-1.5.1.tgz

This will create a directory called ``keyspace``, which contains the source code and make files::

  $ cd keyspace
  $ make

This will build the Keyspace executable appropriate for your system. Keyspace has only one dependency required for it to build and run: **BerkeleyDB 4.6 or later**. In case the make process fails, this is most likely because either you do no have BerkeleyDB installed or it is installed in a non-standard location.

How to make sure BerkeleyDB installed?
--------------------------------------

Issue the following command, which will find one of the standard BerkeleyDB header files on your system::

  $ find / -name db_cxx.h

If ``find`` does not return the location of the file ``db_cxx.h``, you do **not** have BerkeleyDB installed. Please consult your distribution's help on how to install it.

I have BerkeleyDB installed, but ``make`` still fails. What now?
----------------------------------------------------------------

First, we locate the directory containing the BerkeleyDB header files using::

  $ find / -name db_cxx.h

Then, edit the ``Makefile.Linux`` file and add the directory containing ``db_cxx.h``. For example, if your system has ``db_cxx.h`` under ``/usr/include/db4.6``, modify ``Makefile.Linux`` to read (note: Makefiles are sensitive to spaces and tabs, **use tabs!**)::

  INCLUDE = \
  	-I$(KEYSPACE_DIR)/src \
  	-I/usr/include/db4.6

Then, find the BerkeleyDB library file using::

  $ find / -name libdb_cxx*

Then, edit the ``Makefile.Linux`` file and add the directory containing ``libdb_cxx.so``. For example, if your system has ``libdb_cxx.so`` under ``/usr/lib/db4.6``, modify Makefile.Linux to read (Makefiles are sensitive to spaces and tabs, **use tabs!**)::

  LDPATH = \
  	-L/usr/lib/db4.6

Now, run ``make`` again in the Keyspace directory.

Installing on Windows
=====================

Download the latest Windows package of Keyspace from http://scalien.com/downloads. Once you have downloaded the file, locate it in your ``Download`` folder. The file is called keyspace-windows-XXX.zip (replace XXX with the latest version of Keyspace). Now use your favorite ``unzip`` program to unzip the package. This will create a directory called ``keyspace``, which contains a ``bin`` directory which contains the executable file ``keyspace.exe``.

Installing from source on Windows
================================= 

Download the latest Windows package of Keyspace from http://scalien.com/downloads. Once you have downloaded the file, locate it in your ``Download`` folder. The file is called keyspace-windows-XXX.zip (replace XXX with the latest version of Keyspace). Now use your favorite ``unzip`` program to unzip the package. This will create a directory called ``keyspace``, which contains a ``keyspace.vcproj`` directory which contains the appropriate Microsoft Visual Studio 2008 solution file. Fire up Visual Studio and hit Build.
