This FooType5 test is the library that provide the Foo datatype and FooNoKey
data type to support the system test.

steps to build:
1) generate the export header which is committed into CVS.
  generate_export_file.pl FooLib > foolib_export.h

2) generate the implied IDL and servant implementations
  dcps_ts.pl --export=FooLib_Export --module=Mine Xyz::Foo FooDef.idl
  dcps_ts.pl --export=FooLib_Export --module=Mine Xyz::FooNoKey FooDef.idl

  This will generate the FooTypeSupport.idl/FooNoKeyTypeSupport.idl containing
  FooTypeSupport/FooNoKeyTypeSupport, 
  FooDataReader/FooNoKeyDataReader and FooDataWriter/FooNoKeyDataWriter and also
  the FooTypeSupportImpl/FooNoKeyTypeSupportImpl header
  and cpp files containing the servant implementations.

  The FooDataReader/FooNoKeyDataReader also contains zero-copy method overloads.





