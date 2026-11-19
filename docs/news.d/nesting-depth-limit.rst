.. news-prs: 5350

.. news-start-section: Security
- Limited how deeply nested received data can be, so deeply nested or recursive input can't exhaust the stack:

  - Generated deserialization, ``TypeIdentifier`` deserialization, dynamic data reading, and JSON input for dynamic data reject structs, unions, and collections nested deeper than :prop:`[common]DCPSMaxNestingDepth`, which defaults to 256.
  - Content filter and query expressions are limited to the same number of parentheses and ``AND``, ``OR``, and ``NOT`` operators.
  - Validating a received dynamic data sample now takes linear time in the number of collection elements instead of quadratic time.

  Applications with recursive types nested deeper than the default can raise :prop:`[common]DCPSMaxNestingDepth`.

.. news-end-section
