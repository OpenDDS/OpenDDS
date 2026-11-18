.. news-prs: 5308

.. news-start-section: Fixes
- Fixed ``TypeLookupService`` rejecting bitmask TypeObjects with no ``BitmaskTypeFlag`` set, which some DDS implementations send.
  As in DDS-XTypes 1.4, no flags now means appendable.
- ``XmlTypeProvider`` bitmasks are now appendable unless their ``extensibility`` is ``final``, as DDS-XML specifies.
  ``mutable`` bitmasks are rejected, and ``append`` is accepted as a spelling of ``appendable`` for all types.
- Fixed ``XmlTypeProvider`` converting a struct to a minimal TypeObject before the element type of its sequence, array, or map members.
  This logged a spurious ``complete TypeIdentifier not found`` error.
.. news-end-section
