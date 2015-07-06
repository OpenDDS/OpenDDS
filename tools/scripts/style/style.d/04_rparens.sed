#
#

:begin

# Ignore pre-processor directives
/^#/{
  P;D
  b begin
}

# Ignore // style comments
/\/\//{
  P;D
  b begin
}

# Collapse lines which only contain closing parens.
$!N
s/\n *\().*\)$/\1/
t begin
P;D
