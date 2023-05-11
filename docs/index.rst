:hide-toc: true

#######
OpenDDS
#######

.. toctree::
  :hidden:

  building/index
  devguide/index
  internal/index
  glossary

Welcome to the documentation for OpenDDS |release|!

.. ifconfig:: is_release

    It is available :ghrelease:`for download on GitHub`.

.. ifconfig:: not is_release

    .. warning::

        This copy of OpenDDS isn't a release, so this documentation may not be finalized.
        It may be missing documentation on new features or the existing documentation may be incorrect.

        You can find the documentation for the latest release `here <https://opendds.readthedocs.io/en/latest-release/>`_.

OpenDDS is an open-source C++ framework for exchanging data in distributed systems.
See :ref:`introduction--what-is-opendds` for more information.

*************
Using OpenDDS
*************

:doc:`building/index`
  How to build and install OpenDDS

:ref:`introduction--dcps-overview`
  A conceptual overview of how DDS works

:doc:`devguide/getting_started`
  A tutorial on making basic OpenDDS applications

Much more information can be found in the :doc:`devguide/index`

*******************
Other Documentation
*******************

:doc:`internal/index`
  Documentation for OpenDDS contributors

:doc:`glossary`
  A dictionary of common terms

:ref:`genindex`
