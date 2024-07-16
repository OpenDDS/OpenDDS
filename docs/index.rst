:hide-toc: true

#######
OpenDDS
#######

.. toctree::
  :hidden:

  news
  devguide/index
  internal/index
  glossary
  genindex

Welcome to the documentation for OpenDDS |release|!

OpenDDS is an open-source framework for exchanging data in distributed systems.
See :ref:`introduction--what-is-opendds` for more information.

.. ifconfig:: is_release

    OpenDDS |release| is available :ghrelease:`for download on GitHub`.

.. ifconfig:: not is_release

    .. warning::

        This copy of OpenDDS isn't a release, so this documentation may not be finalized.
        It may be missing documentation on new features or the existing documentation may be incorrect.

        You can find the documentation for the latest release `here <https://opendds.readthedocs.io/en/latest-release/>`_.

Looking for the documentation for another version of OpenDDS?
The documentation for version 3.24.0 onwards is hosted on `Read the Docs <https://readthedocs.org/projects/opendds/>`__.
The Developer's Guide PDFs for versions before 3.24.0 are available on `GitHub <https://github.com/OpenDDS/OpenDDS/releases>`__.
They are attached to their corresponding releases as ``OpenDDS-VERSION.pdf``.

*************
Using OpenDDS
*************

:ref:`quickstart`
  Download and compile OpenDDS and then run an example application

:ref:`introduction--dcps-overview`
  A conceptual overview of the Data Distribution Service

:ref:`introduction--what-is-opendds`
  OpenDDS is an open-source DDS

:doc:`devguide/building/index`
  How to build and install OpenDDS

:doc:`devguide/getting_started`
  A tutorial on making basic OpenDDS applications

:doc:`devguide/index`
  The OpenDDS Developer's Guide has more detailed documentation

*******************
Other Documentation
*******************

:doc:`news`
  What are the latest changes in OpenDDS?

:doc:`internal/index`
  Documentation for OpenDDS contributors

:doc:`glossary`
  A dictionary of common terms

:ref:`genindex`
