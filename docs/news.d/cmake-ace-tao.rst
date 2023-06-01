.. news-prs: 4140

.. news-start-section: Additions
.. news-start-section: CMake Config Package
- Libraries and features can be passed to ``find_package(OpenDDS COMPONENTS)`` to change what is required.

  - See :ref:`cmake-components` for details.

.. news-start-section: :cmake:func:`opendds_target_sources`:
- Added :cmake:func:`opendds_target_sources(GENERATE_SERVER_SKELETONS)` to allow ``tao_idl`` to generate code for CORBA servers.
- Added :cmake:func:`opendds_target_sources(AUTO_LINK)` as a fine-grained version of :cmake:var:`OPENDDS_AUTO_LINK_DCPS`.
- Added :cmake:func:`opendds_target_sources(SKIP_TAO_IDL)` to disable ``tao_idl``.
- Added :cmake:func:`opendds_target_sources(SKIP_OPENDDS_IDL)` to disable ``opendds_idl``.
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
