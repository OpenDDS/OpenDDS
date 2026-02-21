.. news-prs: 5176

.. news-start-section: Fixes
- Fixed a null pointer dereference in :ghfile:`dds/DCPS/XTypes/DynamicSample.cpp`
  and :ghfile:`dds/DCPS/XTypes/DynamicSample.h` when ``data_`` is null in
  ``DynamicSample::deserialize`` and ``DynamicSample::copy``, which could occur
  when deserializing samples from a participant being removed.
.. news-end-section
