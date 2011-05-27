This FooType4 test is a test of both the dcps_ts.pl script and TAO_IDL
compiler working together.

steps to build:
1) generate the export header which is committed into CVS.
  generate_export_file.pl FooLib > foolib_export.h

2) generate the implied IDL and servant implementations
  dcps_ts.pl --export=FooLib_Export --module=Mine T1::Foo1 Foo1Def.idl
  dcps_ts.pl --export=FooLib_Export --module=Mine T2::Foo2 Foo2Def.idl
  dcps_ts.pl --export=FooLib_Export --module=Mine T3::Foo3 Foo3Def.idl

