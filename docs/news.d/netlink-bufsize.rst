
.. news-prs: 5142

.. news-start-section: Fixes
- Increase netlink receive buffer size.

  - Recommended netlink receive `buffer size <https://www.kernel.org/doc/html/latest/userspace-api/netlink/intro.html#buffer-sizing>`__ is 32kB.
    Buffer size smaller than 8kB may lead to unexpected truncation of received data.

.. news-end-section

