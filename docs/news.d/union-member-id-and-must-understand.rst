.. news-prs: 5300

.. news-start-section: Fixes
- Union ``MemberId`` numbering now reserves ID 0 for the discriminator and starts case members at 1, per XTypes 7.2.2.4.4.4.6.
- Union discriminators and ``@key`` struct members are now always marked must-understand, per XTypes 7.2.2.4.4.4.6 and 7.2.2.4.4.4.8 respectively.

  - *Backwards Compatibility Note*: The union member ID renumbering changes the equivalence hash and, for mutable/appendable unions, the wire representation of every union type. A reader and writer running OpenDDS versions on opposite sides of this change will see ``INCONSISTENT_TOPIC`` for any union type unless ``ignore_member_names`` is enabled on the ``TypeConsistencyEnforcementQosPolicy``.

.. news-end-section
