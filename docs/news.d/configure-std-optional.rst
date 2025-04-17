.. news-prs: 4976

.. news-start-section: Platform Support and Dependencies
- OpenDDS's use of ``std::optional`` or an emulation is determined by the configuration file

  - See ``OPENDDS_CONFIG_STD_OPTIONAL`` in ``dds/OpenDDSConfig.h``
  - Default is to use ``std::optional`` on compilers that support it
  - See configure script's ``--no-std-optional`` or CMake's :cmake:var:`OPENDDS_STD_OPTIONAL`
.. news-end-section
