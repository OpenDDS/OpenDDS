.. news-start-section: Fixes
- C++11 generated unions now use a common discriminator-to-member index to manage member lifetimes.
  Repeated member modifiers reuse the active member, and copy/move assignment releases an active member when the source selects an implicit default.
  Discriminator modifiers and member accessors throw ``CORBA::BAD_PARAM`` when they would select or access an inactive member; discriminator values selecting the same member remain valid.
.. news-end-section
