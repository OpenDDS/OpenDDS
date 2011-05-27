This library is used by the FooTest3* to test opendds_idl and TAO_IDL compiler
working together.

3) To make idl compiler gerenated code have the FooKeyLessThan defined,
   the FooDef.idl needs have following statements.

---in FooDef.idl
 #pragma DCPS_DATA_TYPE "Xyz::Foo"
 #pragma DCPS_DATA_KEY "Xyz::Foo key"


---in idl compiler generated FooDefTypeSupportImpl.h
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



