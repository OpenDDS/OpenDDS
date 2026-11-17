.. news-prs: 5328

.. news-start-section: Fixes
- Fixed crashes and wrong return codes when using entities that were created but never enabled.

  - Deleting a never-enabled ``DataWriter``, ``DataReader`` (including a ``MultiTopic`` reader), ``Topic``, or ``DomainParticipant``, or calling ``delete_contained_entities`` on their parents, now succeeds and cleans them up.
    ``delete_publisher`` and ``delete_subscriber`` now count never-enabled writers and readers as contained entities.
  - ``DataWriter`` and ``DataReader`` operations such as ``write``, ``register_instance``, ``assert_liveliness``, ``wait_for_acknowledgments``, ``read``, ``take``, and ``wait_for_historical_data`` now return ``RETCODE_NOT_ENABLED`` on a disabled entity.
  - ``set_qos`` on a never-enabled ``DomainParticipant`` or ``Topic`` no longer crashes or fails.

.. news-end-section
