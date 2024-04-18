.. _content_subscription_profile:

############################
Content-Subscription Profile
############################

..
    Sect<5>

.. _content_subscription_profile--introduction:

************
Introduction
************

..
    Sect<5.1>

The Content-Subscription Profile of DDS consists of three features which enable a data reader's behavior to be influenced by the content of the data samples it receives.
These three features are:

* Content-Filtered Topic

* Query Condition

* Multi Topic

The content-filtered topic and multi topic interfaces inherit from the ``TopicDescription`` interface (and not from the ``Topic`` interface, as the names may suggest).

Content-filtered topic and query condition allow filtering (selection) of data samples using a SQL-like parameterized query string.
Additionally, query condition allows sorting the result set returned from a data reader's ``read()`` or ``take()`` operation.
Multi topic also has this selection capability as well as the ability to aggregate data from different data writers into a single data type and data reader.

If you are not planning on using the Content-Subscription Profile features in your application, you can configure OpenDDS to remove support for it at build time (:ref:`building--content-subscription-profile`).

.. _content_subscription_profile--content-filtered-topic:

**********************
Content-Filtered Topic
**********************

..
    Sect<5.2>

The domain participant interface contains operations for creating and deleting a content-filtered topic.
Creating a content-filtered topic requires the following parameters:

* Name

  Assigns a name to this content-filtered topic which could later be used with the ``lookup_topicdescription()`` operation.

* Related topic

  Specifies the topic that this content-filtered topic is based on.
  This is the same topic that matched data writers will use to publish data samples.

* Filter expression

  An :ref:`SQL-like expression <content_subscription_profile--filter-expressions>` which defines the subset of samples published on the related topic that should be received by the content-filtered topic's data readers.

* Expression parameters

  The filter expression can contain parameter placeholders.
  This argument provides initial values for those parameters.
  The expression parameters can be changed after the content-filtered topic is created (the filter expression cannot be changed).

Once the content-filtered topic has been created, it is used by the subscriber's ``create_datareader()`` operation to obtain a content-filtering data reader.
This data reader is functionally equivalent to a normal data reader except that incoming data samples which do not meet the filter expression's criteria are dropped.

Filter expressions are first evaluated at the publisher so that data samples which would be ignored by the subscriber can be dropped before even getting to the transport.
This feature can be turned off by setting :cfg:prop:`DCPSPublisherContentFilter` to ``0``.
The behavior of non-default :ref:`qos-deadline` or :ref:`qos-liveliness` policies may be affected by this policy.
Special consideration must be given to how the "missing" samples impact the QoS behavior, see the document in :ghfile:`docs/design/CONTENT_SUBSCRIPTION`.

.. note:: :ref:`rtps-udp-transport` does not always do Writer-side filtering.
  It does not currently implement transport level filtering, but may be able to filter above the transport layer.

.. _content_subscription_profile--filter-expressions:

Filter Expressions
==================

..
    Sect<5.2.1>

The formal grammar for filter expressions is defined in :omgspec:`dds:Annex B - Syntax for Queries and Filters`.
This section provides an informal summary of that grammar.
:ref:`Query expressions <content_subscription_profile--query-expressions>` and :ref:`topic expressions <content_subscription_profile--topic-expressions>` are also defined in the DDS specification.

Filter expressions are combinations of one or more predicates.
Each predicate is a logical expression taking one of two forms:

* ``<arg1> <RelOp> <arg2>``

* * ``arg1`` and ``arg2`` are arguments which may be either a literal value (integer, character, floating-point, string, or enumeration), a parameter placeholder of the form ``%n`` (where n is a zero-based index into the parameter sequence), or a field reference.

  * At least one of the arguments must be a field reference, which is the name of an IDL struct field, optionally followed by any number of ``.`` and another field name to represent nested structures.

  * ``RelOp`` is a relational operator from the list: ``=``, ``>``, ``>=``, ``<``, ``<=``, ``<>``, and ``like``.
    ``like`` is a wildcard match using ``%`` to match any number of characters and _ to match a single character.

  * Examples of this form of predicate include: ``a = 'z'``, ``b <> 'str'``, ``c < d``, ``e = 'enumerator'``, ``f >= 3.14e3``, ``27 > g``, ``h <> i``, ``j.k.l like %0``

* ``<arg1> [NOT] BETWEEN <arg2> AND <arg3>``

* * In this form, argument 1 must be a field reference and arguments 2 and 3 must each be a literal value or parameter placeholder.

Any number of predicates can be combined through the use of parenthesis and the Boolean operators ``AND``, ``OR``, and ``NOT`` to form a filter expression.

.. _content_subscription_profile--expression-parameters:

Expression Parameters
=====================

..
    Sect<5.2.2>

Expression parameters allow more flexibility since the filter can effectively change at runtime.
To use expression parameters, add parameter placeholders in the filter expression wherever a literal would be used.
For example, an expression to select all samples that have a string field with a fixed value (``m = 'A'``) could instead use a placeholder which would be written as ``m = %0``.
Placeholders consist of a percent sign followed by a decimal integer between 0 and 99 inclusive.

Using a filter that contains placeholders requires values for each placeholder which is used in the expression to be provided by the application in the corresponding index of the expression parameters sequence (placeholder ``%0`` is ``sequence[0]``).
The application can set the parameter sequence when the content-filtered topic is created (``create_contentfilteredtopic``) or after it already exists by using ``set_expression_parameters``.
A valid value for each used placeholder must be in the parameters sequence whenever the filter is evaluated, for example when a data reader using the content-filtered topic is enabled.

The type used for the parameters sequence in the DDS-DCPS API is a sequence of strings.
The application must format this string based on how the parameter is used:

* For a number (integer or floating point), provide the decimal representation in the same way it would appear as a C++ or Java literal.

* For a character or string, provide the character(s) directly without quoting

* For an enumerated type, provide one of the enumerators as if it was a string

.. _content_subscription_profile--filtering-and-dispose-unregister-samples:

Filtering and Dispose/Unregister Samples
========================================

..
    Sect<5.2.3>

DataReaders without filtering can see samples with the ``valid_data`` field of ``SampleInfo`` set to ``false``.
This happens when the matching DataWriter disposes or unregisters the instance.
Content filtering (whether achieved through Content-Filtered Topics, Query Conditions, or Multi Topics) will filter such samples when the filter expression explicitly uses key fields.
Filter expressions that don't meet that criteria will result in no such samples passing the filter.

.. _content_subscription_profile--content-filtered-topic-example:

Content-Filtered Topic Example
==============================

..
    Sect<5.2.4>

The code snippet below creates a content-filtered topic for the Message type.
First, here is the IDL for Message:

.. code-block:: omg-idl

      module Messenger {
        @topic
        struct Message {
          long id;
        };
      };

Next we have the code that creates the data reader:

.. code-block:: cpp

      CORBA::String_var type_name = message_type_support->get_type_name();
      DDS::Topic_var topic = dp->create_topic("MyTopic",
                                              type_name,
                                              TOPIC_QOS_DEFAULT, 0, 0);
      DDS::ContentFilteredTopic_var cft =
        participant->create_contentfilteredtopic("MyTopic-Filtered",
                                                 topic,
                                                 "id > 1",
                                                 StringSeq());
      DDS::DataReader_var dr =
        subscriber->create_datareader(cft,
                                      DATAREADER_QOS_DEFAULT, 0, 0);

The data reader ``dr`` will only receive samples that have values of ``id`` greater than 1.

.. _content_subscription_profile--query-condition:

***************
Query Condition
***************

..
    Sect<5.3>

The query condition interface inherits from the read condition interface, therefore query conditions have all of the capabilities of read conditions along with the additional capabilities described in this section.
One of those inherited capabilities is that the query condition can be used like any other condition with a wait set (:ref:`conditions_and_listeners--conditions`).

The ``DataReader`` interface contains operations for creating (``create_querycondition``) and deleting (``delete_readcondition``) a query condition.
Creating a query condition requires the following parameters:

* Sample, view, and instance state masks

  These are the same state masks that would be passed to ``create_readcondition()``, ``read()``, or ``take()``.

* Query expression

  An SQL-like expression (see :ref:`content_subscription_profile--query-expressions`) describing a subset of samples which cause the condition to be triggered.
  This same expression is used to filter the data set returned from a ``read_w_condition()`` or ``take_w_condition()`` operation.
  It may also impose a sort order (``ORDER BY``) on that data set.

* Query parameters

  The query expression can contain parameter placeholders.
  This argument provides initial values for those parameters.
  The query parameters can be changed after the query condition is created (the query expression cannot be changed).

A particular query condition can be used with a wait set (``attach_condition``), with a data reader (``read_w_condition``, ``take_w_condition``, ``read_next_instance_w_condition``, ``take_next_instance_w_condition``), or both.
When used with a wait set, the ``ORDER BY`` clause has no effect on triggering the wait set.
When used with a data reader's ``read*()`` or ``take*()`` operation, the resulting data set will only contain samples which match the query expression and they will be ordered by the ``ORDER BY`` fields, if an ``ORDER BY`` clause is present.

.. _content_subscription_profile--query-expressions:

Query Expressions
=================

..
    Sect<5.3.1>

Query expressions are a super set of filter expressions (:ref:`content_subscription_profile--filter-expressions`).
Following the filter expression, the query expression can optionally have an ``ORDER BY`` keyword followed by a comma-separated list of field references.
If the ``ORDER BY`` clause is present, the filter expression may be empty.
The following strings are examples of query expressions:

* ``m > 100 ORDER BY n``

* ``ORDER BY p.q, r, s.t.u``

* ``NOT v LIKE 'z%'``

Query expressions can use parameter placeholders in the same way that filter expressions (for content-filtered topics) use them.
See :ref:`content_subscription_profile--expression-parameters` for details.

.. _content_subscription_profile--query-condition-example:

Query Condition Example
=======================

..
    Sect<5.3.2>

The following code snippet creates and uses a query condition for a type that uses struct ``Message`` with field ``key`` (an integral type).

.. code-block:: cpp

      DDS::QueryCondition_var dr_qc =
        dr->create_querycondition(DDS::ANY_SAMPLE_STATE,
                                  DDS::ANY_VIEW_STATE,
                                  DDS::ALIVE_INSTANCE_STATE,
                                  "key > 1",
                                  DDS::StringSeq());
      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(dr_qc);
      DDS::ConditionSeq active;
      DDS::Duration_t three_sec = {3, 0};
      DDS::ReturnCode_t ret = ws->wait(active, three_sec);
        // error handling not shown
      ws->detach_condition(dr_qc);
      MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
      MessageSeq data;
      DDS::SampleInfoSeq infoseq;
      ret = mdr->take_w_condition(data, infoseq, DDS::LENGTH_UNLIMITED, dr_qc);
        // error handling not shown
      dr->delete_readcondition(dr_qc);

Any sample received with ``key <= 1`` would neither trigger the condition (to satisfy the wait) nor be returned in the ``data`` sequence from ``take_w_condition()``.

.. _content_subscription_profile--multi-topic:

***********
Multi Topic
***********

..
    Sect<5.4>

Multi topic is a more complex feature than the other two Content-Subscription features, therefore describing it requires some new terminology.

The ``MultiTopic`` interface inherits from the ``TopicDescription`` interface, just like ``ContentFilteredTopic`` does.
A data reader created for the multi topic is known as a "multi topic data reader." A multi topic data reader receives samples belonging to any number of regular topics.
These topics are known as its "constituent topics." The multi topic has a DCPS data type known as the "resulting type." The multi topic data reader implements the type-specific data reader interface for the resulting type.
For example, if the resulting type is Message, then the multi topic data reader can be narrowed to the ``MessageDataReader`` interface.

The multi topic's topic expression (:ref:`content_subscription_profile--topic-expressions`) describes how the distinct fields of the incoming data (on the constituent topics) are mapped to the fields of the resulting type.

The domain participant interface contains operations for creating and deleting a multi topic.
Creating a multi topic requires the following parameters:

* Name

  Assigns a name to this multi topic which could later be used with the ``lookup_topicdescription()`` operation.

* Type name

  Specifies the resulting type of the multi topic.
  This type must have its type support registered before creating the multi topic.

* Topic expression (also known as subscription expression)

  An SQL-like expression (:ref:`content_subscription_profile--topic-expressions`) which defines the mapping of constituent topic fields to resulting type fields.
  It can also specify a filter (``WHERE`` clause).

* Expression parameters

  The topic expression can contain parameter placeholders.
  This argument provides initial values for those parameters.
  The expression parameters can be changed after the multi topic is created (the topic expression cannot be changed).

Once the multi topic has been created, it is used by the subscriber's ``create_datareader()`` operation to obtain a multi topic data reader.
This data reader is used by the application to receive the constructed samples of the resulting type.
The manner in which these samples are constructed is described in :ref:`content_subscription_profile--how-resulting-samples-are-constructed`.

.. _content_subscription_profile--topic-expressions:

Topic Expressions
=================

..
    Sect<5.4.1>

Topic expressions use a syntax that is very similar to a complete SQL query:

::

    SELECT <aggregation> FROM <selection> [WHERE <condition>]

* The aggregation can be either a ``*`` or a comma separated list of field specifiers.
  Each field specifier has the following syntax:

* * ``<constituent_field> [[AS] <resulting_field>]]``

  * ``constituent_field`` is a field reference (:ref:`content_subscription_profile--filter-expressions`) to a field in one of the constituent topics (which topic is not specified).

  * The optional resulting_field is a field reference to a field in the resulting type.
    If present, the ``resulting_field`` is the destination for the constituent_field in the constructed sample.
    If absent, the ``constituent_field`` data is assigned to a field with the same name in the resulting type.
    The optional ``AS`` has no effect.

  * If a ``*`` is used as the aggregation, each field in the resulting type is assigned the value from a same-named field in one of the constituent topic types.

* The selection lists one or more constituent topic names.
  Topic names are separated by a ``join`` keyword (all 3 join keywords are equivalent):

* * ``<topic> [{NATURAL INNER | NATURAL | INNER NATURAL} JOIN <topic>]...``

  * Topic names must contain only letters, digits, and dashes (but may not start with a digit).

  * The natural join operation is commutative and associative, thus the order of topics has no impact.

  * The semantics of the natural join are that any fields with the same name are treated as "join keys" for the purpose of combining data from the topics in which those keys appear.
    The join operation is described in more detail in subsequent sections.

* The condition has the exact same syntax and semantics as the filter expression (:ref:`content_subscription_profile--filter-expressions`).
  Field references in the condition must match field names in the resulting types, not field names in the constituent topic types.
  The condition in the topic expression can use parameter placeholders in the same way that filter expressions (for content-filtered topics) use them.
  See :ref:`content_subscription_profile--expression-parameters` for details.

.. _content_subscription_profile--usage-notes:

Usage Notes
===========

..
    Sect<5.4.2>

.. _content_subscription_profile--join-keys-and-dcps-data-keys:

Join Keys and DCPS Data Keys
----------------------------

..
    Sect<5.4.2.1>

The concept of DCPS data keys (``@key``) has already been discussed in :ref:`getting_started--defining-data-types-with-idl`.
Join keys for the multi topic are a distinct but related concept.

A join key is any field name that occurs in the struct for more than one constituent topic.
The existence of the join key enforces a constraint on how data samples of those topics are combined into a constructed sample (:ref:`content_subscription_profile--how-resulting-samples-are-constructed`).
Specifically, the value of that key must be equal for those data samples from the constituent topics to be combined into a sample of the resulting type.
If multiple join keys are common to the same two or more topics, the values of all keys must be equal in order for the data to be combined.

The DDS specification requires that join key fields have the same type.
Additionally, OpenDDS imposes two requirements on how the IDL must define DCPS data keys to work with multi topics:

#. Each join key field must also be a DCPS data key for the types of its constituent topics.

#. The resulting type must contain each of the join keys, and those fields must be DCPS data keys for the resulting type.

The example in :ref:`content_subscription_profile--idl-and-topic-expression` meets both of these requirements.
Note that it is not necessary to list the join keys in the aggregation (``SELECT`` clause).

.. _content_subscription_profile--how-resulting-samples-are-constructed:

How Resulting Samples are Constructed
-------------------------------------

..
    Sect<5.4.2.2>

Although many concepts in multi topic are borrowed from the domain of relational databases, a real-time middleware such as DDS is not a database.
Instead of processing a batch of data at a time, each sample arriving at the data reader from one of the constituent topics triggers multi-topic-specific processing that results in the construction of zero, one, or many samples of the resulting type and insertion of those constructed samples into the multi topic data reader.

Specifically, the arrival of a sample on constituent topic ``A`` with type ``TA``  results in the following steps in the multi topic data reader (this is a simplification of the actual algorithm):

#. A sample of the resulting type is constructed, and fields from ``TA`` which exist in the resulting type and are in the aggregation (or are join keys) are copied from the incoming sample to the constructed sample.

#. Each topic ``B`` which has at least one join key in common with ``A`` is considered for a join operation.
   The join reads ``READ_SAMPLE_STATE`` samples on topic ``B`` with key values matching those in the constructed sample.
   The result of the join may be zero, one, or many samples.
   Fields from ``TB`` are copied to the resulting sample as described in step 1.

#. Join keys of topic ``B`` (connecting it to other topics) are then processed as described in step 2, and this continues to all other topics that are connected by join keys.

#. Any constituent topics that were not visited in steps 2 or 3 are processed as "cross joins" (also known as cross-product joins).
   These are joins with no key constraints.

#. If any constructed samples result, they are inserted into the multi topic data reader's internal data structures as if they had arrived via the normal mechanisms.
   Application listeners and conditions are notified.

.. _content_subscription_profile--use-with-subscriber-listeners:

Use with Subscriber Listeners
-----------------------------

..
    Sect<5.4.2.3>

If the application has registered a subscriber listener for read condition status changes (``DATA_ON_READERS_STATUS``) with the same subscriber that also contains a multi topic, then the application must invoke ``notify_datareaders()`` in its implementation of the subscriber listener's ``on_data_on_readers()`` callback method.
This requirement is necessary because the multi topic internally uses data reader listeners, which are preempted when a subscriber listener is registered.

.. _content_subscription_profile--multi-topic-example:

Multi Topic Example
===================

..
    Sect<5.4.3>

This example is based on the example topic expression used in Annex A section A.3 of the DDS specification.
It illustrates how the properties of the multi topic join operation can be used to correlate data from separate topics (and possibly distinct publishers).

.. _content_subscription_profile--idl-and-topic-expression:

IDL and Topic Expression
------------------------

..
    Sect<5.4.3.1>

Often times we will use the same string as both the topic name and topic type.
In this example we will use distinct strings for the type names and topic names, in order to illustrate when each is used.

Here is the IDL for the constituent topic data types:

.. code-block:: omg-idl

    @topic
    struct LocationInfo {
      @key unsigned long flight_id;
      long x;
      long y;
      long z;
    };

    @topic
    struct PlanInfo {
      @key unsigned long flight_id;
      string flight_name;
      string tailno;
    };

Note that the names and types of the key fields match, so they are designed to be used as join keys.
The resulting type (below) also has that key field.

Next we have the IDL for the resulting data type:

.. code-block:: omg-idl

    @topic
    struct Resulting {
      @key unsigned long flight_id;
      string flight_name;
      long x;
      long y;
      long height;
    };

Based on this IDL, the following topic expression can be used to combine data from a topic ``Location`` which uses type ``LocationInfo`` and a topic ``FlightPlan`` which uses type ``PlanInfo``:

.. code-block:: sql

    SELECT flight_name, x, y, z AS height FROM Location NATURAL JOIN FlightPlan WHERE height < 1000 AND x <23

Taken together, the IDL and the topic expression describe how this multi topic will work.
The multi topic data reader will construct samples which belong to instances keyed by ``flight_id``.
The instance of the resulting type will only come into existence once the corresponding instances are available from both the ``Location`` and ``FlightPlan`` topics.
Some other domain participant or participants within the domain will publish data on those topics, and they don't even need to be aware of one another.
Since they each use the same ``flight_id`` to refer to flights, the multi topic can correlate the incoming data from disparate sources.

.. _content_subscription_profile--creating-the-multi-topic-data-reader:

Creating the Multi Topic Data Reader
------------------------------------

..
    Sect<5.4.3.2>

Creating a data reader for the multi topic consists of a few steps.
First the type support for the resulting type is registered, then the multi topic itself is created, followed by the data reader:

.. code-block:: cpp

      ResultingTypeSupport_var ts_res = new ResultingTypeSupportImpl;
      ts_res->register_type(dp, "");
      CORBA::String_var type_name = ts_res->get_type_name();
      DDS::MultiTopic_var mt =
        dp->create_multitopic("MyMultiTopic",
                              type_name,
                              "SELECT flight_name, x, y, z AS height "
                                "FROM Location NATURAL JOIN FlightPlan "
                                "WHERE height < 1000 AND x<23",
                              DDS::StringSeq());
      DDS::DataReader_var dr =
        sub->create_datareader(mt,
                               DATAREADER_QOS_DEFAULT,
                               NULL,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

.. _content_subscription_profile--reading-data-with-the-multi-topic-data-reader:

Reading Data with the Multi Topic Data Reader
---------------------------------------------

..
    Sect<5.4.3.3>

From an API perspective, the multi topic data reader is identical to any other typed data reader for the resulting type.
This example uses a wait set and a read condition in order to block until data is available.

.. code-block:: cpp

      DDS::WaitSet_var ws = new DDS::WaitSet;
      DDS::ReadCondition_var rc =
        dr->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                 DDS::ANY_VIEW_STATE,
                                 DDS::ANY_INSTANCE_STATE);
      ws->attach_condition(rc);
      DDS::Duration_t infinite = {DDS::DURATION_INFINITE_SEC,
                                  DDS::DURATION_INFINITE_NSEC};
      DDS::ConditionSeq active;
      ws->wait(active, infinite); // error handling not shown
      ws->detach_condition(rc);
      ResultingDataReader_var res_dr = ResultingDataReader::_narrow(dr);
      ResultingSeq data;
      DDS::SampleInfoSeq info;
      res_dr->take_w_condition(data, info, DDS::LENGTH_UNLIMITED, rc);

