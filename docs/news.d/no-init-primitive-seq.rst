.. news-prs: 5173

.. news-start-section: Additions

- As an optimization, all IDL mappings will now try to avoid zero-initializing memory of sequences of primitive types (ints, floats, etc) while deserializing a sample.

  - IDL-to-C++11 will require additional opt-in using :ref:`opendds_idl-opendds-no_init_before_deserialize`.

.. news-end-section
