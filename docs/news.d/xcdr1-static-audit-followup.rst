.. news-prs: 5314

.. news-start-section: Fixes
- Fixed XCDR1 content filters, multitopics, and query conditions misreading fields after a ``@mutable`` union nested in a ``@final`` or ``@appendable`` type.
.. news-end-section

.. news-start-section: Removals
- Removed the stale "unsupported combination of XCDR1 encoding and appendable extensibility" discovery warning.
  :ref:`xtypes--xcdr1-support` now describes XCDR1's actual limitation for nested ``@appendable`` types.
.. news-end-section
