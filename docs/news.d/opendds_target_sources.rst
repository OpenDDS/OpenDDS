.. news-prs: 4962

.. news-start-section: Platform Support and Dependencies
.. news-start-section: CMake
.. news-rank: 10
.. news-start-section: :cmake:func:`opendds_target_sources`
- Added support for using the same IDL files in different targets with different options.
- It will not automatically generate an export header when all IDL files are scoped ``PRIVATE`` unless :cmake:func:`opendds_target_sources(ALWAYS_GENERATE_LIB_EXPORT_HEADER)` is set to ``TRUE``.
- It will not export symbols in code generated from ``PRIVATE`` IDL files.
- Added support for the `codegen <https://cmake.org/cmake/help/latest/policy/CMP0171.html>`__ target (CMake 3.31 or later) to build and run IDL compilers on targets with IDL.
.. news-end-section
.. news-end-section
.. news-end-section
