.. news-prs: 4999

.. news-start-section: Additions
.. news-rank: 10
- IDL Maps are now supported by ``opendds_idl``.

  - We would like to thank tmayoff for contributing much of the work needed for this.
  - Usage example:

    .. code-block:: omg-idl

      @nested
      struct Item {
        string desc_name;
        uint32 count;
      };

      @topic
      struct Inventory {
        map<string, Item> items;
      };

  - They map to C++ ``std::map`` in both the classic IDL-to-C++ and IDL-to-C++11 mappings.
  - See :ref:`introduction--idl-compliance` for known limitations.
.. news-end-section
