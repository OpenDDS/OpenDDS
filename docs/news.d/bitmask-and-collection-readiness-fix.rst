.. news-prs: 5308

.. news-start-section: Fixes
- Fixed ``TypeLookupService`` rejecting bitmask TypeObjects with no ``BitmaskTypeFlag`` set.
  The flags are unused for bitmasks, so zero is now accepted along with ``IS_FINAL`` and ``IS_APPENDABLE``.
- Fixed ``XmlTypeProvider`` converting a struct to a minimal TypeObject before the element type of its sequence, array, or map members.
  This logged a spurious ``complete TypeIdentifier not found`` error.
.. news-end-section
