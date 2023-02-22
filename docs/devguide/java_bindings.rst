.. _java_bindings--java-bindings:

#############
Java Bindings
#############

..
    Sect<10>

.. _java_bindings--introduction:

************
Introduction
************

..
    Sect<10.1>

OpenDDS provides JNI bindings.
Java applications can make use of the complete OpenDDS middleware just like C++ applications.

See the :ghfile:`java/INSTALL` file for information on getting started, including the prerequisites and dependencies.

Java versions 9 and up use the `Java Platform Module System <https://en.wikipedia.org/wiki/Java_Platform_Module_System>`__.
To use OpenDDS with one of these Java versions, set the MPC feature java_pre_jpms to 0.
OpenDDS’s configure script will attempt to detect the Java version and set this automatically.

See the :ghfile:`java/FAQ` file for information on common issues encountered while developing applications with the Java bindings.

.. _java_bindings--idl-and-code-generation:

***********************
IDL and Code Generation
***********************

..
    Sect<10.2>

The OpenDDS Java binding is more than just a library that lives in one or two .jar files.
The DDS specification defines the interaction between a DDS application and the DDS middleware.
In particular, DDS applications send and receive messages that are strongly-typed and those types are defined by the application developer in IDL.

In order for the application to interact with the middleware in terms of these user-defined types, code must be generated at compile-time based on this IDL.
C++, Java, and even some additional IDL code is generated.
In most cases, application developers do not need to be concerned with the details of all the generated files.
Scripts included with OpenDDS automate this process so that the end result is a native library (``.so`` or ``.dll``) and a Java library (``.jar`` or just a ``classes`` directory) that together contain all of the generated code.

Below is a description of the generated files and which tools generate them.
In this example, ``Foo.idl`` contains a single struct ``Bar`` contained in module ``Baz`` (IDL modules are similar to C++ namespaces and Java packages).
To the right of each file name is the name of the tool that generates it, followed by some notes on its purpose.

.. _java_bindings--reftable34:

**Table  Generated files descriptions**

.. list-table::
   :header-rows: 1

   * - File

     - Generation Tool

   * - ``Foo.idl``

     - Developer-written description of the DDS sample type

   * - ``Foo{C,S}.``
       ``{h,inl,cpp}``

     - ``tao_idl``: C++ representation of the IDL

   * - ``FooTypeSupport.idl``

     - ``opendds_idl``: DDS type-specific interfaces

   * - ``FooTypeSupport{C,S}.``
       ``{h,inl,cpp}``

     - ``tao_idl``

   * - ``Baz/BarSeq{Helper,Holder}.java``

     - ``idl2jni``

   * - ``Baz/BarData{Reader,Writer}*.java``

     - ``idl2jni``

   * - ``Baz/BarTypeSupport*.java``

     - ``idl2jni`` (except TypeSupportImpl, see below)

   * - ``FooTypeSupportJC.``
       ``{h,cpp}``

     - ``idl2jni``: JNI native method implementations

   * - ``FooTypeSupportImpl.``
       ``{h,cpp}``

     - ``opendds_idl``: DDS type-specific C++ impl.

   * - ``Baz/BarTypeSupportImpl.java``

     - ``opendds_idl``: DDS type-specific Java impl.

   * - ``Baz/Bar*.java``

     - ``idl2jni``: Java representation of IDL struct

   * - ``FooJC.``
       ``{h,cpp}``

     - ``idl2jni``: JNI native method implementations

Foo.idl:

.. code-block:: omg-idl

    module Baz {
      @topic
      struct Bar {
        long x;
      };
    };

.. _java_bindings--setting-up-an-opendds-java-project:

**********************************
Setting up an OpenDDS Java Project
**********************************

..
    Sect<10.3>

These instructions assume you have completed the installation steps in the :ghfile:`java/INSTALL` document, including having the various environment variables defined.

* Start with an empty directory that will be used for your IDL and the code generated from it.
  :ghfile:`java/tests/messenger/messenger_idl/` is set up this way.

* Create an IDL file describing the data structure you will be using with OpenDDS.
  See ``Messenger.idl`` for an example.
  This file will contain at least struct/union annotated with ``@topic``.
  For the sake of these instructions, we will call the file ``Foo.idl``.

* The C++ generated classes will be packaged in a shared library to be loaded at run-time by the JVM.
  This requires the packaged classes to be exported for external visibility.
  ACE provides a utility script for generating the correct export macros.
  The script usage is shown here:

Unix:

.. code-block:: bash

    $ACE_ROOT/bin/generate_export_file.pl Foo > Foo_Export.h

Windows:

.. code-block:: doscon

    %ACE_ROOT%\bin\generate_export_file.pl Foo > Foo_Export.h

* Create an MPC file, Foo.mpc, from this template:

.. code-block:: mpc

    project: dcps_java {
      idlflags += -Wb,stub_export_include=Foo_Export.h \
        -Wb,stub_export_macro=Foo_Export
      dcps_ts_flags += -Wb,export_macro=Foo_Export
      idl2jniflags += -Wb,stub_export_include=Foo_Export.h \
        -Wb,stub_export_macro=Foo_Export
      dynamicflags += FOO_BUILD_DLL

      specific {
     jarname = DDS_Foo_types
      }

     TypeSupport_Files {
        Foo.idl
      }
    }

You can leave out the specific {...} block if you do not need to create a jar file.
In this case you can directly use the Java .class files which will be generated under the classes subdirectory of the current directory.

* Run MPC to generate platform-specific build files.

Unix:

.. code-block:: bash

    $ACE_ROOT/bin/mwc.pl -type gnuace

Windows:

.. code-block:: doscon

    %ACE_ROOT%\bin\mwc.pl -type [CompilerType]

CompilerType can be any supported MPC type (such as “vs2019”)

Make sure this is running ActiveState Perl or Strawberry Perl.

* Compile the generated C++ and Java code

Unix:

::

    make (GNU make, so this may be "gmake" on Solaris systems)

Windows:

Build the generated .``sln`` (Solution) file using your preferred method.
This can be either the Visual Studio IDE or one of the command-line tools.
If you use the IDE, start it from a command prompt using ``devenv`` so that it inherits the environment variables.
Command-line tools for building include ms ``build`` and invoking the IDE (``devenv``) with the appropriate arguments.

When this completes successfully you have a native library and a Java ``.jar`` file.
The native library names are as follows:

Unix:

::

    libFoo.so

Windows:

::

    Foo.dll (Release) or Food.dll (Debug)

You can change the locations of these libraries (including the ``.jar`` file) by adding a line such as the following to the ``Foo.mpc`` file:

::

    libout = $(PROJECT_ROOT)/lib

where ``PROJECT_ROOT`` can be any environment variable defined at build-time.

* You now have all of the Java and C++ code needed to compile and run a Java OpenDDS application.
  The generated ``.jar`` file needs to be added to your ``classpath``, along with the ``.jar`` files that come from OpenDDS (in the lib directory).
  The generated C++ library needs to be available for loading at run-time:

Unix:

Add the directory containing ``libFoo.so`` to the ``LD_LIBRARY_PATH``.

Windows:

Add the directory containing ``Foo.dll`` (or ``Food.dll``) to the ``PATH``.
If you are using the debug version (``Food.dll``) you will need to inform the OpenDDS middleware that it should not look for ``Foo.dll``.
To do this, add ``-Dopendds.native.debug=1`` to the Java VM arguments.

See the publisher and subscriber directories in :ghfile:`java/tests/messenger/` for examples of publishing and subscribing applications using the OpenDDS Java bindings.

* If you make subsequent changes to ``Foo.idl``, start by re-running MPC (step #5 above).
  This is needed because certain changes to ``Foo.idl`` will affect which files are generated and need to be compiled.

.. _java_bindings--a-simple-message-publisher:

**************************
A Simple Message Publisher
**************************

..
    Sect<10.4>

This section presents a simple OpenDDS Java publishing process.
The complete code for this can be found at :ghfile:`java/tests/messenger/publisher/TestPublisher.java`.
Uninteresting segments such as imports and error handling have been omitted here.
The code has been broken down and explained in logical subsections.

.. _java_bindings--initializing-the-participant:

Initializing the Participant
============================

..
    Sect<10.4.1>

DDS applications are boot-strapped by obtaining an initial reference to the Participant Factory.
A call to the static method ``TheParticipantFactory.WithArgs()`` returns a Factory reference.
This also transparently initializes the C++ Participant Factory.
We can then create Participants for specific domains.

.. code-block:: java

        public static void main(String[] args) {

            DomainParticipantFactory dpf =
                TheParticipantFactory.WithArgs(new StringSeqHolder(args));
            if (dpf == null) {
              System.err.println ("Domain Participant Factory not found");
              return;
            }
            final int DOMAIN_ID = 42;
            DomainParticipant dp = dpf.create_participant(DOMAIN_ID,
              PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
            if (dp == null) {
              System.err.println ("Domain Participant creation failed");
              return;
            }

Object creation failure is indicated by a null return.
The third argument to ``create_participant()`` takes a Participant events listener.
If one is not available, a null can be passed instead as done in our example.

.. _java_bindings--registering-the-data-type-and-creating-a-topic:

Registering the Data Type and Creating a Topic
==============================================

..
    Sect<10.4.2>

Next we register our data type with the ``DomainParticipant`` using the ``register_type()`` operation.
We can specify a type name or pass an empty string.
Passing an empty string indicates that the middleware should simply use the identifier generated by the IDL compiler for the type.

::

            MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
            if (servant.register_type(dp, "") != RETCODE_OK.value) {
              System.err.println ("register_type failed");
              return;
            }

Next we create a topic using the type support servant’s registered name.

.. code-block:: java

            Topic top = dp.create_topic("Movie Discussion List",
                                        servant.get_type_name(),
                                        TOPIC_QOS_DEFAULT.get(), null,
                                        DEFAULT_STATUS_MASK.value);

Now we have a topic named “*Movie Discussion List*” with the registered data type and default QoS policies.

.. _java_bindings--creating-a-publisher:

Creating a Publisher
====================

..
    Sect<10.4.3>

Next, we create a publisher:

.. code-block:: java

            Publisher pub = dp.create_publisher(
              PUBLISHER_QOS_DEFAULT.get(),
              null,
              DEFAULT_STATUS_MASK.value);

.. _java_bindings--creating-a-datawriter-and-registering-an-instance:

Creating a DataWriter and Registering an Instance
=================================================

..
    Sect<10.4.4>

With the publisher, we can now create a DataWriter:

.. code-block:: java

            DataWriter dw = pub.create_datawriter(
              top, DATAWRITER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

The ``DataWriter`` is for a specific topic.
For our example, we use the default ``DataWriter`` QoS policies and a null ``DataWriterListener``.

Next, we narrow the generic ``DataWriter`` to the type-specific ``DataWriter`` and register the instance we wish to publish.
In our data definition IDL we had specified the subject_id field as the key, so it needs to be populated with the instance id (99 in our example):

.. code-block:: java

            MessageDataWriter mdw = MessageDataWriterHelper.narrow(dw);
            Message msg = new Message();
            msg.subject_id = 99;
            int handle = mdw.register(msg);

Our example waits for any peers to be initialized and connected.
It then publishes a few messages which are distributed to any subscribers of this topic in the same domain.

::

            msg.from = "OpenDDS-Java";
            msg.subject = "Review";
            msg.text = "Worst. Movie. Ever.";
            msg.count = 0;
            int ret = mdw.write(msg, handle);

.. _java_bindings--setting-up-the-subscriber:

*************************
Setting up the Subscriber
*************************

..
    Sect<10.5>

Much of the initialization code for a subscriber is identical to the publisher.
The subscriber needs to create a participant in the same domain, register an identical data type, and create the same named topic.

.. code-block:: java

        public static void main(String[] args) {

            DomainParticipantFactory dpf =
                TheParticipantFactory.WithArgs(new StringSeqHolder(args));
            if (dpf == null) {
              System.err.println ("Domain Participant Factory not found");
              return;
            }
            DomainParticipant dp = dpf.create_participant(42,
              PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
            if (dp == null) {
              System.err.println("Domain Participant creation failed");
              return;
            }

            MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
                           if (servant.register_type(dp, "") != RETCODE_OK.value) {
              System.err.println ("register_type failed");
              return;
            }
            Topic top = dp.create_topic("Movie Discussion List",
                                        servant.get_type_name(),
                                        TOPIC_QOS_DEFAULT.get(), null,
                                        DEFAULT_STATUS_MASK.value);

.. _java_bindings--creating-a-subscriber:

Creating a Subscriber
=====================

..
    Sect<10.5.1>

As with the publisher, we create a subscriber:

.. code-block:: java

            Subscriber sub = dp.create_subscriber(
              SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

.. _java_bindings--creating-a-datareader-and-listener:

Creating a DataReader and Listener
==================================

..
    Sect<10.5.2>

Providing a ``DataReaderListener`` to the middleware is the simplest way to be notified of the receipt of data and to access the data.
We therefore create an instance of a ``DataReaderListenerImpl`` and pass it as a ``DataReader`` creation parameter:

::

            DataReaderListenerImpl listener = new DataReaderListenerImpl();
             DataReader dr = sub.create_datareader(
               top, DATAREADER_QOS_DEFAULT.get(), listener,
               DEFAULT_STATUS_MASK.value);

Any incoming messages will be received by the Listener in the middleware’s thread.
The application thread is free to perform other tasks at this time.

.. _java_bindings--the-datareader-listener-implementation:

**************************************
The DataReader Listener Implementation
**************************************

..
    Sect<10.6>

The application defined ``DataReaderListenerImpl`` needs to implement the specification’s ``DDS.DataReaderListener`` interface.
OpenDDS provides an abstract class ``DDS._DataReaderListenerLocalBase``.
The application’s listener class extends this abstract class and implements the abstract methods to add application-specific functionality.

Our example ``DataReaderListener`` stubs out most of the Listener methods.
The only method implemented is the message available callback from the middleware:

.. code-block:: java

    public class DataReaderListenerImpl extends DDS._DataReaderListenerLocalBase {

        private int num_reads_;

        public synchronized void on_data_available(DDS.DataReader reader) {
            ++num_reads_;
            MessageDataReader mdr = MessageDataReaderHelper.narrow(reader);
            if (mdr == null) {
              System.err.println ("read: narrow failed.");
              return;
            }

The Listener callback is passed a reference to a generic ``DataReader``.
The application narrows it to a type-specific ``DataReader``:

::

            MessageHolder mh = new MessageHolder(new Message());
            SampleInfoHolder sih = new SampleInfoHolder(new SampleInfo(0, 0, 0,
                new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false));
            int status  = mdr.take_next_sample(mh, sih);

It then creates holder objects for the actual message and associated ``SampleInfo`` and takes the next sample from the ``DataReader``.
Once taken, that sample is removed from the ``DataReader``’s available sample pool.

.. code-block:: java

            if (status == RETCODE_OK.value) {

              System.out.println ("SampleInfo.sample_rank = "+ sih.value.sample_rank);
              System.out.println ("SampleInfo.instance_state = "+
                                  sih.value.instance_state);

              if (sih.value.valid_data) {

                System.out.println("Message: subject    = " + mh.value.subject);
                System.out.println("         subject_id = " + mh.value.subject_id);
                System.out.println("         from       = " + mh.value.from);
                System.out.println("         count      = " + mh.value.count);
                System.out.println("         text       = " + mh.value.text);
                System.out.println("SampleInfo.sample_rank = " +
                                   sih.value.sample_rank);
              }
              else if (sih.value.instance_state ==
                         NOT_ALIVE_DISPOSED_INSTANCE_STATE.value) {
                System.out.println ("instance is disposed");
              }
              else if (sih.value.instance_state ==
                         NOT_ALIVE_NO_WRITERS_INSTANCE_STATE.value) {
                System.out.println ("instance is unregistered");
              }
              else {
                System.out.println ("DataReaderListenerImpl::on_data_available: "+
                                    "received unknown instance state "+
                                    sih.value.instance_state);
              }

            } else if (status == RETCODE_NO_DATA.value) {
              System.err.println ("ERROR: reader received DDS::RETCODE_NO_DATA!");
            } else {
              System.err.println ("ERROR: read Message: Error: "+ status);
            }
        }

    }

The ``SampleInfo`` contains meta-information regarding the message such as the message validity, instance state, etc.

.. _java_bindings--cleaning-up-opendds-java-clients:

********************************
Cleaning up OpenDDS Java Clients
********************************

..
    Sect<10.7>

An application should clean up its OpenDDS environment with the following steps:

::

            dp.delete_contained_entities();

Cleans up all topics, subscribers and publishers associated with that ``Participant``.

::

            dpf.delete_participant(dp);

The ``DomainParticipantFactory`` reclaims any resources associated with the ``DomainParticipant``.

::

            TheServiceParticipant.shutdown();

Shuts down the ``ServiceParticipant``.
This cleans up all OpenDDS associated resources.
Cleaning up these resources is necessary to prevent the ``DCPSInfoRepo`` from forming associations between endpoints which no longer exist.

.. _java_bindings--configuring-the-example:

***********************
Configuring the Example
***********************

..
    Sect<10.8>

OpenDDS offers a file-based configuration mechanism.
The syntax of the configuration file is similar to a Windows INI file.
The properties are divided into named sections corresponding to common and individual transports configuration.

The Messenger example has common properties for the ``DCPSInfoRepo`` objects location and the global transport configuration:

.. code-block:: ini

    [common]
    DCPSInfoRepo=file://repo.ior
    DCPSGlobalTransportConfig=$file

and a transport instance section with a transport type property:

.. code-block:: ini

    [transport/1]
    transport_type=tcp

The ``[transport/1]`` section contains configuration information for the transport instance named “``1``”.
It is defined to be of type ``tcp``.
The global transport configuration setting above causes this transport instance to be used by all readers and writers in the process.

See Chapter :ref:`run_time_configuration--run-time-configuration` for a complete description of all OpenDDS configuration parameters.

.. _java_bindings--running-the-example:

*******************
Running the Example
*******************

..
    Sect<10.9>

To run the Messenger Java OpenDDS application, use the following commands:

.. code-block:: bash

    $DDS_ROOT/bin/DCPSInfoRepo -o repo.ior

    $JAVA_HOME/bin/java -ea -cp classes:$DDS_ROOT/lib/i2jrt.jar:$DDS_ROOT/lib/OpenDDS_DCPS.jar:classes TestPublisher -DCPSConfigFile pub_tcp.ini

    $JAVA_HOME/bin/java -ea -cp classes:$DDS_ROOT/lib/i2jrt.jar:$DDS_ROOT/lib/OpenDDS_DCPS.jar:classes TestSubscriber -DCPSConfigFile sub_tcp.ini

The ``-DCPSConfigFile`` command-line argument passes the location of the OpenDDS configuration file.

.. _java_bindings--java-message-service-jms-support:

**********************************
Java Message Service (JMS) Support
**********************************

..
    Sect<10.10>

OpenDDS provides partial support for JMS version 1.1 <http://docs.oracle.com/javaee/6/tutorial/doc/bncdq.html>.
Enterprise Java applications can make use of the complete OpenDDS middleware just like standard Java and C++ applications.

See the ``INSTALL`` file in the :ghfile:`java/jms/` directory for information on getting started with the OpenDDS JMS support, including the prerequisites and dependencies.

