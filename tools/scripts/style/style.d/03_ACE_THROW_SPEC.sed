#
# $Id$
#

# Collapse multi-line ACE_THROW_SPEC clauses.
# NOTE: Trailing rparens are caught by another rule.
/ACE_THROW_SPEC *(($/{
  $!N
  s/\n *//
}
