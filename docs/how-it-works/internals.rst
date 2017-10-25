################
Behind the Scene
################

.. attention::

  This page is intended for advanced users, and it assumes working knowledge of
  **Elf** files, **shared libraries** and **software instrumentation**.

``libstapsdt`` uses Systemtap SDT format to create runtime SDT probes -
therefore the reason for its name. So why write yet another library for SDT
probes instead of using Systemtap?

=======================
How Systemtap SDT works
=======================

Systemtap uses compiler macros to register its SDT probes, making it impossible
to have probes registered during runtime. An example is shown below, where we
register a probe called `Probe` to a provider called `Provider`.

.. code-block:: c

  #include <sys/sdt.h>

  int main() {
    DTRACE_PROBE(Provider, Probe);
    return 0;
  }


The resulting binary from this code will have a new Elf section called
``.stapsdt.base``, located right after the code (usually being the ``.text``
section). This base is relevant to help tracing tools to calculate the memory
address of any probe after the binary is loaded into memory.

..
  explain why tracing tools needs to calculate the address after a binary
  is loaded into memory

It will also have a Elf note, where all probes data (name, address,
semaphores, arguments) will be stored to be read later by any tracing tool.
The compiler will also replace our ``DTRACE_PROBE`` macro with a function call,
and that's where the probe points to, allowing it to easily pass arguments to
the probe. This function is a ``no-op``.

.. code-block:: python

  Displaying notes found at file offset 0x00001064 with length 0x0000003c:
    Owner                 Data size       Description
    stapsdt              0x00000028       NT_STAPSDT (SystemTap probe descriptors)
      Provider: Provider
      Name: Probe
      Location: 0x00000000004004da, Base: 0x0000000000400574, Semaphore: 0x0000000000000000
      Arguments:

There's more information about how Systemtap implements their SDT probes
`here <https://sourceware.org/systemtap/wiki/UserSpaceProbeImplementation>`_.

==================================
Roses are Red, Violets are Blue...
==================================

**And shared libraries are Elf files!** (on most UNIX systems at least)

Ok, so now we know that Systemtap uses Elf properties to inform tracing tools
about registered probes. We also know that they have a well-defined and rather
simple strucutre. One which can easily be implemented.

But we can't edit our binary just to add new Elf notes pointing to new probes,
and most Systemtap-SDT-capable tracing tools will only look at the binary and
not at the running process for this information.

That means we need to generate an Elf file at runtime, add our probes to it, and
then use it in our running process in a way that our tracing tools will find
it, which means... Shared libraries!

The "secret source" used by ``libstapsdt`` to allow Systemtap SDT probes
registration at runtime is shared libraries. Our public API is rather simple,
but the library has quite some code. Most of this code is used to generate a
shared library from scratch, dynamically adding code to it and registering all
probes as Elf notes.

.. image:: ../_static/internal-flow.svg

The shared library is created (with help from ``libelf``) and loaded into memory
(by using ``dlopen()`` when :c:func:`providerLoad` is executed. That's why it's
not possible to add new probes after a provider is loaded. It's also worth
noting that each provider will generate exactly one shared library when loaded,
and providers don't share a shared library.
