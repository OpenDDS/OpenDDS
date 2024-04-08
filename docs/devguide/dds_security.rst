.. _dds_security:
.. _sec:

############
DDS Security
############

..
    Sect<14>

OpenDDS includes an implementation of the :ref:`DDS Security specification <spec-dds-security>`.
This allows participants to encrypt messages and to authenticate remote participants before engaging with them.

.. important::

  Library filename: ``OpenDDS_Security``

  MPC base project name: :ghfile:`\`\`opendds_security\`\` <MPC/config/opendds_security.mpb>`

  CMake target Name: :cmake:tgt:`OpenDDS::Security`

  :ref:`Initialization header <plugins>`: :ghfile:`dds/DCPS/security/BuiltInPlugins.h`

.. _dds_security--building-opendds-with-security-enabled:

**************************************
Building OpenDDS with Security Enabled
**************************************

OpenDDS isn't built with security enabled by default.
See :ref:`building-sec` for more information.

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

.. list-table::
   :header-rows: 1

   * - Term Group

     - References

   * - Public Key Cryptography (including Private Keys)

     - * `Public Key Cryptography <https://en.wikipedia.org/wiki/Public-key_cryptography>`__

       * `RSA <https://en.wikipedia.org/wiki/RSA_(cryptosystem)>`__

       * `Elliptic Curve Cryptography <https://en.wikipedia.org/wiki/Elliptic_curve_cryptography>`__

   * - Public Key Certificate

     - * `Public Key Certificate <https://en.wikipedia.org/wiki/Public_key_certificate>`__

       * `Certificate Authority <https://en.wikipedia.org/wiki/Certificate_authority>`__

       * `X.509 <https://en.wikipedia.org/wiki/X.509>`__

       * `PEM <https://en.wikipedia.org/wiki/Privacy-enhanced_Electronic_Mail>`__

   * - Signed Documents

     - * `Digital Signature <https://en.wikipedia.org/wiki/Digital_signature>`__

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

  - Contains a "subject name" which matches the participant certificate's Subject

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

   * Via API: ``TheServiceParticipant->set_security(true);`` or

   * Via config file: setting :cfg:prop:`DCPSSecurity` to ``1``.

.. _dds_security--dds-security-configuration-via-propertyqospolicy:

DDS Security Configuration via Property Qos Policy
==================================================

..
    Sect<14.5.1>

When the application creates a :term:`DomainParticipant`, the ``DomainParticipantQos`` passed to the ``create_participant()`` method contains :ref:`qos-property`, which has a sequence of name-value pairs.
The following properties must be included to enable security.
Except where noted, these values take the form of a URI starting with either the scheme ``file:`` followed by a filesystem path (absolute or relative) or the scheme ``data:``, followed by the literal data.

.. list-table::
   :header-rows: 1

   * - Name

     - Value

     - Notes

   * - ``dds.sec.auth.identity_ca``

     - Certificate PEM file

     - Can be the same as ``permissions_ca``

   * - ``dds.sec.access.permissions_ca``

     - Certificate PEM file

     - Can be the same as ``identity_ca``

   * - ``dds.sec.access.governance``

     - Signed XML (.p7s)

     - Signed by ``permissions_ca``

   * - ``dds.sec.auth.identity_certificate``

     - Certificate PEM file

     - Signed by ``identity_ca``

   * - ``dds.sec.auth.private_key``

     - Private Key PEM file

     - Private key for ``identity_certificate``

   * - ``dds.sec.auth.password``

     - Private Key Password (not a URI)

     - Optional, Base64 encoded

   * - ``dds.sec.access.permissions``

     - Signed XML (.p7s)

     - Signed by ``permissions_ca``

.. _dds_security--propertyqospolicy-example-code:

Example Code
------------

..
    Sect<14.5.2>

Below is an example of code that sets the Participant QoS's :ref:`qos-property` in order to configure DDS Security.

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

The "subject_name" element for a signed permissions XML document must match the "Subject:" field provided by the accompanying Identity Certificate which is transmitted during participant discovery, authentication, and authorization.
This ensures that the permissions granted by the Permissions CA do, in fact, correspond to the identity provided.

.. _dds_security--examples-in-the-opendds-source-code-repository:

Examples in the OpenDDS Source Code Repository
==============================================

..
    Sect<14.5.5>

Examples to demonstrate how the DDS Security features are used with OpenDDS can be found in the OpenDDS GitHub repository.

The following table describes the various examples and where to find them in the source tree.

.. list-table::
   :header-rows: 1

   * - **Example**

     - **Source Location**

   * - C++ application that configures security QoS policies via command-line parameters

     - :ghfile:`tests/DCPS/Messenger/publisher.cpp`

   * - Identity CA Certificate (along with private key)

     - :ghfile:`tests/security/certs/identity/identity_ca_cert.pem`

   * - Permissions CA Certificate (along with private key)

     - :ghfile:`tests/security/certs/permissions/permissions_ca_cert.pem`

   * - Participant Identity Certificate (along with private key)

     - :ghfile:`tests/security/certs/identity/test_participant_01_cert.pem`

   * - Governance XML Document (alongside signed document)

     - :ghfile:`tests/DCPS/Messenger/governance.xml`

   * - Permissions XML Document (alongside signed document)

     - :ghfile:`tests/DCPS/Messenger/permissions_1.xml`

.. _dds_security--using-openssl-utilities-for-opendds:

Using OpenSSL Utilities for OpenDDS
===================================

..
    Sect<14.5.6>

To generate certificates using the ``openssl`` command, a configuration file ``openssl.cnf`` is required (see below for example commands).
Before proceeding, it may be helpful to review OpenSSL's man pages to get help with the file format.
In particular, configuration file format and ca command's documentation and configuration file options.

An example OpenSSL CA-Config file used in OpenDDS testing can be found here: :ghfile:`tests/security/certs/identity/identity_ca_openssl.cnf`

.. _dds_security--creating-self-signed-certificate-authorities:

Creating Self-Signed Certificate Authorities
--------------------------------------------

..
    Sect<14.5.6.1>

Generate a self-signed 2048-bit RSA CA:

.. code-block:: bash

    openssl genrsa -out ca_key.pem 2048
    openssl req -config openssl.cnf -new -key ca_key.pem -out ca.csr
    openssl x509 -req -days 3650 -in ca.csr -signkey ca_key.pem -out ca_cert.pem

Generate self-signed 256-bit Elliptic Curve CA:

.. code-block:: bash

    openssl ecparam -name prime256v1 -genkey -out ca_key.pem
    openssl req -config openssl.cnf -new -key ca_key.pem -out ca.csr
    openssl x509 -req -days 3650 -in ca.csr -signkey ca_key.pem -out ca_cert.pem

.. _dds_security--creating-signed-certificates-with-an-existing-ca:

Creating Signed Certificates with an Existing CA
------------------------------------------------

..
    Sect<14.5.6.2>

Generate a signed 2048-bit RSA certificate:

.. code-block:: bash

    openssl genrsa -out cert_1_key.pem 2048
    openssl req -new -key cert_1_key.pem -out cert_1.csr
    openssl ca -config openssl.cnf -days 3650 -in cert_1.csr -out cert_1.pem

Generate a signed 256-bit Elliptic Curve certificate:

.. code-block:: bash

    openssl ecparam -name prime256v1 -genkey -out cert_2_key.pem
    openssl req -new -key cert_2_key.pem -out cert_2.csr
    openssl ca -config openssl.cnf -days 3650 -in cert_2.csr -out cert_2.pem

.. _dds_security--signing-documents-with-smime:

Signing Documents with SMIME
----------------------------

..
    Sect<14.5.6.3>

Sign a document using existing CA & CA private key:

.. code-block:: bash

    openssl smime -sign -in doc.xml -text -out doc_signed.p7s -signer ca_cert.pem -inkey ca_private_key.pem

.. _dds_security--common-xml:

*******************
Common XML Elements
*******************

These are elements that are common to all the XML documents.

.. _dds_security--domains:

Domain Id Set
=============

A list of domain ids and/or domain id ranges of domains impacted by the current domain rule.
This is the type of ``domains`` in the :ref:`governance document <dds_security--gov-domains>` and in the :ref:`permissions document <dds_security--perm-domains>`.

The set is made up of ``<id>`` tags or ``<id_range>`` tags.
An ``<id>`` tag simply contains the domain id that are part of the set.
An ``<id_range>`` tag can be used to add multiple ids at once.
It must contain a ``<min>`` tag to say where the range starts and may also have a ``<max>`` tag to say where the range ends.
If the ``<max>`` tag is omitted then the set includes all valid domain ids starting at ``<min>``.

If the domain rule or permissions grant should to apply to all domains, use the following:

.. code-block:: xml

    <domains>
      <id_range><min>0</min></id_range>
    </domains>

If there's a need to be selective about what domains are chosen, here's an annotated example:

.. code-block:: xml

    <domains>
      <id>2</id>
      <id_range><min>4</min><max>6</max></id_range> <!-- 4, 5, 6 -->
      <id_range><min>10</min></id_range> <!-- 10 and onward -->
    </domains>

.. _dds_security--domain-governance-document:

**************************
Domain Governance Document
**************************

..
    Sect<14.6>

The signed governance document is used by the DDS Security built-in access control plugin in order to determine both per-domain and per-topic security configuration options for specific domains.
For full details regarding the content of the governance document, see :omgspec:`sec:9.4.1.2`.

.. _dds_security--global-governance-model:

Global Governance Model
=======================

..
    Sect<14.6.1>

It's worth noting that the DDS Security Model expects the governance document to be globally shared by all participants making use of the relevant domains described within the governance document.
Even if this is not the case, the local participant will verify incoming authentication and access control requests as if the remote participant shared the same governance document and accept or reject the requests accordingly.

.. _dds_security--key-governance-elements:

Key Governance Elements
=======================

..
    Sect<14.6.2>

The following types and values are used in configuring both per-domain and per-topic security configuration options.
We summarize them here to simplify discussion of the configuration options where they're used, found below.

.. _dds_security--boolean:

Boolean
-------

A boolean value indicating whether a configuration option is enabled or not.
Recognized values are: ``TRUE``/``true``/``1`` and ``FALSE``/``false``/``0``

.. _dds_security--protection-kind:

Protection Kind
---------------

The method used to protect domain data (message signatures or message encryption) along with the ability to include origin authentication for either protection kind.

Recognized values are:

- ``NONE``
- ``SIGN``
- ``ENCRYPT``
- ``SIGN_WITH_ORIGIN_AUTHENTICATION``
- ``ENCRYPT_WITH_ORIGIN_AUTHENTICATION``

.. attention::

  Currently, OpenDDS doesn't implement origin authentication.
  So while the ``_WITH_ORIGIN_AUTHENTICATION`` options are recognized, the underlying configuration is unsupported.

.. _dds_security--basic-protection-kind:

Basic Protection Kind
---------------------

The method used to protect domain data (message signatures or message encryption).
Recognized values are ``NONE``, ``SIGN``, and ``ENCRYPT``

.. _dds_security--domain-rule-configuration-options:

Domain Rule Configuration Options
=================================

..
    Sect<14.6.3>

The following XML elements are used to configure domain participant behaviors.

.. _dds_security--gov-domains:

domains
-------

A :ref:`dds_security--domains` of domains impacted by the current domain rule.

.. _dds_security--allow-unauthenticated-participants:

allow_unauthenticated_participants
----------------------------------

A :ref:`dds_security--boolean` value which determines whether to allow unauthenticated participants for the current domain rule

.. _dds_security--enable-join-access-control:

enable_join_access_control
--------------------------

A :ref:`dds_security--boolean` value which determines whether to enforce domain access controls for authenticated participants

.. _dds_security--discovery-protection-kind:

discovery_protection_kind
-------------------------

The discovery protection element specifies the :ref:`dds_security--protection-kind` used for the built-in DataWriter(s) and DataReader(s) used for secure endpoint discovery messages

.. _dds_security--liveliness-protection-kind:

liveliness_protection_kind
--------------------------

The liveliness protection element specifies the :ref:`dds_security--protection-kind` used for the built-in DataWriter and DataReader used for secure liveliness messages

.. _dds_security--rtps-protection-kind:

rtps_protection_kind
--------------------

Indicate the :ref:`dds_security--protection-kind` for the whole RTPS message.
Very little RTPS data exists outside the "metadata protection" envelope (see topic rule configuration options), and so for most use cases topic-level "data protection" or "metadata protection" can be combined with discovery protection and/or liveliness protection in order to secure domain data adequately.
One item that is not secured by "metadata protection" is the timestamp, since RTPS uses a separate InfoTimestamp submessage for this.
The timestamp can be secured by using ``rtps_protection_kind``

.. _dds_security--topic-rule-configuration-options:

Topic Rule Configuration Options
================================

..
    Sect<14.6.4>

The following XML elements are used to configure topic endpoint behaviors:

.. _dds_security--topic-expression:

topic_expression
----------------

A :ref:`fnmatch expression <fnmatch-exprs>` of the topic names to match.
A default rule to catch all previously unmatched topics can be made with: ``<topic_expression>*</topic_expression>``

.. _dds_security--enable-discovery-protection:

enable_discovery_protection
---------------------------

A :ref:`dds_security--boolean` to enable the use of secure discovery protections for matching user topic announcements.

.. _dds_security--enable-read-access-control:

enable_read_access_control
--------------------------

A :ref:`dds_security--boolean` to enable the use of access control protections for matching user topic DataReaders.

.. _dds_security--enable-write-access-control:

enable_write_access_control
---------------------------

A :ref:`dds_security--boolean` to enable the use of access control protections for matching user topic DataWriters.

.. _dds_security--metadata-protection-kind:

metadata_protection_kind
------------------------

Specifies the :ref:`dds_security--protection-kind` used for the RTPS SubMessages sent by any DataWriter and DataReader whose associated Topic name matches the rule's topic expression.

.. _dds_security--data-protection-kind:

data_protection_kind
--------------------

Specifies the :ref:`dds_security--basic-protection-kind` used for the RTPS SerializedPayload SubMessage element sent by any DataWriter whose associated Topic name matches the rule's topic expression.

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
For full details regarding the content of the permissions document, see :omgspec:`sec:9.4.1.3`.

.. _dds_security--key-permissions-elements:

Key Permissions Elements
========================

..
    Sect<14.7.1>

Each permissions file consists of one or more permissions grants.
Each grant bestows access control privileges to a single subject name for a limited validity period.

.. _dds_security--subject-name:

subject_name
------------

This is a X.509 subject name field.
In order for permissions checks to successfully validate for both local and remote participants, the supplied identity certificate subject name must match the subject name of one of the grants included in the permissions file.

This will look something like:

.. code-block:: xml

  <subject_name>emailAddress=cto@acme.com, CN=DDS Shapes Demo, OU=CTO Office, O=ACME Inc., L=Sunnyvale, ST=CA, C=US</subject_name>

.. versionchanged:: 3.25.0

  The order of attributes in subject names is now significant.

.. _dds_security--validity:

validity
--------

Each grant's validity section contains a start date and time (``<not_before>``) and an end date and time (``<not_after>``) to indicate the period of time during which the grant is valid.

The format of the date and time, which is like `ISO-8601 <https://en.wikipedia.org/wiki/ISO_8601>`__, must take one of the following forms:

#. ``YYYY-MM-DDThh:mm:ss``

   Example: ``2020-10-26T22:45:30``

#. ``YYYY-MM-DDThh:mm:ssZ``

   Example:``2020-10-26T22:45:30Z``

#. ``YYYY-MM-DDThh:mm:ss+hh:mm``

   Example:``2020-10-26T23:45:30+01:00``

#. ``YYYY-MM-DDThh:mm:ss-hh:mm``

   Example:``2020-10-26T16:45:30-06:00``

All fields shown must include leading zeros to fill out their full width, as shown in the examples.
``YYYY-MM-DD`` is the date and ``hh:mm:ss`` is the time in 24-hour format.
The date and time must be able to be represented by the ``time_t`` (C standard library) type of the system.
The seconds field can also include a variable length fractional part, like ``00.0`` or ``01.234``, but it will be ignored because ``time_t`` represents a whole number of seconds.
Examples #1 and #2 are both interpreted using UTC.
To put the date and time in a local time, a time zone offset can to be added that says how far the local timezone is ahead of (using ``+`` as in example #3) or behind (using ``-`` as in example #4) UTC at that date and time.

.. _dds_security--allow-rule-and-deny-rule:

allow_rule and deny_rule
------------------------

Grants will contain one or more allow / deny rules to indicate which privileges are being applied.
When verifying that a particular operation is allowed by the supplied grant, rules are checked in the order they appear in the file.
If the domain, partition, and (when implemented) data tags for an applicable topic rule match the operation being verified, the rule is applied (either allow or deny).
Otherwise, the next rule is considered.
Special Note: If a grant contains any allow rule that matches a given domain (even one with no publish / subscribe / relay rules), the grant may be used to join a domain with join access controls enabled.

.. _dds_security--perm-domains:

domains
^^^^^^^

Every allow or deny rule must contain a set of domain ids to which it applies.
The syntax is the same as the domain id set found in the governance document.
See :ref:`dds_security--domains` for details.

.. _dds_security--psr-rules:

publish, subscribe, and relay Rules (PSR rules)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Every allow or deny rule may optionally contain a list of publish, subscribe, or relay rules bestowing privileges to publish, subscribe, or relay data (respectively).
Each rule applies to a collection of topics in a set of partitions with a particular set of data tags.
As such, each rule must then meet these three conditions (topics, partitions, and (when implemented) data tags) in order to apply to a given operation.
These conditions are governed by their relevant subsection, but the exact meaning and default values will vary depending on the both the PSR type (publish, subscribe, relay) as well as whether this is an allow rule or a deny rule.
Each condition is summarized below.
See the DDS Security specification for full details.
OpenDDS does not currently support relay-only behavior and consequently ignores allow and deny relay rules for both local and remote entities.
Additionally, OpenDDS does not currently support data tags, and so the data tag condition applied is always the "default" behavior described below.

.. _dds_security--topics:

topics
""""""

The list of topics and/or topic expressions for which a rule applies.
Topic names and expressions are matched using :ref:`fnmatch-exprs`.
If the triggering operation matches any of the topics listed, the topic condition is met.
The topic section must always be present for a PSR rule, so there there is no default behavior.

.. _dds_security--partitions:

partitions
""""""""""

The partitions list contains the set of partition names for which the parent PSR rule applies.
Similarly to topics, partition names and expressions are matched using :ref:`fnmatch-exprs`.
For "allow" PSR rules, the DDS entity of the associated triggering operation must be using a strict subset of the partitions listed for the rule to apply.
When no partition list is given for an "allow" PSR rule, the "empty string" partition is used as the default value.
For "deny" PSR rules, the rule will apply if the associated DDS entity is using any of the partitions listed.
When no partition list is given for a "deny" PSR rule, the wildcard expression "*" is used as the default value.

.. _dds_security--data-tags:

data_tags
"""""""""

.. attention::

  Data tags are an optional part of the DDS Security specification and are not currently implemented by OpenDDS.
  If they were implemented, the condition criteria for data tags would be similar to partitions.

For "allow" PSR rules, the DDS entity of the associated triggering operation must be using a strict subset of the data tags listed for the rule to apply.
When no data tag list is given for an "allow" PSR rule, the empty set of data tags is used as the default value.
For "deny" PSR rules, the rule will apply if the associated DDS entity is using any of the data tags listed.
When no data tag list is given for a "deny" PSR rule, the set of "all possible tags" is used as the default value.

.. _dds_security--psr-validity:

validity
""""""""

.. attention::

   This is an OpenDDS extension.

This structure defines the validity of a particular publish or subscribe action.
Thus, it is possible to declare that an action is valid for some subset of the grant's validity.
The format for `validity` is the same as :ref:`dds_security--validity`.

.. _dds_security--default_rule:

default_rule
^^^^^^^^^^^^

The default rule is the rule applied if none of the grant's allow rules or deny rules match the incoming operation to be verified.
Recognized values are ``ALLOW`` and ``DENY``.

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

#. Immutability of Publisher's Partition QoS, see :omgissue:`DDSSEC12-49`

#. Use of multiple plugin configurations (with different Domain Participants)

#. CRL (:rfc:`5280`) and OCSP (:rfc:`2560`) support

#. Certain plugin operations not used by built-in plugins may not be invoked by middleware

#. Origin Authentication

#. PKCS#11 for certificates, keys, passwords

#. Relay as a permissions "action" (Publish and Subscribe are supported)

#. :omgspec:`Legacy matching behavior of permissions based on Partition QoS <sec:9.4.1.3.2.3.1.4>`

#. 128-bit AES keys (256-bit is supported)

#. Configuration of Built-In Crypto's key reuse (within the DataWriter) and blocks-per-session

#. Signing (without encrypting) at the payload level, see :omgissue:`DDSSEC12-59`

The following features are OpenDDS extensions:

#. Validity of publish/subscribe actions :ref:`dds_security--psr-validity`.
