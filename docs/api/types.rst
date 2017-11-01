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

  .. c:member::  SDTError_t errno

    Error code of the last error for this provider

  .. c:member::  char *error

    Error string of the last error for this provider

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

.. c:type:: enum SDTError_t

  Represents all errors thrown by libstapsdt.

  .. c:member:: noError

    This error code means that no error occured so far

  .. c:member:: elfCreationError

    This error code means that we were unable to create an Elf file to store
    our probes

  .. c:member:: tmpCreationError

    This error code means that we were unable to open a temporary file at
    ``/tmp/``. A common mistake here is having a ``/`` in the provider name,
    which will be interpreted by the operating system as a folder.

  .. c:member:: sharedLibraryOpenError

    This error code means that we were unable to open the shared library that we
    just created

  .. c:member:: symbolLoadingError

    This error code means that the we were unable to load a symbol from the
    shared library we just created

  .. c:member:: sharedLibraryCloseError

    This error code means that we were unable to close the shared library for
    this provider


.. c:type:: struct SDTProbeList_t

  Represents a linked-list of :c:type:`SDTProbe_t`. Shouldn't be handled
  manually, use :c:func:`providerAddProbe` instead.

  .. c:member:: SDTProbe_t probe
  .. c:member:: struct SDTProbeList_ *next
