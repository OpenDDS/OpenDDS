#
#

:begin

# Strip trailing newlines.
/^\n*$/{
  $d; N
  b begin
}
