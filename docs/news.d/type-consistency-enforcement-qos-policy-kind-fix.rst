.. news-prs: 5299

.. news-start-section: Fixes
- Fixed the values of the ``TypeConsistencyEnforcementQosPolicyKind_t`` constants: ``DISALLOW_TYPE_COERCION`` and ``ALLOW_TYPE_COERCION`` are now ``0`` and ``1``, as DDS-XTypes specifies, instead of ``1`` and ``2``.
  Other vendors misinterpreted a non-default ``TypeConsistencyEnforcementQosPolicy`` sent by OpenDDS DataReaders, and OpenDDS misinterpreted theirs.

  - *Backwards Compatibility Note*: OpenDDS still interprets the policy sent by DataReaders in older releases correctly, but those releases misinterpret the policy sent by OpenDDS DataReaders.
    Enable :cfg:prop:`[rtps_discovery]UseLegacyTypeConsistencyKind` when DataReaders with a non-default policy must match DataWriters in older releases.
.. news-end-section
