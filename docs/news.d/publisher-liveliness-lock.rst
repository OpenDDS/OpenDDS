.. news-prs: 0

.. news-start-section: Fixes
- Fixed a data race where the participant's liveliness timers and ``DomainParticipant::assert_liveliness`` read a publisher's data writers while another thread created or deleted a data writer.
  This also removes a possible deadlock between the liveliness timers and ``DomainParticipant::delete_contained_entities``.

.. news-end-section
