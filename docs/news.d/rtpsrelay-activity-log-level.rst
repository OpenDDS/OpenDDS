.. news-prs: 5315
.. news-start-section: Additions
.. news-start-section: RtpsRelay
- Added the ``-ActivityLogLevel`` option to control the verbosity of RtpsRelay activity logging.
  The levels range from no activity logging through expiration and refresh processing, with each level including
  the lower levels.

  - The existing ``-LogActivity`` option is deprecated but remains supported for compatibility. When both options are
    specified, ``-ActivityLogLevel`` takes precedence.
  - The default activity log level is 1 (log lifecycle events) whereas the default for ``-LogActivity`` is 0 (disabled).

.. news-end-section
.. news-end-section
