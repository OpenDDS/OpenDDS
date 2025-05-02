.. news-prs: 4884

.. news-start-section: Platform Support and Dependencies
.. news-start-section: CMake
- The :ref:`shapes-demo` will now be built when :ref:`cmake-building` and :cmake:var:`OPENDDS_QT` is set to ``TRUE``.
- Fixed :ghissue:`issue with building iShapes demo with CMake <4849>`.
- Improved C++ standard detection in CMake.
- Invalid :cmake:var:`OPENDDS_ACE_TAO_KIND` values will now be explictly rejected.
.. news-end-section
.. news-end-section

.. news-start-section: Documentation
- Fixed minor typo in documentation of :ref:`ace6tao2` option of :cmake:var:`OPENDDS_ACE_TAO_KIND`.
.. news-end-section
