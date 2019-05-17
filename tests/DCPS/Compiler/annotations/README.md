# Topic Type Annotation Compiler Tests

These test the use of topic type IDL annotations, such as `@key`.
They are tested with the classic IDL C++ mapping in `classic_landmap` and the
IDL C++11 mapping in `cpp11_langmap`. The two should test the same things, with
the only difference being the language mapping.

The tests that start with `idl_test` share IDL with the tests that use
`#DCPS_DATA_*` in the parent directory. `topic_annotations_test` uses
`topic_annotations_test.idl` in this directory and is a test specific to
features of topic type annotations like using @key unions discriminators and
whole arrays.
