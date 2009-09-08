#
# $Id$
#

# Remove #if 0 blocks:
/#if 0/,/#endif/d

# Strip leading global specifiers on known namespaces.
s/^::\(ACE\)/\1/
s/ \{1,\}::\(ACE\)/\1/g

s/^::\(TAO\)/\1/
s/ \{1,\}::\(TAO\)/\1/g

s/^::\(DDS\)/\1/
s/ \{1,\}::\(DDS\)/\1/g

s/^::\(OpenDDS\)/\1/
s/ \{1,\}::\(OpenDDS\)/\1/g

# Replace obsolete uses of void for empty parameter lists.
# NOTE: This may break C-style casts.
s/(void)/()/

# Strip trailing spaces from known macros.
s/\(ACE_THROW_SPEC\) \{1,\}/\1/g
