.. news-prs: 5311

.. news-start-section: Fixes
- Fixed ``long double`` (float128) values being sent in the native ``long double`` format instead of IEEE 754 binary128 on platforms such as x86-64 Linux, Intel macOS, and PowerPC with IBM double-double ``long double``.
  The JSON hex representation of float128 values in ``DynamicData`` is now big-endian binary128 on all platforms as well.

  - *Backwards Compatibility Note*: On those platforms, ``long double`` values exchanged with older OpenDDS versions will be misread.

.. news-end-section
