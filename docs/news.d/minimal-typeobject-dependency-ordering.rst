.. news-prs: 5307

.. news-start-section: Fixes
- Fixed building the complete-to-minimal ``TypeIdentifier`` map so a type is converted only after its dependencies.
  Converting a type too early logged a spurious ``complete TypeIdentifier not found`` error.
- Fixed an IDL file with a recursive type losing the complete-to-minimal ``TypeIdentifier`` mappings for all of its types.
  Recursive types themselves still have no mapping, and a warning now names them.
.. news-end-section
