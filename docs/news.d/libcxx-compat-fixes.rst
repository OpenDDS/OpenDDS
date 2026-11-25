.. news-prs: 5316

.. news-start-section: Fixes
- Fixed ``DynamicType`` member lookups by name failing when built with libc++.
  libc++ ordered the members by address because it doesn't use TAO's ``std::less`` specialization for ``TAO::String_Manager``.
.. news-end-section
