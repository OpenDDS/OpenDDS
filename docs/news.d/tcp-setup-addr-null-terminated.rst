.. news-prs: 5323

.. news-start-section: Fixes
- The passive side of the TCP transport now rejects a connection setup message whose address length is zero or larger than 1024 bytes, or whose address isn't null terminated within its declared length, instead of allocating a buffer of that size and reading past the end of it.

.. news-end-section
