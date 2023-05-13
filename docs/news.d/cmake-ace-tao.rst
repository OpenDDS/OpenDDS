.. news-prs: 4140

.. news-start-section: Additions
.. news-start-section: CMake Config Package
- Added two new config packages called ``OpenDDS-ACE`` and ``OpenDDS-TAO`` that allow using ACE/TAO without OpenDDS being built.
- Added ``SKIP_TAO_IDL`` option to ``opendds_target_sources``.
.. news-end-section
.. news-end-section

.. news-start-section: Fixes
.. news-start-section: CMake Config Package
- Made linking dependencies and macro definitions closer match using MPC with OpenDDS and TAO.
- Fixed configure issues from passing to ``OPENDDS_IDL_OPTIONS -SI`` in ``opendds_target_sources``.
- Reduced unnecessary error logging in cases the packages fail to find dependencies.
.. news-end-section
.. news-end-section

.. news-start-section: Notes
.. news-start-section: CMake Config Package
- ``OPENDDS_TARGET_SOURCES`` is now called ``opendds_target_sources`` as part of an internal style refactor.

  - CMake macros and functions names are case insensitive, so this should have no effect on CMake code.
.. news-end-section
.. news-end-section
