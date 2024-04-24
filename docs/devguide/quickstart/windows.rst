#######
Windows
#######

The goal of this guide is help you download and compile OpenDDS and run a simple example.

****************
Build Directions
****************

#. Ensure that your environment has:

   * Visual Studio
   * Perl
   * Optional Java SDK 1.5 or greater for Java JNI binding support

   See :ref:`deps` for a complete list of dependencies and :ghfile:`README.md#supported-platforms` for supported platforms.

#. Download and extract the latest zip from the `download site <https://github.com/OpenDDS/OpenDDS/releases/latest/>`__

#. Start a Visual Studio Command Prompt

#. Enter the ``OpenDDS-<version>`` directory

#. .. code-block:: batch

      configure

   To enable Java support, use

   .. code-block:: batch

      configure --java

#. Determine the solution to build from the output of the configure script.  The solution will have a ``.sln`` extension.

#. Start Visual Studio by executing the solution from the command prompt, e.g., ``DDS_TAOv2_all.sln``

#. Select **Build** -> **Build Solution**

See `Support <https://opendds.org/support.html>`__ if you encounter problems with configuration or building.

*************************
Run the Messenger Example
*************************

#. .. code-block:: batch

      setenv

#. For the C++ example

   .. code-block:: batch

      cd DevGuideExamples\DCPS\Messenger

   For the Java example

   .. code-block:: batch

      cd java\tests\messenger

#. .. code-block:: batch

      perl run_test.pl

The Messenger Example starts an InfoRepo, publisher, and subscriber.
The InfoRepo allows the publisher and subscriber to find each other.
Once the publisher finds the subscriber, it sends 10 messages to the subscriber and waits 30 seconds for the subscriber to acknowledge the messages.

.. important::

   The ``setenv.cmd`` script sets various environment variables needed for building, linking, and running with OpenDDS.
   Be sure to execute ``setenv.cmd`` if you start a new Visual Studio Command Prompt.

**********
Next Steps
**********

See :ref:`getting_started` for a detailed explanation of the Messenger C++ Example or :ref:`java` for the Java Example.
