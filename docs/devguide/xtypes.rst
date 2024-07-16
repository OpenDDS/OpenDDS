.. _xtypes:

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

The :ref:`DDS specification <spec-dds>` defines a way to build distributed applications using a data-centric publish and subscribe model.
In this model, publishing and subscribing applications communicate via Topics and each Topic has a data type.
An assumption built into this model is that all applications agree on data type definitions for each Topic that they use.
This assumption is not practical as systems must be able to evolve while remaining compatible and interoperable.

The :ref:`spec-xtypes` specification loosens the requirement on applications to have a common notion of data types.
Using XTypes, the application developer adds IDL annotations that indicate where the types may vary between publisher and subscriber and how those variations are handled by the middleware.

OpenDDS implements the XTypes specification at the Basic Conformance level, with a partial implementation of the Dynamic Language Binding.
Some features described by the specification are not yet implemented in OpenDDS - those are noted in :ref:`xtypes--unimplemented-features`.
This includes :ref:`IDL annotations <xtypes--annotations>` that are not yet implemented.
See :ref:`xtypes--differences-from-the-specification` for situations where the implementation of XTypes in OpenDDS departs from or infers something about the specification.
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

There are 3 kinds of extensibility for types: appendable, mutable, and final.
They allow IDL authors to define how type can or can not evolve in a backwards and forwards compatible manner.
Structs, unions, and enums are the only types which can use any of the extensibilities.
Appendable is the default extensibility.
The default extensibility for structs and unions can be changed with :option:`opendds_idl --default-extensibility`.

.. note::

  The default extensibility for enums is "appendable" and is not governed by ``--default-extensibility``.
  ``TypeObject``\s for received enums that do not set any flags are treated as a wildcard.

.. _xtypes--appendable:

Appendable
----------

Appendable denotes a constructed type which may have additional members added onto or removed from the end, but not both at the same time.
A type can be explicitly marked as appendable with the :ref:`@appendable <xtypes--anno-appendable>` annotation.
More information can be found in :ref:`xtypes--appendable-extensibility`.

.. _xtypes--mutable:

Mutable
-------

Mutable denotes a constructed type that allows for members to be added, removed, and reordered so long as the keys and the required members of the sender and receiver remain.
Mutable extensibility is accomplished by assigning a stable identifier to each member.
A type can be marked as mutable with the :ref:`@mutable <xtypes--anno-mutable>` annotation.
More information can be found in :ref:`xtypes--mutable-extensibility`.

.. _xtypes--final:

Final
-----

Final denotes a constructed type that can not add, remove, or reorder members.
This can be considered a non-extensible constructed type, with behavior similar to that of a type created before XTypes.
A type can be marked as final with the :ref:`@final <xtypes--anno-final>` annotation.
More information can be found in :ref:`xtypes--final-extensibility`.

.. _xtypes--assignability:

Assignability
=============

..
    Sect<16.2.2>

Assignability describes the ability of values of one type to be coerced to values of a possibility different type.

Assignability between the type of a writer and reader is checked as part of discovery.
If the types are assignable but not identical, then the :ref:`"try construct" <xtypes--try-construct>` mechanism will be used to coerce values of the writer's type to values of the reader's type.

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

* Types cannot use mutable extensibility

* Data Writers must have their :ref:`qos-data-representation` policy set to ``DDS::XCDR_DATA_REPRESENTATION``

* Data Readers must include ``DDS::XCDR_DATA_REPRESENTATION`` in the list of data representations in their Data Representation QoS (this is the case by default)

:ref:`xtypes--data-representation` shows how to change the data representation.
:ref:`xtypes--xcdr1-support` details XCDR1 support.

.. _xtypes--dynamic-language-binding:

Dynamic Language Binding
========================

..
    Sect<16.2.4>

Before the XTypes specification, all DDS applications worked by mapping the topic's data type directly into the programming language and having the data handling APIs such as read, write, and take, all defined in terms of that type.
As an example, :term:`topic type` ``A`` (an IDL structure) generates code generation of IDL interfaces ``ADataWriter`` and ``ADataReader`` while topic type ``B`` generated IDL interfaces ``BDataWriter`` and ``BDataReader``.
If an application attempted to pass an object of type ``A`` to the ``BDataWriter``, a compile-time error would occur (at least for statically typed languages including C++ and Java).
Advantages to this design include efficiency and static type safety, however, the code generation required by this approach is not desirable for every DDS application.

The XTypes Dynamic Language Binding defines a generic data container ``DynamicData`` and the interfaces ``DynamicDataWriter`` and ``DynamicDataReader``.
Applications can create instances of ``DynamicDataWriter`` and ``DynamicDataReader`` that work with various topics in the domain without needing to incorporate the generated code for those topic types.
The system is still type safe but the type checks occur at runtime instead of at compile time.
The Dynamic Language Binding is described in detail in :ref:`xtypes--dynamic-language-binding-1`.

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

``SEQUENTIAL`` causes ids to be assigned based on the position in the type.
``HASH`` causes ids to be computed by hashing the name of the member.
If no ``@autoid`` annotation is specified, the policy is ``SEQUENTIAL``.

Suppose that Version 3 was used in the initial deployment of the weather stations and the decision was made to switch to ``@autoid(HASH)`` when adding the new fields for wind speed and direction.
In this case, the ids of the pre-existing members can be set with ``@id``:

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

See the :ref:`xtypes--member-id-assignment` for more details.

.. _xtypes--appendable-extensibility:

Appendable Extensibility
========================

..
    Sect<16.3.4>

Mutable extensibility requires a certain amount of overhead both in terms of processing and network traffic.
A more efficient but less flexible form of extensibility is :ref:`xtypes--appendable`.
It's the default, but can be explictly declared using the :ref:`xtypes--anno-appendable` annotation.
Appendable is limited in that members can only be added to or removed from the end of the type.
With appendable, the initial version of the weather station IDL would be:

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

As with mutable, when a Version 7 Writer interacts with a Version 6 Reader, the additional fields will be ignored by the reader.
When a Version 6 Writer interacts with a Version 7 Reader, the additional fields will be initialized to default values based on Table 9 of the XTypes specification.

.. _xtypes--final-extensibility:

Final Extensibility
===================

..
    Sect<16.3.5>

The third kind of extensibility is final.
Annotating a type with :ref:`xtypes--anno-final` means that it will not be compatible with (assignable to/from) a type that is structurally different.
The ``@final`` annotation can be used to define types for pre-XTypes compatibility or in situations where the overhead of mutable or appendable is unacceptable.

.. _xtypes--try-construct:

Try Construct
=============

..
    Sect<16.3.6>

From a reader's perspective, there are three possible scenarios when attempting to initialize a member.
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
Fields marked as ``@try_construct(USE_DEFAULT)`` will receive a default value if value construction fails.
In the previous example, this means the reader would receive an empty string for the station ID if it exceeds 8 characters.
Fields marked as ``@try_construct(DISCARD)`` cause the entire sample to be discarded.
In the previous example, the Version 1 reader will never see a sample from a Version 2 writer where the original station ID contains more than 8 characters.
``@try_construct(DISCARD)`` is the default behavior.

.. _xtypes--data-representation:

*******************
Data Representation
*******************

..
    Sect<16.4>

Data representation is the way a data sample can be encoded for transmission.

The possible data representations are:

XML
    This isn't currently supported.

    The ``DataRepresentationId_t`` value is ``DDS::XML_DATA_REPRESENTATION``

    The annotation is :ref:`xtypes--anno-opendds-data-representation-xml`.

XCDR1
    This is the pre-XTypes standard CDR extended with XTypes features.
    Support is limited to non-XTypes features, see :ref:`xtypes--xcdr1-support` for details.

    The ``DataRepresentationId_t`` value is ``DDS::XCDR_DATA_REPRESENTATION``

    The annotation is :ref:`xtypes--anno-opendds-data-representation-xcdr1`.

XCDR2
    This is default for writers when using the :ref:`rtps-udp-transport` and should be preferred in most cases.
    It is a more robust and efficient version of XCDR1.

    The ``DataRepresentationId_t`` value is ``DDS::XCDR2_DATA_REPRESENTATION``

    The annotation is :ref:`xtypes--anno-opendds-data-representation-xcdr2`.

Unaligned CDR
    This is an OpenDDS-specific encoding that is the default for writers using only non-RTPS-UDP transports.
    It can't be used by a DataWriter using the :ref:`rtps-udp-transport`.

    The ``DataRepresentationId_t`` value is ``OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION``

    The annotation is :ref:`xtypes--anno-opendds-data-representation-unaligned-cdr`.

Use :ref:`qos-data-representation` to define what representations writers and readers should use.
Writers can only encode samples using only one data representation, but readers can accept multiple data representations.
:ref:`@OpenDDS::data_representation <xtypes--specifying-allowed-data-representations>` can be used to restrict what data representation can be used for a topic type in IDL.

.. warning::

  Because writers default to XCDR2 instead of XCDR1, they aren't likely to be compatible with readers from OpenDDS versions before 3.16 and other DDS implementations by default.
  Either the remote readers will have to set to use XCDR2 if they support it or OpenDDS writers will have to be set to use XCDR1.

  The example below shows a possible configuration for an XCDR1 DataWriter.

  .. code-block:: cpp

    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.representation.value.length(1);
    qos.representation.value[0] = DDS::XCDR_DATA_REPRESENTATION;
    DDS::DataWriter_var dw = pub->create_datawriter(topic, qos, 0, 0);

  Note that the IDL constant used for XCDR1 is ``XCDR_DATA_REPRESENTATION`` (without the digit).

.. _xtypes--type-consistency-enforcement:

****************************
Type Consistency Enforcement
****************************

..
    Sect<16.5>

When a reader/writer match is happening, type consistency enforcement checks that the two types are compatible according to the type objects if they are available.
This check will not happen if OpenDDS has been :ref:`configured not to generate or use type objects <xtypes--representing-types-with-typeobject-and-dynamictype>` or if the remote DDS doesn't support type objects.
Some parts of the compatibility check can be controlled on the reader side using :ref:`qos-type-consistency-enforcement`.
The full type object compatibility check is too detailed to reproduce here.
It can be found in :omgspec:`xtypes:7.2.4`.
In general though two topic types and their nested types are compatible if:

* Extensibilities of shared types match
* Extensibility rules haven't been broken, for example:

  * Changing a ``@final`` struct
  * Adding a member in the middle of an ``@appendable`` struct

* Length bounds of strings and sequences are the same or greater
* Lengths of arrays are exactly the same
* The keys of the types match exactly
* Shared member IDs match when required, like when they are final or are being used as keys

If the type objects are compatible then the match goes ahead.
If one or both type objects are not available, then OpenDDS falls back to checking the names each entity's ``TypeSupport`` was given.
This is the name passed to the ``register_type`` method of a ``TypeSupport`` object or if that string is empty then the name of the topic type in IDL.

An interesting side effect of these rules is when type objects are always available, then the topic type names passed to ``register_type`` are only used within that process.
This means they can be changed and remote readers and writers will still match, assuming the new name is used consistently within the process and the types are still compatible.

.. _xtypes--idl-annotations:

***************
IDL Annotations
***************

..
    Sect<16.6>

.. _xtypes--indicating-which-types-can-be-topic-types:

Indicating Which Types Can Be Topic Types
=========================================

..
    Sect<16.6.1>

.. _xtypes--anno-topic:

``@topic``
----------

..
    Sect<16.6.1.1>

Applies To: struct or union type declarations

Aliases: :ref:`@nested(FALSE) <xtypes--anno-nested>`

The topic annotation marks a topic type for samples to be transmitted from a publisher or received by a subscriber.
A topic type may contain other topic and non-topic types as members.
See :ref:`getting_started--defining-data-types-with-idl` for more details.

.. _xtypes--anno-nested:

``@nested(<boolean>)``
----------------------

..
    Sect<16.6.1.2>

Applies To: struct or union type declarations

Aliases: :ref:`xtypes--anno-topic` is the same as ``@nested(FALSE)``

The ``@nested`` annotation marks a type that will always be contained within another.
``@nested`` or ``@nested(TRUE)`` is the inverse of :ref:`xtypes--anno-topic`.
This can be used to prevent a type from being used as in a topic.
One reason to do so is to reduce the amount of code generated for that type.

.. _xtypes--anno-default-nested:

``@default_nested(<boolean>)``
------------------------------

..
    Sect<16.6.1.3>

Applies To: modules

The ``@default_nested(TRUE)`` or ``@default_nested(FALSE)`` sets the default nesting behavior for a module.
Types within a module marked with ``@default_nested`` can still set their own behavior with :ref:`xtypes--anno-topic` or :ref:`xtypes--anno-nested`.
The default is controlled using :option:`opendds_idl --default-nested` and :option:`opendds_idl --no-default-nested`.

.. _xtypes--specifying-allowed-data-representations:

Specifying Allowed Data Representations
=======================================

..
    Sect<16.6.2>

If there are ``@OpenDDS::data_representation`` annotations are on the topic type, then the representations are limited to ones the specified in the annotations, otherwise all representations are allowed.
Trying to create a reader or writer with the disallowed representations will result in an error.
See :ref:`xtypes--data-representation` for more information.

.. _xtypes--anno-opendds-data-representation-xml:

``@OpenDDS::data_representation(XML)``
--------------------------------------

..
    Sect<16.6.2.1>

Applies To: topic types

Limitations: XML is not currently supported

.. _xtypes--anno-opendds-data-representation-xcdr1:

``@OpenDDS::data_representation(XCDR1)``
----------------------------------------

..
    Sect<16.6.2.2>

Applies To: topic types

Limitations: XCDR1 doesn't support XTypes features
See :ref:`xtypes--data-representation` for details

.. _xtypes--anno-opendds-data-representation-xcdr2:

``@OpenDDS::data_representation(XCDR2)``
----------------------------------------

..
    Sect<16.6.2.3>

Applies To: topic types

XCDR2 is currently the recommended data representation for most cases.

.. _xtypes--anno-opendds-data-representation-unaligned-cdr:

``@OpenDDS::data_representation(UNALIGNED_CDR)``
------------------------------------------------

Applies To: topic types

Limitations: OpenDDS specific, can't be used with RTPS-UDP, and doesn't support XTypes features
See :ref:`xtypes--data-representation` for details

.. _xtypes--anno-data-representation:

Standard ``@data_representation``
---------------------------------

..
    Sect<16.6.2.4>

``tao_idl`` doesn't support ``bitset``, which the standard ``@data_representation`` requires.
Instead use ``@OpenDDS::data_representation`` which is similar, but doesn't support bitmask value chaining like ``@data_representation(XCDR|XCDR2)``.
The equivalent would require two separate annotations:

.. code-block:: omg-idl

  @OpenDDS::data_representation(XCDR1)
  @OpenDDS::data_representation(XCDR2)

.. _xtypes--determining-extensibility:

Determining Extensibility
=========================

..
    Sect<16.6.3>

The extensibility annotations can explicitly define the :ref:`extensibility <xtypes--extensibility>` of a type.
If no extensibility annotation is used, then the type will have the default extensibility.
This will be ``appendable`` unless the :option:`opendds_idl --default-extensibility` is used to override the default.

.. _xtypes--anno-mutable:

``@mutable``
------------

..
    Sect<16.6.3.1>

Alias: ``@extensibility(MUTABLE)``

Applies To: structures, unions, and enums

Declares a type as :ref:`xtypes--mutable`.

.. _xtypes--anno-appendable:

``@appendable``
---------------

..
    Sect<16.6.3.2>

Alias: ``@extensibility(APPENDABLE)``

Applies To: structures, unions, and enums

Declares a type as :ref:`xtypes--appendable`.

.. _xtypes--anno-final:

``@final``
----------

..
    Sect<16.6.3.3>

Alias: ``@extensibility(FINAL)``

Applies To: structures, unions, and enums

Declares a type as :ref:`xtypes--final`.

.. _xtypes--customizing-xtypes-per-member:

Customizing XTypes Per-Member
=============================

..
    Sect<16.6.4>

Try Construct annotations dictate how members of one object should be converted from members of a different but assignable object.
If no try construct annotation is added, it will default to discard.

.. _xtypes--anno-try-construct-use-default:

``@try_construct(USE_DEFAULT)``
-------------------------------

..
    Sect<16.6.4.1>

Applies to: structure and union members, sequence and array elements

The use_default try construct annotation will set the member whose deserialization failed to a default value which is determined by the XTypes specification.
Sequences will be of length 0, with the same type as the original sequence.
Primitives will be set equal to 0.
Strings will be replaced with the empty string.
Arrays will be of the same length but have each element set to the default value.
Enums will be set to the first enumerator defined.

.. _xtypes--anno-try-construct-trim:

``@try_construct(TRIM)``
------------------------

..
    Sect<16.6.4.2>

Applies to: structure and union members, sequence and array elements

The trim try construct annotation will, if possible, shorten a received value to one fitting the receiver's bound.
As such, trim only makes logical sense on bounded strings and bounded sequences.

.. _xtypes--anno-try-construct-discard:

``@try_construct(DISCARD)``
---------------------------

..
    Sect<16.6.4.3>

Applies to: structure and union members, sequence and array elements

The discard try construct annotation will "throw away" the sample if an element fails to deserialize.

.. _xtypes--member-id-assignment:

Member ID Assignment
====================

..
    Sect<16.6.5>

If no explicit id annotation is used, then member IDs will automatically be assigned sequentially.

.. _xtypes--anno-id:

``@id(<value>)``
----------------

..
    Sect<16.6.5.1>

Applies to: structure and union members

``<value>`` is an unsigned 32-bit integer which assigns that member's ID.

.. _xtypes--anno-autoid:

``@autoid(<value>)``
--------------------

..
    Sect<16.6.5.2>

Applies to: module declarations, structure declarations, union declarations

The autoid annotation can take two values, ``HASH`` or ``SEQUENTIAL``.
``SEQUENTIAL`` states that the identifier shall be computed by incrementing the preceding one.
``HASH`` states that the identifier should be calculated with a hashing algorithm - the input to this hash is the member's name.
``HASH`` is the default value of ``@autoid``.

.. _xtypes--anno-hashid:

``@hashid(<value>)``
--------------------

..
    Sect<16.6.5.3>

Applies to: structure and union members

The ``@hashid`` sets the identifier to the hash of the ``<value>`` parameter, if one is specified.
If the ``<value>`` parameter is omitted or is the empty string, the member's name is used as if it was the ``<value>``.

.. _xtypes--determining-the-key-fields-of-a-type:

Determining the Key Fields of a Type
====================================

..
    Sect<16.6.6>

.. _xtypes--anno-key:

``@key``
--------

..
    Sect<16.6.6.1>

Applies to: structure members, union discriminator

The ``@key`` annotation marks a member used to determine the Instances of a topic type.
See :ref:`getting_started--keys` for more details on the general concept of a Key.
For XTypes specifically, two types can only be compatible if each contains the members that are keys within the other.

Customizing the Values of Enumerators
=====================================


.. _xtypes--anno-value:

``@value(<v>)``
---------------

..
    Sect<16.6.7.1>

Applies to: enumerators (only when :ref:`opendds_idl--using-the-idl-to-c-11-mapping`)

Without annotations, the enumerators of each enum type take on consecutive integer values starting at 0.
The ``@value(<v>)`` annotation customizes the integer value of an individual enumerator. The parameter ``<v>`` is an integer constant (signed, 4 bytes wide).
Any enumerators that are not annotated take on the integer value one higher than the value of the previously-declared enumerator, with the first declared taking value 0.

.. code-block:: omg-idl

  enum MyAnnotionEnabledEnum {
    ZERO,
    @value(3) THREE,
    FOUR,
    @value(2) TWO
  };


Marking member as optional
==========================

.. _xtypes--optional:

``@optional``
-------------

Applies to: struct members (only when :ref:`opendds_idl--using-the-idl-to-c-11-mapping`)

This annotation marks a member as optional. An optional member doesn't require a value to be set, which means it can be skipped when publishing to the topic.
In the IDL-to-C++11 mapping it generates either a ``std::optional<T>`` or an equivalent custom implementation.

.. code-block:: omg-idl

    struct Foo {
        @optional string name;
    };

.. code-block:: cpp

    Foo foo;
    if (foo.name().has_value()) {
        std::cout << foo.name().value() << std::endl;
    } else {
        foo.name("bar");
    }

More information about other language bindings for ``@optional`` can be found `here <https://github.com/OpenDDS/OpenDDS/wiki/Proposal:-@optional-support>`__

.. _xtypes--dynamic-language-binding-1:

************************
Dynamic Language Binding
************************

..
    Sect<16.7>

For an overview of the Dynamic Language Binding, see :ref:`xtypes--dynamic-language-binding`.
This section describes the features of the Dynamic Language Binding that OpenDDS supports.

There are two main usage patterns supported:

* Applications can receive DynamicData from a :ref:`Recorder object <alternate_interfaces_to_data--recorder-and-replayer>`.

* Applications can use XTypes :ref:`DynamicDataWriter and/or DynamicDataReader <xtypes--dynamicdatawriters-and-dynamicdatareaders>`.

To use DynamicDataWriter and/or DynamicDataReader for a given topic, the data type definition for that topic must be available to the local DomainParticipant.
There are a few ways this can be achieved, see :ref:`xtypes--obtaining-dynamictype-and-registering-typesupport` for details.

.. _xtypes--representing-types-with-typeobject-and-dynamictype:

Representing Types with TypeObject and DynamicType
==================================================

..
    Sect<16.7.1>

In XTypes, the types of the peers may not be identical, as in the case of appendable or mutable extensibility.
In order for a peer to be aware of its remote peer's type, there must be a way for the remote peer to communicate its type.
TypeObject is an alternative to IDL for representing types, and one of the purposes of TypeObject is to communicate the peers' types.

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

To enable use of ``CompleteTypeObject``\s needed for the dynamic binding, they must be generated and OpenDDS must be configured to use them.
To generate them, use :option:`opendds_idl -Gxtypes-complete`.
For MPC, this can be done by adding this to the opendds_idl arguments for idl files in the project, like this:

.. code-block:: mpc

    TypeSupport_Files {
      dcps_ts_flags += -Gxtypes-complete
      Messenger.idl
    }

To do the same for CMake:

.. code-block:: cmake

    opendds_target_sources(target
      Messenger.idl
      OPENDDS_IDL_OPTIONS -Gxtypes-complete
    )

Once set up to be generated, OpenDDS has to be configured to send and receive the ``CompleteTypeObject``\s.
This can be done by setting :cfg:prop:`[rtps_discovery]UseXTypes` or programmatically using the ``OpenDDS::RTPS::RtpsDiscovery::use_xtypes()`` setter methods.

.. _xtypes--interpreting-data-samples-with-dynamicdata:

Interpreting Data Samples with DynamicData
==========================================

..
    Sect<16.7.2>

Together with DynamicType, DynamicData allows users to interpret a received data sample and read individual fields from it.
Each DynamicData object is associated with a type, represented by a DynamicType object, and the data corresponding to an instance of that type.
Consider the following example:

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
      @id(7) long mdim_field[2][3];
    };

The samples for ``MyStruct`` are written by a normal, statically-typed DataWriter.
The writer application needs to have the IDL-generated code including the "complete" form of TypeObjects.
Use :option:`opendds_idl -Gxtypes-complete` to enable CompleteTypeObjects since the default is to generate MinimalTypeObjects.

One way to obtain a DynamicData object representing a data sample received by the participant is using the :ref:`Recorder and RecorderListener classes <alternate_interfaces_to_data--recorder-and-replayer>`.
Recorder's ``get_dynamic_data`` can be used to construct a DynamicData object for each received sample from the writer.
Internally, the CompleteTypeObjects received from discovering that writer are converted to DynamicTypes and they are then used to construct the DynamicData objects.
Once a DynamicData object for a ``MyStruct`` sample is constructed, its members can be read as described in the following sections.
Another way to obtain a DynamicData object is from a :ref:`DynamicDataReader <xtypes--creating-and-using-a-dynamicdatareader>`.

.. _xtypes--reading-basic-types:

Reading Basic Types
-------------------

..
    Sect<16.7.2.1>

DynamicData provides methods for reading members whose types are basic such as integers, floating point numbers, characters, boolean.
See the XTypes specification for a complete list of basic types for which DynamicData provides an interface.
To call a correct method for reading a member, we need to know the type of the member as well as its id.
For our example, we first want to get the number of members that the sample contains.
In these examples, the ``data`` object is an instance of DynamicData.

.. code-block:: cpp

    DDS::UInt32 count = data.get_item_count();

Then, each member's id can be read with ``get_member_id_at_index``.
The input for this function is the index of the member in the sample, which can take a value from ``0`` to ``count - 1``.

.. code-block:: cpp

    XTypes::MemberId id = data.get_member_id_at_index(0);

The ``MemberDescriptor`` for the corresponding member then can be obtained as follows.

.. code-block:: cpp

    XTypes::MemberDescriptor md;
    DDS::ReturnCode_t ret = data.get_descriptor(md, id);

The returned ``MemberDescriptor`` allows us to know the type of the member.
Suppose id is 1, meaning that the member at index 0 is ``l_field``, we now can get its value.

.. code-block:: cpp

    DDS::Int32 int32_value;
    ret = data.get_int32_value(int32_value, id);

After the call, ``int32_value`` contains the value of the member ``l_field`` from the sample.
The method returns ``DDS::RETCODE_OK`` if successful.
In case the type has an optional member and it is not present in the DynamicData instance, ``DDS::RETCODE_NO_DATA`` is returned.

Similarly, suppose we have already found out the types and ids of the members ``us_field`` and ``f_field``, their values can be read as follows.

.. code-block:: cpp

    DDS::UInt16 uint16_value;
    ret = data.get_uint16_value(uint16_value, 2); // Get the value of us_field
    DDS::Float32 float32_value;
    ret = data.get_float32_value(float32_value, 3); // Get the value of f_field

Reading members from union is a little different as there is at most one active branch at any time.
In general, DynamicData in OpenDDS follows the IDL-to-C++ mappings for union.
Consider the following union as an example.

.. code-block:: omg-idl

    @mutable
    union MyUnion switch (short) {
    case 1:
    case 2:
      @id(1) short s_field;
    case 3:
      @id(2) long l_field;
    case 4:
      @id(3) string str_field;
    };

The discriminator can be read using the appropriate method for the discriminator type and id ``XTypes::DISCRIMINATOR_ID`` (see :ghfile:`dds/DCPS/XTypes/TypeObject.h`).

.. code-block:: cpp

    DDS::Int32 disc_value;
    ret = data.get_int16_value(disc_val, XTypes::DISCRIMINATOR_ID);

Using the value of the discriminator, user can decide which branch is activated and read its value in a similar way as reading a struct member.
Reading a branch that is not activated returns ``DDS::RETCODE_PRECONDITION_NOT_MET``.

At any time, a DynamicData instance of a union represents a valid state of that union.
A special case is an empty DynamicData instance.
In this case, the discriminator takes the default value of the discriminator type (the XTypes specification specifies the default value for each type).
If that discriminator value selects a branch, the selected branch also takes the default value corresponding to its type.
If it doesn't select a branch, the union contains only the discriminator.

.. _xtypes--reading-collections-of-basic-types:

Reading Collections of Basic Types
----------------------------------

..
    Sect<16.7.2.2>

Besides a list of methods for getting values of members of basic types, DynamicData also defines methods for reading sequence members.
In particular, for each method that reads value from a basic type, there is a counterpart that reads a sequence of the same basic type.
For instance, ``get_int32_value`` reads the value from a member of type ``int32``/``long``, and ``get_int32_values`` reads the value from a member of type ``sequence<int32>``.
For the member ``ul_seq_field`` in our example, its value can be read as follows.

.. code-block:: cpp

    DDS::UInt32Seq my_ul_seq;
    ret = data.get_uint32_values(my_ul_seq, id); // id is 5

Because ``ul_seq_field`` is a sequence of unsigned 32-bit integers, the ``get_uint32_values`` method is used.
Again, the second argument is the id of the requested member, which is 5 for ``ul_seq_field``.
When successful, ``my_ul_seq`` contains values of all elements of the member ``ul_seq_field`` in the sample.

To get the values of the array member ``d_field``, we first need to create a separate DynamicData object for it, and then read individual elements of the array using the new DynamicData object.

.. code-block:: cpp

    DDS::DynamicData_var array_data;
    DDS::ReturnCode_t ret = data.get_complex_value(array_data, id); // id is 6

    const DDS::UInt32 num_items = array_data->get_item_count();
    for (DDS::UInt32 i = 0; i < num_items; ++i) {
      const XTypes::MemberId my_id = array_data->get_member_id_at_index(i);
      DDS::Float64 my_double;
      ret = array_data->get_float64_value(my_double, my_id);
    }

In the example code above, ``get_item_count`` returns the number of elements of the array.
Inside the for loop, the index of each element is converted to an id within the array using ``get_member_id_at_index``.
Then, this id is used to read the element's value into ``my_double``.
Note that the second parameter of the interfaces provided by DynamicData must be the id of the requested member.
In case of collection, elements are considered members of the collection.
However, the collection element doesn't have a member id.
And thus, we need to convert its index into an id before calling a ``get_*_value`` (or ``get_*_values``) method.

Accessing a multi-dimensional array is a little different as ``get_member_id_at_index`` accepts a single index as its sole argument.
OpenDDS provides function ``flat_index`` to convert an index to a multi-dimensional array to a flat index that can then be passed to ``get_member_id_at_index``.

.. code-block:: cpp

    DDS::DynamicData_var mdim_arr_data;
    DDS::ReturnCode_t ret = data.get_complex_value(mdim_arr_data, id); // id is 7
    DDS::DynamicType_var mdim_type = mdim_arr_data->type();
    DDS::TypeDescriptor_var mdim_td;
    ret = mdim_type->get_descriptor(mdim_td);
    const DDS::BoundSeq& bound = mdim_td->bound();

    DDS::UInt32Seq index_vec;
    index_vec.length(2);
    for (DDS::UInt32 i = 0; i < bound[0]; ++i) {
        index_vec[0] = i;
        for (DDS::UInt32 j = 0; j < bound[1]; ++j) {
            index_vec[1] = j;
            DDS::UInt32 flat_idx;
            ret = OpenDDS::XTypes::flat_index(flat_idx, index_vec, bound);
            const XTypes::MemberId id = mdim_arr_data->get_member_id_at_index(flat_idx);
            DDS::Int32 my_long;
            ret = mdim_arr_data->get_int32_value(my_long, id);
        }
    }

``flat_index`` takes as input an index sequence to the multi-dimensional array and the dimensions of the array and returns a flat index.
The same function is used when serializing the dynamic data object to make sure the mapping from index to id is consistent and conforms to the XTypes spec regarding the order of the elements.

.. _xtypes--reading-members-of-more-complex-types:

Reading Members of More Complex Types
-------------------------------------

..
    Sect<16.7.2.3>

For a more complex member such as a nested structure or union, the discussed DynamicData methods are not suitable.
And thus, users first need to get a new DynamicData object that represents the sole data of the member with ``get_complex_value``.
This new DynamicData object can then be used to get the values of the inner members of the nested member.
For example, a DynamicData object for the ``nested_field`` member of the ``MyStruct`` sample can be obtained as follows.

.. code-block:: cpp

    DDS::DynamicData_var nested_data;
    DDS::ReturnCode_t ret = data.get_complex_value(nested_data, id); // id is 4

Recall that ``nested_field`` has type ``NestedStruct`` which has one member ``s_field`` with id 1.
Now the value of ``s_field`` can be read from ``nested_data`` using ``get_int16_value``, since ``s_field`` has type ``short``.

.. code-block:: cpp

    DDS::Int16 my_short;
    ret = nested_data->get_int16_value(my_short, id); // id is 1

The ``get_complex_value`` method is also suitable for any other cases where the value of a member cannot be read directly using the ``get_*_value`` or ``get_*_values`` methods.
As an example, suppose we have a struct ``MyStruct2`` defined as follows.

.. code-block:: omg-idl

    @appendable
    struct MyStruct2 {
      @id(1) sequence<NestedStruct> seq_field;
    };

And suppose we already have a DynamicData object, called ``data``, that represents a sample of ``MyStruct2``.
To read the individual elements of ``seq_field``, we first get a new DynamicData object for the ``seq_field`` member.

.. code-block:: cpp

    DDS::DynamicData_var seq_data;
    DDS::ReturnCode_t ret = data.get_complex_value(seq_data, id); // id is 1

Since the elements of ``seq_field`` are structures, for each of them we create another new DynamicData object to represent it, which can be used to read its member.

.. code-block:: cpp

    const DDS::UInt32 num_elems = seq_data->get_item_count();
    for (DDS::UInt32 i = 0; i < num_elems; ++i) {
      const XTypes::MemberId my_id = seq_data->get_member_id_at_index(i);
      DDS::DynamicData_var elem_data; // Represent each element.
      ret = seq_data->get_complex_value(elem_data, my_id);
      DDS::Int16 my_short;
      ret = elem_data->get_int16_value(my_short, 1);
    }

.. _xtypes--populating-data-samples-with-dynamicdata:

Populating Data Samples With DynamicData
========================================

..
    Sect<16.7.3>

DynamicData objects can be created by the application and populated with data so that they can be used as data samples which are written to a :ref:`DynamicDataWriter <xtypes--creating-and-using-a-dynamicdatawriter-or-dynamicdatareader>`.

To create a DynamicData object, use the DynamicDataFactory API defined by the XTypes spec:

.. code-block:: cpp

    DDS::DynamicData_var dynamic =
      DDS::DynamicDataFactory::get_instance()->create_data(type);

Like other data types defined by IDL interfaces (for example, the ``*TypeSupportImpl`` types), the "dynamic" object's lifetime is managed with a smart pointer - in this case ``DDS::DynamicData_var``.

The ``type`` argument to ``create_data()`` is an object that implements the ``DDS::DynamicType`` interface.
The DynamicType representation of any type that's supported as a topic data type is available from its corresponding :ref:`TypeSupport object <xtypes--obtaining-dynamictype-and-registering-typesupport>` using the ``get_type()`` operation.
Once the application has access to that top-level type, the DynamicType interface can be used to obtain complete information about the type including nested and referenced data types.
See the file :ghfile:`dds/DdsDynamicData.idl` in OpenDDS for the definition of the DynamicType and related interfaces.

Once the application has created the DynamicData object, it can be populated with data members of any type.
The operations used for this include the DynamicData operations named ``set_*`` for the various data types.
They are analogous to the ``get_*`` operations that are described in :ref:`xtypes--interpreting-data-samples-with-dynamicdata`.
When populating the DynamicData of complex data types, use :ref:`get_complex_value <xtypes--reading-members-of-more-complex-types>` to navigate from DynamicData representing containing types to DynamicData representing contained types.

Setting the value of a member of a DynamicData union using a ``set_*`` method implicitly 1) activates the branch corresponding to the member and 2) sets the discriminator to a value corresponding to the active branch.
For example, the ``l_field`` member of ``MyUnion`` above can be set as follows:

.. code-block:: cpp

    DDS::Int32 l_field_value = 12;
    data.set_int32_value(id, l_field_value); // id is 2

The discriminator can also be set directly in the following two cases.
First, the new discriminator value selects the same branch that is currently activated.
Second, the new discriminator value selects no branch. In all other cases, ``DDS::RETCODE_PRECONDITION_NOT_MET`` is returned.
As an example for the first case, suppose the union currently has the discriminator value of 1 and the member ``s_field`` is active.
We can set the discriminator value to 2 as it selects the same member.

.. code-block:: cpp

    DDS::Int16 new_disc_value = 2;
    data.set_int16_value(XTypes::DISCRIMINATOR_ID, new_disc_value);

For the second case, setting the discriminator to any value that doesn't select a member will succeed. After that, the union contains only the discriminator.

.. code-block:: cpp

    DDS::Int16 new_disc_value = 5; // does not select any branch
    data.set_int16_value(XTypes::DISCRIMINATOR_ID, new_disc_value);

Unions start in an "empty" state as described in :ref:`xtypes--interpreting-data-samples-with-dynamicdata`.
Consequently, at the point of serialization, empty and non-empty unions are not differentiated.

Expandable collection types such as sequences or strings can be extended one element at a time.
To extend a sequence (or string), we first get the id of the new element at index equal to the current length of the sequence using the ``get_member_id_at_index`` operation.
The length of the sequence can be got using ``get_item_count``.
After we obtain the id, we can write the new element using the ``set_*`` operation as usual.

.. _xtypes--dynamicdatawriters-and-dynamicdatareaders:

DynamicDataWriters and DynamicDataReaders
=========================================

..
    Sect<16.7.4>

DynamicDataWriters and DynamicDataReaders are designed to work like any other DataWriter and DataReader except that their APIs are defined in terms of the DynamicData type instead of a type generated from IDL.
Each DataWriter and DataReader has an associated Topic and that Topic has a data type (represented by a TypeSupport object).
Behavior related to keys, QoS policies, discovery and built-in topics, DDS Security, and transport is not any different for a DynamicDataWriter or DynamicDataReader.
One exception is that in the current implementation, :ref:`content_subscription_profile--multi-topic` is not supported for DynamicDataWriters and DynamicDataReaders.

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
This can be done with the ACE cross-platform support library that OpenDDS itself uses, or using a platform-specific function like ``LoadLibrary`` or ``dlopen``.
The application code does not need to include any generated headers from this IDL.
This makes the type support library a true plug-in, meaning it can be loaded into an application that had no knowledge of it when that application was built.

Once the shared library is loaded, an internal singleton class in OpenDDS called ``Registered_Data_Types`` can be used to obtain a reference to the ``TypeSupport`` object.

.. code-block:: cpp

    DDS::TypeSupport_var ts_static = Registered_Data_Types->lookup(0, "TypeName");

This TypeSupport object ``ts_static`` is not registered with the DomainParticipant and is not set up for the Dynamic Language Binding.
But, crucially, it does have the DynamicType object that we'll need to set up a second TypeSupport object which is registered with the DomainParticipant.

.. code-block:: cpp

    DDS::DynamicType_var type = ts_static->get_type();
    DDS::DynamicTypeSupport_var ts_dynamic = new DynamicTypeSupport(type);
    DDS::ReturnCode_t ret = ts_dynamic->register_type(participant, "");

Now the type support object ``ts_dynamic`` can be used in the usual DataWriter/DataReader setup sequence (creating a Topic first, etc.) but the created DataWriters and DataReaders will be :ref:`DynamicDataWriters and DynamicDataReaders <xtypes--creating-and-using-a-dynamicdatawriter-or-dynamicdatareader>`.

The other approach to obtaining TypeSupport objects for use with the Dynamic Language Binding is to have DDS discovery's built-in endpoints get TypeObjects from remote domain participants.
To do this, use the ``get_dynamic_type`` method on the singleton ``Service_Participant`` object.

.. code-block:: cpp

    DDS::DynamicType_var type; // NOTE: passed by reference below
    DDS::ReturnCode_t ret = TheServiceParticipant->get_dynamic_type(type, participant, key);

The two input parameters to ``get_dynamic_type`` are the ``participant`` (an object reference to the DomainParticipant that will be used to register our TypeSupport and create Topics, DataWriters, and/or DataReders) and the ``key`` which is the ``DDS::BuiltinTopicKey_t`` that identifies the remote entity which has the data type that we'll use.
This key can be obtained from the :ref:`built_in_topics--dcpspublication-topic` or the :ref:`built_in_topics--dcpssubscription-topic`.

The type obtained from ``get_dynamic_type`` can be used to create and register a TypeSupport object.

.. code-block:: cpp

    DDS::DynamicTypeSupport_var ts_dynamic = new DynamicTypeSupport(type);
    DDS::ReturnCode_t ret = ts_dynamic->register_type(participant, "");

.. _xtypes--creating-and-using-a-dynamicdatawriter-or-dynamicdatareader:
.. _xtypes--creating-and-using-a-dynamicdatawriter:
.. _xtypes--creating-and-using-a-dynamicdatareader:

Creating and Using a DynamicDataWriter or DynamicDataReader
-----------------------------------------------------------

..
    This used to be section 16.7.4.2 and 16.7.4.3

Following the steps in :ref:`xtypes--obtaining-dynamictype-and-registering-typesupport`, a DynamicTypeSupport object is registered with the domain participant.
The type name used to register with the participant may be the default type name (used when an empty string is passed to the ``register_type`` operation), or some other type name.
If the default type name was used, the application can access that name by invoking the ``get_type_name`` operation on the TypeSupport object.

The registered type name is then used as one of the input parameters to ``create_topic``, just like when creating a topic for the Plain (non-Dynamic) Language Binding.
Once a Topic object exists, create a DataWriter or DataReader using this Topic.
They can be narrowed to the DynamicDataWriter or DynamicDataReader IDL interface:

.. code-block:: cpp

    DDS::DynamicDataWriter_var w = DDS::DynamicDataWriter::_narrow(writer);
    DDS::DynamicDataReader_var r = DDS::DynamicDataReader::_narrow(reader);

The IDL interfaces are defined in :ghfile:`dds/DdsDynamicTypeSupport.idl` in OpenDDS.
They provides the same operations as any other DataWriter or DataReader, but with DynamicData as their data type.
See :ref:`xtypes--populating-data-samples-with-dynamicdata` for details on creating DynamicData objects for use with the DynamicDataWriter interface.
See :ref:`xtypes--interpreting-data-samples-with-dynamicdata` for details on using DynamicData objects obtained from the DynamicDataReader interface.

.. _xtypes-dynamicdata-and-idl-generated-types:

``DynamicData`` and IDL-Generated Types
=======================================

``DynamicData`` allows writing code that is much more generic than using the IDL-generated types.
There are still reasons to use the IDL-generated types though.
They are more convenient to use and are more likely to be used in existing code.
You can still make use of the flexibility of ``DynamicData`` though by either interchanging with or wrapping IDL-generated types.

Interchanging Data using ``create_sample`` and ``create_dynamic_sample``
------------------------------------------------------------------------

.. versionadded:: 3.29.0

The XTypes specification defines ``create_sample`` and ``create_dynamic_sample`` on the ``TypeSupport`` for every :term:`topic type` to interchange ``DynamicData`` and IDL-generated types.
In addition to this, OpenDDS adds ``create_sample_rc`` and ``create_dynamic_sample_rc`` to indicate failure and provide a more consistent interface.
The standard methods will only warn if there was an error and can't indicate that an error occured.

``create_sample`` and ``create_sample_rc``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``create_sample`` takes a ``DynamicData`` and returns the IDL-generated type.
One thing to note about this method is the IDL-to-C++ specification requires that returns like this can be either returned on the stack or the heap depending on if they are :omgspec:`"variable-length" <cpp03:5.11 Mapping for Structured Types>`.
For example if there's a topic type ``Foo`` that doesn't contain a ``sequence`` or ``string`` anywhere inside it, then:

..
  TODO: Add map above and below when we support that, assuming it's also counted as variable length by tao_idl

.. code-block:: omg-idl

  struct Foo {
    int32 bar;
  };

.. code-block:: cpp

  FooTypeSupport_var type_support = new FooTypeSupportImpl();
  DynamicData_var data = /* ... */;

  Foo foo = type_support->create_sample(data);

If the type contains a ``sequence`` or ``string`` anywhere, then then it returns a pointer that's meant to go inside a ``_var`` class:

.. code-block:: omg-idl

  struct Foo {
    string bar;
  };

.. code-block:: cpp

  FooTypeSupport_var type_support = new FooTypeSupportImpl();
  DynamicData_var data = /* ... */;

  Foo foo = type_support->create_sample(data);

Using ``create_sample_rc`` is the same regardless of the type and returns a ``ReturnCode_t`` to indicate failure:

.. code-block:: cpp

  FooTypeSupport_var type_support = new FooTypeSupportImpl();
  DynamicData_var data = /* ... */;

  Foo foo;
  const DDS::ReturnCode_t rc = type_support->create_sample_rc(foo, data);
  if (rc != DDS::RETCODE_OK) /* (handle error) */;

``create_dynamic_sample`` and ``create_dynamic_sample_rc``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``create_dynamic_sample`` takes the IDL-generated type and returns a ``DynamicData``.

.. code-block:: cpp

  FooTypeSupport_var type_support = new FooTypeSupportImpl();
  Foo foo = /* ... */;

  DynamicData_var data = type_support->create_dynamic_sample(foo);

``create_dynamic_sample_rc`` returns a ``ReturnCode_t`` to indicate failure:

.. code-block:: cpp

  FooTypeSupport_var type_support = new FooTypeSupportImpl();
  Foo foo = /* ... */;

  DynamicData_var data;
  const DDS::ReturnCode_t rc = type_support->create_dynamic_sample_rc(data, foo);
  if (rc != DDS::RETCODE_OK) /* (handle error) */;

.. _xtypes-dynamic-data-adapter:

Wrapping Data using ``DynamicDataAdapter``
------------------------------------------

.. versionadded:: 3.29.0 (but unofficially present in its current form in 3.24.0)

``DynamicDataAdapter`` is an OpenDDS-specific implementation of ``DynamicData`` that wraps an IDL-generated type.
This allows direct interaction instead of copying all the data every time.
See :ghfile:`dds/DCPS/XTypes/DynamicDataAdapter.h` for a list of known issues and unimplemented features.

``DynamicDataAdapter`` requires the :ref:`building--content-subscription-profile` (enabled by default) and can be acquired using the ``DDS::DynamicData* OpenDDS::XTypes::get_dynamic_data_adapter<T>(DDS::DynamicType* type, T& value)`` function in the type's ``*TypeSupportImpl.h`` file.
Here is an example for a topic type called ``Foo``:

.. code-block:: cpp

  #include "FooTypeSupportImpl.h"

  // ...

  Foo foo;
  foo.bar = 10;

  FooTypeSupport_var type_support = new FooTypeSupportImpl();
  DDS::DynamicType_var type = type_support->get_type();

  DDS::DynamicData_var data = OpenDDS::XTypes::get_dynamic_data_adapter<Foo>(type, foo);

After this ``data`` can be read and the values will reflect those in ``foo``.

``DynamicDataAdapter`` has a reference to the IDL-generated type value and this has 3 consequences:

- If a const value or const reference is passed to ``get_dynamic_data_adapter``, ``DynamicDataAdapter`` will attempt to respect that and return ``DDS::RETCODE_ILLEGAL_OPERATION`` from the set methods and other methods that would change the data.
- If it's non-const, then those methods on ``DynamicDataAdapter`` will modify the IDL-generated type value.
- The IDL-generated type value must continue to exist while the ``DynamicDataAdapter`` is being used.
  The ``clone`` method can be used to get a copy of the data as a normal ``DynamicData`` if desired.

.. _xtypes--limitations-of-the-dynamic-language-binding:

Limitations of the Dynamic Language Binding
===========================================

..
    Sect<16.7.4.4>

The Dynamic Language Binding doesn't currently support:

* Access from Java applications

* :ref:`content_subscription_profile--multi-topic`

* XCDRv1 Data Representation

* Constructing types at runtime

.. _xtypes--unimplemented-features:

**********************
Unimplemented Features
**********************

..
    Sect<16.8>

OpenDDS implements the :ref:`XTypes specification <spec-xtypes>` at the Basic Conformance level, with a partial implementation of the Dynamic Language Binding (supported features of which are described in :ref:`xtypes--dynamic-language-binding-1`).
Specific unimplemented features listed below.
The two optional profiles, XTypes 1.1 Interoperability (XCDR1) and XML, are not implemented.

.. _xtypes--xcdr1-support:

XCDR1 Support
=============

Pre-XTypes standard CDR is fully supported, but the XTypes-specific features are not fully supported and should be avoided.
Types can be marked as final or appendable, but all types should be treated as if they were final.
Nothing should be marked as mutable.
Readers and writers of topic types that are mutable or contain nested types that are mutable will fail to initialize.

.. _xtypes--type-system:

Type System
===========

..
    Sect<16.8.1>

* IDL ``map`` type

* IDL ``bitmask`` type

* IDL ``bitset`` type

* Struct inheritance

.. _xtypes--annotations:

Annotations
===========

..
    Sect<16.8.2>

IDL4 defines many standardized annotations and XTypes uses some of them.
The Annotations recognized by XTypes are in Table 21 in :omgspec:`xtypes:7.3.1.2.2 Using Built-in Annotations`.
Of those listed in that table, the following are not supported in OpenDDS.
They are listed in groups defined by the rows of that table.
Some annotations in that table, and not listed here, can only be used with new capabilities of the :ref:`xtypes--type-system`.

* Struct members

  * :ref:`xtypes--optional` (only implemented for IDL-to-C++11 mapping)

  * ``@must_understand``

  * ``@non_serialized``

* Struct or union members

  * ``@external``

* Enums

  * ``@bit_bound``

  * ``@default_literal``

  * :ref:`xtypes--anno-value` when using the IDL-to-C++03 mapping

* ``@verbatim``

* ``@data_representation``

  * See :ref:`xtypes--anno-data-representation` for details.

.. _xtypes--differences-from-the-specification:

**********************************
Differences From the Specification
**********************************

..
    Sect<16.9>

* Inconsistent topic status isn't set for reader/reader or writer/writer in non-XTypes use cases

* :omgissue:`Define the encoding and extensibility used by Type Lookup Service <DDSXTY14-29>`

* :omgissue:`Enums must have the same "bit bound" to be assignable <DDSXTY14-33>`

* :omgissue:`Default data representation is XCDR2 <DDSXTY14-27>`

* :omgissue:`Anonymous types in Strongly Connected Components <DDSXTY14-35>`

* :omgissue:`Meaning of ignore_member_names in TypeConsistencyEnforcement <DDSXTY14-40>`
