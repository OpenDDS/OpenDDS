.. news-prs: 5171

.. news-start-section: Additions
- Added callbacks for when an OpenDDS thread starts and finishes to control thread behavior.

  - For example on Linux, using :manpage:`pthread_setaffinity_np` to set which CPU core the thread runs on.
  - See ``set_thread_status_listener`` in :ghfile:`dds/DCPS/Service_Participant.h` and ``ThreadStatusListener`` in :ghfile:`dds/DCPS/ThreadStatusManager.h` for details.
.. news-end-section
