.. libstapsdt documentation master file, created by
   sphinx-quickstart on Wed Oct  4 13:29:04 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

##########################################
libstapsdt - Runtime USDT probes on Linux!
##########################################

**libstapsdt** is a library which allows creating and firing Systemtap's USDT
probes at runtime. It's inspired on
`chrisa/libusdt <https://github.com/chrisa/libusdt/>`_. The goal of this
library is to add USDT probes functionality to dynamic languages.

Motivation
==========

Systemtap's USDT implementation allows only statically defined probes because
they are set as ELF notes by the compiler. To create probes at runtime,
`libstapsdt` takes advantage of shared libraries: it creates a small
library with an ELF note and links it at runtime. This way, most existing tools
will keep working as expected.


Table of Contents
=================


.. toctree::
  :maxdepth: 1

  getting-started/getting-started
  how-it-works/index
  wrappers/wrappers
  api/api
