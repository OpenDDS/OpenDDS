.. news-prs: 0

.. news-start-section: Fixes
- Fixed parsing of comma-separated ``StringList`` config values (e.g.
  ``transports`` in a ``[config/…]`` section) so that whitespace around each
  element is trimmed.
  ``transports=net1, net2`` now resolves ``net2`` instead of failing with
  "The inst ( net2) in [config/…] section is undefined."
  See `GitHub issue #1470 <https://github.com/OpenDDS/OpenDDS/issues/1470>`__.

.. news-end-section
