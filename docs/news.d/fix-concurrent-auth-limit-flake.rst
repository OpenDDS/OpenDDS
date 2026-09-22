.. news-prs: 5298

.. news-start-section: Fixes
- Fixed a race in the ``ConcurrentAuthLimit`` security test where SPDP
  announcements from its two fake writers were sent concurrently, so either
  writer could claim the single ``MaxParticipantsInAuthentication`` slot and
  the test would fail intermittently. The test now announces the first writer
  until it is discovered before announcing the second one.

.. news-end-section
