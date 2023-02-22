.. _xtypes--xtypes:

######
XTypes
######

..
    Sect<16>

.. _xtypes--overview:

********
Overview
********

..
    Sect<16.1>

The DDS specification defines a way to build distributed applications using a data-centric publish and subscribe model.
In this model, publishing and subscribing applications communicate via Topics and each Topic has a data type.
An assumption built into this model is that all applications agree on data type definitions for each Topic that they use.
This assumption is not practical as systems must be able to evolve while remaining compatible and interoperable.

The DDS XTypes (Extensible and Dynamic Topic Types) specification loosens the requirement on applications to have a common notion of data types.
Using XTypes, the application developer adds IDL annotations that indicate where the types may vary between publisher and subscriber and how those variations are handled by the middleware.

This release of OpenDDS implements the XTypes specification version 1.3 at the Basic Conformance level, with a partial implementation of the Dynamic Language Binding.
Some features described by the specification are not yet implemented in OpenDDS — those are noted below in section :ref:`xtypes--unimplemented-features`.
This includes IDL annotations that are not yet implemented.
The “Specification Differences” section (:ref:`xtypes--differences-from-the-specification`) describes situations where the implementation of XTypes in OpenDDS departs from or infers something about the specification.
Specification issues have been raised for these situations.

.. _xtypes--features:

********
Features
********

..
    Sect<16.2>

.. _xtypes--extensibility:

Extensibility
=============

..
    Sect<16.2.1>

There are 3 kinds of extensibility for types: Appendable, Mutable, and Final

* *Appendable* denotes a constructed type which may have additional members added onto or removed from the end.

* *Mutable* denotes a constructed type that allows for members to be added, removed, and reordered so long as the keys and the required members of the sender and receiver remain.
  Mutable extensibility is accomplished by assigning a stable identifier to each member.

* *Final* denotes a constructed type that can not add, remove, or reorder members,.
  This can be considered a non-extensible constructed type, with behavior similar to that of a type created before XTypes.

Extensibility is set by the user in the IDL with the annotations: ``@appendable``, ``@mutable``, ``@final``

The default extensibility is Appendable.
This default extensibility can be changed with the IDL compiler command line option --default-extensibility EXTENSIBILITY Where EXTENSIBILITY is "final", "appendable" or "mutable".

Structs, unions, and enums are the only types which can use any of the extensibilities.

The default extensibility for enums is “appendable” and is not governed by --default-extensibility.
TypeObjects for received enums that do not set any flags are treated as a wildcard.

.. _xtypes--assignability:

Assignability
=============

..
    Sect<16.2.2>

Assignability describes the ability of values of one type to be coerced to values of a possibility different type.

Assignability between the type of a writer and reader is checked as part of discovery.
If the types are assignable but not identical, then the “Try Construct” mechanism will be used to coerce values of the writer’s type to values of the reader’s type.

In order for two constructed types to be assignable they must

* Have the same extensibility.

* Have the same set of keys.

Each member of a constructed type has an identifier.
This identifier may be assigned automatically or explicitly.

Union assignability depends on two dimensions.
First, unions are only assignable if their discriminators are assignable.
Second, for any branch label or default that exists in both unions, the members selected by that branch label must be assignable.

.. _xtypes--interoperability-with-non-xtypes-implementations:

Interoperability with non-XTypes Implementations
================================================

..
    Sect<16.2.3>

Communication with a non-XTypes DDS (either an older OpenDDS or another DDS implementation which has RTPS but not XTypes 1.2+) requires compatible IDL types and the use of RTPS Discovery.
Compatible IDL types means that the types are structurally equivalent and serialize to the same bytes using XCDR version 1.

Additionally, the XTypes-enabled participant needs to be set up as follows:

* Types cannot use Mutable extensibility

* Data Writers must have their Data Representation QoS policy set to DDS::XCDR_DATA_REPRESENTATION

* Data Readers must include DDS::XCDR_DATA_REPRESENTATION in the list of data representations in their Data Representation QoS (This is true by default)

The “Data Representation" section below shows how to change the data representation.

.. _xtypes--dynamic-language-binding:

Dynamic Language Binding
========================

..
    Sect<16.2.4>

Before the XTypes specification, all DDS applications worked by mapping the topic's data type directly into the programming language and having the data handling APIs such as read, write, and take, all defined in terms of that type.
As an example, topic type A (an IDL structure) caused code generation of IDL interfaces ADataWriter and ADataReader while topic type B generated IDL interfaces BDataWriter and BDataReader.
If an application attempted to pass an object of type A to the BDataWriter, a compile-time error would occur (at least for statically typed languages including C++ and Java).
Advantages to this design include efficiency and static type safety, however, the code generation required by this approach is not desirable for every DDS application.

The XTypes Dynamic Language Binding defines a generic data container DynamicData and the interfaces DynamicDataWriter and DynamicDataReader.
Applications can create instances of DynamicDataWriter and DynamicDataReader that work with various topics in the domain without needing to incorporate the generated code for those topics' data types.
The system is still type safe but the type checks occur at runtime instead of at compile time.
The Dynamic Language Binding is described in detail in section :ref:`xtypes--dynamic-language-binding-1`.

.. _xtypes--examples-and-explanation:

************************
Examples and Explanation
************************

..
    Sect<16.3>

Suppose you are in charge of deploying a set of weather stations that publish temperature, pressure, and humidity.
The following examples show how various features of XTypes may be applied to address changes in the schema published by the weather station.
Specifically, without XTypes, one would either need to create a new type with its own DataWriters/DataReaders or update all applications simultaneously.
With proper planning and XTypes, one can simply modify the existing type (within limits) and writers and readers using earlier versions of the topic type will remain compatible with each other and be compatible with writers and readers using new versions of the topic type.

.. _xtypes--mutable-extensibility:

Mutable Extensibility
=====================

..
    Sect<16.3.1>

The type published by the weather stations can be made extensible with the ``@mutable`` annotation:

.. code-block:: omg-idl

    // Version 1
    @topic
    @mutable
    struct StationData {
      short temperature;
      double pressure;
      double humidity;
    };

Suppose that some time in the future, a subset of the weather stations are upgraded to monitor wind speed and direction:

.. code-block:: omg-idl

    enum WindDir {N, NE, NW, S, SE, SW, W, E};
    // Version 2
    @topic
    @mutable
    struct StationData {
      short temperature;
      double pressure;
      double humidity;
      short wind_speed;
      WindDir wind_direction;
    };

When a Version 2 writer interacts with a Version 1 reader, the additional fields will be ignored by the reader.
When a Version 1 writer interacts with a Version 2 reader, the additional fields will be initialized to a "logical zero" value for its type (empty string, ``FALSE`` boolean) - see Table 9 of the XTypes specification for details.

.. _xtypes--assignability-1:

Assignability
=============

..
    Sect<16.3.2>

The first and second versions of the ``StationData`` type are *assignable* meaning that it is possible to construct a version 2 value from a version 1 value and vice-versa.
The assignability of non-constructed types (e.g., integers, enums, strings) is based on the types being identical or identical up to parameterization, i.e., bounds of strings and sequences may differ.
The assignability of constructed types like structs and unions is based on finding corresponding members with assignable types.
Corresponding members are those that have the same id.

A type marked as ``@mutable`` allows for members to be added, removed, or reordered so long as member ids are preserved through all of the mutations.

.. _xtypes--member-ids:

Member IDs
==========

..
    Sect<16.3.3>

Member ids are assigned using various annotations.
A policy for a type can be set with either ``@autoid(SEQUENTIAL)`` or ``@autoid(HASH)``:

.. code-block:: omg-idl

    // Version 3
    @topic
    @mutable
    @autoid(SEQUENTIAL)
    struct StationData {
      short temperature;
      double pressure;
      double humidity;
    };

    // Version 4
    @topic
    @mutable
    @autoid(HASH)
    struct StationData {
      short temperature;
      double pressure;
      double humidity;
    };

SEQUENTIAL causes ids to be assigned based on the position in the type.
HASH causes ids to be computed by hashing the name of the member.
If no @autoid annotation is specified, the policy is SEQUENTIAL.

Suppose that Version 3 was used in the initial deployment of the weather stations and the decision was made to switch to @autoid(HASH) when adding the new fields for wind speed and direction.
In this case, the ids of the pre-existing members can be set with @id:

.. code-block:: omg-idl

    enum WindDir {N, NE, NW, S, SE, SW, W, E};

    // Version 5
    @topic
    @mutable
    @autoid(HASH)
    struct StationData {
      @id(0) short temperature;
      @id(1) double pressure;
      @id(2) double humidity;
      short wind_speed;
      WindDir wind_direction;
    };

See the “Member ID Annotations” section for more details.

.. _xtypes--appendable-extensibility:

Appendable Extensibility
========================

..
    Sect<16.3.4>

Mutable extensibility requires a certain amount of overhead both in terms of processing and network traffic.
A more efficient but less flexible form of extensibility is @appendable.
Extensibility with @appendable is limited in that members can only be added to or removed from the end of the type.
With @appendable, the initial version of the weather station IDL would be:

.. code-block:: omg-idl

    // Version 6
    @topic
    @appendable
    struct StationData {
      short temperature;
      double pressure;
      double humidity;
    };

And the subsequent addition of the wind speed and direction members would be:

.. code-block:: omg-idl

    enum WindDir {N, NE, NW, S, SE, SW, W, E};

    // Version 7
    @topic
    @appendable
    struct StationData {
      short temperature;
      double pressure;
      double humidity;
      short wind_speed;
      WindDir wind_direction;
    };

As with @mutable, when a Version 7 Writer interacts with a Version 6 Reader, the additional fields will be ignored by the reader.
When a Version 6 Writer interacts with a Version 7 Reader, the additional fields will be initialized to default values based on Table 9 of the XTypes specification.

Appendable is the default extensibility.

.. _xtypes--final-extensibility:

Final Extensibility
===================

..
    Sect<16.3.5>

The third kind of extensibility is @final.
Annotating a type with @final means that it will not be compatible with (assignable to/from) a type that's structurally different.
The @final annotation can be used to define types for pre-XTypes compatibility or in situations where the overhead of @mutable or @appendable is unacceptable.

.. _xtypes--try-construct:

Try Construct
=============

..
    Sect<16.3.6>

From a reader’s perspective, there are three possible scenarios when attempting to initialize a member.
First, the member type is identical to the member type of the reader.
This is the trivial case the value from the writer is copied to the value for the reader.
Second, the writer does not have the member.
In this case, the value for the reader is initialized to a default value based on Table 9 of the XTypes specification (this is the "logical zero" value for the type).
Third, the type offered by the writer is assignable but not identical to the type required by the reader.
In this case, the reader must try to construct its value from the corresponding value provided by the writer.

Suppose that the weather stations also publish a topic containing station information:

.. code-block:: omg-idl

    typedef string<8> StationID;
    typedef string<256> StationName;

    // Version 1
    @topic
    @mutable
    struct StationInfo {
      @try_construct(TRIM) StationID station_id;
      StationName station_name;
    };

Eventually, the pool of station IDs is exhausted so the IDL must be refined as follows:

.. code-block:: omg-idl

    typedef string<16> StationID;
    typedef string<256> StationName;

    // Version 2
    @topic
    @mutable
    struct StationInfo {
      @try_construct(TRIM) StationID station_id;
      StationName station_name;
    };

If a Version 2 writer interacts with a Version 1 reader, the station ID will be truncated to 8 characters.
While perhaps not ideal, it will still allow the systems to interoperate.

There are two other forms of try-construct behavior.
Fields marked as @try_construct(USE_DEFAULT) will receive a default value if value construction fails.
In the previous example, this means the reader would receive an empty string for the station ID if it exceeds 8 characters.
Fields marked as @try_construct(DISCARD) cause the entire sample to be discarded.
In the previous example, the Version 1 reader will never see a sample from a Version 2 writer where the original station ID contains more than 8 characters.
@try_construct(DISCARD) is the default behavior.

.. _xtypes--data-representation:

*******************
Data Representation
*******************

..
    Sect<16.4>

Data representation is the way a data sample can be encoded for transmission.
Data representation can be XML, XCDR1, or XCDR2.

* XML is unsupported and should not be used

* XCDR1 with appendable extensibility should not be used

* XCDR2 is completely supported and preferred

XCDR2 is a more robust version of XCDR1 and should be used in preference to XCDR1 unless there is a reason to do otherwise.

Data representation is a QoS policy alongside the other QoS options.
Its listed values represent allowed serialized forms of the data sample.
The DataWriter and DataReader need to have at least one matching data representation for communication between them to be possible.

The default value of the DataRepresentationQoS policy is an empty sequence.
This is interpreted by the middleware as XCDR2 for DataWriters and the alternatives XCDR1 | XCDR2 for DataReaders.
A writer or reader without an explicitly-set DataRepresentationQoS will therefore be able to communicate with another reader or writer which is compatible with XCDR2.
The example below shows a possible configuration for an XCDR1 DataWriter.

.. code-block:: cpp

    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.representation.value.length(1);
    qos.representation.value[0] = DDS::XCDR_DATA_REPRESENTATION;
    DDS::DataWriter_var dw = pub->create_datawriter(topic, qos, 0, 0);

Note that the IDL constant used for XCDR1 is ``XCDR_DATA_REPRESENTATION`` (without the digit).

In addition to a DataWriter/DataReader QoS setting for data representation, each type defined in IDL can have its own data representation specified via an annotation.
This value restricts which data representations can be used for that type.
A DataWriter/DataReader must have at least one data representation in common with the type it uses.

The default value for an unspecified data representation annotation is to allow all forms of serialization.

The type's set of allowed data representations can be specified by the user in IDL with the notation: ``@OpenDDS::data_representation(XCDR2)`` where XCDR2 is replaced with the specific data representation.

.. _xtypes--type-consistency-enforcement:

****************************
Type Consistency Enforcement
****************************

..
    Sect<16.5>

The Type Consistency Enforcement QoS policy lets the application fine-tune details of how types may differ between writers and readers.
The policy is only applies to data readers.
This means that each reader can set its own policy for how its type may vary from the types of the writers that it may match.

There are six members of the ``TypeConsistencyEnforcementQosPolicy`` struct defined by XTypes, but OpenDDS only supports setting one of them: ``ignore_member_names``.
All other members should be kept at their default values.

``ignore_member_names`` defaults to ``FALSE`` so member names (along with Member IDs, see :ref:`xtypes--member-id-assignment`) are significant for type compatibility.
Changing this to TRUE means that only Member IDs are used for type compatibility.

.. _xtypes--idl-annotations:

***************
IDL Annotations
***************

..
    Sect<16.6>

.. _xtypes--indicating-which-types-can-be-topic-types:

Indicating which Types can be topic types
=========================================

..
    Sect<16.6.1>

.. _xtypes--topic:

@topic
------

..
    Sect<16.6.1.1>

Applies To: struct or union type declarations

The topic annotation marks a topic type for samples to be transmitted from a publisher or received by a subscriber.
A topic type may contain other topic and non-topic types.
See section :ref:`getting_started--defining-data-types-with-idl` for more details.

.. _xtypes--nested:

@nested
-------

..
    Sect<16.6.1.2>

Applies To: struct or union type declarations

The ``@`` nested annotation marks a type that will always be contained within another.
This can be used to prevent a type from being used as a topic.
One reason to do so is to reduce the amount of code generated for that type.

.. _xtypes--default-nested:

@default_nested
---------------

..
    Sect<16.6.1.3>

Applies To: modules

The ``@default_nested(TRUE)`` or ``@default_nested(FALSE)`` sets the default nesting behavior for a module.
Types within a module marked with ``@default_nested(FALSE)`` can still set their own behavior with ``@nested``.

.. _xtypes--specifying-allowed-data-representations:

Specifying allowed Data Representations
=======================================

..
    Sect<16.6.2>

Data Representation annotations mark the formats in which data samples of this type can be represented in a serialized form.
The Data Representation annotations listed on the type will be compared to those in the QoS policies of the reader or writer that is trying to use the type.
If a data representation is shared between the type and entity, then they can be used together.
OpenDDS’s default data representation for entities is XCDR2.
If no data representation is specified for a type, there are no restrictions on which data representations that a QoS can use with the type.

.. _xtypes--opendds-data-representation-xml:

@OpenDDS::data_representation(XML)
----------------------------------

..
    Sect<16.6.2.1>

Applies To: topic types

Limitations: XML is not currently supported

.. _xtypes--opendds-data-representation-xcdr1:

@OpenDDS::data_representation(XCDR1)
------------------------------------

..
    Sect<16.6.2.2>

Applies To: topic types

Limitations: XCDR1 is not recommended.
See section :ref:`xtypes--data-representation` for details

.. _xtypes--opendds-data-representation-xcdr2:

@OpenDDS::data_representation(XCDR2)
------------------------------------

..
    Sect<16.6.2.3>

Applies To: topic types

XCDR2 is currently the recommended data representation.

.. _xtypes--standard-data-representation:

Standard @data_representation
-----------------------------

..
    Sect<16.6.2.4>

``tao_idl`` doesn’t support bitset, which the standard ``@data_representation`` requires.
Instead use ``@OpenDDS::data_representation`` which is similar, but doesn’t support bitmask value chaining like ``@data_representation(XCDR|XCDR2)``.
The equivalent would require two separate annotations: ``@OpenDDS::data_representation(XCDR1) @OpenDDS::data_representation(XCDR2).``

.. _xtypes--determining-extensibility:

Determining Extensibility
=========================

..
    Sect<16.6.3>

The extensibility annotations determine how a type may be changed and still be compatible.
If no extensibility annotation is set, the type will default to appendable.
The default can be changed with the command line option --default-extensibility *type*, where *type* can be final, appendable, or mutable.

.. _xtypes--mutable:

@mutable
--------

..
    Sect<16.6.3.1>

Alias: ``@extensibility(MUTABLE)``

Applies To: type declarations

This annotation indicates a type may have non-key or non-must-understand members removed.
It may also have additional members added.

.. _xtypes--appendable:

@appendable
-----------

..
    Sect<16.6.3.2>

Alias: ``@extensibility(APPENDABLE)``

Applies To: type declarations

This annotation indicates a type may have additional members added or members at the end of the type removed.

Limitations: Appendable is not currently supported when XCDR1 is used as the data representation.

.. _xtypes--final:

@final
------

..
    Sect<16.6.3.3>

Alias: ``@extensibility(FINAL)``

Applies To: type declarations

This annotation marks a type that cannot be changed and still be compatible.
Final is most similar to pre-XTypes.

.. _xtypes--customizing-xtypes-per-member:

Customizing XTypes per-member
=============================

..
    Sect<16.6.4>

Try Construct annotations dictate how members of one object should be converted from members of a different but assignable object.
If no try construct annotation is added, it will default to discard.

.. _xtypes--try-construct-use-default:

@try_construct(USE_DEFAULT)
---------------------------

..
    Sect<16.6.4.1>

Applies to: structure and union members, sequence and array elements

The use_default try construct annotation will set the member whose deserialization failed to a default value which is determined by the XTypes specification.
Sequences will be of length 0, with the same type as the original sequence.
Primitives will be set equal to 0.
Strings will be replaced with the empty string.
Arrays will be of the same length but have each element set to the default value.
Enums will be set to the first enumerator defined.

.. _xtypes--try-construct-trim:

@try_construct(TRIM)
--------------------

..
    Sect<16.6.4.2>

Applies to: structure and union members, sequence and array elements

The trim try construct annotation will, if possible, shorten a received value to one fitting the receiver’s bound.
As such, trim only makes logical sense on bounded strings and bounded sequences.

.. _xtypes--try-construct-discard:

@try_construct(DISCARD)
-----------------------

..
    Sect<16.6.4.3>

Applies to: structure and union members, sequence and array elements

The discard try construct annotation will “throw away” the sample if an element fails to deserialize.

.. _xtypes--member-id-assignment:

Member ID assignment
====================

..
    Sect<16.6.5>

If no explicit id annotation is used, then Member IDs will automatically be assigned sequentially.

.. _xtypes--id-value:

@id(value)
----------

..
    Sect<16.6.5.1>

Applies to: structure and union members

The *value* is a 32-bit integer which assigns that member’s ID.

.. _xtypes--autoid-value:

@autoid(value)
--------------

..
    Sect<16.6.5.2>

Applies to: module declarations, structure declarations, union declarations

The autoid annotation can take two values, ``HASH`` or ``SEQUENTIAL``\.
``SEQUENTIAL`` states that the identifier shall be computed by incrementing the preceding one.
``HASH`` states that the identifier should be calculated with a hashing algorithm – the input to this hash is the member’s name.
``HASH`` is the default value of ``@autoid``.

.. _xtypes--hashid-value:

@hashid(value)
--------------

..
    Sect<16.6.5.3>

Applies to: structure and union members

The ``@hashid`` sets the identifier to the hash of the ``value`` parameter, if one is specified.
If the**``value`` parameter is omitted or is the empty string, the member’s name is used as if it was the ``value``.

.. _xtypes--determining-the-key-fields-of-a-type:

Determining the Key Fields of a Type
====================================

..
    Sect<16.6.6>

.. _xtypes--key:

@key
----

..
    Sect<16.6.6.1>

Applies to: structure members, union discriminator

The ``@key`` annotation marks a member used to determine the Instances of a topic type.
See section :ref:`getting_started--keys` for more details on the general concept of a Key.
For XTypes specifically, two types can only be compatible if each contains the members that are keys within the other.

.. _xtypes--dynamic-language-binding-1:

************************
Dynamic Language Binding
************************

..
    Sect<16.7>

For an overview of the Dynamic Language Binding, see section :ref:`xtypes--dynamic-language-binding`.
This section describes the features of the Dynamic Language Binding that OpenDDS supports.

There are two main usage patterns supported:

* Applications can receive DynamicData from a Recorder object (see section :ref:`alternate_interfaces_to_data--recorder-and-replayer`)

* Applications can use XTypes DynamicDataWriter and/or DynamicDataReader (see section :ref:`xtypes--dynamicdatawriters-and-dynamicdatareaders`)

To use DynamicDataWriter and/or DynamicDataReader for a given topic, the data type definition for that topic must be available to the local DomainParticipant.
There are a few ways this can be achieved, see section :ref:`xtypes--obtaining-dynamictype-and-registering-typesupport` for details.

.. _xtypes--representing-types-with-typeobject-and-dynamictype:

Representing Types with TypeObject and DynamicType
==================================================

..
    Sect<16.7.1>

In XTypes, the types of the peers may not be identical, as in the case of appendable or mutable extensibility.
In order for a peer to be aware of its remote peer’s type, there must be a way for the remote peer to communicate its type.
TypeObject is an alternative to IDL for representing types, and one of the purposes of TypeObject is to communicate the peers’ types.

There are two classes of TypeObject: MinimalTypeObject and CompleteTypeObject.
A MinimalTypeObject object contains minimal information about the type that is sufficient for a peer to perform type compatibility checking.
However, MinimalTypeObject may not contain all information about the type as represented in the corresponding user IDL file.
In cases where the complete information about the type is required, CompleteTypeObject should be used.
When XTypes is enabled, peers communicate their TypeObject information during the discovery process automatically.
Internally, the local and received TypeObjects are stored in a TypeLookupService object, which is shared between the entities in the same DomainParticipant.

In the Dynamic Language Binding, each type is represented using a DynamicType object, which has a TypeDescriptor object that describes all the information needed to correctly process the type.
Likewise, each member in a type is represented using a DynamicTypeMember object, which has a MemberDescriptor object that describes any information needed to correctly process the type member.
DynamicType is converted from the corresponding CompleteTypeObject internally by the system.

.. _xtypes--enabling-use-of-completetypeobjects:

Enabling Use of CompleteTypeObjects
-----------------------------------

..
    Sect<16.7.1.1>

To enable use of ``CompleteTypeObject`` s needed for the dynamic binding, they must be generated and OpenDDS must be configured to use them.
To generate them, :ref:`-Gxtypes-complete <opendds_idl--gxtypes-complete>` must be passed to ``opendds_idl`` (Table :ref:`opendds_idl--opendds-idl-command-line-options`).
For MPC, this can be done by adding this to the opendds_idl arguments for idl files in the project, like this:

::

    TypeSupport_Files {
      dcps_ts_flags += -Gxtypes-complete
      Messenger.idl
    }

To do the same for CMake:

::

    OPENDDS_TARGET_SOURCES(target
      Messenger.idl
      OPENDDS_IDL_OPTIONS -Gxtypes-complete
    )

Once set up to be generated, OpenDDS has to be configured to send and receive the ``CompleteTypeObject`` s.
This can be done by setting the :ref:`UseXTypes <run_time_configuration--usextypes>` RTPS discovery configuration option (Table :ref:`run_time_configuration--configuring-for-ddsi-rtps-discovery`) or programmatically using the ``OpenDDS::RTPS::RtpsDiscovery::use_xtypes()`` setter methods.

.. _xtypes--interpreting-data-samples-with-dynamicdata:

Interpreting Data Samples with DynamicData
==========================================

..
    Sect<16.7.2>

Together with DynamicType, DynamicData allows users to interpret a received data sample and read individual fields from it.
Each DynamicData object is associated with a type, represented by a DynamicType object, and the data corresponding to an instance of that type.
Let’s take a look at an example with the following type, described below in IDL:

.. code-block:: omg-idl

    @appendable
    struct NestedStruct {
      @id(1) short s_field;
    };

    @topic
    @mutable
    struct MyStruct {
      @id(1) long l_field;
      @id(2) unsigned short us_field;
      @id(3) float f_field;
      @id(4) NestedStruct nested_field;
      @id(5) sequence<unsigned long> ul_seq_field;
      @id(6) double d_field[10];
    };

The samples for MyStruct are written by a normal, statically-typed DataWriter.
The writer application needs to have the IDL-generated code including the “complete” form of TypeObjects.
Use a command-line option to opendds_idl to enable CompleteTypeObjects since the default is to generate MinimalTypeObjects (see section :ref:`opendds_idl--opendds-idl-command-line-options`).

One way to obtain a DynamicData object representing a data sample received by the participant is using the Recorder and RecorderListener classes (see section :ref:`alternate_interfaces_to_data--recorder-and-replayer`).
Recorder’s get_dynamic_data can be used to construct a DynamicData object for each received sample from the writer.
Internally, the CompleteTypeObjects received from discovering that writer are converted to DynamicTypes and they are then used to construct the DynamicData objects.
Once a DynamicData object for a MyStruct sample is constructed, its members can be read as described in the following sections.
Another way to obtain a DynamicData object is from a DynamicDataReader (section :ref:`xtypes--creating-and-using-a-dynamicdatareader`).

.. _xtypes--reading-basic-types:

Reading Basic Types
-------------------

..
    Sect<16.7.2.1>

DynamicData provides methods for reading members whose types are basic such as integers, floating point numbers, characters, boolean.
For a complete list of basic types for which DynamicData provides an interface, please refer to the XTypes specification.
To call a correct method for reading a member, we need to know the type of the member as well as its id.
For our example, we first want to get the number of members that the sample contains.
In these examples, the “data” object is an instance of DynamicData.

::

    ACE_CDR::ULong count = data.get_item_count();

Then, each member’s id can be read with get_member_id_at_index.
The input for this function is the index of the member in the sample, which can take a value from 0 to count - 1.

::

    XTypes::MemberId id = data.get_member_id_at_index(0);

The MemberDescriptor for the corresponding member then can be obtained as follows.

::

    XTypes::MemberDescriptor md;
    DDS::ReturnCode_t ret = data.get_descriptor(md, id);

The returned MemberDescriptor allows us to know the type of the member.
Suppose id is 1, meaning that the member at index 0 is l_field, we now can get its value.

::

    ACE_CDR::Long my_long;
    ret = data.get_int32_value(my_long, id);

After the call, my_long contains the value of the member l_field from the sample.
The method returns ``DDS::RETCODE_OK`` if successful and ``DDS::RETCODE_ERROR`` in case of failure.
Note that the method called on the DynamicData object must match the type of the requested member; in this example, the member has type long (from its IDL) and thus ``get_int32_value`` is called.
If the method called doesn’t match the type of the member, it will return ``DDS::RETCODE_ERROR``.

Similarly, suppose we have already found out the types and ids of the members us_field and f_field, their values can be read as follows.

::

    ACE_CDR::UShort my_ushort;
    ret = data.get_get_uint16_value(my_ushort, 2); // Get the value of us_field
    ACE_CDR::Float my_float;
    ret = data.get_float32_value(my_float, 3); // Get the value of f_field

.. _xtypes--reading-collections-of-basic-types:

Reading Collections of Basic Types
----------------------------------

..
    Sect<16.7.2.2>

Besides a list of methods for getting values of members of basic types, DynamicData also defines methods for reading sequence members.
In particular, for each method that reads value from a basic type, there is a counterpart that reads a sequence of the same basic type.
For instance, get_int32_value reads the value from a member of type long, and get_int32_values reads the value from a member of type sequence<long>.
For the member ul_seq_field in our example, its value can be read as follows.

::

    CORBA::ULongSeq my_ul_seq;
    ret = data.get_uint32_values(my_ul_seq, id); // id is 5

Because ul_seq_field is a sequence of unsigned 32-bit integers, the get_uint32_values method is used.
Again, the second argument is the id of the requested member, which is 5 for ul_seq_field.
When successful, my_ul_seq contains values of all elements of the member ul_seq_field in the sample.

To get the values of the array member d_field, we first need to create a separate DynamicData object for it, and then read individual elements of the array using the new DynamicData object.

::

    XTypes::DynamicData array_data;
    DDS::ReturnCode_t ret = data.get_complex_value(array_data, id); // id is 6

    const ACE_CDR::ULong num_items = array_data.get_item_count();
    for (ACE_CDR::ULong i = 0; i < num_items; ++i) {
      const XTypes::MemberId my_id = array_data.get_member_id_at_index(i);
      ACE_CDR::Double my_double;
      ret = array_data.get_float64_value(my_double, my_id);
    }

In the example code above, get_item_count returns the number of elements of the array.
Inside the for loop, the index of each element is converted to an id within the array using get_member_id_at_index.
Then, this id is used to read the element’s value into my_double.
Note that the second parameter of the interfaces provided by DynamicData must be the id of the requested member.
In case of collection, elements are considered members of the collection.
However, the collection element doesn’t have a member id.
And thus, we need to convert its index into an id before calling a get_*_value (or get_*_values) method.

.. _xtypes--reading-members-of-more-complex-types:

Reading Members of More Complex Types
-------------------------------------

..
    Sect<16.7.2.3>

For a more complex member such as a nested structure or union, the discussed DynamicData methods are not suitable.
And thus, users first need to get a new DynamicData object that represents the sole data of the member with get_complex_value.
This new DynamicData object can then be used to get the values of the inner members of the nested member.
For example, a DynamicData object for the nested_field member of the MyStruct sample can be obtained as follows.

::

    XTypes::DynamicData nested_data;
    DDS::ReturnCode_t ret = data.get_complex_value(nested_data, id); // id is 4

Recall that nested_field has type NestedStruct which has one member s_field with id 1.
Now the value of s_field can be read from nested_data using get_int16_value, since s_field has type short.

::

    ACE_CDR::Short my_short;
    ret = nested_data.get_int16_value(my_short, id); // id is 1

The get_complex_value method is also suitable for any other cases where the value of a member cannot be read directly using the get_*_value or get_*_values methods.
As an example, suppose we have a struct MyStruct2 defined as follows.

.. code-block:: omg-idl

    @appendable
    struct MyStruct2 {
      @id(1) sequence<NestedStruct> seq_field;
    };

And suppose we already have a DynamicData object, called data, that represents a sample of MyStruct2.
To read the individual elements of seq_field, we first get a new DynamicData object for the seq_field member.

::

    XTypes::DynamicData seq_data;
    DDS::ReturnCode_t ret = data.get_complex_value(seq_data, id); // id is 1

Since the elements of seq_field are structures, for each of them we create another new DynamicData object to represent it, which can be used to read its member.

::

    const ACE_CDR::ULong num_elems = seq_data.get_item_count();
    for (ACE_CDR::ULong i = 0; i < num_elems; ++i) {
      const XTypes::MemberId my_id = seq_data.get_member_id_at_index(i);
      XTypes::DynamicData elem_data; // Represent each element.
      ret = seq_data.get_complex_value(elem_data, my_id);
      ACE_CDR::Short my_short;
      ret = elem_data.get_int16_value(my_short, 1);
    }

.. _xtypes--populating-data-samples-with-dynamicdata:

Populating Data Samples With DynamicData
========================================

..
    Sect<16.7.3>

DynamicData objects can be created by the application and populated with data so that they can be used as data samples which are written to a DynamicDataWriter (section :ref:`xtypes--creating-and-using-a-dynamicdatawriter`).

To create a DynamicData object, use the DynamicDataFactory API defined by the XTypes spec:

.. code-block:: cpp

    DDS::DynamicData_var dynamic =
      DDS::DynamicDataFactory::get_instance()->create_data(type);

Like other data types defined by IDL interfaces (for example, the DataWriter types), the "dynamic" object's lifetime is managed with a smart pointer – in this case DDS::DynamicData_var.

The "type" input parameter to create_data() is an object that implements the DDS::DynamicType interface.
The DynamicType representation of any type that's supported as a topic data type is available from its corresponding TypeSupport object (see section :ref:`xtypes--obtaining-dynamictype-and-registering-typesupport`) using the get_type() operation.
Once the application has access to that top-level type, the DynamicType interface can be used to obtain complete information about the type including nested and referenced data types.
See the file :ghfile:`dds/DdsDynamicData.idl` in OpenDDS for the definition of the DynamicType and related interfaces.

Once the application has created the DynamicData object, it can be populated with data members of any type.
The operations used for this include the DynamicData operations named "set_*" for the various data types.
They work similarly to the "get_*" operations that are described in section :ref:`xtypes--interpreting-data-samples-with-dynamicdata`.
When populating the DynamicData of complex data types, use get_complex_value() (see :ref:`xtypes--reading-members-of-more-complex-types`) to navigate from DynamicData representing containing types to DynamicData representing contained types.

.. _xtypes--dynamicdatawriters-and-dynamicdatareaders:

DynamicDataWriters and DynamicDataReaders
=========================================

..
    Sect<16.7.4>

DynamicDataWriters and DataReaders are designed to work like any other DataWriter and DataReader except that their APIs are defined in terms of the DynamicData type instead of a type generated from IDL.
Each DataWriter and DataReader has an associated Topic and that Topic has a data type (represented by a TypeSupport object).
Behavior related to keys, QoS policies, discovery and built-in topics, DDS Security, and transport is not any different for a DynamicDataWriter or DataReader.
One exception is that in the current implementation, Content-Subscription features (Chapter :ref:`content_subscription_profile--content-subscription-profile`) are not supported for DynamicDataWriters and DataReaders.

.. _xtypes--obtaining-dynamictype-and-registering-typesupport:

Obtaining DynamicType and Registering TypeSupport
-------------------------------------------------

..
    Sect<16.7.4.1>

OpenDDS currently supports two usage patterns for obtaining a TypeSupport object that can be used with the Dynamic Language Binding:

* Dynamically load a library that has the IDL-generated code

* Get the DynamicType of a peer DomainParticipant that has CompleteTypeObjects

The XTypes specification also describes how an application can construct a new type at runtime, but this is not yet implemented in OpenDDS.

To use a shared library (``*.dll`` on Windows, ``*.so`` on Linux, ``*.dylib`` on macOS, etc.)
as a type support plug-in, an application simply needs to load the library into its process.
This can be done with the ACE cross-platform support library that OpenDDS itself uses, or using a platform-specific function like LoadLibrary or dlopen.
The application code does not need to include any generated headers from this IDL.
This makes the type support library a true plug-in, meaning it can be loaded into an application that had no knowledge of it when that application was built.

Once the shared library is loaded, an internal singleton class in OpenDDS called Registered_Data_Types can be used to obtain a reference to the TypeSupport object.

.. code-block:: cpp

    DDS::TypeSupport_var ts_static = Registered_Data_Types->lookup(0, "TypeName");

This TypeSupport object "ts_static" is not registered with the DomainParticipant and is not set up for the Dynamic Language Binding.
But, crucially, it does have the DynamicType object that we'll need to set up a second TypeSupport object which is registered with the DomainParticipant.

.. code-block:: cpp

    DDS::DynamicType_var type = ts_static->get_type();
    DDS::DynamicTypeSupport_var ts_dynamic = new DynamicTypeSupport(type);
    DDS::ReturnCode_t ret = ts_dynamic->register_type(participant, "");

Now the type support object ts_dynamic can be used in the usual DataWriter/DataReader setup sequence (creating a Topic first, etc.)
but the created DataWriters will be DynamicDataWriters (see section :ref:`xtypes--creating-and-using-a-dynamicdatawriter`) and the created DataReaders will be DynamicDataReaders (see section :ref:`xtypes--creating-and-using-a-dynamicdatareader`).

The other approach to obtaining TypeSupport objects for use with the Dynamic Language Binding is to have DDS discovery's built-in endpoints get TypeObjects from remote domain participants.
To do this, use the get_dynamic_type method on the singleton Service_Participant object.

.. code-block:: cpp

    DDS::DynamicType_var type; // NOTE: passed by reference below
    DDS::ReturnCode_t ret = TheServiceParticipant->get_dynamic_type(type, participant, key);

The two input parameters to get_dynamic_type are the 'participant' (an object reference to the DomainParticipant that will be used to register our TypeSupport and create Topics, DataWriters, and/or DataReders) and the 'key' which is the DDS::BuiltinTopicKey_t that identifies the remote entity which has the data type that we'll use.
This key can be obtained from the Built-In Publications topic (which identifies remote DataWriters) or the Built-In Subscriptions topic (which identifies remote DataReaders).
See :ref:`built_in_topics--built-in-topics` for details on using the Built-In Topics.

The type obtained from get_dynamic_type can be used to create and register a TypeSupport object.

.. code-block:: cpp

    DDS::DynamicTypeSupport_var ts_dynamic = new DynamicTypeSupport(type);
    DDS::ReturnCode_t ret = ts_dynamic->register_type(participant, "");

.. _xtypes--creating-and-using-a-dynamicdatawriter:

Creating and Using a DynamicDataWriter
--------------------------------------

..
    Sect<16.7.4.2>

Following the steps in section :ref:`xtypes--obtaining-dynamictype-and-registering-typesupport`, a DynamicTypeSupport object is registered with the domain participant.
The type name used to register with the participant may be the default type name (used when an empty string is passed to the register_type operation), or some other type name.
If the default type name was used, the application can access that name by invoking the get_type_name operation on the TypeSupport object.

The registered type name is then used as one of the input parameters to create_topic, just like when creating a topic for the Plain (non-Dynamic) Language Binding.
Once a Topic object exists, create a DataWriter using this Topic.
The DataWriter object can be narrowed to the DynamicDataWriter IDL interface:

.. code-block:: cpp

    DDS::DynamicDataWriter_var w = DDS::DynamicDataWriter::_narrow(writer);

The DynamicDataWriter IDL interface is defined in ``dds/DdsDynamicTypeSupport.idl`` in OpenDDS.
It provides the same operations as any other DataWriter, but with DynamicData as its data type.
See section :ref:`xtypes--populating-data-samples-with-dynamicdata` for details on creating DynamicData objects for use with the DynamicDataWriter interface.

.. _xtypes--creating-and-using-a-dynamicdatareader:

Creating and Using a DynamicDataReader
--------------------------------------

..
    Sect<16.7.4.3>

Following the steps in section :ref:`xtypes--obtaining-dynamictype-and-registering-typesupport`, a DynamicTypeSupport object is registered with the domain participant.
The type name used to register with the participant may be the default type name (used when an empty string is passed to the register_type operation), or some other type name.
If the default type name was used, the application can access that name by invoking the get_type_name operation on the TypeSupport object.

The registered type name is then used as one of the input parameters to create_topic, just like when creating a topic for the Plain (non-Dynamic) Language Binding.
Once a Topic object exists, create a DataReader using this Topic.
The DataReader object can be narrowed to the DynamicDataReader IDL interface:

.. code-block:: cpp

    DDS::DynamicDataReader_var r = DDS::DynamicDataReader::_narrow(reader);

The DynamicDataReader IDL interface is defined in :ghfile:`dds/DdsDynamicTypeSupport.idl` in OpenDDS.
It provides the same operations as any other DataReader, but with DynamicData as its data type.
See section :ref:`xtypes--interpreting-data-samples-with-dynamicdata` for details on using DynamicData objects obtained from the DynamicDataReader interface.

.. _xtypes--limitations-of-the-dynamic-language-binding:

Limitations of the Dynamic Language Binding
-------------------------------------------

..
    Sect<16.7.4.4>

The Dynamic Language Binding doesn't currently support:

* Access from Java applications

* Content-Subscription Profile features (Content-Filtered Topics, Multi Topics, Query Conditions)

* XCDRv1 Data Representation

* Constructing types at runtime

.. _xtypes--unimplemented-features:

**********************
Unimplemented Features
**********************

..
    Sect<16.8>

OpenDDS implements the XTypes specification version 1.3 at the Basic Conformance level, with a partial implementation of the Dynamic Language Binding (supported features of which are described in section :ref:`xtypes--dynamic-language-binding-1`).
Specific unimplemented features listed below.
The two optional profiles, XTypes 1.1 Interoperability (XCDR1) and XML, are not implemented.

.. _xtypes--type-system:

Type System
===========

..
    Sect<16.8.1>

* IDL map type

* IDL bitmask type

* .. _xtypes--refheading-toc32438-4273764768:

  Struct and union inheritance

.. _xtypes--annotations:

Annotations
===========

..
    Sect<16.8.2>

IDL4 defines many standardized annotations and XTypes uses some of them.
The Annotations recognized by XTypes are in Table 21 in XTypes 1.3.
Of those listed in that table, the following are not supported in OpenDDS.
They are listed in groups defined by the rows of that table.
Some annotations in that table, and not listed here, can only be used with new capabilities of the Type System (see :ref:`xtypes--type-system`).

* Struct members

  * ``@optional``

  * ``@must_understand``

  * ``@non_serialized``

* Struct or union members

  * ``@external``

* Enums

  * ``@bit_bound``

  * ``@default_literal``

  * ``@value``

* ``@verbatim``

* ``@data_representation``

  * See :ref:`xtypes--standard-data-representation` for details.

.. _xtypes--differences-from-the-specification:

**********************************
Differences from the specification
**********************************

..
    Sect<16.9>

Spec issues tracked in OMG's Jira database can be viewed at https://issues.omg.org/issues/lists/dds-xtypes-rtf

* Inconsistent topic status isn’t set for reader/reader or writer/writer in non-XTypes use cases

* DDSXTY14-29: Define the encoding and extensibility used by Type Lookup Service

* DDSXTY14-33: Enums must have the same "bit bound" to be assignable

* DDSXTY14-27: Default data representation is XCDR2

* DDSSEC12-86: Type Lookup Service when using DDS Security

* DDSXTY14-35: Anonymous types in Strongly Connected Components

* DDSXTY14-40: Meaning of ignore_member_names in TypeConsistencyEnforcement

