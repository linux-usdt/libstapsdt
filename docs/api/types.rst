#####
Types
#####


.. c:type:: struct SDTProvider_t

  Represents a USDT Provider. A USDT Provider is basically a namespace for USDT
  probes. Shouldn't be created manually, use :c:func:`providerInit` instead.

  .. c:member::  char *name

    Provider's name

  .. c:member::  SDTProbeList_t *probes

    Linked-list of registered probes

.. c:type:: struct SDTProbe_t

  Represents a USDT Probe. A probe is basically a software breakpoint.
  Shouldn't be created manually, use :c:func:`providerAddProbe` instead.

  .. c:member:: char *name

    Probe's name

  .. c:member:: ArgType_t argFmt[MAX_ARGUMENTS]

    Array holding all arguments accepted by this probe.

  .. c:member:: struct SDTProvider *provider

    Pointer to this probe's provider

  .. c:member:: int argCount

    Number of accepted arguments

.. c:type:: enum SDTArgTypes_t

  Represents all accepted arguments defined by Systeptap's SDT probes.

  .. c:member:: noarg

    No argument

  .. c:member:: uint8

    8 bits unsigned int

  .. c:member:: int8

    8 bits signed int

  .. c:member:: uint16

    16 bits unsigned int

  .. c:member:: int16

    16 bits signed int

  .. c:member:: uint32

    32 bits unsigned int

  .. c:member:: int32

    32 bits signed int

  .. c:member:: uint64

    64 bits unsigned int

  .. c:member:: int64

    64 bits signed int


.. c:type:: struct SDTProbeList_t

  Represents a linked-list of :c:type:`SDTProbe_t`. Shouldn't be handled
  manually, use :c:func:`providerAddProbe` instead.

  .. c:member:: SDTProbe_t probe
  .. c:member:: struct SDTProbeList_ *next
