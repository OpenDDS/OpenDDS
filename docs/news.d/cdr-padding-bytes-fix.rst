.. news-prs: 5306

.. news-start-section: Fixes
- Fixed the CDR encapsulation header declaring padding bytes at the end of a serialized sample that were never written.
  This affected any encapsulated sample whose length wasn't a multiple of 4 bytes, most visibly with ``@appendable`` and ``@mutable`` types.
  Peers that use the declared padding to determine where the serialized data ends, as DDS-XTypes 1.3 §7.6.3.1.2 requires, would see the sample as truncated.
  The builtin TypeLookup, liveliness, and security messages now declare and write this padding too.
- Readers now exclude declared padding from the serialized data when the padding is present, so it can't be read as a member of an XCDR1 ``@appendable`` type.
  Samples from older OpenDDS writers, which declare the padding without writing it, are still read in full.
  Older OpenDDS readers don't exclude the padding, so an XCDR1 ``@appendable`` member that the writer's type lacks may read as zero instead of being absent.
.. news-end-section
