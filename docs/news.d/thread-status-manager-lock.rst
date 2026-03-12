.. news-prs: 5172

.. news-start-section: Fixes
- ``ThreadStatusManager`` now contains multiple slots for storing thread statuses, each has its own lock.
  Only threads assigned to the same slot contend with each other for the slot's lock.
.. news-end-section
