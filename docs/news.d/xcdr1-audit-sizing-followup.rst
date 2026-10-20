.. news-prs: 0

.. news-start-section: Security
- Fixed unchecked allocations when deserializing ``TypeObject`` bound and union label sequences, where a small malformed discovery parameter could cause a multi-gigabyte allocation.
.. news-end-section

.. news-start-section: Additions
- Appendable and mutable structs and unions now report a finite ``serialized_size_bound()`` and ``key_only_serialized_size_bound()`` when their contents are bounded.
  This lets a ``DataWriter`` for such a topic use its preallocated sample buffers instead of always allocating from the heap.
- Sequences can now be used as ``@key`` fields, directly or nested in a keyed struct or union, including sequences of structs and unions.
  Unlike an array, a sequence key is treated as a single key value, since its length isn't known at compile time.
.. news-end-section
