#############
Running Tests
#############

***************
Main Test Suite
***************

Building
========

Tests are not built by default, ``--tests`` must be passed to the ``configure`` script.
This will build all the tests.
There are a few ways to only have specific tests built:

* If using Make, specify the targets instead of leaving it default to the ``all`` target.
* Run MPC on the test directory and build separately.
  Make sure to also build the test's dependencies.
* Create a custom workspace with the tests and pass it to the ``configure`` script using the ``--workspace`` option.
  Also make sure to include the test's dependencies.

Running
=======

.. note:: Make sure :envvar:`ACE_ROOT` and :envvar:`DDS_ROOT` are set, which can be done by running ``source setenv.sh`` on Linux and macOS or ``call setenv.cmd`` on Windows.

OpenDDS' main suite of tests is ran by the :ghfile:`tests/auto_run_tests.pl` Perl script that reads lists of tests from files and selectively runs based on how the script has been configured.
By default it configures itself, but it can be configured manually.

For Unixes (Linux, macOS, BSDs, etc)
------------------------------------

Run this in :envvar:`DDS_ROOT`::

  ./bin/auto_run_tests.pl

For Windows
-----------

Run this in :envvar:`DDS_ROOT`::

  bin\auto_run_tests.pl

If OpenDDS was built in Release mode add ``-ExeSubDir Release``.
If it was built as static libraries add ``-ExeSubDir Static_Debug`` or ``-ExeSubDir Static_Release``.

Manual Configuration
====================

Manual configuration is done by passing ``-Config``, ``-Exclude``, and test list files arguments to the script.

To manually configure what tests to run:

* See the ``--list-configs`` or ``--show-configs`` options to see the existing configurations used by the tests.
* See the test list files for the tests themselves:

  * :ghfile:`tests/dcps_tests.lst`

    * This is included by default.
      Use ``--no-dcps`` to exclude this list.

  * :ghfile:`tests/security/security_tests.lst`

    * Use ``--security`` to include this list.

  * :ghfile:`java/tests/dcps_java_tests.lst`

    * Use ``--java`` to include this list.

  * :ghfile:`tools/modeling/tests/modeling_tests.lst`

    * Use ``--modeling`` to include this list.

* In a test list file each of the-space delimited words after the colon determines when the test is ran.
* Passing ``-Config RTPS`` will run tests that have ``RTPS`` and leave out tests with ``!RTPS``.
* Passing ``-Exclude RTPS`` will exclude all tests that have ``RTPS`` in the entry.
  This option matches using RegEx, so a test with ``SUPER_DUPER_RTPS`` will also be excluded.
  It also ignores inverse entries, so it will not exclude a test with ``!SUPER_DUPER_RTPS``.
* Assuming they were built, CMake tests are ran if ``--cmake`` is passed.
  This uses CTest, which is a system that is separate from the one previously described.
* See ``--help`` for all the available options.
