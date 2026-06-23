.. news-prs: 5138

.. news-start-section: Additions
- ``RtpsRelayControl`` can now specify the partitions to be denied from ``RtpsRelay`` instances.

  - A new option ``-Deny`` specifies a single denied partition.
    Multiple ``-Deny`` options can be used within a single invocation of ``RtpsRelayControl``.
  - A new option ``-DenyReadersCount`` specifies the number of readers to wait for before sending
    the denied partitions.

.. news-end-section
