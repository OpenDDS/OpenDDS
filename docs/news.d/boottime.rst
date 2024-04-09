.. news-prs: 4568

.. news-start-section: Additions
- Allow compile-time configuration of CLOCK_BOOTTIME as the clock used for timers

  - If the platform supports it, this can be done using ``--boottime`` when building with the configure script or :cmake:var:`OPENDDS_BOOTTIME_TIMERS` when building with CMake.
.. news-end-section
