# This will have to be replaced with the PR number after the PR is created:
.. news-prs: 0

.. news-start-section: Security
- The XTypes DynamicData reader now rejects a sample whose mutable struct carries an unrecognized member flagged must-understand, instead of skipping it (XTypes 1.3 section 7.6.3).

  - Applies to XCDR1 and XCDR2, including mutable structs nested inside mutable structs.
  - Mutable unions and final/appendable nesting are not yet covered.

.. news-end-section
