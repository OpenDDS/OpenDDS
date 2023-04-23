..
  The following means don't show subsections like "Additions" in local table of contents

:tocdepth: 2

########
Releases
########

These are all the recent releases of OpenDDS.

.. ifconfig:: not is_release

  .. include:: wip_release.rst

..
  NEW NEWS GOES BELOW HERE

*******
v3.24.1
*******

Released 2023-04-22

Download :ghrelease:`this release on GitHub <DDS-3.24.1>`.

Read `the documenation for this release on Read the Docs <https://opendds.readthedocs.io/en/dds-3.24.1>`__.

Fixes
=====

- Fixed compile warnings in TypeSupport that can happen with GCC and ``-O2`` or higher (:ghpr:`4117`)
- Fixed compile error in TypeSupport for IDL that contains a typedef of a typedef (:ghpr:`4117`)
- Fixed bug in the tcp transport where readers and writers can fail to associate (:ghpr:`4120`)
- Fixed issue in some headers that could leak ``#pragma pack (push, 8)`` into user code on Visual Studio (:ghpr:`4123`)
- Fixed theoretical infinite loop in rtps_udp transport code (:ghpr:`4124`)

Documentation
=============

- Removed invalid links and references in README and the Developer's Guide and fixed other minor issues (:ghpr:`4115`, :ghpr:`4116`, :ghpr:`4121`, :ghpr:`4126`)
- Changed theme used by the Sphinx documentation to make the Developer's Guide easier to navigate (:ghpr:`4127`)
- Added copy buttons to embedded code and code-like examples (:ghpr:`4127`)

*******
v3.24.0
*******

Released 2023-04-11

Download :ghrelease:`this release on GitHub <DDS-3.24>`.

Read `the documenation for this release on Read the Docs <https://opendds.readthedocs.io/en/dds-3.24>`__.

Additions
=========

- The OpenDDS Developer's Guide is now available at https://opendds.readthedocs.io/ (:ghpr:`4100`, :ghpr:`4101`, :ghpr:`4102`, :ghpr:`4103`, :ghpr:`4104`, :ghpr:`4105`, :ghpr:`4051`, :ghpr:`4092`, :ghpr:`4094`, :ghpr:`4095`)

  - The Sphinx/reStructuredText source for this new format is now located in the repo at :ghfile:`docs/devguide`

- DOCGroup ACE6/TAO2 is now the default ACE/TAO for OpenDDS, OCI ACE/TAO is no longer supported (:ghpr:`4069`)
- Dynamic content subscription (:ghpr:`3988`)

  - This allows ``DynamicDataReader``\s to use ``QueryCondition`` and ``ContentFilteredTopic`` and allows ``DynamicDataWriter``\s to do filtering on behalf of matched ``DataReader``\s that use ``ContentFilteredTopic``.

- ``DynamicData``

  - Can now read and write enum members as strings (:ghpr:`4022`)
  - ``DynamicDataImpl`` now uses lazy initialization to reduce memory usage (:ghpr:`4024`)
  - ``get_int64_value`` and ``get_uint64_value`` can now cast from different types (:ghpr:`4078`)

- Added aliases for IDL types from XTypes spec such as ``DDS::UInt32`` (:ghpr:`3394`)

  - See :ghfile:`dds/DdsDcpsCore.idl` for all of them.

- Added PublicationMatchedStatus Current Count To RtpsRelay Statistics (:ghpr:`4006`)
- Allow reassembly of overlapping fragment ranges in RTPS (:ghpr:`4035`, :ghpr:`4047`)
- Added hardening features to RtpsRelay (:ghpr:`4045`)

  - These are configured with the new options ``-MaxAddrSetSize`` and ``-RejectedAddressDuration``.

- Can now cross-compile on macOS (:ghpr:`4048`)
- Expanded support for using C++ keywords in IDL (:ghpr:`4073`)
- IDL file and generated TypeSupport.idl can now be in different directories (:ghpr:`4077`)
- Improved support for anonymous types in unions branches (:ghpr:`4078`)

Fixes
=====

- Fixed ``rtps_relay_address_change`` deadlocks (:ghpr:`3989`)
- Fixed RtpsUdpTransport data race from ``relay_stun_mutex_`` (:ghpr:`3990`)
- Fixed invalid socket handles in RtpsUdpTransport (:ghpr:`4002`)
- Fixed index increment in ``GuidPartitionTable::prepare_relay_partitions`` (:ghpr:`4005`)
- Improved reliability of the shared memory transport (:ghpr:`4028`)
- Fixed a bug in content filtering with enum comparisons on serialized samples (:ghpr:`4038`)
- Secure writers and readers in same participant can now associate (:ghpr:`4041`)
- Fixed transport config and transport instance derived from template conflicting (:ghpr:`4058`)
- Fixed issue with using ``-o`` in ``tao_idl``/``opendds_idl`` options in ``OPENDDS_TARGET_SOURCES`` and those directories are now automatically included (:ghpr:`4071`)
- XTypes (:ghpr:`4078`)

  - ``TypeObject``\s struct and union members used to be sorted by member ID, but they are now sorted by declaration order as the XTypes spec calls for.

    By default member IDs increment starting at 0, and in that case the ``TypeObject``\s will be the same.
    If ``@autoid(hash)``, ``--default-autoid hash``, or ``@id(ID)`` are being used then the order could be different.
    This could cause some reader/writer matching incompatibility with older versions of OpenDDS:

    - Topics with final and appendable structs will no longer match.
    - If ``DISALLOW_TYPE_COERCION`` QoS is being used, then all topics where the order differ will not longer match.
      Note that this is true for any time the type hash changes.
    - Pass the ``--old-typeobject-member-order`` option to ``opendds_idl`` to use the non-standard order.

  - The size of XCDR2 member parameters in mutable structs and unions is now correctly interpreted when the "length code" is 5, 6, or 7.

    - This is an optimization that OpenDDS doesn't serialize samples with, so this could only be an issue when dealing with samples from other DDS implementations.

  - ``DynamicDataImpl`` (``DynamicData`` made by ``DynamicDataFactory`` that can be passed to ``DynamicDataWriter``):

    - ``get_member_id_at_index`` now returns ids for members that haven't been initialized yet.
    - Fixed incorrect serialization of keyed unions for instance registration, disposal, and unregistration samples.
    - Fixed errors from serializing some cases of arrays and sequences.

Notes
=====

- Release files will only be uploaded to GitHub from now on
- ``OpenDDS::DCPS::RepoId`` has been removed, if needed use ``OpenDDS::DCPS::GUID_t`` instead (:ghpr:`3972`)
