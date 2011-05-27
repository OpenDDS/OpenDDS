package JAVAHHelper;

# ************************************************************
# Description   : Assist in determining the output from javah
# Author        : Adam Mitz based on code by Chad Elliott
# Create Date   : 7/16/2008
# ************************************************************

# ************************************************************
# Pragmas
# ************************************************************

use strict;

use CommandHelper;
our @ISA = qw(CommandHelper);

# ************************************************************
# Public Interface Section
# ************************************************************

sub get_output {
  my($self, $file, $flags) = @_;

  $file =~ s(^classes[/\\])();
  $file =~ s([/\\])(_)g;
  $file =~ s(\.class$)(.h);
  return [$file];
}

sub get_outputexts {
  return ['\\.h']; #this is a regexp pattern, so . gets escaped
}

1;
