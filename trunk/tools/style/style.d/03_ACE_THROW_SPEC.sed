#
# $Id$
#

# Collapse multi-line ACE_THROW_SPEC clauses.
/ACE_THROW_SPEC(($/{
  $!N;$!N
  s/\n *//g
}
