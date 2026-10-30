.. news-prs: 5323

.. news-start-section: Fixes
- The passive side of the TCP transport now rejects a connection setup message whose address isn't null terminated within its declared length, instead of reading past the end of the setup buffer.

.. news-end-section
