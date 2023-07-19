.. news-prs: 4203
.. news-start-section: Additions
.. news-rank: 10
- OpenDDS can now be built using CMake in many cases.

  - See :ref:`cmake-building` for details.

.. news-rank: 0
.. news-start-section: CMake Config Package
- Added :cmake:func:`opendds_install_interface_files`.
.. news-rank: 0
.. news-start-section: :cmake:func:`opendds_target_sources`:
.. news-rank: 10
- Added :cmake:func:`opendds_target_sources(INCLUDE_BASE)` to designate a directory structure for IDL files passed in.
.. news-rank: 0
- Added :cmake:func:`opendds_target_sources(USE_VERSIONED_NAMESPACE)`.
.. news-end-section
.. news-end-section
.. news-end-section

.. news-start-section: Deprecations
- Deprecated :cmake:var:`OPENDDS_FILENAME_ONLY_INCLUDES` in favor of :cmake:func:`opendds_target_sources(INCLUDE_BASE)`.
.. news-end-section
