.. news-prs: 5317

.. news-start-section: Fixes
- Fixed ``opendds_idl`` generating wrong ``#include`` paths for included IDL files with ACE/TAO versions that pass IDL files directly to the preprocessor.
  ``--filename-only-includes`` now also applies to included files that aren't under an include path.
.. news-end-section
