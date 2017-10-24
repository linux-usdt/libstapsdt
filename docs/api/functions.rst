#########
Functions
#########


.. c:function:: SDTProvider_t *providerInit(const char *name)

  This function received a name as argument, creates a :c:type:`SDTProvider_t`
  with all attributes correctly initialized and then returns a pointer to it,
  or ``NULL`` if there was an error.

.. c:function:: SDTProbe_t *providerAddProbe(SDTProvider_t *provider, const char *name, int argCount, ...)

  This function received a :c:type:`SDTProvider_t` created by
  :c:func:`providerInit`, a name and any number of :c:type:`ArgType_t` (you
  must pass the actual number of arguments as argCount). It will then
  create a :c:type:`SDTProbe_t` with all attributes correctly initialized and
  register it to the given :c:type:`SDTProvider_t`. The return would be a
  pointer to the created :c:type:`SDTProbe_t`, or ``NULL`` if there was an
  error.

.. c:function:: int providerLoad(SDTProvider_t *provider)

  When you created all probes you wanted, you should call this function to load
  the provider correctly. The returning value will be ``0`` if everything
  went well, or another number otherwise.

  After calling this function, all probes will be efectively
  available for tracing, and you shouldn't add new probes or load this
  provider again.

.. c:function:: int providerUnload(SDTProvider_t *provider)

  Once you don't want your probes to be available anymore, you can call this
  function. This will clean-up everything that :c:func:`providerLoad` did. The
  returning value will be ``0`` if everything went well, or another number
  otherwise.

  After calling this function all probes will be unavailable for tracing, and
  you can add new probes to the provider again.

.. c:function:: void providerDestroy(SDTProvider_t *provider)

  This function frees a :c:type:`SDTProvider_t` from memory, along with all
  registered :c:type:`SDTProbe_t` created with :c:func:`providerAddProbe`.

.. c:function:: void probeFire(SDTProbe_t *probe, ...)

  This function fires a probe if it's available for tracing (which means it
  will only fire the probe if :c:func:`providerLoad` was called before).

.. c:function:: int probeIsEnabled(SDTProbe_t *probe)

  This function returns ``1`` if the probe is being traced, or ``0`` otherwise.
