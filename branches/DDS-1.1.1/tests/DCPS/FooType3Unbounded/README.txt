This library is similar to the FooType3 lib except the data contains
unbounded sequence.


steps to build:
1) generate the export header which is committed into CVS.
  generate_export_file.pl FooLib > foolib_export.h

2) generate the implied IDL and servant implementations
  dcps_ts.pl --export=FooLib_Export --module=Mine Xyz::Foo FooDef.idl

  This will generate the FooTypeSupport.idl containing FooTypeSupport, 
  FooDataReader and FooDataWriter and also the FooTypeSupportImpl header
  and cpp files containing the servant implementations.

  The FooKeyLessThan will always return false, so the datatype map in 
  FooDataWriter just has a single instance handle. 



-----------------------------------------------------
generate_export_file.pl FooLib > foolib_export.h



