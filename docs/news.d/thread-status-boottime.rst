.. news-prs: 4677

.. news-start-section: Additions
- Added a new data member, monotonic_timestamp to the InternalThreadBuiltInTopic IDL struct.
  - monotonic_timestamp is the time of the sample was written (time of last update of this instance) on the monotonic clock.
  - On systems that don't support a monotonic clock, this will be the same value as the corresponding SampleInfo's source_timestamp.
.. news-end-section

.. news-start-section: Fixes
- When :cfg:prop:`DCPSThreadStatusInterval` is enabled, threads that run the ACE Reactor now use timers instead of a time-limited select() system call to update the InternalThreadBuiltInTopic.
  - This allows the InternalThreadBuiltInTopic to be updated accurately on systems that suspend/resume and are configured for boottime timers.
.. news-end-section
