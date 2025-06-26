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

For Unixes (Linux, macOS, BSDs, etc)
------------------------------------

Run this in :envvar:`DDS_ROOT`::

  bin/auto_run_tests.pl

For Windows
-----------

Run this in :envvar:`DDS_ROOT`::

  bin\auto_run_tests.pl

If OpenDDS was built in Release mode add ``-ExeSubDir Release``.
If it was built as static libraries add ``-ExeSubDir Static_Debug`` or ``-ExeSubDir Static_Release``.

Manual Configuration
====================

Manual configuration is done by passing ``-Config`` and test list files arguments to the script.

To manually configure what tests to run:

* See the ``--list-all-configs`` or ``--show-all-configs`` options to see the existing configurations used by all test list files.
* See the ``--list-configs`` or ``--show-configs`` options to see the existing configurations used by specific test list files.
* ``--list-tests`` lists the tests that would run for the current options.
  ``--list-all-tests`` is the same but includes all the known list files.
* See the test list files for the tests themselves:

  * :ghfile:`tests/dcps_tests.lst`

    * This is included by default.
      Use ``--no-dcps`` to exclude this list.
    * If ``--no-auto-config`` was passed, then ``--dcps`` will have to be passed to include this.

  * :ghfile:`tests/security/security_tests.lst`

    * Use ``--security`` to include this list.

  * :ghfile:`java/tests/dcps_java_tests.lst`

    * Use ``--java`` to include this list.

* In a test list file each of the-space delimited words after the colon determines when the test is ran.
* Passing ``-Config RTPS`` will run tests that have ``RTPS`` and leave out tests with ``!RTPS``.
* There are ``-Config`` options that are added automatically if ``--no-auto-config`` wasn't passed:

  * ``-Config RTPS``
  * ``-Config GH_ACTIONS`` if running on :ref:`GitHub Actions <github-actions-art>`
  * These are based on the OS ``auto_run_tests.pl`` is running under:

    * ``-Config Win32``
    * ``-Config macOS``
    * ``-Config Linux``

* ``-Exclude`` excludes test paths and arguments (but not the configurations) that match the argument.
  Passing ``-Exclude tests/DCPS`` will exclude all tests in :ghfile:`tests/DCPS`.
  This option uses RegEx, so passing ``-Exclude tests/(DCPS|FACE)`` will also exclude tests in :ghfile:`tests/FACE`.
* Assuming they were built, CMake tests are ran if ``--cmake`` is passed.
  This uses CTest, which is a system that is separate from the one previously described.
* See ``--help`` for all the available options.

.. note:: For those editing and creating test list files:
  The ``ConfigList`` code in ACE can't properly handle multiple test list entries with the same command.
  It will run all those entries if the last one will run, even if based on the configs only one entry should run.
  ``auto_run_tests.pl`` will warn about this if it's using a test list file that has this problem.
