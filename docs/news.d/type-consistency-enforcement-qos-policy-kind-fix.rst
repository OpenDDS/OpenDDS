.. news-prs: 5299

.. news-start-section: Fixes

  - Corrected the numeric values of the ``TypeConsistencyEnforcementQosPolicyKind_t`` constants: ``DISALLOW_TYPE_COERCION`` and ``ALLOW_TYPE_COERCION`` were ``1``/``2``, an off-by-one from the spec-mandated ``0``/``1``.

  - *Backwards Compatibility Note*: This constant's numeric value is sent on the wire and used in OpenDDS's own assignability checks. The bug was invisible between two OpenDDS peers of any version (both sides always agreed with themselves), but a spec-compliant peer reading a non-default policy from an OpenDDS writer/reader, or OpenDDS reading a non-default policy from such a peer, previously misinterpreted it -- and a pre-fix OpenDDS peer talking to a post-fix one will now do the same, since the two no longer agree on the encoding.

.. news-end-section
