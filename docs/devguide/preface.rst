#######
Preface
#######

****************
What Is OpenDDS?
****************

OpenDDS is an open source implementation of a group of related Object
Management Group (OMG) specifications.

Data Distribution Service (DDS) for Real-Time Systems v1.4 (OMG Document
formal/2015-04-10). This specification details the core functionality
implemented by OpenDDS for real-time publish and subscribe applications and is
described throughout this document.

The Real-time Publish-Subscribe Wire Protocol DDS Interoperability Wire
Protocol Specification (DDSI-RTPS) v2.2 (OMG Document formal/2014-09-01). This
specification describes the primary requirements for interoperability between
industry DDS implementations. This is not the only protocol for which a
specifications exists, however, it is the protocol used for interoperability
testing among DDS implementations.

DDS Security v1.1 (OMG Document formal/2018-04-01) extends DDS with
capabilities for authentication and encryption. OpenDDS’s support for the DDS
Security specification is described in a separate document, “Using DDS Security
in OpenDDS.”

OpenDDS is sponsored by Object Computing, Inc. (OCI) and is available via
http://www.opendds.org/.

***************
Licensing Terms
***************

OpenDDS is made available under the open source software model. The source code
may be freely downloaded and is open for inspection, review, comment, and
improvement. Copies may be freely installed across all your systems and those
of your customers. There is no charge for development or run-time licenses. The
source code is designed to be compiled, and used, across a wide variety of
hardware and operating systems architectures. You may modify it for your own
needs, within the terms of the license agreements. You must not copyright
OpenDDS software. For details of the licensing terms, see the file named
LICENSE that is included in the OpenDDS source code distribution or visit
http://www.opendds.org/license.html.

OpenDDS also utilizes other open source software products, including MPC (Make
Project Creator), ACE (the ADAPTIVE Communication Environment), and TAO (The
ACE ORB). More information about these products is available from OCI’s web
site at http://www.objectcomputing.com/products.

OpenDDS is open source and the development team welcomes contributions of code,
tests, and ideas. Active participation by users ensures a robust
implementation. Contact OCI if you are interested in contributing to the
development of OpenDDS. Please note that any code that is contributed to and
becomes part of the OpenDDS open source code base is subject to the same
licensing terms as the rest of the OpenDDS code base.

****************
About This Guide
****************

This Developer’s Guide corresponds to OpenDDS version 3.13. This guide is
primarily focused on the specifics of using and configuring OpenDDS to build
distributed, publish-subscribe applications. While it does give a general
overview of the OMG Data Distribution Service, this guide is not intended to
provide comprehensive coverage of the specification. The intent of this guide
is to help you become proficient with OpenDDS as quickly as possible.

Highlights of the 3.13 Release

.. attention::
  Removed here, but sphinx could insert this, as well as the version of OpenDDS
  automatically.

TAO Version Compatibility
=========================

OpenDDS 3.13 is compatible with the current patch level of OCI TAO 2.2a, as
well as the current DOC Group beta/micro release. See the
``$DDS_ROOT/README.md`` file for details.

Conventions
===========

.. attention::
  Would have to be revised or removed.

This guide uses the following conventions:

Fixed pitch text

Indicates example code or information a user would enter using a keyboard.

Bold fixed pitch text

Indicates example code that has been modified from a previous example or text
appearing in a menu or dialog box.

Italic text

Indicates a point of emphasis.

...

A horizontal ellipsis indicates that the statement is omitting text.

.

.

.

A vertical ellipsis indicates that a segment of code is omitted from the
example.

Coding Examples
===============

Throughout this guide, we illustrate topics with coding examples. The examples
in this guide are intended for illustration purposes and should not be
considered to be “production-ready” code. In particular, error handling is
sometimes kept to a minimum to help the reader focus on the particular feature
or technique that is being presented in the example. The source code for all
these examples is available as part of the OpenDDS source code distribution in
the ``$DDS_ROOT/DevGuideExamples/`` directory. MPC files are provided with the
examples for generating build-tool specific files, such as GNU Makefiles or
Visual C++ project and solution files. A Perl script named run_test.pl is
provided with each example so you can easily run it.

Related Documents
=================

Throughout this guide, we refer to various specifications published by the
Object Management Group (OMG) and from other sources.

OMG references take the form group/number where group represents the OMG
working group responsible for developing the specification, or the keyword
formal if the specification has been formally adopted, and number represents
the year, month, and serial number within the month the specification was
released. For example, the OMG DDS version 1.4 specification is referenced as
formal/2015-04-10.

You can download any referenced OMG specification directly from the OMG web
site by prepending ``http://www.omg.org/cgi-bin/doc?`` to the specification’s
reference. Thus, the specification formal/07-01-01 becomes
http://www.omg.org/cgi-bin/doc?formal/07-01-01. Providing this destination to a
web browser should take you to a site from which you can download the
referenced specification document.

Additional documentation on OpenDDS is produced and maintained by Object
Computing, Inc. and is available from the OpenDDS Website at
http://www.opendds.org.

Here are some documents of interest and their locations:

  * `Data Distribution Service (DDS) for Real-Time Systems v1.4 (OMG Document
    formal/2015-04-10) <http://www.omg.org/spec/DDS/1.4/PDF>`_

  * `The Real-time Publish-Subscribe Wire Protocol DDS Interoperability Wire
    Protocol Specification (DDSI-RTPS) v2.2 (OMG Document formal/2014-09-01)
    <http://www.omg.org/spec/DDSI-RTPS/2.2/PDF>`_

  * `OMG Data Distribution Portal <http://portals.omg.org/dds/>`_

  * `OpenDDS Buid Instructions, Architecture, and Doxygen Documentation
    <http://www.opendds.org/documentation.html>`_

  * `OpenDDS Frequently Asked Questions <http://www.opendds.org/faq.html>`_

*******************
Supported Platforms
*******************

OCI regularly builds and tests OpenDDS on a wide variety of platforms,
operating systems, and compilers. We continually update OpenDDS to support
additional platforms. See the ``$DDS_ROOT/README.md`` file in the distribution
for the most recent platform support information.

****************
Customer Support
****************

Enterprises are discovering that it takes considerable experience, knowledge,
and money to design and build a complex distributed application that is robust
and scalable. OCI can help you successfully architect and deliver your solution
by drawing on the experience of seasoned architects who have extensive
experience in today's middleware technologies and who understand how to
leverage the power of DDS.

Our service areas include systems architecture, large-scale distributed
application architecture, and object oriented design and development. We excel
in technologies such as DDS (OpenDDS), CORBA (ACE+TAO, JacORB, and opalORB),
Java EE (JBoss), FIX (QuickFIX), and FAST (QuickFAST).

Support offerings for OpenDDS include:

Consulting services to aid in the design of extensible, scalable, and robust
publish-subscribe solutions, including the validation of domain-specific
approaches, service selection, product customization and extension, and
migrating your applications to OpenDDS from other publish-subscribe
technologies and products.

24x7 support that guarantees the highest response level for your
production-level systems.

On-demand service agreement for identification and assessment of minor bugs and
issues that may arise during the development and deployment of OpenDDS-based
solutions.

Our architects have specific and extensive domain expertise in security,
telecommunications, defense, financial, and other real-time distributed
applications.

We can provide professionals who can assist you on short-term engagements, such
as architecture and design review, rapid prototyping, troubleshooting, and
debugging. Alternatively, for larger engagements, we can provide mentors,
architects, and programmers to work alongside your team, providing assistance
and thought leadership throughout the life cycle of the project.

Contact us at +1.314.579.0066 or email <sales@objectcomputing.com> for more
information.

***********************
OCI Technology Training
***********************

OCI provides a rich program of more than 50 well-focused courses designed to
give developers a solid foundation in a variety of technical topics, such as
Object Oriented Analysis and Design, C++ Programming, Java Programming,
Distributed Computing Technologies (including DDS), Patterns, XML, and
UNIX/Linux. Our courses clearly explain major concepts and techniques, and
demonstrate, through hands-on exercises, how they map to real-world
applications.

.. note::

  Our training offerings are constantly changing to meet the latest needs of
  our clients and to reflect changes in technology. Be sure to check out our
  web site at http://www.objectcomputing.com/training for updates to our
  Educational Programs.

On-Site Classes
===============

We can provide the following courses at your company’s facility, integrating
them seamlessly with other employee development programs. For more information
about these or other courses in the OCI curriculum, visit our course catalog
on-line at http://www.objectcomputing.com/training.

.. attention::

  Update this list?

Introduction to CORBA
---------------------

In this one-day course, you will learn the benefits of distributed object
computing; the role CORBA plays in developing distributed applications; when
and where to apply CORBA; and future development trends in CORBA.

CORBA Programming with C++
--------------------------

In this hands-on, four-day course, you will learn: the role CORBA plays in
developing distributed applications; the OMG’s Object Management Architecture;
how to write CORBA clients and servers in C++; how to use CORBA services such
as Naming and Events; using CORBA exceptions; and basic and advanced features
of the Portable Object Adapter (POA). This course also covers the specification
of interfaces using OMG Interface Definition Language (IDL) and details of the
OMG IDL-to-C++ language mapping, and provides hands-on practice in developing
CORBA clients and servers in C++ (using TAO).

Advanced CORBA Programming Using TAO
------------------------------------

In this intensive, hands-on, four-day course, you will learn: several advanced
CORBA concepts and techniques and how they are supported by TAO; how to
configure TAO components for performance and space optimizations; and how to
use TAO’s various concurrency models to meet your application’s end-to-end QoS
guarantees. The course covers recent additions to the CORBA specifications and
to TAO to support real-time CORBA programming, including Real-Time CORBA. It
also covers TAO’s Real-Time Event Service, Notification Service, and
Implementation Repository, and provides extensive hands-on practice in
developing advanced TAO clients and servers in C++. This course is intended for
experienced and serious CORBA/C++ programmers.

Using the ACE C++ Framework
---------------------------

In this hands-on, four-day course, you will learn how to implement Interprocess
Communication (IPC) mechanisms using the ACE (ADAPTIVE Communication
Environment) IPC Service Access Point (SAP) classes and the Acceptor/Connector
pattern. The course will also show you how to use a Reactor in event
demultiplexing and dispatching; how to implement thread-safe applications using
the ACE thread encapsulation class categories; and how to identify appropriate
ACE components to use for your specific application needs.

Object-Oriented Design Patterns and Frameworks
----------------------------------------------

In this three-day course, you will learn the critical language and terminology
relating to design patterns, gain an understanding of key design patterns,
learn how to select the appropriate pattern to apply in a given situation, and
learn how to apply patterns to construct robust applications and frameworks.
The course is designed for software developers who wish to utilize advanced
object oriented design techniques and managers with a strong programming
background who will be involved in the design and implementation of object
oriented software systems.

OpenDDS Programming with C++
----------------------------

In this four-day course, you will learn to build applications using OpenDDS,
the open source implementation of the OMG’s Data Distribution Service (DDS) for
Real-Time Systems. You will learn how to build data-centric systems that share
data via OpenDDS. You will also learn to configure OpenDDS to meet your
application’s Quality of Service requirements. This course if intended for
experienced C++ developers.

OpenDDS Modeling Software Development Kit (SDK)
-----------------------------------------------

In this two-day course, developers and architects gain hands-on experience
using the OpenDDS Modeling SDK to design and build publish/subscribe
applications that use OpenDDS. The Eclipse-based, open source Modeling SDK
enables developers to define an application's middleware components and data
structures as a UML model, then generate the code to implement the model using
OpenDDS. The generated code can then be compiled and linked with the
application to provide seamless middleware support to the application.

C++ Programming Using Boost
---------------------------

In this four-day course, you will learn about the most widely used and useful
libraries that make up Boost. Students will learn how to easily apply these
powerful libraries in their own development through detailed expert
instructor-led training and by hands-on exercises. After finishing this course,
class participants will be prepared to apply Boost to their project, enabling
them to more quickly produce powerful, efficient, and platform independent
applications.

.. note::

  For information about training dates, contact us by phone at
  +1.314.579.0066, via email at training@objectcomputing.com, or
  visit our web site at http://www.objectcomputing.com/training
  to review the current course schedule.
