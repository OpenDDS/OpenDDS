.. news-prs: 0

.. news-start-section: Fixes
- Fixed the CDR encapsulation header declaring padding bytes at the end of a serialized sample that were never written.
  This affected any encapsulated sample whose length wasn't a multiple of 4 bytes, most visibly with ``@appendable`` and ``@mutable`` types.
  Peers that subtract the declared padding from the received length, as DDS-XTypes requires, would see the sample as truncated.
.. news-end-section
