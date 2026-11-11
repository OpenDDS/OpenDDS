.. news-prs: 0

.. news-start-section: Removals
- Removed the DCPS Information Repository's undocumented multicast discovery responder, which was enabled by the ``MulticastDiscoveryEnabled`` environment variable.

  - It never replied to requests, and a valid request could crash the ``DCPSInfoRepo``.

.. news-end-section
