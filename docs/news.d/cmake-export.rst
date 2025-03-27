.. news-prs: 4954

.. news-start-section: Platform Support and Dependencies
.. news-start-section: CMake
- Fixes and improvements for :cmake:func:`opendds_export_header`:

  - ``opendds_export_header`` now allows specifying an existing export header and gives control over what names are used.

  - ``opendds_export_header`` and :cmake:func:`opendds_target_sources` now set symbol visibility to hidden using `CXX_VISIBILITY_PRESET <https://cmake.org/cmake/help/latest/prop_tgt/LANG_VISIBILITY_PRESET.html>`__ and `VISIBILITY_INLINES_HIDDEN <https://cmake.org/cmake/help/latest/prop_tgt/VISIBILITY_INLINES_HIDDEN.html>`__ on platforms where this applies.

  - Fixed Visual Studio failing because of inconsistent linkage of exported symbols when linking between a DLL and a static library.

- Added :cmake:func:`opendds_bigobj` to set ``/bigobj`` on targets that need it on Windows.
.. news-end-section
.. news-end-section
