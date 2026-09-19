.. news-prs: 0

.. news-start-section: Fixes
- Fixed the wire encoding of ``long double`` (float128) values on platforms where the native ``long double`` is 80-bit x87 extended precision (e.g. Linux and Intel macOS x86-64). OpenDDS was copying the native x87 bytes directly to/from the wire with no format conversion; it now converts through a manual, portable bit-level conversion to genuine IEEE 754 binary128 at the wire boundary, which is what the CDR wire format actually requires. This does not depend on any compiler extension (e.g. GCC/Clang's ``__float128``, unavailable on MSVC and on Clang for Darwin/macOS), so it applies uniformly across compilers. Cross-validated against the compiler-intrinsic conversion this fix originally used, across 600,000+ generated values and wire patterns (denormals, infinities, NaNs, and the round-to-nearest-even boundary cases), on this platform; other x87-based platforms/compilers were not separately built or tested.

  - *Backwards Compatibility Note*: This changes the wire bytes OpenDDS sends for ``long double``/float128 values. Two OpenDDS peers both before or both after this fix will continue to interoperate correctly with each other; a peer on one side of the fix communicating with a peer on the other side will not, since only one side will be emitting/expecting spec-correct bytes.
.. news-end-section
