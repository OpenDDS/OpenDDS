#############
ZeroEnum Test
#############

Previous versions of OpenDDS did not set the extensibility flags for enums (they were zero) in minimal and complete type objects.
These are treated as wild cards meaning that they will not cause the type assignability check to fail.
The idl compiler option `-Gxtypes-complete --default-enum-extensibility-zero` was added to force the old behavior.

This test checks that

1. Zero extensibility flags for enums are handled correctly.
2. The extensibility is set to appendable when converting to a dynamic type.

This test use two processes to force the use of the type lookup service which should log a warning about zero enum flags, e.g., `(48706|123145452138496) WARNING: TypeLookupService::set_type_object_defaults: Zero extensibility flags in TK_ENUM`.
