.. news-prs: 4973

.. news-start-section: Platform Support and Dependencies
.. news-start-section: CMake
- Fixed :ghissue:`issue with CMake being unable to find RapidJSON <4905>` after installing OpenDDS.

.. news-start-section: Building OpenDDS with CMake
- GoogleTest will no longer be installed if it was built as part of the OpenDDS tests.
- Added :cmake:var:`OPENDDS_INSTALL_RAPIDJSON` to disable installing RapidJSON automatically.
- :cmake:var:`OPENDDS_ACE` and :cmake:var:`OPENDDS_TAO` can now be overrode after OpenDDS is installed.
.. news-end-section
.. news-end-section
.. news-end-section
