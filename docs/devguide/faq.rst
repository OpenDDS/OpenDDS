###
FAQ
###

*******
General
*******

.. _prf:

=============
What's a PRF?
=============

"PRF" refers to the PROBLEM-REPORT-FORM, i.e., :ghfile:`PROBLEM-REPORT-FORM`.

The odds of getting questions answered, bugs fixed, etc., goes up significantly when you report problems using the PRF because it insures that developers and support personnel get the most commonly required information right away.
Failure to use the PRF means that those folks have to spend valuable time (yours and theirs) having a conversation to get that information just to start debugging.
Unless you've got a paid support contract, there's even a chance that nobody will answer!

So, always use the PRF!

===========================================================================
Is there a mailing list or forum or usenet group for discussion of OpenDDS?
===========================================================================

Yes.
The `GitHub Discussions <https://github.com/OpenDDS/OpenDDS/discussions>`__ for OpenDDS can be used to ask questions and get help.
Please remember to use the :ref:`Problem Report Form <prf>` when posting questions (and not just "problems").

=====================================
Which platforms does OpenDDS support?
=====================================

Please refer to :ghfile:`README.md#supported-platforms` for a complete list of supported platforms.

========================================================
Is OpenDDS interoperable with other DDS implementations?
========================================================

Starting with version 3.1, OpenDDS contains an implementation of the RTPS (Real Time Publish-Subscribe) specification required for interoperability.
It is not enabled by default.
See :ref:`run_time_configuration--configuring-for-ddsi-rtps-discovery` for configuration details.

=======================
Does OpenDDS use CORBA?
=======================

OpenDDS uses CORBA when configured to use the DCPSInfoRepo for discovery, which is the default.
See :ref:`run_time_configuration--configuring-applications-for-dcpsinforepo`.
CORBA is never used in the distribution of application data.
When configured for RTPS peer-to-peer discovery, the CORBA libraries (TAO) are present but not used for any inter-process communication at all.
When configured for Safety Profile, which uses RTPS, no TAO libraries are used at runtime.

=======================================================================================
Summarize the network connections established by OpenDDS in a two participant scenario.
=======================================================================================

This scenario has two participants (one only publishes and the other only subscribes) using the TCP transport with no Built-In Topics.
The DCPSInfoRepo is used for discovery.
Here, we would expect to see three established TCP connections at each participant:

* The participant contacts the InfoRepo over IIOP (CORBA).
  The endpoint used for this connection can be supplied as ``-DCPSInfoRepo HOST:PORT`` or a similar setting in the .ini file.
  The HOST is the host where the InfoRepo is running.
  The PORT for the InfoRepo can be controlled by passing an argument to the InfoRepo process:  ``-ORBListenEndpoints iiop://:PORT``.

  Another option that's useful for port-forwarding solutions such as NAT traversal and SSH tunneling is ``hostname_in_ior``.
  This gets appended to the endpoint like so ``-ORBListenEndpoints iiop://:PORT/hostname_in_ior=HOST_ALIAS`` where HOST_ALIAS is a hostname (or dotted-decimal IP) that can be used by the client to contact the server.
  In the NAT traversal example, the HOST_ALIAS would be the external side of the NAT and the PORT would be the port that's forwarded to the host running the server.

  You can also specify more than one endpoint (just repeating the ``-ORBListenEndpoints`` will work) so you can provide endpoints that will work for both "inside" and "outside" clients.

* The InfoRepo will make an IIOP connection *back* to the participant.
  This is just the inverse of connection #1, but there is no equivalent to the ``-DCPSInfoRepo corbaloc:...`` option.
  It's not necessary because the InfoRepo knows how to find the participant already (from data transmitted over connection #1).
  To control the endpoint here, use the same ``-ORBListenEndpoints ...`` option but this time with the participant and not the InfoRepo.

* Once the publishing participant and the subscribing participant have been associated, they connect directly using the tcp transport (by default) instance in each one.
  Here the endpoint is determined by the subscriber's transport config (the ``local_address`` value).
  Unfortunately there is no equivalent to the ``hostname_in_ior`` for DDS transports.

******************************
Obtaining and building OpenDDS
******************************

==============================================
How do I obtain, configure, and build OpenDDS?
==============================================

Please refer to :ref:`building` for complete requirements and build steps.

==============================================================
What could be causing std lib related build failures on Linux?
==============================================================

We have seen gcc 4.0.x-series compiler users see this problem the most often.
The visibility feature in this compiler still seems to be in a state of flux.
If you see these errors, we recommend turning off the visibility feature by adding ``no_hidden_visibility=1`` to your platform_macros.GNU file and rebuilding all of ACE+TAO and OpenDDS.

*******************
Configuring OpenDDS
*******************

=========================================================================
Does OpenDDS support broadcast or multicast? If so, how do you set it up?
=========================================================================

Reliable multicast has been available in OpenDDS since the 0.12 release (unreliable multicast since 0.11 release).
A broadcast based transport is currently not available.
Please see :ref:`run_time_configuration--ip-multicast-transport-configuration-options` for how to configure the multicast transport.
See the :ghfile:`tests/DCPS/Messenger` test for an example that can use the multicast transport.
Using ``run_test.pl multicast`` to run the test with the multicast transport.
Also, see :ref:`run_time_configuration--rtps-udp-transport-configuration-options` for details of the interoperable RTPS transport that supports multicast and unicast.

=======================================================================
Why did a subscriber did not receive all samples sent by the publisher?
=======================================================================

While there can be multiple reasons that the samples are lost or dropped, a common problem is that the publisher starts sending samples before the subscriber is ready to receive.
To synchronize, the publisher can use a WaitSet before writing.
This ensures the subscriber is ready to receive samples.
Here is the example code:

.. code-block:: cpp

   DDS::StatusCondition_var cond = writer->get_statuscondition();
   cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

   DDS::WaitSet_var ws = new DDS::WaitSet;
   ws->attach_condition(cond);

   while (true) {
     DDS::PublicationMatchedStatus matches;
     if (writer->get_publication_matched_status(matches) != DDS::RETCODE_OK) {
       // failure
     } else if (matches.current_count >= 1) {
       break;
     }

     DDS::ConditionSeq conditions;
     DDS::Duration_t timeout =
       { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
     if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
       // failure
     }
   }

   ws->detach_condition(cond);

=======================================================================
Why can't my publisher establish a connection with a remote subscriber?
=======================================================================

While there can be multiple reasons for inter-host communication failure, this entry deals with connection establishment failures due to improper endpoint configuration.

The configuration option ``local_address`` represents the endpoint address.
This host/port tuple is used by the publisher to initiate connection establishment.
In order to allow inter-host communication, make sure the endpoint address is publicly visible.

A common mistake is to re-use the configuration provided in the exercises without modifications.
Since the examples are intended to be run on an intra-host environment, they generally use the 'localhost' interface.
This configuration will fail when the test is run in an inter-host setting.

**********************
Built-In Topics (BITs)
**********************

=============================================
What is the status of BIT support in OpenDDS?
=============================================

Support for Built-In-Topics is complete since the 0.12 release.
This means that BITs will now work as specified.
BIT support is now turned on by default.

============================================
How do I turn off BIT support at build-time?
============================================

It is possible to build OpenDDS without BIT support which will reduce overall footprint.
For this, you will need to generate new project files:

.. code-block:: bash

   mwc.pl -type <yourtype> -features built_in_topics=0 DDS.mwc

If ``yourtype`` happens to be ``gnuace``, add ``built_in_topics=0`` to the ``platform_macros.GNU`` file or the ``MAKEFLAGS`` environment variable.
Alternatively, use the configure script with ``--no-built-in-topics``.

==========================================
How do I turn off BIT support at run-time?
==========================================

Turning off BIT support involves it being turned off in the DCPS Information Repository and any associated participants.

The DCPSInfoRepo option ``-NOBITS`` disables BIT support for the InfoRepo.

The option ``-DCPSBit 0`` will disable BIT support for all participants in the process.

*********
Using IDL
*********

================================================
How do I use sequences of built-in types in IDL?
================================================

If you'd like to use a sequence of one of the following IDL built-in types:

* boolean, octet
* char, wchar, string, wstring
* float, double, long double
* short, long, long long
* unsigned short, unsigned long, unsigned long long

TAO already has a typedef for these sequences.
The typedef name is the name of the built-in type in CamelCase with a "Seq" suffix.
Unsigned types use a "U" prefix as in "ULongSeq".

To use the existing typedef, add an ``#include`` for the pidl file from the tao directory:

.. code-block:: omg-idl

   #include <tao/LongSeq.pidl>

The typedefs are in the "CORBA" IDL module, so the typedef for LongSeq could be used as:

.. code-block:: omg-idl

   struct X {
     CORBA::LongSeq seq;
   };

******************************
Using the OpenDDS Modeling SDK
******************************

============================================
How do I obtain the OpenDDS Modeling SDK?
============================================

Please refer to the :ref:`modeling_sdk` for complete installation instructions.

Make sure that Eclipse's list of Available Software Sites contains an enabled site for the Eclipse release itself.  Version 3.5 uses URL: http://download.eclipse.org/releases/galileo.  If not, you need to add the correct update site for your version of Eclipse.

=================================================
Why can't I see the element I added to my figure?
=================================================

If auto-sizing isn't enabled for the figure and depending on the figure's size, an element added to one of the figure's compartments may not be immediately visible.
By increasing the size of the figure, the element should appear.
See the OpenDDS Modeling SDK Guide > Tasks > Working with Diagrams > Creating Figures in the Eclipse help for information on how to make the figure automatically re-size to accommodate additional content.

====================================================================
How to I open a library (for example a DataLib) on the main diagram?
====================================================================

Double clicking on the library should open the library in a subdiagram.
However, sometimes no action will be taken after double clicking.
An alternative way to open a library is to select the library and then press the Enter key.
This topic, along with other topics related to libraries, is in the Eclipse help content under OpenDDS Modeling SDK Guide > Tasks > Modeling > Working with OpenDDS Models.

**********************
Using the DDS DCPS API
**********************

=======================================================================
How should I override a specific QoS policy and leave others defaulted?
=======================================================================

#. Create an empty QoS variable of the proper type (for example ``DDS::DataWriterQos dw_qos``).
#. Call the parent's ``get_default_*_qos()`` method (for example ``Publisher::get_default_datawriter_qos(dw_qos)``.
#. Modify the QoS and pass it to the proper ``create_*`` method.

Note that the default QoS returned by ``get_default_*_qos()`` can be changed with ``set_default_*_qos()``.

The ``TheServiceParticipant`` also has methods to return the code default QoS for different entities, e.g., ``TheServiceParticipant->initial_DataWriterQos()``.

OpenDDS contains various ``*QosPolicyBuilder`` classes that can be used to configure QoS policies using a builder pattern.
The QoS returned by a default constructed ``*QosPolicyBuilder`` corresponds to the code default.
Here's an example using a builder that sets transient local durability for a DataWriter:

.. code-block:: cpp

   #include <dds/DCPS/Qos_Helper.h>

   ...

   DDS::DataWriter_var writer =
      publisher->create_datawriter(topic,
                                   DataWriterQosBuilder().durability_transient_local(),
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

======================================
What are the ``*_QOS_DEFAULT`` macros?
======================================

These macros (for example ``DATAWRITER_QOS_DEFAULT``) are placeholders used only to provide a default value to the ``create_*`` methods.
These macros themselves do not expand to QoS structure instances that can be used for any purpose other than passing to the ``create_*`` methods.
This includes ``DATAWRITER_QOS_USE_TOPIC_QOS``, ``DATAREADER_QOS_DEFAULT``, etc.
In Java, they are classes in the DDS package that have a public static ``get()`` method and not macros.

===============================================
Why do I get samples with invalid data in them?
===============================================

Make sure the call to ``read/take`` returns a status of ``RETCODE_OK`` and the SampleInfo's ``valid_data`` is set to true.
Both of these conditions are required for the sample to be valid.
If ``valid_data`` is not true, then the sample could be indicating an unregister and/or dispose.

========================================================================
What could account for increasing memory usage in subscribing processes?
========================================================================

If the subscribing process contains Data Readers which have received many instances from the corresponding Data Writers, it will need some resources to track each instance.
This can be bounded by the :ref:`qos-resource-limits` policy and by the subscribing application taking samples from the Data Reader.
The :ref:`qos-reader-data-lifecycle` policy may also be helpful.
