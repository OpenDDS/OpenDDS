# This will have to be replaced with the PR number after the PR is created:
.. news-prs: 0

.. news-start-section: Fixes
- Fixed misaligned XCDR1 skipping of a ``@mutable`` union nested in a ``@final`` or ``@appendable`` type.

  - ``MetaStruct::getValue()`` (used for content-filtered topics, multi-topic, and query conditions) skipped a nested mutable union without consuming its XCDR1 parameter-list sentinel, desynchronizing the read of any following field.
  - The MetaStruct test now also runs with XCDR1.

.. news-end-section

.. news-start-section: Removals
- Removed the stale "unsupported combination of XCDR1 encoding and appendable extensibility" discovery warning.

  - XCDR1 with appendable (and mutable) types is supported; :ref:`xtypes--xcdr1-support` now describes the encoding's actual limitations instead.

.. news-end-section
