#
#

# Strip trailing whitespace.
s/[ ]*$//

# Strip extraneous whitespace.
##s/\([^ ]\{1,\}\) \{2,\}/\1/g

# Strip leading spaces from ! operators.
s/\(!\) \{1,\}/\1/g

# Adjust initializer list alignment by astyle(1).
s/^ \{2,4\}\( \{2\}:\)/\1/
s/^ \{2,4\}\( \{2\},\)/\1/

# Strip leading spaces from template specifiers.
s/\([^  <]<\) \{1,\}\([^<]\)/\1\2/g
