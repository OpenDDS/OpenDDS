.. news-prs: 5208

.. news-start-section: Additions
- Added ``DCPSEventDispatcherThreads`` and transport ``event_dispatcher_threads`` runtime-configuration options to control ``EventDispatcher`` thread counts.
  Setting transport ``event_dispatcher_threads=0`` reuses the global dispatcher instead of creating a transport-local dispatcher.

.. news-end-section
