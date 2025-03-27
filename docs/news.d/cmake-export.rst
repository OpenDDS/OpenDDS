.. news-prs: 4954

.. news-start-section: Platform Support and Dependencies
.. news-start-section: CMake
- Fixes and improvements for :cmake:func:`opendds_export_header`

  - ``opendds_export_header`` and :cmake:func:`opendds_target_sources` now set symbol visibility to hidden using `CXX_VISIBILITY_PRESET <https://cmake.org/cmake/help/latest/prop_tgt/LANG_VISIBILITY_PRESET.html>`__ and `VISIBILITY_INLINES_HIDDEN <https://cmake.org/cmake/help/latest/prop_tgt/VISIBILITY_INLINES_HIDDEN.html>`__ on platforms where this applies.

  - Fixed an issue on Windows where public export macro should be set to override the ACE default.

    - Previously the ``OpenDDS_Util`` library that's only used for ``opendds_idl`` was always a DLL on Windows when building OpenDDS using CMake.
      Now it's always a static library on all platforms.

  - ``opendds_export_header`` now gives control over what names are used.

- Added :cmake:func:`opendds_bigobj` to set ``/bigobj`` on targets that need it on Windows.
.. news-end-section
.. news-end-section
