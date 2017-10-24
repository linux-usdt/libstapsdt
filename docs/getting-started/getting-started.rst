###############
Getting Started
###############

``libstapsdt``

========
Packages
========

We currently offer package installation for Ubuntu via PPA.

Ubuntu via PPA
--------------

To install ``libstapsdt`` on Ubuntu, you need to add Sthima's Open Source
Software PPA first, and then you'll be able to install it:

.. code-block:: bash

  sudo add-apt-repository ppa:sthima/oss
  sudo apt-get update
  sudo apt-get install libstapsdt0 libstapsdt-dev


=================
Build from source
=================

If you need to install libstapsdt in a different distribution or want to use the
latest version, you can build it from source as follow.

Dependencies
------------

First, you need to install ``libstapsdt`` dependencies. For now, the only
dependency is libelf (from elfutils).

Ubuntu
......

.. code-block:: bash

  sudo apt install libelf1 libelf-dev


Build
-----

To build and install libstapsdt, you just need to run:

.. code-block:: bash

  make
  sudo make install
  sudo ldconfig

Now ``libstapsdt`` is installed on your system!

Demo
----

There's a demo program available. To build it, run:

.. code-block:: bash

  make demo  # Executable will be available at ./demo

You can then try it by running:

.. code-block:: bash

  ./demo PROVIDER_NAME PROBE_NAME

After running the demo program, you can.

Here's an example using the latest version of
`iovisor/bcc <https://github.com/iovisor/bcc>`_'s ``trace`` tool:

.. code-block:: bash

  sudo /usr/share/bcc/tools/trace -p $(pgrep demo) 'u::PROBE_NAME'
