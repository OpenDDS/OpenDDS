#!/usr/bin/perl

# Script that wraps around generate_doxygen.pl to generate OpenDDS Doxygen
# documentation with ACE/TAO documentation built-in.

use strict;
use File::Path qw(mkpath rmtree);
use File::Copy qw(move);
use File::Find;
use File::Basename;
use Cwd 'abs_path';

# Parse Arguments
my $usage = "usage: generate_combined_doxygen.pl [-h|--help] DESTINATION ...\n";
my $argc = $#ARGV + 1;
if (!(defined $ENV{"ACE_ROOT"} && defined $ENV{"DDS_ROOT"})) {
  die "ERROR: \$ACE_ROOT and \$DDS_ROOT must be defined\n";
}
if ($argc == 0) {
  die "ERROR: $usage";
}
if ($ARGV[0] eq "-h" || $ARGV[0] eq "--help") {
  print $usage;
  print "\nGenerates OpenDDS Doxygen documentation with ACE/TAO documentation built-in.\n";
  print "Extra arguments are passed to generate_doxygen.pl.\n";

  exit 0;
}
if (! -d $ARGV[0]) {
  die "ERROR: destination directory $ARGV[0] does not exist!\n";
}

# Set up paths
my $dest = abs_path(shift);
my @doxygen = (
  "perl", $ENV{"ACE_ROOT"} . "/bin/generate_doxygen.pl", "-html_output", $dest
);
for (my $i = 0; $i <= $#ARGV; $i++) {
  push(@doxygen, $ARGV[$i]);
}
my $ace_dest = "$dest/html/dds/ace_tao";
mkpath($ace_dest) or die "ERROR: $!";

# Build ACE/TAO Documentation
chdir $ENV{"ACE_ROOT"};
system(@doxygen) == 0 or die "ERROR: ACE/TAO Doxygen failed ($!)\n";
move("$dest/html/libace-doc", "$ace_dest/libace-doc") or die $!;
move("$dest/html/libacexml-doc", "$ace_dest/libacexml-doc") or die $!;
move("$dest/html/libtao-doc", "$ace_dest/libtao-doc") or die $!;

# Find Tagfiles and Inject into dds.doxygen using $ace_tao_tagfiles
my $ace_tao_tagfiles = "";
my (@tagfiles)=();
find (\&check_if_tag, $ace_dest);
sub check_if_tag {
  push (@tagfiles, $File::Find::name) if (!-d && $File::Find::name =~ /.*\.tag$/);
}
foreach $a (@tagfiles) {
  my $file = "ace_tao" . dirname(substr($a, (length $ace_dest)));
  $ace_tao_tagfiles = sprintf(
    "%s \"%s = %s\"", $ace_tao_tagfiles, $a, $file
  );
}
$ENV{ace_tao_tagfiles} = $ace_tao_tagfiles;

# Generate ACE/TAO links page
open(my $f, '>', $ENV{'DDS_ROOT'} . "/docs/doxygen_ace_tao_generated_links.h");
print $f "/*! \\page ace_tao_links Built-In ACE/TAO Documentation\n";
foreach $a (@tagfiles) {
  my $link = "ace_tao" . dirname(substr($a, (length $ace_dest)));
  print $f sprintf(" * <a href=\"%s/index.html\">%s</a><br>\n", $link, $link);
}
print $f " */\n";
close $f;

# Build OpenDDS Documentation
chdir $ENV{"DDS_ROOT"};
system(@doxygen) == 0 or die "ERROR: DDS Doxygen failed ($!)\n";

