.. news-prs: 5297

.. news-start-section: Fixes
- Fixed ``DomainParticipant::delete_multitopic`` deleting this participant's multitopic when passed a multitopic with a matching name that belongs to a different participant.
  It now returns ``RETCODE_PRECONDITION_NOT_MET`` in that case.

.. news-end-section
