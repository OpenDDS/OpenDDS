.. news-prs: 5334

.. news-start-section: Fixes
- Reduced the reassembly work the ``multicast`` and ``udp`` transports do for each gap in received datagrams.
  Expired partially-received samples are now also discarded at that point.

.. news-end-section
