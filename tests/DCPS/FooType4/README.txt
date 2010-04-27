This FooType4 test is a test of both the dcps_ts.pl script and TAO_IDL
compiler working together.

steps to build:
1) generate the export header which is committed into CVS.
  generate_export_file.pl FooLib > foolib_export.h

2) generate the implied IDL and servant implementations
  dcps_ts.pl --export=FooLib_Export --module=Mine Xyz::Foo FooDef.idl

  This will generate the FooTypeSupport.idl containing FooTypeSupport, 
  FooDataReader and FooDataWriter and also the FooTypeSupportImpl header
  and cpp files containing the servant implementations.

  The FooDataReader also contains zero-copy method overloads.

3) To make idl compiler gerenated code have the FooKeyLessThan defined, 
   the FooDef.idl needs have following statements.

---in FooDef.idl
 #pragma DCPS_DATA_TYPE "Xyz::Foo"
 #pragma DCPS_DATA_KEY "Xyz::Foo key"


---in idl compiler generated FooDefC.h
struct FooType_Export FooKeyLessThan 
{
bool operator ()(const Xyz::Foo& v1, const Xyz::Foo& v2) const
{
return v1.key < v2.key;
}
};
FooType_Export CORBA::Boolean operator<< (OpenDDS::DCPS::Serializer &, const Xyz::Foo &);
FooType_Export CORBA::Boolean operator>> (OpenDDS::DCPS::Serializer &, Xyz::Foo &);




-----------------------------------------------------
generate_export_file.pl FooLib > foolib_export.h



