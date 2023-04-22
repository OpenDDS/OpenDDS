.. news-prs: 4078
.. news-start-section: Additions
.. news-start-section: ``DynamicData``
- ``get_int64_value`` and ``get_uint64_value`` can now cast from different types
.. news-end-section
- Improved support for anonymous types in unions branches
.. news-end-section

.. news-start-section: Fixes
- XTypes

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

.. news-end-section

