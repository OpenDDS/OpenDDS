This library is similar to the FooType3 lib except the data type does not
have a key.



  The FooKeyLessThan will always return false, so the datatype map in
  FooDataWriter just has a single instance handle.



---in FooDef.idl
 #pragma DCPS_DATA_TYPE "Xyz::Foo"
 // #pragma DCPS_DATA_KEY "Xyz::Foo key"


---in idl compiler generated FooDefTypeSupportImpl.h
struct FooType_Export FooKeyLessThan
{
bool operator ()(const Xyz::Foo& v1, const Xyz::Foo& v2) const
{
return false;
}
};


-----------------------------------------------------
generate_export_file.pl FooLib > foolib_export.h



