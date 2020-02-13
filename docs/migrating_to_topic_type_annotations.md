# Migrating to Topic Type Annotations

In OpenDDS 3.14, we have deprecated the `#pragma DCPS_DATA_TYPE` and `#pragma
DCPS_DATA_KEY` preprocessor statements in favor of IDL 4 annotations. This
document will just give advice on how to migrate existing IDL to use topic type
annotations. It is recommended to also read the "Defining the Data Types"
section in the OpenDDS Developer's Guide, where topic type annotations are
covered in more detail.

**Table of Contents:**

* [Basic Example](#basic-example)
  * [Backwards Compatible Example](#backwards-compatible-example)
* [Array Keys](#array-keys)
* [Implied Keys](#implied-keys)
* [Suppressing Deprecation Warnings](#suppressing-deprecation-warnings)

## Basic Example

The following IDL is a basic example of using the old preprocessor statements
to declare topic types and keys.

```
#pragma DCPS_DATA_TYPE "BasicType"
#pragma DCPS_DATA_KEY "BasicType value"
struct BasicType {
  long value;
};
```

It can be rewritten with annotations as:

```
@topic
struct BasicType {
  @key long value;
};
```

### Backwards Compatible Example

If you need the IDL to be backwards compatible with an older version of
OpenDDS, you can make use of the `__OPENDDS_IDL_HAS_ANNOTATIONS` macro defined
by `opendds_idl`.

```
#ifdef __OPENDDS_IDL_HAS_ANNOTATIONS
#  define KEY @key
#else
#  define KEY
#endif

#ifdef __OPENDDS_IDL_HAS_ANNOTATIONS
@topic
#else
#  pragma DCPS_DATA_TYPE "BasicType"
#  pragma DCPS_DATA_KEY "BasicType value"
#endif
struct BasicType {
  KEY long value;
};
```

## Array Keys

In the old system you had to specify every element in an array to make all the
elements part of the key:

```
#pragma DCPS_DATA_TYPE "ArrayType"
#pragma DCPS_DATA_KEY "ArrayType array[0]"
#pragma DCPS_DATA_KEY "ArrayType array[1]"
#pragma DCPS_DATA_KEY "ArrayType array[2]"
#pragma DCPS_DATA_KEY "ArrayType array[3]"
struct ArrayType {
  long array[4];
};
```

That is no longer necessary, as applying `@key` to an array will do that same
thing.

```
@topic
struct ArrayType {
  @key long array[4];
};
```

**NOTE:** If you relied on the old behavior to make selective elements part of
the key, this will not work with annotations. We would suggest separating out
the elements into a separate field.

## Implied Keys

If you had a nested struct where you wanted every member to be a key like such:

```
struct NestedType {
  long a;
  short b;
  char c;
};

#pragma DCPS_DATA_TYPE "CompositeType"
#pragma DCPS_DATA_KEY "CompositeType nested.a"
#pragma DCPS_DATA_KEY "CompositeType nested.b"
#pragma DCPS_DATA_KEY "CompositeType nested.c"
#pragma DCPS_DATA_KEY "CompositeType another_key"
struct CompositeType {
  NestedType nested;
  long some_value;
  long another_key;
};
```

Then you can omit specifying them all as a key with annotations if specify the
field with the nested struct type as a key:

```
struct NestedType {
  long a;
  short b;
  char c;
};

@topic
struct CompositeType {
  @key NestedType nested;
  long some_value;
  @key long another_key;
};
```

If you specify any keys in the nested struct, then only those members will be
used as keys, instead of all of them:

```
struct NestedType {
  @key long a;
  @key short b;
  char c;
};
```

Finally, if desired, you can specify all the fields manually:

```
struct NestedType {
  @key long a;
  @key short b;
  @key char c;
};
```

This has the same effect as the example where none of the fields in
`NestedType` were marked with `@key`.

## Suppressing Deprecation Warnings

Starting in version 3.14, `opendds_idl` will warn when it encounters a type
declared as a topic type using `DCPS_DATA_TYPE`. You may suppress these warning
by passing `--no-dcps-data-type-warnings` to `opendds_idl`, but **we will be
removing support for these pragma statments at some point in the future**. How
to pass the option to `opendds_idl` depends on the build system being used:

- When using MPC, add `dcps_ts_flags += --no-dcps-data-type-warnings` to the
  MPC project.
- When using CMake, add `--no-dcps-data-type-warnings` to the
  `OPENDDS_IDL_OPTIONS` parameter for `OPENDDS_TARGET_SOURCES`. See
  [`docs/cmake.md`](cmake.md) for more information about using OpenDDS with
  CMake.
