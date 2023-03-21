#######
OpenDDS
#######

Welcome to the documentation for OpenDDS |release|!

.. ifconfig:: is_release

    It is available :ghrelease:`for download on GitHub`.

.. ifconfig:: not is_release

    .. warning::

        This copy of OpenDDS isn’t a release copy, so this documentation may not be finalized.
        It may be missing documentation on new features or the existing documentation may be incorrect.

*****************
Developer's Guide
*****************

.. toctree::
  :maxdepth: 3

  devguide/index
  Common Terms <devguide/common_terms>

**********************
Internal Documentation
**********************

This documentation are for those who want to contribute to OpenDDS and those who are just curious!

.. toctree::
  :maxdepth: 2

  Development Guidelines <internal/dev_guidelines>
  Documentation Guidelines <internal/docs>
  Unit Testing Guidelines <internal/unit_tests>
  Github Actions <internal/github_actions>
  Running Tests <internal/running_tests>
  Bench <internal/bench>

******************
Indices and tables
******************

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
