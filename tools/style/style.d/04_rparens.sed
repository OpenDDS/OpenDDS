#
# $Id$
#

:begin

# Collapse lines which only contain closing parens.
$!N
s/\n *\().*\)$/\1/
t begin
P
D
