This test is due to the fact the `GENERATED` property (and properties in general) are unable to be read from those outside of the directory setting them.

As a result, running this test with without the creation of the \_export.h file at makefile creation time would display the issue that a generated \_export.h file is not seen as generated and is therefore expected to already exist.

Explanation of issue: the top level CMakeLists delegates to the two subdirectories: idl and cpp. The idl directory creates the idl\_messenger library which is needed by the cpp directory. At makefile creation time, the cpp CMakeLists.txt "target\_link\_libraries section finds the idl\_messenger library and tries to add files that do not have the property `GENERATED` to the add\_executable section. Despite the fact the idl\_messenger directory had set the `GENERATED` property to true, the cpp directory is unable to access that property, and thus tries to add idl\_messenger\_export.h to the add\_executable.

By having the file created at makefile creation time, we have eliminated the dependency on the `GENERATED` flag, avoiding the problem.

These changes should not force the user to have to do anything, as they should be automatic in OpenDDS.

The issue corresponding to this problem is here: https://gitlab.kitware.com/cmake/cmake/-/issues/18399

As of 3/17/2021 it is unresolved.
