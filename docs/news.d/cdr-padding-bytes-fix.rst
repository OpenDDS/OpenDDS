.. news-prs: 5306

.. news-start-section: Fixes
- Fixed the CDR encapsulation header declaring padding bytes at the end of a serialized sample that were never written.
  This affected any encapsulated sample whose length wasn't a multiple of 4 bytes, most visibly with ``@appendable`` and ``@mutable`` types.
  Peers that use the declared padding to determine where the serialized data ends, as DDS-XTypes 1.3 §7.6.3.1.2 requires, would see the sample as truncated.
.. news-end-section
