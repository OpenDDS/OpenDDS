This test is due to the fact the GENERATED property (and properties in general) are unable to be read from those outside of the directory setting them.

As a result, running this test with "run\_cmake.sh" will display the issue that a generated \_export.h file is not seen as generated 
and is therefore expected to already exist.

Explanation of issue: the top level CMakeLists delegates to the two subdirectories: idl and cpp. The idl directory creates the idl\_messenger library which is needed by the cpp directory. At makefile creation time, the cpp CMakeLists.txt "target\_link\_libraries section finds the idl\_messenger library and tries to add files that do not have the property GENERATED to the add\_executable section. Despite the fact the idl\_messenger directory had set the GENERATED property to true, the cpp directory is unable to access that property, and thus tries to add idl\_messenger\_export.h to the add\_executable.

The cpp directory is unable to access the GENERATED property because only the that sets a source property is able to read it.

As a result, in order for this to function properly, the GENERATED property must be set on the file in the cpp directory. A macro has been created "OPENDDS\_MARK\_AS\_GENERATED" which will loop through all include\_directories and passed libraries and use those names to find the corresponding \_export.h files and mark them as GENERATED.  This will need to be called in every directory which adds a library with an \_export.h until CMake is changed to allow GENERATED to be a globally readable property.

Uncommenting the "OPENDDS\_MARK\_AS\_GENERATED" line will demonstate this macro fixing the issue.

The issue corresponding to this problem is here: https://gitlab.kitware.com/cmake/cmake/-/issues/18399

As of 3/17/2021 it is unresolved.
