.. _getting_started:

###############
Getting Started
###############

..
    Sect<2>

.. _getting_started--using-dcps:

**********
Using DCPS
**********

..
    Sect<2.1>

This section focuses on an example application using DCPS to distribute data from a single publisher process to a single subscriber process.
It is based on a simple messenger application where a single publisher publishes messages and a single subscriber subscribes to them.
We use the default QoS properties and the default TCP/IP transport.
Full source code for this example may be found under the :ghfile:`DevGuideExamples/DCPS/Messenger/` directory.
Additional DDS and DCPS features are discussed in later sections.

.. _getting_started--defining-data-types-with-idl:

Defining Data Types with IDL
============================

..
    Sect<2.1.1>

In this example, data types for topics will be defined using the OMG Interface Definition Language (IDL).
For details on how to build OpenDDS applications that don't use IDL for topic data types, see :ref:`xtypes--dynamicdatawriters-and-dynamicdatareaders`.

.. _getting_started--identifying-topic-types:

Identifying Topic Types
-----------------------

..
    Sect<2.1.1.1>

Each data type used by DDS is defined using OMG Interface Definition Language (IDL).
OpenDDS uses IDL annotations [#footnote1]_ to identify the data types that it transmits and processes.
These data types are processed by the TAO IDL compiler and the OpenDDS IDL compiler to generate the necessary code to transmit data of these types with OpenDDS.
Here is the IDL file that defines our Message data type:

.. code-block:: omg-idl

    module Messenger {

      @topic
      struct Message {
        string from;
        string subject;
        @key long subject_id;
        string text;
        long count;
      };
    };

The ``@topic`` annotation marks a data type that can be used as a topic's type.
This must be a structure or a union.
The structure or union may contain basic types (short, long, float, etc.
), enumerations, strings, sequences, arrays, structures, and unions.
See :ref:`introduction--idl-compliance` for more details on the use of IDL for OpenDDS topic types.
The IDL above defines the structure Message in the Messenger module for use in this example.

.. _getting_started--keys:

Keys
----

..
    Sect<2.1.1.2>

The ``@key`` annotation identifies a field that is used as a key for this topic type.
A topic type may have zero or more key fields.
These keys are used to identify different DDS Instances within a topic.
Keys can be of scalar type, structures or unions containing key fields, or arrays of any of these constructs.

Multiple keys are specified with separate ``@key`` annotations.
In the above example, we identify the ``subject_id`` member of ``Messenger::Message`` as a key.
Each sample published with a unique ``subject_id`` value will be defined as belonging to a different DDS Instance within the same topic.
Since we are using the default QoS policies, subsequent samples with the same subject_id value are treated as replacement values for that DDS Instance.

``@key`` can be applied to a structure field of the following types:

* Any primitive, such as booleans, integers, characters, and strings.

* Other structures that have a defined key or set of keys.
  For example:

.. code-block:: omg-idl

    struct StructA {
      @key long key;
    };

    struct StructB {
      @key StructA main_info;
      long other_info;
    };

    @topic
    struct StructC {
      @key StructA keya; // keya.key is one key
      @key StructB keyb; // keyb.main_info.key is another
      DDS::OctetSeq data;
    };

In this example, every type from the key marked on the topic type down to what primitive data types to use as the key is annotated with ``@key``.
That isn't strictly necessary though, as the next section shows.

* Other structures that don't have any defined keys.
  In the following example, it's implied that all the fields in InnerStruct are keys.

.. code-block:: omg-idl

    struct InnerStruct {
      long a;
      short b;
      char c;
    };

    @topic
    struct OuterStruct {
      @key InnerStruct value;
      // value.a, value.b, and value.c are all keys
    };

If none of the fields in a struct are marked with ``@key`` or ``@key(TRUE)``, then when the struct is used in another struct and marked as a key, all the fields in the struct are assumed to keys.
Fields marked with ``@key(FALSE)`` are always excluded from being a key, such as in this example:

.. code-block:: omg-idl

    struct InnerStruct {
      long a;
      short b;
      @key(FALSE) char c;
    };

    @topic
    struct OuterStruct {
      @key InnerStruct value;
      // Now just value.a and value.b are the keys
    };

* Unions can also be used as keys if their discriminator is marked as a key.
  There is an example of a keyed union topic type in the next section, but keep in mind a union being used as a key doesn't have to be a topic type.

* Arrays of any of the previous data types.
  ``@key`` can't be applied to sequences, even if the base type would be valid in an array.
  Also ``@key``, when applied to arrays, it makes every element in the array part of the key.
  They can't be applied to individual array elements.

.. _getting_started--union-topic-types:

Union Topic Types
-----------------

..
    Sect<2.1.1.3>

Unions can be used as topic types.
Here is an example:

.. code-block:: omg-idl

    enum TypeKind {
      STRING_TYPE,
      LONG_TYPE,
      FLOAT_TYPE
    };

    @topic
    union MyUnionType switch (@key TypeKind) {
    case STRING_TYPE:
      string string_value;
    case LONG_TYPE:
      long long_value;
    case FLOAT_TYPE:
      float float_value;
    };

Unions can be keyed like structures, but only the union discriminator can be a key, so the set of possible DDS Instances of topics using keyed unions are values of the discriminator.
Designating a key for a union topic type is done by putting ``@key`` before the discriminator type like in the example above.
Like structures, it is also possible to have no key fields, in which case ``@key`` would be omitted and there would be only one DDS Instance.

.. _getting_started--topic-types-vs-nested-types:

Topic Types vs. Nested Types
----------------------------

..
    Sect<2.1.1.4>

In addition to ``@topic``, the set of IDL types OpenDDS can use can also be controlled using ``@nested`` and ``@default_nested``.
Types that are "nested" are the opposite of topic types; they can't be used for the top-level type of a topic, but they can be nested inside the top-level type (at any level of nesting).
All types are nested by default in OpenDDS to reduce the code generated for type support, but there a number of ways to change this:

* The type can be annotated with ``@topic`` (see :ref:`getting_started--identifying-topic-types`), or with ``@nested(FALSE)``, which is equivalent to ``@topic``.

* The enclosing module can be annotated with ``@default_nested(FALSE)``.

* The global default for ``opendds_idl`` can be changed by adding ``--no-default-nested,`` in which case it would be as if all valid types were marked with ``@topic``.
  If desired for IDL compatibility with other DDS implementations or based on preference, this can be done through the build system:

  * When using MPC, add ``dcps_ts_flags += --no-default-nested`` to the project.

  * When using CMake, this can be done by setting either :cmake:var:`OPENDDS_DEFAULT_NESTED` to ``FALSE`` or passing ``--no-default-nested`` to :cmake:func:`opendds_target_sources(OPENDDS_IDL_OPTIONS)`.

In cases where the module default is not nested, you can reverse this by using ``@nested`` or ``@nested(TRUE)`` for structures/unions and ``@default_nested`` or ``@default_nested(TRUE)`` for modules.
NOTE: the ``@topic`` annotation doesn't take a boolean argument, so ``@topic(FALSE)`` would cause an error in the OpenDDS IDL Compiler.

.. _getting_started--processing-the-idl:

Processing the IDL
==================

..
    Sect<2.1.2>

This section uses the OMG IDL-to-C++ mapping ("C++ classic") as part of the walk-through.
OpenDDS also supports the OMG IDL-to-C++11 mapping, see :ref:`opendds_idl--using-the-idl-to-c-11-mapping` for details.

The OpenDDS IDL is first processed by the TAO IDL compiler.

.. code-block:: bash

    tao_idl Messenger.idl

In addition, we need to process the IDL file with the OpenDDS IDL compiler to generate the serialization and key support code that OpenDDS requires to marshal and demarshal the Message, as well as the type support code for the data readers and writers.
This IDL compiler is located in :ghfile:`bin` and generates three files for each IDL file processed.
The three files all begin with the original IDL file name and would appear as follows:

* ``<filename>TypeSupport.idl``

* ``<filename>TypeSupportImpl.h``

* ``<filename>TypeSupportImpl.cpp``

For example, running ``opendds_idl`` as follows

.. code-block:: bash

    opendds_idl Messenger.idl

generates ``MessengerTypeSupport.idl``, ``MessengerTypeSupportImpl.h``, and ``MessengerTypeSupportImpl.cpp``.
The IDL file contains the ``MessageTypeSupport``, ``MessageDataWriter``, and ``MessageDataReader`` interface definitions.
These are type-specific DDS interfaces that we use later to register our data type with the domain, publish samples of that data type, and receive published samples.
The implementation files contain implementations for these interfaces.
The generated IDL file should itself be compiled with the TAO IDL compiler to generate stubs and skeletons.
These and the implementation file should be linked with your OpenDDS applications that use the Message type.
The OpenDDS IDL compiler has a number of options that specialize the generated code.
These options are described in :ref:`opendds_idl`.

Typically, you do not directly invoke the TAO or OpenDDS IDL compilers as above, but let your build system do it for you.
Two different build systems are supported for projects that use OpenDDS:

* MPC, the "Make Project Creator" which is used to build OpenDDS itself and the majority of its included tests and examples

* `CMake <https://cmake.org>`__, a build system that's commonly used across the industry

Even if you will eventually use some custom build system that's not one of the two listed above, start by building an example OpenDDS application using one of the supported build systems and then migrate the code generator command lines, compiler options, etc., to the custom build system.

The remainder of this section will assume MPC.
For more details on using CMake, see the :doc:`/devguide/building/cmake`.

The code generation process is simplified when using MPC, by inheriting from the dcps base project.
Here is the MPC file section common to both the publisher and subscriber

.. code-block:: mpc

    project(*idl): dcps {
      // This project ensures the common components get built first.

      TypeSupport_Files {
        Messenger.idl
      }
      custom_only = 1
    }

The dcps parent project adds the Type Support custom build rules.
The TypeSupport_Files section above tells MPC to generate the Message type support files from ``Messenger.idl`` using the OpenDDS IDL complier.
Here is the publisher section:

.. code-block:: mpc

    project(*Publisher): dcpsexe_with_tcp {
      exename = publisher
      after  += *idl

      TypeSupport_Files {
        Messenger.idl
      }

      Source_Files {
        Publisher.cpp
      }
    }

The ``dcpsexe_with_tcp`` project links in the DCPS library.

For completeness, here is the subscriber section of the MPC file:

.. code-block:: mpc

    project(*Subscriber): dcpsexe_with_tcp {

      exename = subscriber
      after  += *idl

      TypeSupport_Files {
        Messenger.idl
      }

      Source_Files {
        Subscriber.cpp
        DataReaderListenerImpl.cpp
      }
    }

.. _getting_started--a-simple-message-publisher:

A Simple Message Publisher
==========================

..
    Sect<2.1.3>

In this section we describe the steps involved in setting up a simple OpenDDS publication process.
The code is broken into logical sections and explained as we present each section.
We omit some uninteresting sections of the code (such as ``#include`` directives, error handling, and cross-process synchronization).
The full source code for this sample publisher is found in the ``Publisher.cpp`` and ``Writer.cpp`` files in :ghfile:`DevGuideExamples/DCPS/Messenger/`.

.. _getting_started--initializing-the-participant:

Initializing the Participant
----------------------------

..
    Sect<2.1.3.1>

The first section of ``main()`` initializes the current process as an OpenDDS participant.

.. code-block:: cpp

    int main (int argc, char *argv[]) {
      try {
        DDS::DomainParticipantFactory_var dpf =
          TheParticipantFactoryWithArgs(argc, argv);
        DDS::DomainParticipant_var participant =
          dpf->create_participant(42, // domain ID
                                  PARTICIPANT_QOS_DEFAULT,
                                  0,  // No listener required
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!participant) {
          std::cerr << "create_participant failed." << std::endl;
          return 1;
        }
        // ...
      }
    }

The ``TheParticipantFactoryWithArgs`` macro is defined in ``Service_Participant.h`` and initializes the Domain Participant Factory with the command line arguments.
These command line arguments are used to initialize the ORB that the OpenDDS service uses as well as the service itself.
This allows us to pass ``ORB_init()`` options on the command line as well as OpenDDS configuration options of the form ``-DCPS*``.
Available OpenDDS options are fully described in :ref:`config`.

The ``create_participant()`` operation uses the domain participant factory to register this process as a participant in the domain specified by the ID of 42.
The participant uses the default QoS policies and no listeners.
Use of the OpenDDS default status mask ensures all relevant communication status changes (e.g., data available, liveliness lost) in the middleware are communicated to the application (e.g., via callbacks on listeners).

Users may define any number of domains using IDs in the range (0x0 ~ 0x7FFFFFFF).
All other values are reserved for internal use by the implementation.

The Domain Participant object reference returned is then used to register our Message data type.

.. _getting_started--registering-the-data-type-and-creating-a-topic:

Registering the Data Type and Creating a Topic
----------------------------------------------

..
    Sect<2.1.3.2>

First, we create a ``MessageTypeSupportImpl`` object, then register the type with a type name using the ``register_type()`` operation.
In this example, we register the type with a nil string type name, which causes the ``MessageTypeSupport`` interface repository identifier to be used as the type name.
A specific type name such as "*Message*" can be used as well.

.. code-block:: cpp

      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();
      if (DDS::RETCODE_OK != mts->register_type(participant, "")) {
        std::cerr << "register_type failed." << std::endl;
        return 1;
      }

Next, we obtain the registered type name from the type support object and create the topic by passing the type name to the participant in the ``create_topic()`` operation.

.. code-block:: cpp

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name,
                                   TOPIC_QOS_DEFAULT,
                                   0,  // No listener required
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!topic) {
        std::cerr << "create_topic failed." << std::endl;
        return 1;
      }

We have created a topic named "*Movie Discussion List*" with the registered type and the default QoS policies.

.. _getting_started--creating-a-publisher:

Creating a Publisher
--------------------

..
    Sect<2.1.3.3>

Now, we are ready to create the publisher with the default publisher QoS.

.. code-block:: cpp

        DDS::Publisher_var pub =
          participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                        0,  // No listener required
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!pub) {
          std::cerr << "create_publisher failed." << std::endl;
          return 1;
        }

.. _getting_started--creating-a-datawriter-and-waiting-for-the-subscriber:

Creating a DataWriter and Waiting for the Subscriber
----------------------------------------------------

..
    Sect<2.1.3.4>

With the publisher in place, we create the data writer.

.. code-block:: cpp

      // Create the datawriter
        DDS::DataWriter_var writer =
          pub->create_datawriter(topic,
                                 DATAWRITER_QOS_DEFAULT,
                                 0,  // No listener required
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!writer) {
          std::cerr << "create_datawriter failed." << std::endl;
          return 1;
        }

When we create the data writer we pass the topic object reference, the default QoS policies, and a null listener reference.
We now narrow the data writer reference to a ``MessageDataWriter`` object reference so we can use the type-specific publication operations.

.. code-block:: cpp

        Messenger::MessageDataWriter_var message_writer =
             Messenger::MessageDataWriter::_narrow(writer);

The example code uses *conditions* and *wait* sets so the publisher waits for the subscriber to become connected and fully initialized.
In a simple example like this, failure to wait for the subscriber may cause the publisher to publish its samples before the subscriber is connected.

The basic steps involved in waiting for the subscriber are:

* Get the status condition from the data writer we created

* Enable the Publication Matched status in the condition

* Create a wait set

* Attach the status condition to the wait set

* .. _getting_started--refnumpara-2987-508699783:

  .. _getting_started--rtf35353737353a204e756d6265:

  Get the publication matched status

* If the current count of matches is one or more, detach the condition from the wait set and proceed to publication

* Wait on the wait set (can be bounded by a specified period of time)

* Loop back around to step :ref:`5) <getting_started--refnumpara-2987-508699783>`

Here is the corresponding code:

.. code-block:: cpp

        // Block until Subscriber is available
        DDS::StatusCondition_var condition = writer->get_statuscondition();
        condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        while (true) {
          DDS::PublicationMatchedStatus matches;
          if (writer->get_publication_matched_status(matches) != DDS::RETCODE_OK) {
            std::cerr << "get_publication_matched_status failed!"
                      << std::endl;
            return 1;
          }

          if (matches.current_count >= 1) {
            break;
          }

          DDS::ConditionSeq conditions;
          DDS::Duration_t timeout = { 60, 0 };
          if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
            std::cerr << "wait failed!" << std::endl;
            return 1;
          }

        }

        ws->detach_condition(condition);

For more details about status, conditions, and wait sets, see :ref:`conditions_and_listeners`.

.. _getting_started--sample-publication:

Sample Publication
------------------

..
    Sect<2.1.3.5>

The message publication is quite straightforward:

.. code-block:: cpp

        // Write samples
        Messenger::Message message;
        message.subject_id = 99;
        message.from = "Comic Book Guy";
        message.subject = "Review";
        message.text = "Worst. Movie. Ever.";
        message.count = 0;
        for (int i = 0; i < 10; ++i) {
          DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
          ++message.count;
          ++message.subject_id;
          if (error != DDS::RETCODE_OK) {
            // Log or otherwise handle the error condition
            return 1;
          }
        }

For each loop iteration, calling ``write()`` causes a message to be distributed to all connected subscribers that are registered for our topic.
Since the subject_id is the key for Message, each time subject_id is incremented and ``write()`` is called, a new instance is created (see :ref:`introduction--topic`).
The second argument to ``write()`` specifies the instance on which we are publishing the sample.
It should be passed either a handle returned by ``register_instance()`` or ``DDS::HANDLE_NIL``.
Passing a ``DDS::HANDLE_NIL`` value indicates that the data writer should determine the instance by inspecting the key of the sample.
See :ref:`getting_started--registering-and-using-instances-in-the-publisher` for details on using instance handles during publication.

.. _getting_started--setting-up-the-subscriber:

Setting up the Subscriber
=========================

..
    Sect<2.1.4>

Much of the subscriber's code is identical or analogous to the publisher that we just finished exploring.
We will progress quickly through the similar parts and refer you to the discussion above for details.
The full source code for this sample subscriber is found in the ``Subscriber.cpp`` and ``DataReaderListener.cpp`` files in :ghfile:`DevGuideExamples/DCPS/Messenger/`.

.. _getting_started--initializing-the-participant-1:

Initializing the Participant
----------------------------

..
    Sect<2.1.4.1>

The beginning of the subscriber is identical to the publisher as we initialize the service and join our domain:

.. code-block:: cpp

    int main (int argc, char *argv[])
    {
     try {
        DDS::DomainParticipantFactory_var dpf =
          TheParticipantFactoryWithArgs(argc, argv);
        DDS::DomainParticipant_var participant =
          dpf->create_participant(42, // Domain ID
                                  PARTICIPANT_QOS_DEFAULT,
                                  0,  // No listener required
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!participant) {
          std::cerr << "create_participant failed." << std::endl;
          return 1;
        }

.. _getting_started--registering-the-data-type-and-creating-a-topic-1:

Registering the Data Type and Creating a Topic
----------------------------------------------

..
    Sect<2.1.4.2>

Next, we initialize the message type and topic.
Note that if the topic has already been initialized in this domain with the same data type and compatible QoS, the ``create_topic()`` invocation returns a reference corresponding to the existing topic.
If the type or QoS specified in our ``create_topic()`` invocation do not match that of the existing topic then the invocation fails.
There is also a ``find_topic()`` operation our subscriber could use to simply retrieve an existing topic.

.. code-block:: cpp

        Messenger::MessageTypeSupport_var mts =
          new Messenger::MessageTypeSupportImpl();
        if (DDS::RETCODE_OK != mts->register_type(participant, "")) {
          std::cerr << "Failed to register the MessageTypeSupport." << std::endl;
          return 1;
        }

        CORBA::String_var type_name = mts->get_type_name();

        DDS::Topic_var topic =
          participant->create_topic("Movie Discussion List",
                                    type_name,
                                    TOPIC_QOS_DEFAULT,
                                    0,  // No listener required
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!topic) {
          std::cerr << "Failed to create_topic." << std::endl;
          return 1;
        }

.. _getting_started--creating-the-subscriber:

Creating the subscriber
-----------------------

..
    Sect<2.1.4.3>

Next, we create the subscriber with the default QoS.

.. code-block:: cpp

        // Create the subscriber
        DDS::Subscriber_var sub =
          participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                         0,  // No listener required
                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!sub) {
          std::cerr << "Failed to create_subscriber." << std::endl;
          return 1;
        }

.. _getting_started--creating-a-datareader-and-listener:

Creating a DataReader and Listener
----------------------------------

..
    Sect<2.1.4.4>

We need to associate a listener object with the data reader we create, so we can use it to detect when data is available.
The code below constructs the listener object.
The ``DataReaderListenerImpl`` class is shown in the next subsection.

.. code-block:: cpp

        DDS::DataReaderListener_var listener(new DataReaderListenerImpl);

The listener is allocated on the heap and assigned to a ``DataReaderListener_var`` object.
This type provides reference counting behavior so the listener is automatically cleaned up when the last reference to it is removed.
This usage is typical for heap allocations in OpenDDS application code and frees the application developer from having to actively manage the lifespan of the allocated objects.

Now we can create the data reader and associate it with our topic, the default QoS properties, and the listener object we just created.

.. code-block:: cpp

        // Create the Datareader
        DDS::DataReader_var dr =
          sub->create_datareader(topic,
                                 DATAREADER_QOS_DEFAULT,
                                 listener,
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!dr) {
          std::cerr << "create_datareader failed." << std::endl;
          return 1;
        }

This thread is now free to perform other application work.
Our listener object will be called on an OpenDDS thread when a sample is available.

.. _getting_started--the-data-reader-listener-implementation:

The Data Reader Listener Implementation
=======================================

..
    Sect<2.1.5>

Our listener class implements the ``DDS::DataReaderListener`` interface defined by the DDS specification.
The ``DataReaderListener`` is wrapped within a ``DCPS::LocalObject`` which resolves ambiguously-inherited members such as ``_narrow`` and ``_ptr_type``.
The interface defines a number of operations we must implement, each of which is invoked to inform us of different events.
The ``OpenDDS::DCPS::DataReaderListener`` defines operations for OpenDDS's special needs such as disconnecting and reconnected event updates.
Here is the interface definition:

.. code-block:: omg-idl

    module DDS {
      local interface DataReaderListener : Listener {
        void on_requested_deadline_missed(in DataReader reader,
                                          in RequestedDeadlineMissedStatus status);
        void on_requested_incompatible_qos(in DataReader reader,
                                          in RequestedIncompatibleQosStatus status);
        void on_sample_rejected(in DataReader reader,
                                in SampleRejectedStatus status);
        void on_liveliness_changed(in DataReader reader,
                                   in LivelinessChangedStatus status);
        void on_data_available(in DataReader reader);
        void on_subscription_matched(in DataReader reader,
                                     in SubscriptionMatchedStatus status);
        void on_sample_lost(in DataReader reader, in SampleLostStatus status);
      };
    };

Our example listener class stubs out most of these listener operations with simple print statements.
The only operation that is really needed for this example is ``on_data_available()`` and it is the only member function of this class we need to explore.

.. code-block:: cpp

    void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
    {
      ++num_reads_;

      try {
        Messenger::MessageDataReader_var reader_i =
          Messenger::MessageDataReader::_narrow(reader);
        if (!reader_i) {
          std::cerr << "read: _narrow failed." << std::endl;
          return;
        }

The code above narrows the generic data reader passed into the listener to the type-specific ``MessageDataReader`` interface.
The following code takes the next sample from the message reader.
If the take is successful and returns valid data, we print out each of the message's fields.

.. code-block:: cpp

        Messenger::Message message;
        DDS::SampleInfo si;
        DDS::ReturnCode_t status = reader_i->take_next_sample(message, si);

        if (status == DDS::RETCODE_OK) {
          if (si.valid_data) {
              std::cout << "Message: subject = " << message.subject.in() << std::endl
                << "  subject_id = " << message.subject_id << std::endl
                << "  from = " << message.from.in() << std::endl
                << "  count = " << message.count << std::endl
                << "  text = " << message.text.in() << std::endl;
          } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
            std::cout << "instance is disposed" << std::endl;
          } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
            std::cout << "instance is unregistered" << std::endl;
          } else {
            std::cerr << "ERROR: received unknown instance state "
                      << si.instance_state << std::endl;
          }
        } else if (status == DDS::RETCODE_NO_DATA) {
            cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << std::endl;
        } else {
            cerr << "ERROR: read Message: Error: " << status << std::endl;
        }

Note the sample read may contain invalid data.
The valid_data flag indicates if the sample has valid data.
There are two samples with invalid data delivered to the listener callback for notification purposes.
One is the *dispose* notification, which is received when the ``DataWriter`` calls ``dispose()`` explicitly.
The other is the *unregistered* notification, which is received when the ``DataWriter`` calls ``unregister()`` explicitly.
The dispose notification is delivered with the instance state set to ``NOT_ALIVE_DISPOSED_INSTANCE_STATE`` and the unregister notification is delivered with the instance state set to ``NOT_ALIVE_NO_WRITERS_INSTANCE_STATE``.

If additional samples are available, the service calls this function again.
However, reading values a single sample at a time is not the most efficient way to process incoming data.
The Data Reader interface provides a number of different options for processing data in a more efficient manner.
We discuss some of these operations in :ref:`getting_started--data-handling-optimizations`.

.. _getting_started--cleaning-up-in-opendds-clients:

Cleaning up in OpenDDS Clients
==============================

..
    Sect<2.1.6>

After we are finished in the publisher and subscriber, we can use the following code to clean up the OpenDDS-related objects:

.. code-block:: cpp

        participant->delete_contained_entities();
        dpf->delete_participant(participant);
        TheServiceParticipant->shutdown();

The domain participant's ``delete_contained_entities()`` operation deletes all the topics, subscribers, and publishers created with that participant.
Once this is done, we can use the domain participant factory to delete our domain participant.

Since the publication and subscription of data within DDS is decoupled, data is not guaranteed to be delivered if a publication is disassociated (shutdown) prior to all data that has been sent having been received by the subscriptions.
If the application requires that all published data be received, the ``wait_for_acknowledgments()`` operation is available to allow the publication to wait until all written data has been received.
Data readers must have :ref:`qos-reliability` set to ``RELIABLE_RELIABILITY_QOS`` (which is the default) in order for ``wait_for_acknowledgments()`` to work.
This operation is called on individual ``DataWriters`` and includes a timeout value to bound the time to wait.
The following code illustrates the use of ``wait_for_acknowledgments()`` to block for up to 15 seconds to wait for subscriptions to acknowledge receipt of all written data:

.. code-block:: cpp

      DDS::Duration_t shutdown_delay = {15, 0};
      DDS::ReturnCode_t result;
      result = writer->wait_for_acknowledgments(shutdown_delay);
      if (result != DDS::RETCODE_OK) {
        std::cerr << "Failed while waiting for acknowledgment of "
                  << "data being received by subscriptions, some data "
                  << "may not have been delivered." << std::endl;
      }

.. _getting_started--running-the-example:

Running the Example
===================

..
    Sect<2.1.7>

We are now ready to run our simple example.
Running each of these commands in its own window should enable you to most easily understand the output.

First we will start a ``DCPSInfoRepo`` service so our publishers and subscribers can find one another.

.. note:: This step is not necessary if you are using peer-to-peer discovery by configuring your environment to use RTPS discovery.

The ``DCPSInfoRepo`` executable is found in ``bin/DCPSInfoRepo``.
When we start the ``DCPSInfoRepo`` we need to ensure that publisher and subscriber application processes can also find the started ``DCPSInfoRepo``.
This information can be provided in one of three ways:

#. Pass arguments on the command line.
#. Connection info generated and placed in a shared file for applications to use.
#. Options put in a configuration file for other processes to use.

For our simple example here we will use option 2 by generating the location properties of the ``DCPSInfoRepo`` into a file so that our simple publisher and subscriber can read it in and connect to it.

From your current directory type:

.. tab:: Linux, macOS, BSDs, etc.

 .. code-block:: bash

   $DDS_ROOT/bin/DCPSInfoRepo -o simple.ior

.. tab:: Windows

  .. code-block:: batch

    %DDS_ROOT%\bin\DCPSInfoRepo -o simple.ior

The ``-o`` parameter instructs the ``DCPSInfoRepo`` to generate its connection information to the file ``simple.ior`` for use by the publisher and subscriber.
In a separate window navigate to the same directory that contains the ``simple.ior`` file and start the subscriber application in our example by typing:

.. tab:: Linux, macOS, BSDs, etc.

  .. code-block:: bash

    ./subscriber -DCPSInfoRepo file://simple.ior

.. tab:: Windows

  .. code-block:: batch

    subscriber -DCPSInfoRepo file://simple.ior

The command line parameters direct the application to use the specified file to locate the ``DCPSInfoRepo``.
Our subscriber is now waiting for messages to be sent, so we will now start the publisher in a separate window with the same parameters:

.. tab:: Linux, macOS, BSDs, etc.

  .. code-block:: bash

    ./publisher -DCPSInfoRepo file://simple.ior

.. tab:: Windows

  .. code-block:: batch

    publisher -DCPSInfoRepo file://simple.ior

The publisher connects to the ``DCPSInfoRepo`` to find the location of any subscribers and begins to publish messages as well as write them to the console.
In the subscriber window, you should also now be seeing console output from the subscriber that is reading messages from the topic demonstrating a simple publish and subscribe application.

You can read more about configuring your application for RTPS and other more advanced configuration options in :ref:`run_time_configuration--configuring-for-ddsi-rtps-discovery` and :ref:`run_time_configuration--rtps-udp-transport-configuration-options` .
See :ref:`run_time_configuration--discovery-configuration` and :ref:`inforepo` for configuring and using the ``DCPSInfoRepo`` .
See :ref:`qos` for setting and using QoS features that modify the behavior of your application.

.. _getting_started--running-our-example-with-rtps:

Running Our Example with RTPS
=============================

..
    Sect<2.1.8>

The prior OpenDDS example has demonstrated how to build and execute an OpenDDS application using basic OpenDDS configurations and centralized discovery using the ``DCPSInfoRepo`` service.
The following details what is needed to run the same example using RTPS for discovery and with an interoperable transport.
This is important in scenarios when your OpenDDS application needs to interoperate with a non-OpenDDS implementation of the DDS specification or if you do not want to use centralized discovery in your deployment of OpenDDS.

The coding and building of the Messenger example above is not changed for using RTPS, so you will not need to modify or rebuild your publisher and subscriber services.
This is a strength of the OpenDDS architecture in that to enable the RTPS capabilities, it is an exercise in configuration.
For this exercise, we will enable RTPS for the Messenger example using a configuration file that the publisher and subscriber will share.
More details concerning the configuration of all the available transports including RTPS are described in :ref:`config`.

Navigate to the directory where your publisher and subscriber have been built.
Create a new text file named ``rtps.ini`` and populate it with the following content:

.. code-block:: ini

    [common]
    DCPSGlobalTransportConfig=$file
    DCPSDefaultDiscovery=DEFAULT_RTPS

    [transport/the_rtps_transport]
    transport_type=rtps_udp

The two lines of interest are the one that sets the discovery method and the one that sets the data transport protocol to RTPS.

Now lets re-run our example with RTPS enabled by starting the subscriber process first and then the publisher to begin sending data.
It is best to start them in separate windows to see the two working separately.

Start the subscriber with the ``-DCPSConfigFile`` command line parameter to point to the newly created configuration file...

.. tab:: Linux, macOS, BSDs, etc.

  .. code-block:: bash

    ./subscriber -DCPSConfigFile rtps.ini

.. tab:: Windows

  .. code-block:: batch

    subscriber -DCPSConfigFile rtps.ini

Now start the publisher with the same parameter...

.. tab:: Linux, macOS, BSDs, etc.

  .. code-block:: bash

    ./publisher -DCPSConfigFile rtps.ini

.. tab:: Windows

  .. code-block:: batch

    publisher -DCPSConfigFile rtps.ini

Since there is no centralized discovery in the RTPS specification, there are provisions to allow for wait times to allow discovery to occur.
The specification sets the default to 30 seconds.
When the two above processes are started there may be up to a 30 second delay depending on how far apart they are started from each other.
This time can be adjusted in OpenDDS configuration files and is discussed in :ref:`run_time_configuration--configuring-for-ddsi-rtps-discovery`.

Because the architecture of OpenDDS allows for pluggable discovery and pluggable transports the two configuration entries called out in the ``rtps.ini`` file above can be changed independently with one using RTPS and the other not using RTPS (e.g. centralized discovery using ``DCPSInfoRepo``).
Setting them both to RTPS in our example makes this application fully interoperable with other non-OpenDDS implementations.

.. _getting_started--data-handling-optimizations:

***************************
Data Handling Optimizations
***************************

..
    Sect<2.2>

.. _getting_started--registering-and-using-instances-in-the-publisher:

Registering and Using Instances in the Publisher
================================================

..
    Sect<2.2.1>

The previous example implicitly specifies the instance it is publishing via the sample's data fields.
When ``write()`` is called, the data writer queries the sample's key fields to determine the instance.
The publisher also has the option to explicitly register the instance by calling ``register_instance()`` on the data writer:

.. code-block:: cpp

        Messenger::Message message;
        message.subject_id = 99;
        DDS::InstanceHandle_t handle = message_writer->register_instance(message);

After we populate the Message structure we called the register_instance() function to register the instance.
The instance is identified by the subject_id value of 99 (because we earlier specified that field as the key).

We can later use the returned instance handle when we publish a sample:

.. code-block:: cpp

        DDS::ReturnCode_t ret = data_writer->write(message, handle);

Publishing samples using the instance handle may be slightly more efficient than forcing the writer to query for the instance and is much more efficient when publishing the first sample on an instance.
Without explicit registration, the first write causes resource allocation by OpenDDS for that instance.

Because resource limitations can cause instance registration to fail, many applications consider registration as part of setting up the publisher and always do it when initializing the data writer.

.. _getting_started--reading-multiple-samples:

Reading Multiple Samples
========================

..
    Sect<2.2.2>

The DDS specification provides a number of operations for reading and writing data samples.
In the examples above we used the ``take_next_sample()`` operation, to read the next sample and "take" ownership of it from the reader.
The Message Data Reader also has the following take operations.

* ``take()`` -- Take a sequence of up to max_samples values from the reader

* ``take_instance()`` -- Take a sequence of values for a specified instance

* ``take_next_instance()`` -- Take a sequence of samples belonging to the same instance, without specifying the instance.

There are also "read" operations corresponding to each of these "take" operations that obtain the same values, but leave the samples in the reader and simply mark them as read in the ``SampleInfo``.

Since these other operations read a sequence of values, they are more efficient when samples are arriving quickly.
Here is a sample call to ``take()`` that reads up to 5 samples at a time.

.. code-block:: cpp

        MessageSeq messages(5);
        DDS::SampleInfoSeq sampleInfos(5);
        DDS::ReturnCode_t status = message_dr->take(messages,
                                                    sampleInfos,
                                                    5,
                                                    DDS::ANY_SAMPLE_STATE,
                                                    DDS::ANY_VIEW_STATE,
                                                    DDS::ANY_INSTANCE_STATE);

The three state parameters potentially specialize which samples are returned from the reader.
See the DDS specification for details on their usage.

.. _getting_started--zero-copy-read:

Zero-Copy Read
==============

..
    Sect<2.2.3>

The read and take operations that return a sequence of samples provide the user with the option of obtaining a copy of the samples (single-copy read) or a reference to the samples (zero-copy read).
The zero-copy read can have significant performance improvements over the single-copy read for large sample types.
Testing has shown that samples of 8KB or less do not gain much by using zero-copy reads but there is little performance penalty for using zero-copy on small samples.

The application developer can specify the use of the zero-copy read optimization by calling ``take()`` or ``read()`` with a sample sequence constructed with a max_len of zero.
The message sequence and sample info sequence constructors both take max_len as their first parameter and specify a default value of zero.
The following example code is taken from ``DevGuideExamples/DCPS/Messenger_ZeroCopy/DataReaderListenerImpl.cpp``:

.. code-block:: cpp

          Messenger::MessageSeq messages;
          DDS::SampleInfoSeq info;

          // get references to the samples (zero-copy read of the samples)
          DDS::ReturnCode_t status = dr->take(messages,
                                              info,
                                              DDS::LENGTH_UNLIMITED,
                                              DDS::ANY_SAMPLE_STATE,
                                              DDS::ANY_VIEW_STATE,
                                              DDS::ANY_INSTANCE_STATE);

After both zero-copy takes/reads and single-copy takes/reads, the sample and info sequences' length are set to the number of samples read.
For the zero-copy reads, the ``max_len`` is set to a ``value >= length``.

Since the application code has asked for a zero-copy loan of the data, it must return that loan when it is finished with the data:

.. code-block:: cpp

          dr->return_loan(messages, info);

Calling ``return_loan()`` results in the sequences' ``max_len`` being set to 0 and its owns member set to false, allowing the same sequences to be used for another zero-copy read.

If the first parameter of the data sample sequence constructor and info sequence constructor were changed to a value greater than zero, then the sample values returned would be copies.
When values are copied, the application developer has the option of calling ``return_loan()``, but is not required to do so.

If the ``max_len`` (the first) parameter of the sequence constructor is not specified, it defaults to 0; hence using zero-copy reads.
Because of this default, a sequence will automatically call ``return_loan()`` on itself when it is destroyed.
To conform with the DDS specification and be portable to other implementations of DDS, applications should not rely on this automatic ``return_loan()`` feature.

The second parameter to the sample and info sequences is the maximum slots available in the sequence.
If the ``read()`` or ``take()`` operation's ``max_samples`` parameter is larger than this value, then the maximum samples returned by ``read()`` or ``take()`` will be limited by this parameter of the sequence constructor.

Although the application can change the length of a zero-copy sequence, by calling the ``length(len)`` operation, you are advised against doing so because this call results in copying the data and creating a single-copy sequence of samples.

.. rubric:: Footnotes

.. [#footnote1]

   For backwards compatibility, OpenDDS also parses ``#pragma`` directives which were used before release 3.14.
   This guide will describe IDL annotations only.
