#
# $Id$
#

# Remove #if 0 blocks:
/#if 0/,/#endif/d

# Strip leading global specifiers on known namespaces.
s/::\(ACE\)/\1/g
s/::\(TAO\)/\1/g
s/::\(CORBA\)/\1/g
s/::\(DDS\)/\1/g
s/::\(OpenDDS\)/\1/g

# Replace obsolete uses of void for empty parameter lists.
# NOTE: This may break C-style casts.
s/(void)/()/
