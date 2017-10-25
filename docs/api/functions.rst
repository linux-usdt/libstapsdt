#########
Functions
#########


.. c:function:: SDTProvider_t *providerInit(const char *name)

  This function received a name as argument, creates a :c:type:`SDTProvider_t`
  with all attributes correctly initialized and then returns a pointer to it,
  or ``NULL`` if there was an error.

  :param string name: The name of this provider
  :return: A pointer to the new provider
  :rtype: SDTProvider_t*
  :type str: const char*

.. c:function:: SDTProbe_t *providerAddProbe(SDTProvider_t *provider, const char *name, int argCount, ...)

  This function received a :c:type:`SDTProvider_t` created by
  :c:func:`providerInit`, a name and any number of :c:type:`ArgType_t` (you
  must pass the actual number of arguments as argCount). It will then
  create a :c:type:`SDTProbe_t` with all attributes correctly initialized and
  register it to the given :c:type:`SDTProvider_t`. The return would be a
  pointer to the created :c:type:`SDTProbe_t`, or ``NULL`` if there was an
  error.

  :param SDTProvider_t* provider: The provider where this probe will be created
  :param string name: The name of this probe
  :param int argCount: The number of arguments accepted by this probe
  :param SDTArgTypes_t ...: Any number of arguments (number of arguments must
    match argCount)
  :return: A pointer to the new provider
  :rtype: SDTProbe_t*

.. c:function:: int providerLoad(SDTProvider_t *provider)

  When you created all probes you wanted, you should call this function to load
  the provider correctly. The returning value will be ``0`` if everything
  went well, or another number otherwise.

  After calling this function, all probes will be efectively
  available for tracing, and you shouldn't add new probes or load this
  provider again.

  :param SDTProvider_t* provider: The provider to be loaded
  :return: A status code (``0`` means success, other numbers indicates error)
  :rtype: int


.. c:function:: int providerUnload(SDTProvider_t *provider)

  Once you don't want your probes to be available anymore, you can call this
  function. This will clean-up everything that :c:func:`providerLoad` did. The
  returning value will be ``0`` if everything went well, or another number
  otherwise.

  After calling this function all probes will be unavailable for tracing, and
  you can add new probes to the provider again.

  :param SDTProvider_t* provider: The provider to be unloaded
  :return: A status code (``0`` means success, other numbers indicates error)
  :rtype: int

.. c:function:: void providerDestroy(SDTProvider_t *provider)

  This function frees a :c:type:`SDTProvider_t` from memory, along with all
  registered :c:type:`SDTProbe_t` created with :c:func:`providerAddProbe`.

  :param SDTProvider_t* provider: The provider to be freed from memory, along
    with it's probes

.. c:function:: void probeFire(SDTProbe_t *probe, ...)

  This function fires a probe if it's available for tracing (which means it
  will only fire the probe if :c:func:`providerLoad` was called before).

  :param SDTProbe_t* probe: The probe to be fired
  :param any ...: Any number of arguments (must match the expected
    number of arguments for this probe)

.. c:function:: int probeIsEnabled(SDTProbe_t *probe)

  This function returns ``1`` if the probe is being traced, or ``0`` otherwise.

  :param SDTProbe_t* probe: The probe to be checked
  :return: ``1`` if probe is enabled, ``0`` otherwise
  :rtype: int
