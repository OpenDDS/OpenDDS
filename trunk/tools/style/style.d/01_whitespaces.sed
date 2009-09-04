#
# $Id$
#

# Strip trailing whitespace.
s/[ ]*$//

# Strip extraneous whitespace.
##s/\([^ ]\{1,\}\) \{2,\}/\1/g

# Strip leading spaces from ! operators.
s/\(!\) \{1,\}/\1/g

# Strip extraneous spaces around template specifiers.
s/\([^ ]<\) \+/\1/g
s/> \{1,\}>/>>/g

# Adjust initializer list alignment by astyle(1).
s/^ \{2\}\( \{2\}:\)/\1/
s/^ \{2\}\( \{2\},\)/\1/
