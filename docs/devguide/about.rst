****************
About This Guide
****************

..
    Sect<0.3>

This Developer's Guide corresponds to OpenDDS version |release|.
This guide is primarily focused on the specifics of using and configuring OpenDDS to build distributed publish-subscribe applications.
While it does give a general overview of the OMG Data Distribution Service, this guide is not intended to provide comprehensive coverage of the specification.
The intent of this guide is to help you become proficient with OpenDDS as quickly as possible.
Readers are encouraged to submit corrections to this guide using a GitHub pull request.
The source for this guide can be found at :ghfile:`docs/devguide` and :doc:`/internal/docs` contains guidance for editing and building it.

Conventions
===========

..
    Sect<0.4.2>

This guide uses the following conventions:

.. list-table::
   :header-rows: 0

   * - ``Fixed pitch text``

     - Indicates example code or information a user would enter using a keyboard.

   * - *Italic text*

     - Indicates a point of emphasis.

   * - ...

     - An ellipsis indicates a section of omitted text.

``DDS_ROOT``
============

This guide refers to the ``DDS_ROOT`` environment variable which should point to the base directory of the OpenDDS distribution.
Often, this will be written as ``$DDS_ROOT`` indicating the value of the environment variable.

Examples
========

..
    Sect<0.5>

The examples in this guide are intended for the learning of the reader and should not be considered to be "production-ready" code.
In particular, error handling is sometimes kept to a minimum to help the reader focus on the particular feature or technique that is being presented in the example.
The source code for all these examples is available as part of the OpenDDS source code distribution in the :ghfile:`DevGuideExamples` directory.
MPC files are provided with the examples for generating build-tool specific files, such as GNU Makefiles or Visual C++ project and solution files.
To run an example, execute the ``run_test.pl`` Perl script.
