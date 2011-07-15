Configuring the dissector using text files.

These are traditional *.ini files. The environment variable
"OPENDDS_DISSECTORS" is checked for a path to the directory containing
the desired *.ini files. The current directory is used if the env var is
not set. All *.ini files in the directory are processed.

The general form for configuration is:

[entityname]
name1 = value1
name2 = value2
...

The names are either elements of the dissectable entity, or metadata
describing how the entity is to be composed.  The metadata fields are
named with a dot

entityname.kind = kind identifier
entityname.typeid = "IDL:modules/typename:1.0"

... other kind-specific information
the name of remaining configuration values are either names of actual
struct fields, or are meta-data that is used to manage the parsing. The
meta-data values have the entity name prepended so as to not collide
with an otherwise valid generated field name.

A kind identifier is required and is one of:
"module"
"struct"
"sequence"
"array"
"enum"
"union"
"alias"

Modules are meta-data used to identify type names during parsing. They
aren't used during the actual sample dissection. Sample Dissection
assumes fully qualified names are stored.

Example module configuration:
[MyModule]
MyModule.kind = "module"

[MySubMod]
MySubMod.kind = "module"
MySubMod.module = "MyModule"

All types can have a parent module defined by adding a value:

entityname.module = "modulename"

where there must be some [modulename] section somewhere in the file.

Modules themselves can also contain a ".module" value to indicate
nesting. Only the direct parent need be mentioned. The fully qualified
name is always used when searching for type references. The named
module, and its parents are added to the entityname value to define a
fully qualified name.

Any of the configuration values that take a type name assume that an
unqualified name is scoped to the same module as the entity being
configured. Use the syntax "[::][modulelist::]typename" to specify an
alternative scoping for the type.

-----
Special values for the remaining types -
For kind = "struct":
entityname.order = "field1 field2 ... fieldn"
field1 = "type id"
field2 = "type id"
...
fieldn = "type id"

The special value entityname.order is necessary to ensure the fields are
evaluated in the correct order. The individual fields are then listed by
name giving the type for the field. The type is either a built-in type
or a type that is defined in this file, or in a previously evaluated file.

Example struct configuration:

struct PlanInfo {
   unsigned long flight_id1;
   unsigned long flight_id2;
   string flight_name;
   string tailno;
};

[PlanInfo]
PlanInfo.kind = "struct"
PlanInfo.typeid = "IDL:PlanInfo:1.0"
PlanInfo.order = "flight_id1 flight_id2 flight_name tailno"
flight_id1 = "unsigned long"
flight_id2 = "unsigned long"
flight_name = "string"
tailno = "string"

-----
For kind = "enum":
entityname.order = "value1, value2, ... valuen"
The only special configuration for the enumeration is to list the values
in their natural order.

Eample enum configuration:

enum MyEnum {one, two, three};

[MyEnum]
MyEnum.kind = "enum"
MyEnum.module = "MyModule"
MyEnum.typeid = "IDL:MyModule/MyEnum:1.0"
MyEnum.order = "one two three"

For kind = "sequence":
entityname.element = "type id"

The only special field for sequences is to identify the type of sequence
elements. Bounded sequences are treated the same as unbounded sequences
by the dissector so no size specification is necessary. It could be
added if other consumers of the configuration need it.

Example sequence configuration:

typedef sequence<LocationInfo> seqtest;

[seqtest]
seqtest.kind = "sequence"
seqtest.typeid = "IDL:seqtest:1.0"
seqtest.element = "LocationInfo"

------
For kind = "array":
entityname.element = "type id"
entityname.size = number

Arrays are similar to sequences in that they are ordered groups of
elements using the same type. Unlike sequences, the size of the array
must be provided in the configuration.

For kind = "alias":
entityname.base = "type id"

Specifies typedef to provide an alternative typename for a given base type.
The supplied base can be a built-in type or a previously defined type.

Example array configuration:

module MyModule {
   module SubModule {
     typedef ::PlanInfo planarray[10];
   }
}

[planarray]
playarray.kind = "array"
planarray.module = "SubModule"
planarray.typeid = "IDL:MyModule/SubModule/planarray:1.0"
planarray.element= "::PlanInfo"
planarray.size = 10

-----
For kind = "union"
entityname.order = "field1 field2 ... fieldn"
entityname.discriminator = "type id"
default.name = "name"
default.type = "type id"
field1.name = "name"
field1.type = "type id"
field2.name = "name"
field2.type = "type id"
...
fieldn.name = "name"
fieldn.type = "type id"

The order are stringified versions of each case value for the union. The
values have to be consistent for the type id defined by
"entityname.discriminator" but there is no verification of that by the
dissector.
For each case statement, there is a name and type id for that case. When
there are multiple cases for a single target, only the last field in the
range will have a name and type value.

If there is a default case, that is configured with the default.name and
default.typeid values.

Example Union configuration:

union unionTest switch (short) {
   case 1 : short one;
   case 2 :
   case 3 :
   case 5 :
   case 10: long gt_one;
   default: string all_else;
};

[unionTest]
unionTest.kind = "union"
unionTest.typeid = "IDL:unionTest:1.0"
unionTest.order = "1 2 3 5 10"
unionTest.discriminator = "short"
default.typeid = "string"
default.name = "all_else"
1.typeid = "short"
1.name = "one"
10.typeid = "long"
10.name = "gt_one"

