This is a test of both the dcps_ts.pl script and TAO_IDL compiler working 
together.

steps to build:
1) generate the export header
  generate_export_file.pl FooLib > foolib_export.h

2) generate the implied IDL and servant implementations
  dcps_ts.pl --export=FooLib_Export --module=Mine Xyz::Foo FooDef.idl
  
3) build the library
  make


This is just a test to see that the code compiles.
It does not execute a test driver.