.. news-prs: 4154

.. news-start-section: Additions
- Add ``-Gequality`` option to opendds_idl to generate ``==`` and ``!=`` for structs and unions.

  - The members of the struct or union must have a type that could appear in a DDS topic and be supported by opendds_idl.

  - The motivation for this change was to make the generated code more useful as many users go on to define these operators.

.. news-end-section
