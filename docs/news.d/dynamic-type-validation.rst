.. news-prs: 5356

.. news-start-section: Security
- ``DynamicType``\s built from types received from peers, by ``DomainParticipant::get_dynamic_type`` and by recorders, are now checked before use.
  A type that is incomplete, contains itself without an optional member or a sequence or map in between, or is nested deeper than :prop:`[common]DCPSMaxNestingDepth` is rejected.
  Such types could make reading, comparing, or creating data recurse without end.
.. news-end-section
