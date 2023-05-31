.. news-prs: 4140

.. news-start-section: Additions
.. news-start-section: CMake Config Package
- Libraries and features can now be passed to ``find_package(OpenDDS REQUIRED COMPONENTS [components]...)`` to change what the Config package requires and loads.

  - Libraries such as ``OpenDDS::Security`` can be passed to require them.
  - Features such as ``built_in_topics`` or ``safety_profile=FALSE`` can be passed as a convenient way to require those features.
  - ``NO_OPENDDS`` skips loading OpenDDS libraries, which allows using ACE/TAO without OpenDDS being built.
    ``NO_TAO`` also skips loading TAO libraries, leaving just ACE libraries.

.. news-start-section: ``opendds_target_sources``:
- Added option ``SKIP_TAO_IDL`` to disable ``tao_idl``.
- Added option ``SKIP_OPENDDS_IDL`` to disable ``opendds_idl``.
.. news-end-section
.. news-end-section
.. news-end-section

.. news-start-section: Fixes
.. news-start-section: CMake Config Package
- Made linking dependencies and macro definitions closer match using MPC with OpenDDS and TAO.
- Fixed issues with passing ``OPENDDS_IDL_OPTIONS -SI`` to ``opendds_target_sources``.
.. news-end-section
.. news-end-section

.. news-start-section: Notes
.. news-start-section: CMake Config Package
- ``OPENDDS_TARGET_SOURCES`` is now called ``opendds_target_sources``.

  - CMake macros and functions names are case insensitive, so this should have no effect on CMake code.
.. news-end-section
.. news-end-section
