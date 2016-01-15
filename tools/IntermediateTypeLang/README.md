Intermediate Type Language
==========================

Intermediate Type Language (ITL) is an attempt to provide a common
type system for serialization schemes in a machine-friendly format.

# Problem
A common practice for applications that transmit or store data is to
define the structure of the data in a neutral representation and then
generate types that provide a native representation of the data in a
particular programming language and code that can serialize and
deserialize those types.

There are three common problems with this approach.  First, the
neutral representation used to describe the data is often too heavy
for automated translation.  For example, IDL has a number of
user-friendly features that are not machine friendly.  Second, the
type system is often coupled to the serialization scheme.
Translating between serialization schemes leads to an N^2 problem of
matching types in different systems.  Third, serialization often
assumes that the source or target is a native object in a programming
language.  That is, the language-neutral type has a corresponding
concrete type in a programming language and the goal is to serialize
to and from a value of the concrete type.  This prevents potential
optimization where the data is left in a serialized form and
selectively deserialized as needed.

# Use Case

The primary use case for ITL is the development of translators that
convert from one serialization scheme to another.  The user provides a
description of the incoming/outgoing data using ITL.  The translator
uses the ITL description of the data to perform the appropriate
translation.  The translator may interpret the ITL at run-time or the
translator may be generated from ITL.

# Design Goals

1. General - ITL should support common types found in existing
   infrastructure such as IDL, FAST, Avro, Google Protocol Buffers,
   Thrift, etc.
2. Machine-friendly - ITL should be easy to generate, easy to parse,
   and easy to use once parsed.
3. Extensible - ITL should provide a means of annotating types with
   their intended use and external encoding-specific details, e.g., delta
   compression.

ITL is descriptive and not prescriptive.  The types that can be
described with ITL may be a subset or superset of the types that can
be described in another language.  If a tool cannot describe a type in
ITL, then ITL should not be used (and the user should be informed).
If a serializer or deserializer is given a type that cannot be
represented in that serialization scheme, then an appropriate failure
mode should be adopted.

# Scalar Types

- **int** - Represents an integral number.  An int has the number of
  bits needed to represent values of this type and a flag indicating
  if the values are unsigned.  If the number of bits is not present,
  then the values of this type may have arbitrary magnitude.  The
  unsigned flag is optional and assumed to be false.
- **float** - Represents a floating-point number.  A float has an
  optional model that describes the values represented by this
  type.
- **fixed** - Represents a fixed-point number.  A fixed has a base,
  the total number of digits, and a scale that indicates the number of
  digits after the decimal point.
- **string** - Represents a text sequence.

Integers, fixed-point numbers, and strings have an optional set of
name-value pairs and an optional flag indicating if values of this
type are constrained to the specified set of values.  A value in a
name-value pair is stored as a string for use as a union
discriminator.  If an integer, fixed-point, or string is used as a
discriminator, then the set of name-value pairs must be one-to-one.

# Compound Types

- **sequence** - Represents a homogenous sequence of values of a given
  type.  A sequence has has either:
  1. No size or capacity indicating a dynamic size.
  2. An integer size indicating a fixed size.
  3. An array of sizes indicating the size of each dimension.
  4. A capacity indicating a dynamic size but limit on the number of values in the sequence.
  The size setting is preferred to the capacity.  If the elements
  have a fixed size, then size and capacity can be used to
  pre-allocate buffers.
- **record** - A record represents a potentially heterogeneous sequence of
  named values.  A record is defined by a list of fields.  Each field
  has a name, a type, and an optional flag indicating if the field is
  optional.  The name of each field must be unique.
- **union** - A union represents a value from a finite set of types.
  A union has a discriminator type (int, fixed, string) that is used
  to determine the actual type and a non-empty set of fields.  A union
  field has a name, a type, and a set of labels of the discriminator
  type.  A label must correspond to a named value of the discriminator
  type.  The name of each field must be unique.  The pair-wise
  intersection of union field labels must be disjoint.  An empty set
  of labels means that this field is the default.
- **alias** - An alias for another type.  An alias has a name and type.

# Float Models

A float model refers to a specification for floating-point numbers.
When a model is specified for a floating-point type, it means that any
value of the corresponding type *may* be represented by an
implementation of the model.  An implementation is not restrained by
the model in its approach to encoding the number.  However,
implementations and users must be prepared to handle lossy conversions
and respond appropriately.

- "binary16" - IEEE 754 of same name
- "binary32" - IEEE 754 of same name
- "binary64" - IEEE 754 of same name
- "binary128" - IEEE 754 of same name
- "decimal32" - IEEE 754 of same name
- "decimal64" - IEEE 754 of same name
- "decimal128" - IEEE 754 of same name

# Annotations

Annotations provide a way to capture semantics about encoded data that
govern its use.  To illustrate
the first, consider the problem of serializing a set.  A serialized
set looks like a sequence.  However, when deserializing, the
translator should attempt to restore set semantics by using an
appropriate data type.  In this case, the sequence should
be annotated as a set.  Annotations also provide a way to record
details related to a particular encoding.  For example, FAST delta
compression assumes a know starting value and then sends updates to
that value.  In this case, the field containing the value should be
annotated with delta compression so that translators will know (and
can take advantage of) this fact.

Annotations are a set of key/value pairs where each key corresponds a
system.  For the set example, the key may be "semantic" and the value
may be { "preferredDataType" : "set" }.  For the delta compression
example, the key may be "FAST" and the value may be { "compression" :
"delta" }.  Value nesting is allowed to facilitate the creation of
ontologies for different systems.

# Implementation

ITL is written using JSON to achieve machine friendliness.  ITL
presents a self-contained representation of types.  There is no
facility from importing types from external resources.  There is no
direct support for inheritance.

# Grammar

The grammar is presented as a JSON/BNF hybrid.  Non-terminals are
capitalized (Root) and non-terminals are lower-case (int).
Terminals refer to JSON values with the same name.  The terminal
"value" represents any JSON value.  The construct ( ... )? represents
an optional group.

```
Root:
  { "types" : [ TypeDef ] }

TypeDef:
  { "kind" : "int" (, "bits" : integer)? (, "unsigned" : boolean)? (, "values" : Values)? (, "constrained" : boolean)? }
| { "kind" : "float" (, "model" : FloatModel)? }
| { "kind" : "fixed", "base" : integer, "digits" : integer, "scale" : integer (, "values" : Values)? (, "constrained" : boolean)? }
| { "kind" : "string" (, "values" : Values)? (, "constrained" : boolean)? }
| { "kind" : "sequence", "type" : Type  (,("size" : integer ) | ("size" : [ integer ] )? (, "capacity" : integer )? }
| { "kind" : "record", "fields" : [ Field ] }
| { "kind" : "union", "discriminator" : Type, "fields" : [ UnionField ] }
|  { "kind" : "alias", "name" : string, "type" : Type }

Type:
  string
| TypeDef

Field:
  { "name" : string, "type" : Type, ("optional" : boolean)? }

UnionField:
  { "name" : string, "type" : Type, "labels" : [ string ] }

FloatModel: "binary16" | "binary32" | "binary64" | "binary128" | "decimal32" | "decimal64" | "decimal128"

Values: JSON object where all field values are strings
```

Every JSON Object ({ ... }) has an optional note field ("note" : {
... }) for annotating the field, type, etc.
