##########
Unit Tests
##########

*************************
The Goals of Unit Testing
*************************

The primary goal of a unit test is to provide informal evidence that a piece of code performs correctly.
An alternative to unit testing is writing formal proofs.
However, formal proofs are difficult, expensive, and unmaintainable given the changing nature of software.
Unit tests, while necessarily incomplete, are a practical alternative.

Unit tests document how to use various algorithms and data structures and serve as an informal set of requirements.
As such, a unit test should be developed with the idea that it will serve as a reference for future developers.
Clarity in unit tests serve to accomplish their primary goal of establishing correctness.
That is, a unit test that is difficult to understand casts doubt that the code being tested is correct.
Consequently, unit tests should be clear and concise.

The confidence one has in a piece of code is often related to the number of code paths explored in it.
This is often approximated by "code coverage."
That is, one can run the unit test with a coverage tool to see which code paths were exercised by the unit test.
Code with higher coverage tends to have fewer bugs because the tester has often considered various corner-cases.
Consequently, unit tests should aim for high code coverage.

Unit tests should be executed frequently to provide developers with instant feedback.
This applies to the feature under development and the system as a whole.
That is, developers should frequently execute all of the unit tests to make sure they haven't broken functionality elsewhere in the system.
The more frequently the tests are run, the smaller the increment of development and the easier it is to identify a breaking change.
Thus, unit tests should execute quickly.

Code that is difficult to test will most likely be difficult to use.
Code that is difficult to use correctly will lead to bugs in code that use it.
Consequently, unit tests are vital to the design of useful software as developing a unit test provides feedback on the design of the code under test.
Often, when developing a unit test, one will find parts of the design that can be improved.

Unit tests should promote and not inhibit development.
A robust set of unit tests allows a developer to aggressively refactor since the correctness of the system can be checked after the refactoring.
However, unit tests do produce drag on development since they must be maintained as the code evolves.
Thus, it is important that the unit test code be properly maintained so that they are an asset and not a liability.

Some of the goals mentioned above are in conflict.
Adding code to increase coverage may make the tests less maintainable, slower, and more difficult to understand.
The following metrics can be generated to measure the utility of the unit tests:

* Code coverage
* Test compilation time
* Test execution time
* Test code size vs. code size
* Defect rate vs. code coverage (Are bugs appearing is code that is not tested as well?)

**********************
Unit Test Organization
**********************

The most basic unit when testing is the *test case*.
A test case typically has four phases.

1. Setup - The system is initialized to a known state.
2. Exercise - The code under test is invoked.
3. Check - The resulting state of the system and outputs are checked.
4. Teardown - Any resources allocated in the test are deallocated.

Test cases are grouped into a *test suite*.

Test suites are organized into *a test plan*.

We adopt file boundaries for organizing the unit tests for OpenDDS.
That is, the unit tests for a file group ``dds/DCPS/SomeFile.(h|cpp)`` will be located in ``tests/unit-tests/dds/DCPS/SomeFile.cpp``.
The file ``tests/unit-tests/dds/DCPS/SomeFile.cpp`` is a test suite containing all of the test cases for ``dds/DCPS/SomeFile.(h|cpp)``.
The test plan for OpenDDS will execute all of the test suites under :ghfile:`tests/unit-tests`.
When the complete test plan takes too much time to execute, it will be sub-divided along module boundaries.

In regards to coverage, the coverage of ``dds/DCPS/SomeFile.(h|cpp)`` is measured by executing the tests in its test suite ``tests/unit-tests/dds/DCPS/SomeFile.cpp``.
The purpose of this is to avoid indirect testing where a piece of code may get full coverage without ever being intentionally tested.

***************
Unit Test Scope
***************

A unit test should be completely deterministic with respect to the code paths that it exercises.
This means the test code must have control over all relevant inputs, i.e., inputs that influence the code paths.
To illustrate, the current time is relevant when testing algorithms that perform date related functions, e.g., code that is conditioned on a certificate being expired, while it is not relevant if it is only used when printing log messages.
Sources of non-determinism include time, random numbers, schedulers, and the network.
A dependency on the time is typically mitigated by mocking the service that return the time.
Random numbers can be handled the same way.
A unit test should never sleep.
Avoiding schedulers means a unit test should not have multiple processes and should not have multiple threads unless they cannot impact the code paths being tested.
The network can be avoided by defining a suitable abstraction and mocking.

Code that relies on event dispatching may use a mock dispatcher to control the sequence of events.
One design that makes it possible to unit test in this way is to organize a module as a set of atomic event handlers around a plain old data structure core.
The core should be easy to test.
Event handlers are called for timers, I/O readiness, and method calls into the module.
Event handlers update the core and can perform I/O and call into other modules.
Inter-module calls are problematic in that they create the possibility for deadlock and other hazards.
In the simplest designs, each module has a single lock that is acquired at the beginning of each event handler.
The non-deterministic part of the module can be tested by isolating its dependencies on the operating system and other modules; typically by providing mock objects.

To illustrate the other side of determinism, consider other kinds of tests.
Integration tests often use operating system services, e.g., threads and networking, to test partial or whole system functionality.
A stress test executes the same code over and over hoping that non-determism results in a different outcome.
Performance tests may or may not admit non-determinism and focuses on aggregate behavior as opposed to code-level correctness.
Unit tests should focus on code-level correctness.

**********************
Isolating Dependencies
**********************

More often than not, the code under test will have dependencies on other objects.
For each dependency, the test can either pass in a real object or a stand-in.
Test stand-ins have a variety of names including mocks, spies, dummies, etc. depending on their function.
Some take the position that everything should be mocked.
The author takes the position that real objects should be preferred for the following reasons:

* Less code to maintain
* The design of the real objects improves to accommodate testing
* Tests break in a more meaningful way when dependencies change, i.e., over time, a test stand-in may no longer behave in a realistic way

However, there are cases when a test stand-in is justified:

* It is difficult to configure the real object
* The real object lacks the necessary API for testing and adding it cannot be justified

The use of a mock assumes that an interface exists for the stand-in.

***********************
Writing a New Unit Test
***********************

1. Add the test to :ghfile:`tests/unit-tests/dds/DCPS` or the folder under it.
2. Name the test after the code it is meant to cover.
   For example, the ``AccessControlBuiltInImpl`` unit test covers the ``AccessControlBuiltInImpl.cpp`` file.
3. Add the test to the MPC file in its location.
4. If the test is a safety test, you will need to add it to the ``run_test_safety.pl`` located in ``tests/unit-tests/dds/DCPS``.
5. Add the test to the ``.gitignore`` in its directory.
6. Add the path to the test in either :ghfile:`tests/dcps_tests.lst` or :ghfile:`tests/security/security_tests.lst`.


***********
Using GTest
***********

To use GTest in a test, add ``#include <gtest/gtest.h>``.
Then add the ``googletest`` dependency to the MPC project for your test.
This provides you with many helpful tools to simplify the writing of tests.
When creating your test, the first step is to create a normal ``int main`` function.
Inside the function we need to initialize google tests, then we set the return value as ``RUN_ALL_TESTS();``.

.. code-block:: cpp

  int main(int argc, char* argv[])
  {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  }

This return value will automatically run all test modules and output a series of values corresponding to each test.
Speaking of test modules, you can create an individual test module with the following declaration

.. code-block:: cpp

  TEST(TestModule, TestSubmodule)
  {
  }

Each of these tests contain evaluators.
The most common evaluators are ``EXPECT_EQ``, ``EXPECT_TRUE``, ``EXPECT_FALSE``.

.. code-block:: cpp

  EXPECT_EQ(X, 2)
  EXPECT_EQ(Y, 3)

This will mark the test as a failure if either ``X`` does not equal 2, or ``Y`` does not equal 3.

``EXPECT_TRUE`` and ``EXPECT_FALSE`` are equivalence checks to a boolean value.
In the following examples we pass ``X`` to a function ``is_even`` that returns true if the passed value is an even number and returns false otherwise.

.. code-block:: cpp

  EXPECT_TRUE(is_even(X));

This will mark the test as a failure if ``is_even(X)`` returns false.

.. code-block:: cpp

  EXPECT_FALSE(is_even(X));

This will mark the test as a failure if ``is_even(X)`` returns true.

There are more EXPECT_* and ASSERT_*, but these are the most common ones.
The difference between EXPECT and ASSERT is that an ASSERT will cease the test upon failure, whereas EXPECTS continue to run.
For example if you have multiple ``EXPECT_EQ``, they will all always run.

For more information, visit the google test documentation: https://github.com/google/googletest/blob/master/docs/primer.md.

**********
Final Word
**********

Ignore anything in this document that prevents you from writing unit tests.
