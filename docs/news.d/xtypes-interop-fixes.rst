.. news-prs: 0

.. news-start-section: Fixes
- DDS XTypes Interoperability: Implemented fixes and completeness updates to align the XTypes implementation with the OMG DDS-XTypes 1.3 specification:

  - **Type Assignability**: Enforced checking for sequence and string bounds, structural member validation under the ``prevent_type_widening`` policy, and union default/explicit case label compatibility rules.

    *Backwards Compatibility Note*: If reader endpoints are configured with non-default QoS attributes (``ignore_sequence_bounds = false``, ``ignore_string_bounds = false``, or ``prevent_type_widening = true``), the assignability checks will now correctly reject matching writers that violate these constraints. Previously, these settings were ignored, meaning upgraded readers might now reject writers they previously matched.

  - **TryConstruct Annotation Support**: Enforced try-construct policies (``TRIM``, ``USE_DEFAULT``, ``DISCARD``) on sequences, strings, and enums during runtime deserialization, ensuring correct behavior when deserializing samples that exceed bounds or contain invalid enum literals.

  - **TypeLookup & Discovery**: Pre-populated complete-to-minimal type mappings during type registration and discovery, resolving queries to the TypeLookupService.

  - **Deterministic Serialization**: Canonicalized floating-point padding bytes for 128-bit floats (``float128`` / ``long double``) on platforms with 80-bit x87 representation (e.g., Linux x86_64), ensuring consistent network serialization, hashing, and JSON output.

.. news-end-section
