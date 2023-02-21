.. _dds_security--dds-security:

############
DDS Security
############

..
    Sect<14>

.. _dds_security--building-opendds-with-security-enabled:

**************************************
Building OpenDDS with Security Enabled
**************************************

..
    Sect<14.1>

Prior to utilizing DDS Security, OpenDDS must be built to include security elements into the resulting libraries.
The following instructions show how this is to be completed on various platforms.

.. _dds_security--prerequisites:

Prerequisites
=============

..
    Sect<14.1.1>

OpenDDS includes an implementation of the OMG DDS Security 1.1 specification.
Building OpenDDS with security enabled requires the following dependencies:

#. Xerces-C++ v3.x

#. OpenSSL v1.0.2+, v1.1, or v3.0.1+ (1.1 is preferred)

#. Google Test (only required if building OpenDDS tests)

   * If you are using OpenDDS from a git repository, Google Test is provided as a git submodule.
     Make sure to enable submodules with the ``--recursive`` option to git clone.

#. CMake (required if building OpenDDS tests and building Google Test and other dependencies from source).

**General Notes on Using OpenDDS Configure Script with DDS Security**

#. DDS Security is disabled by default, enable it with ``--security``

#. OpenDDS tests are disabled by default, enable them with ``--tests``

   * Disabling tests skips the Google Test and CMake dependencies

   * If tests are enabled, the configure script can run CMake and build Google Test

.. _dds_security--building-opendds-with-security-on-windows:

Building OpenDDS with Security on Windows
=========================================

..
    Sect<14.1.2>

**Using Microsoft vcpkg**

Microsoft vcpkg is a “C++ Library Manager for Windows, Linux, and macOS” which helps developers build/install dependencies.
Although it is cross-platform, this guide only discusses vcpkg on Windows.

As of this writing, vcpkg is only supported on Visual Studio 2015 Update 3 and later versions; if using an earlier version of Visual Studio, skip down to the manual setup instructions later in this section.

#. * * * If OpenDDS tests will be built, install CMake or put the one that comes with Visual Studio on the PATH (see Common7\IDE\CommonExtensions\Microsoft\CMake).

       * If you need to obtain and install vcpkg, navigate to `https://github.com/Microsoft/vcpkg <#https://github.com/Microsoft/vcpkg>`__ and follow the instructions to obtain vcpkg by cloning the repository and bootstrapping it.

       * Fetch and build the dependencies; by default, vcpkg targets x86 so be sure to specify the x64 target if required by specifying it when invoking vcpkg install, as shown here:``vcpkg install openssl:x64-windows xerces-c:x64-windows``

       * Configure OpenDDS by passing the openssl and xerces3 switches.
         As a convenience, it can be helpful to set an environment variable to store the path since it is the same location for both dependencies.``set VCPKG_INSTALL=c:\path\to\vcpkg\installed\x64-windowsconfigure --security --openssl=%VCPKG_INSTALL% --xerces3=%VCPKG_INSTALL%``

       * Compile with msbuild or by launching Visual Studio from this command prompt so it inherits the correct environment variables and building from there.

``msbuild /m DDS_TAOv2_all.sln``

::


**Manual Build**

Note: for all of the build steps listed here, check that each package targets the same architecture (either 32-bit or 64-bit) by compiling all dependencies within the same type of Developer Command Prompt.

**Compiling OpenSSL**

Official OpenSSL instructions can be found `here <https://wiki.openssl.org/index.php/Compilation_and_Installation#Windows>`__.

#. Install Perl and add it to the Path environment variable.
   For this guide, ActiveState is used.

#. Install Netwide Assembler (NASM).
   Click through the latest stable release and there is a win32 and win64 directory containing executable installers.
   The installer does not update the Path environment variable, so a manual entry ``(%LOCALAPPDATA%\bin\NASM)`` is necessary.

#. Download the required version of OpenSSL by cloning the repository.

#. Open a Developer Command Prompt (32-bit or 64-bit depending on the desired target architecture) and change into the freshly cloned openssl directory.

#. Run the configure script and specify a required architecture (``perl Configure VC-WIN32 or perl Configure VC-WIN64A``).

#. Run ``nmake``.

#. Run ``nmake install``.

Note: if the default OpenSSL location is desired, which will be searched by OpenDDS, open the Developer Command Prompt as an administrator before running the install.
It will write to “C:\Program Files” or “C:\Program Files (x86)” depending on the architecture.

**Compiling Xerces-C++ 3**

Official Xerces instructions can be found `here <https://xerces.apache.org/xerces-c/build-3.html>`__.

#. Download/extract the Xerces source files.

#. Create a cmake build directory and change into it (from within the Xerces source tree).

::

    mkdir build
    cd build

#. Run cmake with the appropriate generator.
   In this case Visual Studio 2017 with 64-bit is being used so:

::

    cmake -G "Visual Studio 15 2017 Win64" ..

#. Run cmake again with the build switch and install target (this should be done in an administrator command-prompt to install in the default location as mentioned above).

::

    cmake --build . --target install

**Configuring and Building OpenDDS**:

#. Change into the OpenDDS root folder and run configure with security enabled.

   * If the default location was used for OpenSSL and Xerces, configure should automatically find the dependencies:

::

    configure --security

#. * If a different location was used (assuming environment variables ``NEW_SSL_ROOT`` and ``NEW_XERCES_ROOT`` point to their respective library directories):

``configure --security --openssl=%NEW_SSL_ROOT%   --xerces3=%NEW_XERCES_ROOT%``

#. Compile with msbuild (or by opening the solution file in Visual Studio and building from there).

``msbuild /m DDS_TAOv2_all.sln``

.. _dds_security--building-opendds-with-security-on-linux:

Building OpenDDS with Security on Linux
=======================================

..
    Sect<14.1.3>

Xerces-C++ and OpenSSL may be installed using the system package manager, or built from source.
If using the system package manager (that is, headers can be found under /usr/include), invoke the configure script with the --security option.
If Xerces-C++ and/or OpenSSL are built from source or installed in a custom location, also provide the ``--xerces3=/foo`` and ``--openssl=/bar`` command line options.

.. _dds_security--building-opendds-with-security-on-macos:

Building OpenDDS with Security on macOS
=======================================

..
    Sect<14.1.4>

Xerces-C++ and OpenSSL may be installed using homebrew or another developer-focused package manager, or built from source.
The instructions above for Linux also apply to macOS but the package manager will not install directly in ``/usr`` so make sure to specify the library locations to the configure script.

.. _dds_security--building-opendds-with-security-for-android:

Building OpenDDS with Security for Android
==========================================

..
    Sect<14.1.5>

See the ``docs/android.md`` file included in the OpenDDS source code.

.. _dds_security--architecture-of-the-dds-security-specification:

**********************************************
Architecture of the DDS Security Specification
**********************************************

..
    Sect<14.2>

The DDS Security specification defines plugin APIs for Authentication, Access Control, and Cryptographic operations.
These APIs provide a level of abstraction for DDS implementations as well as allowing for future extensibility and version control.
Additionally, the specification defines Built-In implementations of each of these plugins, which allows for a baseline of functionality and interoperability between DDS implementations.
OpenDDS implements these Built-In plugins, and this document assumes that the Built-In plugins are being used.
Developers using OpenDDS may also implement their own custom plugins, but those efforts are well beyond the scope of this document.

.. _dds_security--terms-and-background-info:

*************************
Terms and Background Info
*************************

..
    Sect<14.3>

DDS Security uses current industry standards and best-practices in security.
As such, this document makes use of several security concepts which may warrant additional research by OpenDDS users.

+--------------------------------------------------+-------------------------------------------------------------------------------------------+
| Term Group                                       | References                                                                                |
+==================================================+===========================================================================================+
| Public Key Cryptography (including Private Keys) | * https://en.wikipedia.org/wiki/Public-key_cryptography                                   |
|                                                  |                                                                                           |
|                                                  | * RSA – https://en.wikipedia.org/wiki/RSA_(algorithm)                                     |
|                                                  |                                                                                           |
|                                                  | * Elliptic Curve Cryptography - https://en.wikipedia.org/wiki/Elliptic_curve_cryptography |
+--------------------------------------------------+-------------------------------------------------------------------------------------------+
| Public Key Certificate                           | * https://en.wikipedia.org/wiki/Public_key_certificate                                    |
|                                                  |                                                                                           |
|                                                  | * Certificate Authority – https://en.wikipedia.org/wiki/Certificate_authority             |
|                                                  |                                                                                           |
|                                                  | * X.509 – https://en.wikipedia.org/wiki/X.509                                             |
|                                                  |                                                                                           |
|                                                  | * PEM - https://en.wikipedia.org/wiki/Privacy-enhanced_Electronic_Mail                    |
+--------------------------------------------------+-------------------------------------------------------------------------------------------+
| Signed Documents                                 | * https://en.wikipedia.org/wiki/Digital_signature                                         |
+--------------------------------------------------+-------------------------------------------------------------------------------------------+

.. _dds_security--reftable36:

**Table**

.. _dds_security--required-dds-security-artifacts:

*******************************
Required DDS Security Artifacts
*******************************

..
    Sect<14.4>

.. _dds_security--per-domain-artifacts:

Per-Domain Artifacts
====================

..
    Sect<14.4.1>

These are shared by all participants within the secured DDS Domain:

* Identity CA Certificate

* Permissions CA Certificate (may be same as Identity CA Certificate)

* Governance Document

- Signed by Permissions CA using its private key

.. _dds_security--per-participant-artifacts:

Per-Participant Artifacts
=========================

..
    Sect<14.4.2>

These are specific to the individual Domain Participants within the DDS Domain:

* Identity Certificate and its Private Key

- Issued by Identity CA (or a CA that it authorized to act on its behalf)

* Permissions Document

- Contains a “subject name” which matches the participant certificate’s Subject

- Signed by Permissions CA using its private key

.. _dds_security--required-opendds-configuration:

******************************
Required OpenDDS Configuration
******************************

..
    Sect<14.5>

The following configuration steps are required to enable OpenDDS Security features:

#. Select RTPS Discovery and the RTPS-UDP Transport; because DDS Security only works with these configurations, both must be specified for any security-enabled participant.

#. Enable OpenDDS security-features, which can be done two ways:

   * Via API: ``“TheServiceParticipant->set_security(true);”`` or

   * Via config file: ``“DCPSSecurity=1”`` in the ``[common]`` section.

.. _dds_security--dds-security-configuration-via-propertyqospolicy:

DDS Security Configuration via PropertyQosPolicy
================================================

..
    Sect<14.5.1>

When the application creates a DomainParticipant object, the DomainParticipantQos passed to the ``create_participant()`` method now contains a PropertyQosPolicy object which has a sequence of name-value pairs.
The following properties must be included to enable security.
Except where noted, these values take the form of a URI starting with either the scheme “file:” followed by a filesystem path (absolute or relative) or the scheme “data:” followed by the literal data.

+---------------------------------------+----------------------------------+------------------------------------------+
| Name                                  | Value                            | Notes                                    |
+=======================================+==================================+==========================================+
| ``dds.sec.auth.identity_ca``          | Certificate PEM file             | Can be the same as ``permissions_ca``    |
+---------------------------------------+----------------------------------+------------------------------------------+
| ``dds.sec.access.permissions_ca``     | Certificate PEM file             | Can be the ``same as identity_ca``       |
+---------------------------------------+----------------------------------+------------------------------------------+
| ``dds.sec.access.governance``         | Signed XML (.p7s)                | Signed by ``permissions_ca``             |
+---------------------------------------+----------------------------------+------------------------------------------+
| ``dds.sec.auth.identity_certificate`` | Certificate PEM file             | Signed by ``identity_ca``                |
+---------------------------------------+----------------------------------+------------------------------------------+
| ``dds.sec.auth.private_key``          | Private Key PEM file             | Private key for ``identity_certificate`` |
+---------------------------------------+----------------------------------+------------------------------------------+
| ``dds.sec.auth.password``             | Private Key Password (not a URI) | Optional, Base64 encoded                 |
+---------------------------------------+----------------------------------+------------------------------------------+
| ``dds.sec.access.permissions``        | Signed XML (.p7s)                | Signed by ``permissions_ca``             |
+---------------------------------------+----------------------------------+------------------------------------------+

.. _dds_security--reftable37:

**Table**

.. _dds_security--propertyqospolicy-example-code:

PropertyQosPolicy Example Code
==============================

..
    Sect<14.5.2>

Below is an example of code that sets the DDS Participant QoS’s PropertyQoSPolicy in order to configure DDS Security.

.. code-block:: cpp

    // DDS Security artifact file locations
    const char auth_ca_file[] = "file:identity_ca_cert.pem";
    const char perm_ca_file[] = "file:permissions_ca_cert.pem";
    const char id_cert_file[] = "file:test_participant_01_cert.pem";
    const char id_key_file[] = "file:test_participant_01_private_key.pem";
    const char governance_file[] = "file:governance_signed.p7s";
    const char permissions_file[] = "file:permissions_01_signed.p7s";

    // DDS Security property names
    const char DDSSEC_PROP_IDENTITY_CA[] = "dds.sec.auth.identity_ca";
    const char DDSSEC_PROP_IDENTITY_CERT[] = "dds.sec.auth.identity_certificate";
    const char DDSSEC_PROP_IDENTITY_PRIVKEY[] = "dds.sec.auth.private_key";
    const char DDSSEC_PROP_PERM_CA[] = "dds.sec.access.permissions_ca";
    const char DDSSEC_PROP_PERM_GOV_DOC[] = "dds.sec.access.governance";
    const char DDSSEC_PROP_PERM_DOC[] = "dds.sec.access.permissions";

    void append(DDS::PropertySeq& props, const char* name, const char* value)
    {
      const DDS::Property_t prop = {name, value, false /*propagate*/};
      const unsigned int len = props.length();
      props.length(len + 1);
      props[len] = prop;
    }

    int main(int argc, char* argv[])
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      // Start with the default Participant QoS
      DDS::DomainParticipantQos part_qos;
      dpf->get_default_participant_qos(part_qos);

      // Add properties required by DDS Security
      DDS::PropertySeq& props = part_qos.property.value;
      append(props, DDSSEC_PROP_IDENTITY_CA, auth_ca_file);
      append(props, DDSSEC_PROP_IDENTITY_CERT, id_cert_file);
      append(props, DDSSEC_PROP_IDENTITY_PRIVKEY, id_key_file);
      append(props, DDSSEC_PROP_PERM_CA, perm_ca_file);
      append(props, DDSSEC_PROP_PERM_GOV_DOC, governance_file);
      append(props, DDSSEC_PROP_PERM_DOC, permissions_file);

      // Create the participant
      participant = dpf->create_participant(4, // DomainID
                                            part_qos,
                                            0, // No listener
                                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

.. _dds_security--identity-certificates-and-certificate-authorities:

Identity Certificates and Certificate Authorities
=================================================

..
    Sect<14.5.3>

All certificate inputs to OpenDDS, including self-signed CA certificates, are expected to be an X.509 v3 certificate in PEM format for either a 2048-bit RSA key or a 256-bit Elliptic Curve key (using the prime256v1 curve).

.. _dds_security--identity-permissions-and-subject-names:

Identity, Permissions, and Subject Names
========================================

..
    Sect<14.5.4>

The “subject_name” element for a signed permissions XML document must match the “Subject:” field provided by the accompanying Identity Certificate which is transmitted during participant discovery, authentication, and authorization.
This ensures that the permissions granted by the Permissions CA do, in fact, correspond to the identity provided.

.. _dds_security--examples-in-the-opendds-source-code-repository:

Examples in the OpenDDS Source Code Repository
==============================================

..
    Sect<14.5.5>

Examples to demonstrate how the DDS Security features are used with OpenDDS can be found in the OpenDDS GitHub repository.

The following table describes the various examples and where to find them in the source tree.

+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+
| **Example**                                                                       | **Source Location**                                                  |
+===================================================================================+======================================================================+
| C++ application that configures security QoS policies via command-line parameters | :ghfile:`tests/DCPS/Messenger/publisher.cpp`                         |
+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+
| Identity CA Certificate (along with private key)                                  | :ghfile:`tests/security/certs/identity/identity_ca_cert.pem`         |
+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+
| Permissions CA Certificate (along with private key)                               | :ghfile:`tests/security/certs/permissions/permissions_ca_cert.pem`   |
+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+
| Participant Identity Certificate (along with private key)                         | :ghfile:`tests/security/certs/identity/test_participant_01_cert.pem` |
+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+
| Governance XML Document (alongside signed document)                               | :ghfile:`tests/DCPS/Messenger/governance.xml`                        |
+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+
| Permissions XML Document (alongside signed document)                              | :ghfile:`tests/DCPS/Messenger/permissions_1.xml`                     |
+-----------------------------------------------------------------------------------+----------------------------------------------------------------------+

.. _dds_security--reftable38:

**Table**

.. _dds_security--using-openssl-utilities-for-opendds:

Using OpenSSL Utilities for OpenDDS
===================================

..
    Sect<14.5.6>

To generate certificates using the openssl command, a configuration file "openssl.cnf" is required (see below for example commands).
Before proceeding, it may be helpful to review OpenSSL’s manpages to get help with the file format.
In particular, configuration file format and ca command’s documentation and configuration file options.

An example OpenSSL CA-Config file used in OpenDDS testing can be found here: :ghfile:`tests/security/certs/identity/identity_ca_openssl.cnf`

.. _dds_security--creating-self-signed-certificate-authorities:

Creating Self-Signed Certificate Authorities
--------------------------------------------

..
    Sect<14.5.6.1>

Generate a self-signed 2048-bit RSA CA:

::

    openssl genrsa -out ca_key.pem 2048
    openssl req -config openssl.cnf -new -key ca_key.pem -out ca.csr
    openssl x509 -req -days 3650 -in ca.csr -signkey ca_key.pem -out ca_cert.pem

Generate self-signed 256-bit Elliptic Curve CA:

::

    openssl ecparam -name prime256v1 -genkey -out ca_key.pem
    openssl req -config openssl.cnf -new -key ca_key.pem -out ca.csr
    openssl x509 -req -days 3650 -in ca.csr -signkey ca_key.pem -out ca_cert.pem

.. _dds_security--creating-signed-certificates-with-an-existing-ca:

Creating Signed Certificates with an Existing CA
------------------------------------------------

..
    Sect<14.5.6.2>

Generate a signed 2048-bit RSA certificate:

::

    openssl genrsa -out cert_1_key.pem 2048
    openssl req -new -key cert_1_key.pem -out cert_1.csr
    openssl ca -config openssl.cnf -days 3650 -in cert_1.csr -out cert_1.pem

Generate a signed 256-bit Elliptic Curve certificate:

::

    openssl ecparam -name prime256v1 -genkey -out cert_2_key.pem
    openssl req -new -key cert_2_key.pem -out cert_2.csr
    openssl ca -config openssl.cnf -days 3650 -in cert_2.csr -out cert_2.pem

.. _dds_security--signing-documents-with-smime:

Signing Documents with SMIME
----------------------------

..
    Sect<14.5.6.3>

Sign a document using existing CA & CA private key:

::

    openssl smime -sign -in doc.xml -text -out doc_signed.p7s -signer ca_cert.pem -inkey ca_private_key.pem

.. _dds_security--domain-governance-document:

**************************
Domain Governance Document
**************************

..
    Sect<14.6>

The signed governance document is used by the DDS Security built-in access control plugin in order to determine both per-domain and per-topic security configuration options for specific domains.
For full details regarding the content of the governance document, see the OMG DDS Security specification section 9.4.1.2.

.. _dds_security--global-governance-model:

Global Governance Model
=======================

..
    Sect<14.6.1>

It’s worth noting that the DDS Security Model expects the governance document to be globally shared by all participants making use of the relevant domains described within the governance document.
Even if this is not the case, the local participant will verify incoming authentication and access control requests as if the remote participant shared the same governance document and accept or reject the requests accordingly.

.. _dds_security--key-governance-elements:

Key Governance Elements
=======================

..
    Sect<14.6.2>

Domain Id Set

A list of domain ids and/or domain id ranges of domains impacted by the current domain rule.
The syntax is the same as the domain id set found in the governance document.

The set is made up of <id> tags or <id_range> tags.
An <id> tag simply contains the domain id that are part of the set.
An <id_range> tag can be used to add multiple ids at once.
It must contain a <min> tag to say where the range starts and may also have a <max> tag to say where the range ends.
If the <max> tag is omitted then the set includes all valid domain ids starting at <min>.

If the domain rule or permissions grant should to apply to all domains, use the following:

::

    <domains>
      <id_range><min>0</min></id_range>
    </domains>

If there’s a need to be selective about what domains are chosen, here’s an annotated example:

::

    <domains>
      <id>2</id>
      <id_range><min>4</min><max>6</max></id_range> <!-- 4, 5, 6 -->
      <id_range><min>10</min></id_range> <!-- 10 and onward -->
    </domains>

Governance Configuration Types

The following types and values are used in configuring both per-domain and per-topic security configuration options.
We summarize them here to simplify discussion of the configuration options where they’re used, found below.

**Boolean**

A boolean value indicating whether a configuration option is enabled or not.
Recognized values are: ``TRUE/true/1`` or ``FALSE/false/0.``

**ProtectionKind**

The method used to protect domain data (message signatures or message encryption) along with the ability to include origin authentication for either protection kind.
Currently, OpenDDS doesn’t implement origin authentication.
So while the "_WITH_ORIGIN_AUTHENTICATION" options are recognized, the underlying configuration is unsupported.
Recognized values are: ``{NONE, SIGN, ENCRYPT,SIGN_WITH_ORIGIN_AUTHENTICATION``, or ``ENCRYPT_WITH_ORIGIN_AUTHENTICATION}``

**BasicProtectionKind**

The method used to protect domain data (message signatures or message encryption).
Recognized values are: ``{NONE, SIGN, or ENCRYPT}``

**FnmatchExpression**

A wildcard-capable string used to match topic names.
Recognized values will conform to POSIX ``fnmatch()`` function as specified in POSIX 1003.2-1992, Section B.6.

.. _dds_security--domain-rule-configuration-options:

Domain Rule Configuration Options
=================================

..
    Sect<14.6.3>

The following XML elements are used to configure domain participant behaviors.

+------------------------------------------+----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Element                                  | Type           | Description                                                                                                                                                                                                                                                                                                        |
+==========================================+================+====================================================================================================================================================================================================================================================================================================================+
| ``<allow_unauthenticated_participants>`` | Boolean        | A boolean value which determines whether to allow unauthenticated participants for the current domain rule                                                                                                                                                                                                         |
+------------------------------------------+----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``<enable_join_access_control>``         | Boolean        | A boolean value which determines whether to enforce domain access controls for authenticated participants                                                                                                                                                                                                          |
+------------------------------------------+----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| <discovery_protection_kind>              | ProtectionKind | The discovery protection element specifies the protection kind used for the built-in DataWriter(s) and DataReader(s) used for secure endpoint discovery messages                                                                                                                                                   |
+------------------------------------------+----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| <liveliness_protection_kind>             | ProtectionKind | The liveliness protection element specifies the protection kind used for the built-in DataWriter and DataReader used for secure liveliness messages                                                                                                                                                                |
+------------------------------------------+----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| <rtps_protection_kind>                   | ProtectionKind | Indicate the desired level of protection for the whole RTPS message.                                                                                                                                                                                                                                               |
|                                          |                | Very little RTPS data exists outside the “metadata protection” envelope (see topic rule configuration options), and so for most use cases topic-level “data protection” or “metadata protection” can be combined with discovery protection and/or liveliness protection in order to secure domain data adequately. |
|                                          |                | One item that is not secured by "metadata protection" is the timestamp, since RTPS uses a separate InfoTimestamp submessage for this.                                                                                                                                                                              |
|                                          |                | The timestamp can be secured by using <rtps_protection_kind>                                                                                                                                                                                                                                                       |
+------------------------------------------+----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. _dds_security--reftable39:

**Table**

.. _dds_security--topic-rule-configuration-options:

Topic Rule Configuration Options
================================

..
    Sect<14.6.4>

The following XML elements are used to configure topic endpoint behaviors:

``<topic_expression>`` : **FnmatchExpression**

A wildcard-capable string used to match topic names.
See description above.
A “default” rule to catch all previously unmatched topics can be made with: ``<topic_expression>*</topic_expression>``

``<enable_discovery_protection>`` : **Boolean**

Enables the use of secure discovery protections for matching user topic announcements.

``<enable_read_access_control>`` : **Boolean**

Enables the use of access control protections for matching user topic DataReaders.

``<enable_write_access_control>`` : **Boolean**

Enables the use of access control protections for matching user topic DataWriters.

``<metadata_protection_kind>`` : **ProtectionKind**

Specifies the protection kind used for the RTPS SubMessages sent by any DataWriter and DataReader whose associated Topic name matches the rule’s topic expression.

<data_protection_kind> : **BasicProtectionKind**

Specifies the basic protection kind used for the RTPS SerializedPayload SubMessage element sent by any DataWriter whose associated Topic name matches the rule’s topic expression.

.. _dds_security--governance-xml-example:

Governance XML Example
======================

..
    Sect<14.6.5>

.. code-block:: xml

    <?xml version="1.0" encoding="utf-8"?>
    <dds xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://www.omg.org/spec/DDS- Security/20170801/omg_shared_ca_domain_governance.xsd">
      <domain_access_rules>
        <domain_rule>
          <domains>
            <id>0</id>
            <id_range>
              <min>10</min>
              <max>20</max>
            </id_range>
          </domains>
    <allow_unauthenticated_participants>FALSE</allow_unauthenticated_participants>
          <enable_join_access_control>TRUE</enable_join_access_control>
          <rtps_protection_kind>SIGN</rtps_protection_kind>
          <discovery_protection_kind>ENCRYPT</discovery_protection_kind>
          <liveliness_protection_kind>SIGN</liveliness_protection_kind>
          <topic_access_rules>
            <topic_rule>
              <topic_expression>Square*</topic_expression>
              <enable_discovery_protection>TRUE</enable_discovery_protection>
              <enable_read_access_control>TRUE</enable_read_access_control>
              <enable_write_access_control>TRUE</enable_write_access_control>
              <metadata_protection_kind>ENCRYPT</metadata_protection_kind>
              <data_protection_kind>ENCRYPT</data_protection_kind>
            </topic_rule>
            <topic_rule>
              <topic_expression>Circle</topic_expression>
              <enable_discovery_protection>TRUE</enable_discovery_protection>
              <enable_read_access_control>FALSE</enable_read_access_control>
              <enable_write_access_control>TRUE</enable_write_access_control>
              <metadata_protection_kind>ENCRYPT</metadata_protection_kind>
              <data_protection_kind>ENCRYPT</data_protection_kind>
            </topic_rule>
            <topic_rule>
              <topic_expression>Triangle</topic_expression>
              <enable_discovery_protection>FALSE</enable_discovery_protection>
              <enable_read_access_control>FALSE</enable_read_access_control>
              <enable_write_access_control>TRUE</enable_write_access_control>
              <metadata_protection_kind>NONE</metadata_protection_kind>
              <data_protection_kind>NONE</data_protection_kind>
            </topic_rule>
            <topic_rule>
              <topic_expression>*</topic_expression>
              <enable_discovery_protection>TRUE</enable_discovery_protection>
              <enable_read_access_control>TRUE</enable_read_access_control>
              <enable_write_access_control>TRUE</enable_write_access_control>
              <metadata_protection_kind>ENCRYPT</metadata_protection_kind>
              <data_protection_kind>ENCRYPT</data_protection_kind>
            </topic_rule>
          </topic_access_rules>
        </domain_rule>
      </domain_access_rules>
    </dds>

.. _dds_security--participant-permissions-document:

********************************
Participant Permissions Document
********************************

..
    Sect<14.7>

The signed permissions document is used by the DDS Security built-in access control plugin in order to determine participant permissions to join domains and to create endpoints for reading, writing, and relaying domain data.
For full details regarding the content of the permissions document, see the OMG DDS Security specification section 9.4.1.3.

.. _dds_security--key-permissions-elements:

Key Permissions Elements
========================

..
    Sect<14.7.1>

**Grants**

Each permissions file consists of one or more permissions grants.
Each grant bestows access control privileges to a single subject name for a limited validity period.

**Subject Name**

Each grant’s subject name is intended to match against a corresponding identity certificate’s “subject” field.
In order for permissions checks to successfully validate for both local and remote participants, the supplied identity certificate subject name must match the subject name of one of the grants included in the permissions file.

**Validity**

Each grant’s validity section contains a start date and time (``<not_before>``) and an end date and time (``<not_after>``) to indicate the period of time during which the grant is valid.

The format of the date and time, which is like ISO-8601, must take one of the following forms:

#. * * * * ``YYYY-MM-DDThh:mm:ss``

* * * * * Example: ``2020-10-26T22:45:30``

#. * * * * ``YYYY-MM-DDThh:mm:ssZ``

* * * * * Example:``2020-10-26T22:45:30Z``

#. * * * * ``YYYY-MM-DDThh:mm:ss+hh:mm``

* * * * * Example:``2020-10-26T23:45:30+01:00``

#. * * * * ``YYYY-MM-DDThh:mm:ss-hh:mm``

* * * * * Example:``2020-10-26T16:45:30-06:00``

All fields shown must include leading zeros to fill out their full width, as shown in the examples.
YYYY-MM-DD is the date and hh:mm:ss is the time in 24-hour format.
The date and time must be able to be represented by the time_t (C standard library) type of the system.
The seconds field can also include a variable length fractional part, like 00.0 or 01.234, but it will be ignored because time_t represents a whole number of seconds.
Examples #1 and #2 are both interpreted to be using UTC.
To put the date and time in a local time, a time zone offset can to be added that says how far the local timezone is ahead of (using ‘+’ as in example #3) or behind (using ‘-’ as in example #4) UTC at that date and time.

**Allow / Deny Rules**

Grants will contain one or more allow / deny rules to indicate which privileges are being applied.
When verifying that a particular operation is allowed by the supplied grant, rules are checked in the order they appear in the file.
If the domain, partition, and (when implemented) data tags for an applicable topic rule match the operation being verified, the rule is applied (either allow or deny).
Otherwise, the next rule is considered.
Special Note: If a grant contains any allow rule that matches a given domain (even one with no publish / subscribe / relay rules), the grant may be used to join a domain with join access controls enabled.

**Default Rule**

The default rule is the rule applied if none of the grant’s allow rules or deny rules match the incoming operation to be verified.

**Domain Id Set**

Every allow or deny rule must contain a set of domain ids to which it applies.
The syntax is the same as the domain id set found in the governance document.
See section :ref:`dds_security--key-governance-elements` for details.

**Publish / Subscribe / Relay Rules (PSR rules)**

Every allow or deny rule may optionally contain a list of publish, subscribe, or relay rules bestowing privileges to publish, subscribe, or relay data (respectively).
Each rule applies to a collection of topics in a set of partitions with a particular set of data tags.
As such, each rule must then meet these three conditions (topics, partitions, and (when implemented) data tags) in order to apply to a given operation.
These conditions are governed by their relevant subsection, but the exact meaning and default values will vary depending on the both the PSR type (publish, subscribe, relay) as well as whether this is an allow rule or a deny rule.
Each condition is summarized below, but please refer to the OMG DDS Security specification for full details.
OpenDDS does not currently support relay-only behavior and consequently ignores allow and deny relay rules for both local and remote entities.
Additionally, OpenDDS does not currently support data tags, and so the data tag condition applied is always the “default” behavior described below.

**Topic List**

The list of topics and/or topic expressions for which a rule applies.
Topic names and expressions are matched using POSIX fnmatch() rules and syntax.
If the triggering operation matches any of the topics listed, the topic condition is met.
The topic section must always be present for a PSR rule, so there there is no default behavior.

**Partition List**

The partitions list contains the set of partition names for which the parent PSR rule applies.
Similarly to topics, partition names and expressions are matched using POSIX ``fnmatch()`` rules and syntax.
For “allow” PSR rules, the DDS entity of the associated triggering operation must be using a strict subset of the partitions listed for the rule to apply.
When no partition list is given for an “allow” PSR rule, the “empty string” partition is used as the default value.
For “deny” PSR rules, the rule will apply if the associated DDS entity is using any of the partitions listed.
When no partition list is given for a “deny” PSR rule, the wildcard expression “*” is used as the default value.

**Data Tags List**

Data tags are an optional part of the DDS Security specification and are not currently implemented by OpenDDS.
If they were implemented, the condition criteria for data tags would be similar to partitions.
For “allow” PSR rules, the DDS entity of the associated triggering operation must be using a strict subset of the data tags listed for the rule to apply.
When no data tag list is given for an “allow” PSR rule, the empty set of data tags is used as the default value.
For “deny” PSR rules, the rule will apply if the associated DDS entity is using any of the data tags listed.
When no data tag list is given for a “deny” PSR rule, the set of “all possible tags” is used as the default value.

.. _dds_security--permissions-xml-example:

Permissions XML Example
=======================

..
    Sect<14.7.2>

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <dds xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://www.omg.org/spec/DDS-Security/20170801/omg_shared_ca_permissions.xsd">
      <permissions>
        <grant name="ShapesPermission">
          <subject_name>emailAddress=cto@acme.com, CN=DDS Shapes Demo, OU=CTO Office, O=ACME Inc., L=Sunnyvale, ST=CA, C=US</subject_name>
          <validity>
            <not_before>2015-10-26T00:00:00</not_before>
            <not_after>2020-10-26T22:45:30</not_after>
          </validity>
          <allow_rule>
            <domains>
              <id>0</id>
            </domains>
          </allow_rule>
          <deny_rule>
            <domains>
              <id>0</id>
            </domains>
            <publish>
              <topics>
                <topic>Circle1</topic>
              </topics>
            </publish>
            <publish>
              <topics>
                <topic>Square</topic>
              </topics>
              <partitions>
                <partition>A_partition</partition>
              </partitions>
            </publish>
            <subscribe>
              <topics>
                <topic>Square1</topic>
              </topics>
            </subscribe>
            <subscribe>
              <topics>
                <topic>Tr*</topic>
              </topics>
              <partitions>
                <partition>P1*</partition>
              </partitions>
            </subscribe>
          </deny_rule>
          <default>DENY</default>
        </grant>
      </permissions>
    </dds>

.. _dds_security--dds-security-implementation-status:

**********************************
DDS Security Implementation Status
**********************************

..
    Sect<14.8>

The following DDS Security features are not implemented in OpenDDS.

#. Optional parts of the DDS Security v1.1 specification

   * Ability to write a custom plugin in C or in Java (C++ is supported)

   * Logging Plugin support

   * Built-in Logging Plugin

   * Data Tagging

#. Use of RTPS KeyHash for encrypted messages

   * OpenDDS doesn't use KeyHash, so it meets the spec requirements of not leaking secured data through KeyHash

#. Immutability of Publisher’s Partition QoS (see OMG Issue DDSSEC12-49)

#. Use of multiple plugin configurations (with different Domain Participants)

#. CRL (RFC 5280) and OCSP (RFC 2560) support

#. Certain plugin operations not used by built-in plugins may not be invoked by middleware

#. Origin Authentication

#. PKCS#11 for certificates, keys, passwords

#. Relay as a permissions “action” (Publish and Subscribe are supported)

#. Legacy matching behavior of permissions based on Partition QoS (9.4.1.3.2.3.1.4 in spec)

#. 128-bit AES keys (256-bit is supported)

#. Configuration of Built-In Crypto’s key reuse (within the DataWriter) and blocks-per-session

#. Signing (without encrypting) at the payload level, see OMG Issue DDSSEC12-59

