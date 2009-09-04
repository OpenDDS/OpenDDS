#
# $Id$
#

:begin

# Strip trailing newlines.
/^\n*$/{
  $d; N
  bbegin
}
