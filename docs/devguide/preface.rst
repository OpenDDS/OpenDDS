.. _preface--preface:

#######
Preface
#######

..
    Sect<0>

.. _preface--what-is-opendds:

****************
What is OpenDDS?
****************

..
    Sect<0.1>

OpenDDS is an open source implementation of a group of related Object Management Group (OMG) specifications.

#. **Data Distribution Service (DDS) for Real-Time Systems v1.4** (OMG document formal/2015-04-10).
   This specification details the core functionality implemented by OpenDDS for real-time publish and subscribe applications and is described throughout this document.

#. **The Real-time Publish-Subscribe Wire Protocol DDS Interoperability Wire Protocol Specification (DDSI-RTPS) v2.3** (OMG document formal/2019-04-03).
   Although the document number is v2.3, it specifies protocol version 2.4.
   This specification describes the requirements for interoperability between DDS implementations.

#. **DDS Security v1.1** (OMG document formal/2018-04-01) extends DDS with capabilities for authentication and encryption.
   OpenDDS’s support for the DDS Security specification is described below in Chapter :ref:`dds_security--dds-security`.

#. **Extensible and Dynamic Topic Types for DDS (XTypes) v1.3** (OMG document formal/2020-02-04) defines details of the type system used for the data exchanged on DDS Topics, including how schema and data are encoded for network transmission.
   etails of DDS-XTypes are below in Chapter :ref:`xtypes--xtypes`.

OpenDDS is sponsored by Object Computing, Inc. (OCI) and is available via `https://www.opendds.org <https://www.opendds.org/>`__.

.. _preface--licensing-terms:

***************
Licensing Terms
***************

..
    Sect<0.2>

OpenDDS is made available under the *open source software* model.
The source code may be freely downloaded and is open for inspection, review, comment, and improvement.
Copies may be freely installed across all your systems and those of your customers.
There is no charge for development or run-time licenses.
The source code is designed to be compiled, and used, across a wide variety of hardware and operating systems architectures.
You may modify it for your own needs, within the terms of the license agreements.
You must not copyright OpenDDS software.
For details of the licensing terms, see the file named ``LICENSE`` that is included in the OpenDDS source code distribution or visit http://www.opendds.org/license.html.

OpenDDS also utilizes other open source software products, including MPC (Make Project Creator), ACE (the ADAPTIVE Communication Environment), and TAO (The ACE ORB).
More information about these products is available from OCI’s web site at http://www.objectcomputing.com/products.

OpenDDS is open source and the development team welcomes contributions of code, tests, and ideas.
Active participation by users ensures a robust implementation.
Contact OCI if you are interested in contributing to the development of OpenDDS.
Please note that any code that is contributed to and becomes part of the OpenDDS open source code base is subject to the same licensing terms as the rest of the OpenDDS code base.

.. _preface--about-this-guide:

****************
About This Guide
****************

..
    Sect<0.3>

This Developer’s Guide corresponds to OpenDDS version 3.23.
This guide is primarily focused on the specifics of using and configuring OpenDDS to build distributed publish-subscribe applications.
While it does give a general overview of the OMG Data Distribution Service, this guide is not intended to provide comprehensive coverage of the specification.
The intent of this guide is to help you become proficient with OpenDDS as quickly as possible.

.. _preface--highlights-of-the-3-23-release:

******************************
Highlights of the 3.23 Release
******************************

..
    Sect<0.4>

*NOTE: Numbers in parenthesis are GitHub pull requests*

Additions:

* DataRepresentationQosPolicy and TypeConsistencyEnforcementQosPolicy can be set through XML (`#3763 <https://github.com/objectcomputing/OpenDDS/pull/3763>`__)

* RTPS send queue performance improvements (`#3794 <https://github.com/objectcomputing/OpenDDS/pull/3794>`__)

* Cross-compiling improvements (`#3853 <https://github.com/objectcomputing/OpenDDS/pull/3853>`__)

* New support for DynamicDataWriter and enhanced support for DynamicDataReader (`#3827 <https://github.com/objectcomputing/OpenDDS/pull/3827>`__, `#3727 <https://github.com/objectcomputing/OpenDDS/pull/3727>`__, `#3871 <https://github.com/objectcomputing/OpenDDS/pull/3871>`__, `#3718 <https://github.com/objectcomputing/OpenDDS/pull/3718>`__, `#3830 <https://github.com/objectcomputing/OpenDDS/pull/3830>`__, `#3893 <https://github.com/objectcomputing/OpenDDS/pull/3893>`__, `#3904 <https://github.com/objectcomputing/OpenDDS/pull/3904>`__, `#3885 <https://github.com/objectcomputing/OpenDDS/pull/3885>`__, `#3933 <https://github.com/objectcomputing/OpenDDS/pull/3933>`__, `#3935 <https://github.com/objectcomputing/OpenDDS/pull/3935>`__)

* New config option for RtpsDiscovery ``SpdpRequestRandomPort`` (`#3903 <https://github.com/objectcomputing/OpenDDS/pull/3903>`__)

* New ``opendds_mwc.pl`` Wrapper Script (`#3821 <https://github.com/objectcomputing/OpenDDS/pull/3821>`__, `#3913 <https://github.com/objectcomputing/OpenDDS/pull/3913>`__)

* Improve support for loading signed documents (`#3864 <https://github.com/objectcomputing/OpenDDS/pull/3864>`__)

Fixes:

* Unauthenticated participant leads to invalid iterator (`#3748 <https://github.com/objectcomputing/OpenDDS/pull/3748>`__)

* Shmem Association race (`#3549 <https://github.com/objectcomputing/OpenDDS/pull/3549>`__)

* Shmem and tcp null pointer (`#3779 <https://github.com/objectcomputing/OpenDDS/pull/3779>`__)

* Submodule checkout on Windows (`#3812 <https://github.com/objectcomputing/OpenDDS/pull/3812>`__)

Notes:

* Docker images are built for release tags https://github.com/objectcomputing/OpenDDS/pkgs/container/opendds (`#3776 <https://github.com/objectcomputing/OpenDDS/pull/3776>`__)

.. _preface--ace-tao-version-compatibility:

ACE/TAO Version Compatibility
=============================

..
    Sect<0.4.1>

OpenDDS 3.23 is compatible with the current patch level of OCI TAO 2.2a (which includes ACE), as well as the current DOC Group micro release in the ACE 6.x / TAO 2.x series.
See the :ghfile:`README.md` file for details.

.. _preface--conventions:

Conventions
===========

..
    Sect<0.4.2>

This guide uses the following conventions:

.. list-table::
   :header-rows: 1

   * - ``Fixed pitch text``

     - Indicates example code or information a user would enter using a keyboard.

   * - ``Bold fixed pitch text``

     - Indicates example code that has been modified from a previous example or text appearing in a menu or dialog box.

   * - *Italic text*

     - Indicates a point of emphasis.

   * - ...

     - A horizontal ellipsis indicates that the statement is omitting text.

   * - .

       .

       .

     - A vertical ellipsis indicates that a segment of code is omitted from the example.

.. _preface--coding-examples:

***************
Coding Examples
***************

..
    Sect<0.5>

Throughout this guide, we illustrate topics with coding examples.
The examples in this guide are intended for illustration purposes and should not be considered to be “production-ready” code.
In particular, error handling is sometimes kept to a minimum to help the reader focus on the particular feature or technique that is being presented in the example.
The source code for all these examples is available as part of the OpenDDS source code distribution in the :ghfile:`DevGuideExamples` directory.
MPC files are provided with the examples for generating build-tool specific files, such as GNU Makefiles or Visual C++ project and solution files.
A Perl script named ``run_test.pl`` is provided with each example so you can easily run it.

.. _preface--related-documents:

*****************
Related Documents
*****************

..
    Sect<0.6>

Throughout this guide, we refer to various specifications published by the Object Management Group (OMG) and from other sources.

OMG references take the form *group/number* where *group* represents the OMG working group responsible for developing the specification, or the keyword ``formal`` if the specification has been formally adopted, and *number* represents the year, month, and serial number within the month the specification was released.
For example, the OMG DDS version 1.4 specification is referenced as ``formal/2015-04-10``.

You can download any referenced OMG specification directly from the OMG web site by prepending ``http://www.omg.org/cgi-bin/doc?`` to the specification’s reference.
Thus, the specification ``formal/07-01-01`` becomes http://www.omg.org/cgi-bin/doc?formal/07-01-01.
Providing this destination to a web browser should take you to a site from which you can download the referenced specification document.

Additional documentation on OpenDDS is produced and maintained by Object Computing, Inc. and is available from the OpenDDS Website at  `https://www.opendds.org <https://www.opendds.org/>`__.

Here are some documents of interest and their locations:

.. list-table::
   :header-rows: 1

   * - Document

     - Location

   * - Data Distribution Service (DDS) for Real-Time Systems v1.4 (OMG Document formal/2015-04-10)

     - http://www.omg.org/spec/DDS/1.4/PDF

   * - The Real-time Publish-Subscribe Wire Protocol DDS Interoperability Wire Protocol Specification (DDSI-RTPS) v2.3 (OMG Document formal/2019-04-03)

     - http://www.omg.org/spec/DDSI-RTPS/2.3/PDF

   * - OMG Data Distribution Portal

     - http://portals.omg.org/dds/

   * - OpenDDS Build Instructions, Architecture, and Doxygen Documentation

     - http://www.opendds.org/documentation.html

   * - OpenDDS Frequently Asked Questions

     - http://www.opendds.org/faq.html

.. _preface--supported-platforms:

*******************
Supported Platforms
*******************

..
    Sect<0.7>

OCI regularly builds and tests OpenDDS on a wide variety of platforms, operating systems, and compilers.
We continually update OpenDDS to support additional platforms.
See the :ghfile:`README.md` file in the distribution for the most recent platform support information.

.. _preface--customer-support:

****************
Customer Support
****************

..
    Sect<0.8>

Enterprises are discovering that it takes considerable experience, knowledge, and money to design and build a complex distributed application that is robust and scalable.
OCI can help you successfully architect and deliver your solution by drawing on the experience of seasoned architects who have extensive experience in today's middleware technologies and who understand how to leverage the power of DDS.

Our service areas include systems architecture, large-scale distributed application architecture, and object oriented design and development.
We excel in technologies such as DDS (OpenDDS), CORBA (ACE+TAO, JacORB, and opalORB), Java EE (JBoss), FIX (QuickFIX), and FAST (QuickFAST).

Support offerings for OpenDDS include:

Consulting services to aid in the design of extensible, scalable, and robust publish-subscribe solutions, including the validation of domain-specific approaches, service selection, product customization and extension, and migrating your applications to OpenDDS from other publish-subscribe technologies and products.

24x7 support that guarantees the highest response level for your production-level systems.

On-demand service agreement for identification and assessment of minor bugs and issues that may arise during the development and deployment of OpenDDS-based solutions.

Our architects have specific and extensive domain expertise in security, telecommunications, defense, financial, and other real-time distributed applications.

We can provide professionals who can assist you on short-term engagements, such as architecture and design review, rapid prototyping, troubleshooting, and debugging.
Alternatively, for larger engagements, we can provide mentors, architects, and programmers to work alongside your team, providing assistance and thought leadership throughout the life cycle of the project.

Contact us at +1.314.579.0066 or email ``<sales@objectcomputing.com>`` for more information.

.. _preface--oci-technology-training:

***********************
OCI Technology Training
***********************

..
    Sect<0.9>

OCI provides a rich program of more than 50 well-focused courses designed to give developers a solid foundation in a variety of technical topics, such as Object Oriented Analysis and Design, C++ Programming, Java Programming, Distributed Computing Technologies (including DDS), Patterns, XML, and UNIX/Linux.
Our courses clearly explain major concepts and techniques, and demonstrate, through hands-on exercises, how they map to real-world applications.

.. note:: Our training offerings are constantly changing to meet the latest needs of our clients and to reflect changes in technology.
  Be sure to check out our web site at http://www.objectcomputing.com/training for updates to our Educational Programs.

.. _preface--on-site-or-remote-classes:

*************************
On-Site or Remote Classes
*************************

..
    Sect<0.10>

We can provide the following courses at your company’s facility or remotely, integrating them seamlessly with other employee development programs.
For more information about these or other courses in the OCI curriculum, visit our course catalog on-line at http://www.objectcomputing.com/training.

.. _preface--introduction-to-corba:

*********************
Introduction to CORBA
*********************

..
    Sect<0.11>

In this one-day course, you will learn the benefits of distributed object computing; the role CORBA plays in developing distributed applications; when and where to apply CORBA; and future development trends in CORBA.

.. _preface--corba-programming-with-c:

**************************
CORBA Programming with C++
**************************

..
    Sect<0.12>

In this hands-on, four-day course, you will learn: the role CORBA plays in developing distributed applications; the OMG’s Object Management Architecture; how to write CORBA clients and servers in C++; how to use CORBA services such as Naming and Events; using CORBA exceptions; and basic and advanced features of the Portable Object Adapter (POA).
This course also covers the specification of interfaces using OMG Interface Definition Language (IDL) and details of the OMG IDL-to-C++ language mapping, and provides hands-on practice in developing CORBA clients and servers in C++ (using TAO).

.. _preface--advanced-corba-programming-using-tao:

************************************
Advanced CORBA Programming Using TAO
************************************

..
    Sect<0.13>

In this intensive, hands-on, four-day course, you will learn: several advanced CORBA concepts and techniques and how they are supported by TAO; how to configure TAO components for performance and space optimizations; and how to use TAO’s various concurrency models to meet your application’s end-to-end QoS guarantees.
The course covers recent additions to the CORBA specifications and to TAO to support real-time CORBA programming, including Real-Time CORBA.
It also covers TAO’s Real-Time Event Service, Notification Service, and Implementation Repository, and provides extensive hands-on practice in developing advanced TAO clients and servers in C++.
This course is intended for experienced and serious CORBA/C++ programmers.

.. _preface--using-the-ace-c-framework:

***************************
Using the ACE C++ Framework
***************************

..
    Sect<0.14>

In this hands-on, four-day course, you will learn how to implement Interprocess Communication (IPC) mechanisms using the ACE (ADAPTIVE Communication Environment) IPC Service Access Point (SAP) classes and the Acceptor/Connector pattern.
The course will also show you how to use a Reactor in event demultiplexing and dispatching; how to implement thread-safe applications using the ACE thread encapsulation class categories; and how to identify appropriate ACE components to use for your specific application needs.

.. _preface--object-oriented-design-patterns-and-frameworks:

**********************************************
Object-Oriented Design Patterns and Frameworks
**********************************************

..
    Sect<0.15>

In this three-day course, you will learn the critical language and terminology relating to design patterns, gain an understanding of key design patterns, learn how to select the appropriate pattern to apply in a given situation, and learn how to apply patterns to construct robust applications and frameworks.
The course is designed for software developers who wish to utilize advanced object oriented design techniques and managers with a strong programming background who will be involved in the design and implementation of object oriented software systems.

.. _preface--opendds-programming-with-c-or-with-java:

*****************************************
OpenDDS Programming with C++ or with Java
*****************************************

..
    Sect<0.16>

In this four-day course, you will learn to build applications using OpenDDS, the open source implementation of the OMG’s Data Distribution Service (DDS) for Real-Time Systems.
You will learn how to build data-centric systems that share data via OpenDDS.
You will also learn to configure OpenDDS to meet your application’s Quality of Service requirements.
This course is intended for experienced C++ or Java developers.

.. _preface--opendds-modeling-software-development-kit-sdk:

***********************************************
OpenDDS Modeling Software Development Kit (SDK)
***********************************************

..
    Sect<0.17>

In this two-day course, developers and architects gain hands-on experience using the OpenDDS Modeling SDK to design and build publish/subscribe applications that use OpenDDS.
The Eclipse-based, open source Modeling SDK enables developers to define an application's middleware components and data structures as a UML model, then generate the code to implement the model using OpenDDS.
The generated code can then be compiled and linked with the application to provide seamless middleware support to the application.

.. _preface--c-programming-using-boost:

***************************
C++ Programming Using Boost
***************************

..
    Sect<0.18>

In this four-day course, you will learn about the most widely used and useful libraries that make up Boost.
Students will learn how to easily apply these powerful libraries in their own development through detailed expert instructor-led training and by hands-on exercises.
After finishing this course, class participants will be prepared to apply Boost to their project, enabling them to more quickly produce powerful, efficient, and platform independent applications.

.. note:: For information about training dates, contact us by phone at +1.314.579.0066, via email at ``training@objectcomputing.com``, or visit our web site at `http://www.objectcomputing.com/training <https://www.objectcomputing.com/training>`__ to review the current course schedule.

