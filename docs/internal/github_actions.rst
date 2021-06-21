##############################
Github Actions Summary and FAQ
##############################

Github Actions is the continuous integration technique currently being
used to evaluate the readiness of pull requests. It runs our suite of
tests accross a wide variation of operation systems and configurations.

When run on a pull request, Github Actions will simulate a merge between
the pull request branch, and master.

When not on a pull request, Github Actions can made to run by giving your
branch the prefix "gh_wf_". These local runs of Github Actions be viewed in
the "actions" tab in your fork. Local runs of the workflows do not
simulate a merge and run only the code on the branch. This prefix is
useful when you are early in development and are not ready to open
a PR for your branch.

*****************
Table of Contents
*****************

* [Legend for Github Actions Builds]
  * [Operating System]
  * [Build Configuration]
  * [Build Type]
  * [Build Options]
  * [Feature Mask]
* [Introduction to Github Actions Workflows]
* [Cacheing]
* [Job Types]
* [Test Results]
* [Artifacts]
  * [Using Artifacts to Replicate Builds]
  * [Using Artifacts to View More Test Information]

********************************
Legend for Github Actions Builds
********************************

Operating System
================
* u18/u20 - Ubuntu 18.04/Ubuntu 20.04
* w16/w19 - Windows 2016/Windows 2019
* m10 - MacOS 10.15

Build Configuration
===================
* x86 - Windows 32 bit. If not specified, x64 is implied.
* re - Release build.  If not specified, Debug is implied.
* clang5/clang10/gcc6/gcc8/gcc10 - compiler used to build
OpenDDS. If not specified, default system compiler is used.

Build Type
==========
* stat - Static build
* bsafe/esafe - Base Safety/Extended Safety build
* sec - Security build
* asan - Address Sanitizer build

Build Options
=============
* o1 - enables --optimize
* d0 - enables --no-debug
* i0 - enables --no-inline
* p1 - enables ipv6x
* w1 - enables wide characters
* v1 - enables versioned namespace
* cpp03 - --std=c++03
* j/j8/j12 - Default System Java/Java8/Java12
* ace7 - uses ace7tao3 rather than ace6tao2
* xer0 - disables xerces
* qt - enables --qt
* ws - enables --wireshark
* js0 - enables --no-rapidjson

Feature Mask
============
This is a mask in an attempt to keep names shorter.
* FM-08
  * --no-built-in-topics
  * --no-content-subscription
  * --no-ownership-profile
  * --no-object-model-profile
  * --no-persistence-profile
* FM-1f
  * --no-built-in-topics
* FM-2c
  * --no-content-subscription
  * --no-object-model-profile
  * --no-persistence-profile
* FM-2f
  * --no-content-subscription
* FM-37
  * --no-content-filtered-topics

****************************************
Introduction to Github Actions Workflows
****************************************

Our main workflow which dictates our Github Actions run is
build_and_test.yml. It defines jobs, which are the tasks that
are run by the CI.

Cacheing
========

In order to save time on running jobs, a cacheing systems is used.
This cacheing system creates .tar.xz files which are called artifacts.
A cache name comparison is run to determine if there is no new commit
to what is being built, and use an existing artifact if that is the case.

To make CI runs quicker, a seperate build is made for ACE_TAO, rather
than building it at the same time as OpenDDS. This allows us to cache
ACE_TAO relatively often due to the stability of the ace6tao2 branch.

Job Types
=========

There a number of job types that are contained in the built_and_test.yml.
Where possible, a configuration will contain 3 jobs. The first job that
is run is the *ACE_TAO_*. This will create an artifact which is used later
by the OpenDDS build. The second job is *build_*, which uses the previous
*ACE_TAO_* job to configure and build OpenDDS. This job will then export
an artifact to be used in the third step. The third step is the *test_*
job, which runs the appropriate tests for the associated OpenDDS
configuration.

Certain builds do not follow this 3 step model. Safety builds are done
in one step due to the cross-compile nature causing problems. Static and Release
builds have a large footprint and therefore cannot fit the entire test suite onto
a Github Actions runner.  As a result, they instead build tests during the test
step, and run different groups of tests in individual jobs. These individual test
groups are prefixed by *compiler_* which runs the tests/DCPS/Compiler tests, *unit_*
which runs the unit tests located in tests/DCPS/UnitTest and tests/unit-tests, and
*messenger_* which runs the tests in tests/DCPS/Messenger and tests/DCPS/C++11/Messenger.

In addition to these builds, there are some builds which will not run the test suite, in
an effort to shorten the runtime of the continuous integration.  An exception to this is
that all builds which are not safety, and have ownership profile enabled, will run the
tests/cmake tests. Test runs which only contain CMake tests are prefixed by *cmake_*.

Test Results
============

The tests are run using autobuild which will generate an artifact containing the test
results. These test results will be appended to the continuous intergration checks
once all test runs for a commit are completed. This test analysis is handled by
check_test_results.yml. The output from the test results will either be posted in
build_and_test or lint, it is random which one of the workflows the results will appear
in.

Artifacts
=========
Artifacts from the continuous integration run can be downloaded by clicking details
on one of the Build & Test runs. There is a dropdown on the bar next to "Re-run jobs"
called "Artifacts" which lists each artifact that can be downloaded.

Using Artifacts to Replicate Builds
-----------------------------------
You can download the *ACE_TAO* and *build_* artifacts then use them for a local build,
so long as your operating system is the same as the one on the runner.

1. "git clone" the ACE_TAO branch which is targeted by the build. This is usually going to be
ace6tao2.
2. "git clone --recursive" the OpenDDS branch on which the CI was run.
3. Merge OpenDDS master into your cloned branch.
4. run "tar xvfJ" from inside the cloned ACE_TAO, targeting the *ACE_TAO_* .tar.xz file.
5. run "tar xvfJ" from inside the cloned OpenDDS, targeting the *build_* .tar.xz file.
6. Adjust the setenv.sh located inside OpenDDS to match the new locations for your ACE_TAO,
and OpenDDS. The word "runner" should not appear within the setenv.sh once you are finished.

You should now have a working duplicate of the build that was run on Github Actions. This can
be used for debugging as a way to quickly set up a problematic build.

Using Artifacts to View More Test Information
---------------------------------------------
Tests failures which are recorded on github only contain a brief capture of output surrounding
a failure. This is useful for some tests, but it can often be helpful to view more of a test run.
This can be done by downloading the artifact for a test step you are viewing. This test step
artifact contains a number of files including "output.log_Full.html". This is the full log of
all output from all test runs done for the corresponding job.  It should be opened in either a
text editor or Firefox, as Chrome will have issues due to the length of the file.
