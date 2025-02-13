# This is a news fragment example. Copy this file in this directory and change
# it to have content added to the release notes for the next release.
# See https://opendds.readthedocs.io/en/master/internal/docs.html#news for details

# This will have to be replaced with the PR number after the PR is created:
.. news-prs: 4887

.. news-start-section: Fixes
- Added ``RTPS_HARVEST_THREAD_STATUS`` property to select the participant that harvests thread status.

  - This addresses erroneous results from multiple participants harvesting thread status.

  - See :ref:`_built_in_topics--openddsinternalthread-topic` for usage.
.. news-end-section
