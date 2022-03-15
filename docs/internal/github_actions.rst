##############################
GitHub Actions Summary and FAQ
##############################

********
Overview
********

GitHub Actions is the continuous integration solution currently being used to evaluate the readiness of pull requests.
It builds OpenDDS and runs the test suite across a wide variety of operation systems and build configurations.

*************************************
Legend for GitHub Actions Build Names
*************************************

Operating System
================

* u18/u20 - Ubuntu 18.04/Ubuntu 20.04
* w19/w22 - Windows Server 2019 (Visual Studio 2019)/Windows Server 2022 (Visual Studio 2022)
* m10 - MacOS 10.15

.. seealso::

  `GitHub Virtual Environments <https://github.com/actions/virtual-environments>`_

Build Configuration
===================

* x86 - Windows 32 bit. If not specified, x64 is implied.
* re - Release build.  If not specified, Debug is implied.
* clang5/clang10/gcc6/gcc8/gcc10 - compiler used to build OpenDDS.
  If not specified, the default system compiler is used.

Build Type
==========

* stat - Static build
* bsafe/esafe - Base Safety/Extended Safety build
* sec - Security build
* asan - Address Sanitizer build

Build Options
=============

* o1 - enables ``--optimize``
* d0 - enables ``--no-debug``
* i0 - enables ``--no-inline``
* p1 - enables ``--ipv6``
* w1 - enables wide characters
* v1 - enables versioned namespace
* cpp03 - ``--std=c++03``
* j/j8/j12 - Default System Java/Java8/Java12
* ace7 - uses ace7tao3 rather than ace6tao2
* xer0 - disables xerces
* qt - enables ``--qt``
* ws - enables ``--wireshark``
* js0 - enables ``--no-rapidjson``

Feature Mask
============

This is a mask in an attempt to keep names shorter.

* FM-08

  * ``--no-built-in-topics``
  * ``--no-content-subscription``
  * ``--no-ownership-profile``
  * ``--no-object-model-profile``
  * ``--no-persistence-profile``

* FM-1f

  * ``--no-built-in-topics``

* FM-2c

  * ``--no-content-subscription``
  * ``--no-object-model-profile``
  * ``--no-persistence-profile``

* FM-2f

  * ``--no-content-subscription``

* FM-37

  * ``--no-content-filtered-topics``

***************************
build_and_test.yml Workflow
***************************

Our main `workflow <https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions>`_ which dictates our GitHub Actions run is :ghfile:`.github/workflows/build_and_test.yml`.
It defines jobs, which are the tasks that are run by the CI.

Triggering the Build And Test Workflow
======================================

There are a couple ways in which a run of build and test workflow can be `started <https://docs.github.com/en/actions/reference/events-that-trigger-workflows>`_.

Any pull request targeting master will automatically run the OpenDDS workflows.
This form of workflow run will simulate a merge between the branch and master.

Push events on branches prefixed ``gh_wf_`` will trigger workflow runs on the fork in which the branch resides.
These fork runs of GitHub Actions can be viewed in the "Actions" tab.
Runs of the workflow on forks will not simulate a merge between the branch and master.

Job Types
=========

There are a number of job types that are contained in the file build_and_test.yml.
Where possible, a configuration will contain 3 jobs.
The first job that is run is *ACE_TAO_*.
This will create an artifact which is used later by the OpenDDS build.
The second job is *build_*, which uses the previous *ACE_TAO_* job to configure and build OpenDDS.
This job will then export an artifact to be used in the third step.
The third step is the *test_* job, which runs the appropriate tests for the associated OpenDDS configuration.

Certain builds do not follow this 3 step model.
Static and Release builds have a large footprint and therefore cannot fit the entire test suite onto a GitHub Actions runner.
As a result, they only build and run a subset of the tests in their final jobs, but then have multiple final jobs to increase test coverage.
These jobs are prefixed by:

- *compiler_* (and for some build configurations, *compiler2_*) which runs the :ghfile:`tests/DCPS/Compiler` tests.
- *unit_* which runs the unit tests located in :ghfile:`tests/unit-tests`.
- *messenger_* which runs the tests in :ghfile:`tests/DCPS/Messenger` and :ghfile:`tests/DCPS/C++11/Messenger`.

To shorten the runtime of the continuous integration, some other builds will not run the test suite.

All builds with safety profile disabled and ownership profile enabled, will run the :ghfile:`tests/cmake` tests.
Test runs which only contain CMake tests are prefixed by ``cmake_``.

.. _github-actions-art:

.lst Files
==========

.lst files contain a list of tests with configuration options that will turn tests on or off.
The *test_* jobs pass in :ghfile:`tests/dcps_tests.lst`.
Static and Release builds instead use :ghfile:`tests/static_ci_tests.lst`.
The Thread Sanatizer build uses :ghfile:`tests/tsan_tests.lst`.
This separation of .lst files is due to how excluding all but a few tests in the ``dcps_tests.lst`` would require adding a new config option to every test we didn't want to run.
There is a separate security test list, :ghfile:`tests/security/security_tests.lst`, which governs the security tests which are run when ``--security`` is passed to ``auto_run_tests.pl``.
The last list file used by ``build_and_test.yml`` is :ghfile:`tools/modeling/tests/modeling_tests.lst`, which is included by passing ``--modeling`` to ``auto_run_tests.pl``.

To disable a test in GitHub Actions, ``!GH_ACTIONS`` must be added next to the test in the .lst file.
There are similar test blockers which only block for specific GitHub Actions configurations from running marked tests:

* ``!GH_ACTIONS_OPENDDS_SAFETY_PROFILE`` blocks Safety Profile builds

* ``!GH_ACTIONS_M10`` blocks the MacOS10 runners

* ``!GH_ACTIONS_ASAN`` blocks the Address Sanitizer builds

* ``!GH_ACTIONS_W22`` blocks the Windows Server 2022 runner

These blocks are necessary because certain tests cannot properly run on GitHub Actions due to how the runners are configured.
``-Config GH_ACTIONS`` is assumed by ``auto_run_tests.pl`` when running on GitHub Actions, but the other test configurations must be passed using ``-Config``.

.. seealso::

  :doc:`running_tests`
    For how ``auto_run_tests.pl`` and the lst files work in general.

Workflow Checks
===============

The :ghfile:`.github/workflows/lint.yml` workflow runs :ghfile:`.github/workflows/lint_build_and_test.pl`, which checks that the :ghfile:`.github/workflows/build_and_test.yml` workflow has `gcc-problem-matcher <https://github.com/ammaraskar/gcc-problem-matcher>`_ and `msvc-problem-matcher <https://github.com/ammaraskar/msvc-problem-matcher>`_ in the correct places.

Running this script requires the `YAML CPAN module <https://metacpan.org/pod/YAML>`_.
As a safety measure, it has some picky rules about how steps are named and ordered.
In simplified terms, these rules include:

  * If used, the problem matcher must be appropriate for the platform the job is running on.
  * The problem matcher must not be declared before steps that are named "setup gtest" or named like "build ACE/TAO".
    This should reduce any warnings from Google Test or ACE/TAO.
  * A problem matcher should be declared before steps that start with "build" or contain "make".
    These steps should also contain ``cmake --build``, ``make``, or ``msbuild`` in their ``run`` string.

Blocked Tests
=============

Certain tests are blocked from GitHub actions because their failures are either unfixable, or are not represented on the scoreboard.
If this is the case, we have to assume that the failure is due to some sort of limitation caused by the GitHub Actions runners.

Only Failing on CI
------------------

* tests/DCPS/SharedTransport/run_test.pl multicast

  * Multicast times out waiting for remote peer. Fails on ``test_u20_p1_j8_FM-1f`` and ``test_u20_p1_sec``.

* tests/DCPS/StringKey/run_test.pl

  * A timeout occurs during the writer writing.  Fails on ``test_u18_bsafe_js0_FM-1f``.

* tests/DCPS/Thrasher/run_test.pl high/aggressive/medium XXXX XXXX

  * The more intense thrasher tests cause consistent failures due to the increased load from ASAN.
    GitHub Actions fails these tests very consistently compared to the scoreboard which is more intermittent.
    Fails on ``test_u20_p1_asan_sec``.

* tests/stress-tests/dds/DCPS/run_test.pl

  * This test fails due to only getting ``17 of the expected >=19 total_count``.
    Fails on ``test_m10_i0_j_FM-1f`` and ``test_m10_o1d0_sec``.

* tests/DCPS/StaticDiscoveryReconnect/run_test.pl

  * This test fails due to ``<StaticDiscoveryTest> failed: No such file or directory``.
    Fails on ``test_m10_i0_j_FM-1f`` and ``test_m10_o1d0_sec``.

Failing Both CI and scoreboard
------------------------------

These tests fail on the CI as well as the scoreboard, but will remain blocked on the CI until fixed.
Each test has a list of the builds it was failing on before being blocked.

* tests/DCPS/BuiltInTopicTest/run_test.pl

  * ``test_u18_esafe_js0``

* tests/DCPS/CompatibilityTest/run_test.pl rtps_disc

  * ``test_m10_o1d0_sec``

* tests/DCPS/Federation/run_test.pl

  * ``test_u18_w1_sec``

  * ``test_u18_j_cft0_FM-37``

  * ``test_u18_w1_j_FM-2f``

  * ``test_u20_ace7_j_qt_ws_sec``

  * ``test_u20_p1_asan_sec``

  * ``test_u20_p1_asan_sec``

* tests/DCPS/MultiDPTest/run_test.pl

  * ``test_u18_bsafe_js0_FM-1f``

  * ``test_u18_esafe_js0``

* tests/DCPS/NotifyTest/run_test.pl

  *  ``test_u18_esafe_js0``

* tests/DCPS/Reconnect/run_test.pl restart_pub

  * ``test_w22_x86_i0_sec``

* tests/DCPS/Reconnect/run_test.pl restart_sub

  * ``test_w22_x86_i0_sec``

* tests/DCPS/TimeBasedFilter/run_test.pl -reliable

  * ``test_u18_bsafe_js0_FM-1f``

  * ``test_u18_esafe_js0``

Test Results
============

The tests are run using `autobuild <https://github.com/DOCGroup/autobuild>`_ which creates a number of output files that are turned into a GitHub artifact.
This artifact is processed by the "check results" step which uses the script :ghfile:`tools/scripts/autobuild_brief_html_to_text.pl` to catch failures and print them in an organized manner.
Due to this being a part of the "test" jobs, the results of each run will appear as soon as the job is finished.

Artifacts
=========

Artifacts from the continuous integration run can be downloaded by clicking details on one of the Build & Test runs.
Once all jobs are completed, a dropdown will appear on the bar next to "Re-run jobs", called "Artifacts" which lists each artifact that can be downloaded.

Alternatively, clicking the "Summary" button at the top of the list of jobs will list all the available artifacts at the bottom of the page.

Using Artifacts to Replicate Builds
-----------------------------------

You can download the ``ACE_TAO_`` and ``build_`` artifacts then use them for a local build, so long as your operating system is the same as the one on the runner.

1. ``git clone`` the ACE_TAO branch which is targeted by the build.
   This is usually going to be ``ace6tao2``.
2. ``git clone --recursive`` the OpenDDS branch on which the CI was run.
3. Merge OpenDDS master into your cloned branch.
4. run ``tar xvfJ`` from inside the cloned ACE_TAO, targeting the ``ACE_TAO_*.tar.xz`` file.
5. run ``tar xvfJ`` from inside the cloned OpenDDS, targeting the ``build_*.tar.xz`` file.
6. Adjust the setenv.sh located inside OpenDDS to match the new locations for your ACE_TAO, and OpenDDS.
   The word "runner" should not appear within the setenv.sh once you are finished.

You should now have a working duplicate of the build that was run on GitHub Actions.
This can be used for debugging as a way to quickly set up a problematic build.

Using Artifacts to View More Test Information
---------------------------------------------

Tests failures which are recorded on GitHub only contain a brief capture of output surrounding a failure.
This is useful for some tests, but it can often be helpful to view more of a test run.
This can be done by downloading the artifact for a test step you are viewing.
This test step artifact contains a number of files including ``output.log_Full.html``.
This is the full log of all output from all test runs done for the corresponding job.
It should be opened in either a text editor or Firefox, as Chrome will have issues due to the length of the file.

Caching
========

The OpenDDS workflows create .tar.xz archives of certain build artifacts which can then be up uploaded and shared between jobs (and the user) as part of GitHub Actions' "artifact" API.
A cache key comparison made using the relevant git commit SHA will determine whether to rebuild the artifact, or to use the cached artifact.
