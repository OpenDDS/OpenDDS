.. news-prs: 4492

.. news-start-section: Additions
- Allow compile-time configuration of CLOCK_BOOTTIME as the monotonic time source

  - When the platform supports it, this can be done using ``--boottime`` when building with the configure script or :cmake:var:`OPENDDS_MONOTONIC_USES_BOOTTIME` when building with CMake.
.. news-end-section
