.. news-prs: 5353

.. news-start-section: Security
- The TypeLookup service now checks what it receives before caching it:

  - A reply is only used if it comes from the participant the request was sent to, on the same secure or non-secure endpoint, and is the same kind of reply as the request.
  - A ``TypeObject`` is only cached if hashing it gives its ``TypeIdentifier``, and the members of a strongly connected component are only cached together once all of them match.
    This can be turned off with :prop:`[common]DCPSValidateTypeObjects`.
  - A mapping from a complete to a minimal ``TypeIdentifier`` is only cached if converting the complete ``TypeObject`` gives that minimal ``TypeIdentifier``.

  Requests for a strongly connected component no longer trust its length, so they can't make the service use excessive memory, and replies include all of its members or none.
.. news-end-section
