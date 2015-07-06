eval '(exit $?0)' && eval 'exec perl -pSi.bak $0 ${1+"$@"}'
    & eval 'exec perl -pSi.bak $0 $argv:q'
    if 0;

use warnings;
use strict;

=head1 NAME

expandColors.pl - Convert the TiddlyWiki ColorPalette slices to actual CSS colors


=head1 SYNOPSIS

  expandColors.pl <stylesheet>

=head1 DESCRIPTION

This script will replace the TiddlyWiki ColorPalette slice specifications
in the CSS stylesheet(s).  The map is currently staticly defined in this
script and needs to be synchronized manually with the TiddlyWiki
ColorPalette.

Processing is performed in place.  That is, the input file is written
back with the updated information.  The original file is saved with an
appended '.bak' extension.

This script is intended to be run on the publish/style.css file created
when publishing a TiddlyWiki file as a series of static HTML documents.
The export plugin does not do this conversion for us.

=cut

our ($patterns);

BEGIN {
  $patterns = {
    Background     => q(#fff),
    Foreground     => q(#000),
    PrimaryPale    => q(#eee),
    PrimaryLight   => q(#ccc),
    PrimaryMid     => q(#600),
    PrimaryDark    => q(#600),
    SecondaryPale  => q(#eee),
    SecondaryLight => q(#ccc),
    SecondaryMid   => q(#999),
    SecondaryDark  => q(#666),
    TertiaryPale   => q(#eee),
    TertiaryLight  => q(#ccc),
    TertiaryMid    => q(#999),
    TertiaryDark   => q(#666),
    Error          => q(#f88),
  };
}

my $line = \$_;
map { $$line =~ s/\[\[ColorPalette::$_]]/$patterns->{$_}/g } keys %$patterns;

