#
# $Id$
#

# Collapse lines which only contain closing parens.
$!N
/\n *)\{1,\};$/s/\n *//
