# This will have to be replaced with the PR number after the PR is created:
.. news-prs: 0

.. news-start-section: Security
- Fixed an unchecked allocation in XTypes ``TypeObject`` deserialization: the hand-written readers for ``LBoundSeq``, ``SBoundSeq``, and ``UnionCaseLabelSeq`` resized their destination buffer to a wire-supplied element count before validating it against the remaining input, so a single small, malformed discovery parameter could drive a multi-gigabyte allocation.

.. news-end-section

.. news-start-section: Additions
- Appendable and mutable XCDR1/XCDR2 structs and unions now report a finite ``serialized_size_bound()``/``key_only_serialized_size_bound()`` when their contents are actually bounded, instead of always being treated as unbounded.

  - This lets a ``DataWriter`` for such a topic use the pre-allocated sample buffer pool instead of always allocating from the heap, and lets the RTPS ``KeyHash`` builtin parameter embed the key directly instead of always hashing it, the same as it already did for final types.

- Sequences can now be used as ``@key`` fields (directly, or nested in a keyed struct or union), including sequences of structs and unions.
  A sequence key is treated as a single key value; unlike an array, it isn't expanded into one key per element, since its length isn't known at compile time.

.. news-end-section
