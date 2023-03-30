#######
OpenDDS
#######

Welcome to the documentation for OpenDDS |release|!

.. ifconfig:: is_release

    It is available :ghrelease:`for download on GitHub`.

.. ifconfig:: not is_release

    .. warning::

        This copy of OpenDDS isn't a release, so this documentation may not be finalized.
        It may be missing documentation on new features or the existing documentation may be incorrect.

        You can find the documentation for the latest release `here <https://opendds.readthedocs.io/en/latest-release/>`_.

*****************
Developer's Guide
*****************

.. toctree::
  :maxdepth: 2

  devguide/preface
  devguide/introduction
  devguide/getting_started
  devguide/quality_of_service
  devguide/conditions_and_listeners
  devguide/content_subscription_profile
  devguide/built_in_topics
  devguide/run_time_configuration
  devguide/opendds_idl
  devguide/the_dcps_information_repository
  devguide/java_bindings
  devguide/modeling_sdk
  devguide/alternate_interfaces_to_data
  devguide/safety_profile
  devguide/dds_security
  devguide/internet_enabled_rtps
  devguide/xtypes
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
