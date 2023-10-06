###########
Linux/macOS
###########

The goal of this guide is help you download and compile OpenDDS and run a simple example.

****************
Build Directions
****************

#. Ensure that your environment has:

   * a C++ compiler (GCC or Clang)
   * GNU Make
   * Perl
   * Optional Java SDK 1.5 or greater for Java JNI binding support

   See :ref:`deps` for a complete list of dependencies and :ghfile:`README.md#supported-platforms` for supported platforms.

#. Download and extract the latest tar.gz file from the `download site <https://github.com/OpenDDS/OpenDDS/releases/latest/>`__

#. Enter the ``OpenDDS-<version>`` directory

#. .. code-block:: bash

      ./configure

   To enable Java bindings, use

   .. code-block:: bash

      ./configure --java

#. .. code-block:: bash

      make

See `Support <https://opendds.org/support.html>`__ if you encounter problems with configuration or building.

*************************
Run the Messenger Example
*************************

#. .. code-block:: bash

      source setenv.sh

#. For the C++ example

   .. code-block:: bash

      cd DevGuideExamples/DCPS/Messenger

   For the Java example

   .. code-block:: bash

      cd java/tests/messenger

#. .. code-block:: bash

      ./run_test.pl

The Messenger Example starts an InfoRepo, publisher, and subscriber.
The InfoRepo allows the publisher and subscriber to find each other.
Once the publisher finds the subscriber, it sends 10 messages to the subscriber and waits 30 seconds for the subscriber to acknowledge the messages.

.. important::

   ``The setenv.sh`` script sets various environment variables needed for running OpenDDS tests.
   Be sure to ``source setenv.sh`` if you start a new terminal session.

**********
Next Steps
**********

See :ref:`getting_started` for a detailed explanation of the Messenger C++ Example or :ref:`java` for the Java Example.
