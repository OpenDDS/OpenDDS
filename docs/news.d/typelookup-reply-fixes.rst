.. news-prs: 5319

.. news-start-section: Fixes
- Fixed the TypeLookup service sending an uninitialized reply when a request was addressed to a different instance name.
  Requesters could not match that reply to their request, so endpoints that needed a type lookup never matched.
- The TypeLookup service now also answers requests whose instance name carries only this participant's GUID prefix or an unknown (all zero) GUID, as some DDS implementations send.
.. news-end-section
