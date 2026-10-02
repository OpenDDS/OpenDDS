.. news-prs: 5300

.. news-start-section: Fixes
- Union ``MemberId`` numbering now reserves ID 0 for the discriminator and starts case members at 1, per XTypes 7.2.2.4.4.3 and 7.3.1.2.1.1.

  - *Backwards Compatibility Note*: This changes the equivalence hash of every union type and the wire representation of mutable unions.
    A reader and writer on opposite sides of this change will see ``INCONSISTENT_TOPIC`` for union types unless ``ignore_member_names`` is enabled in the ``TypeConsistencyEnforcementQosPolicy``.

- Union discriminators and ``@key`` struct members are now always marked must-understand, per XTypes 7.2.2.4.4.4.6 and 7.2.2.4.4.4.8 respectively.
  This changes the equivalence hash of keyed struct types, but they remain assignable across versions.

.. news-end-section
