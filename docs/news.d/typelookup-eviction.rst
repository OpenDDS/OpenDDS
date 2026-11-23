.. news-prs: 0

.. news-start-section: Security
- ``TypeObject``\s received through the TypeLookup service are now removed once every participant that sent them has been removed, so a long-running process no longer keeps the types of every peer it has seen.
  Types used by a ``DynamicType`` the application got from ``DomainParticipant::get_dynamic_type`` are kept.
- Added :prop:`[common]DCPSMaxTypeObjectsPerParticipant` to limit how many ``TypeObject``\s are cached from each remote participant.
.. news-end-section
