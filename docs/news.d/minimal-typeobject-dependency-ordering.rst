.. news-prs: 5307

.. news-start-section: Fixes
- Fixed building the complete-to-minimal ``TypeIdentifier`` map so a type is converted only after its dependencies.
  Converting a type too early logged a spurious ``complete TypeIdentifier not found`` error.
.. news-end-section
