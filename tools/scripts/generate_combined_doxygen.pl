#!/usr/bin/env perl

# Script that wraps around generate_doxygen.pl to generate OpenDDS Doxygen
# documentation with ACE/TAO documentation built-in.

use strict;
use File::Path 'mkpath';
use File::Path 'rmtree';
use File::Find;
use File::Basename;
use Cwd 'abs_path';

# Parse Arguments
my $usage = "usage: generate_combined_doxygen.pl [-h|--help] DESTINATION ...\n";
my $argc = $#ARGV + 1;
if (!(defined $ENV{"ACE_ROOT"} && defined $ENV{"DDS_ROOT"})) {
  die("ERROR: \$ACE_ROOT and \$DDS_ROOT must be defined\n");
}
if ($argc == 0) {
  die("ERROR: $usage");
}
if ($ARGV[0] eq "-h" || $ARGV[0] eq "--help") {
  print($usage);
  print("\nGenerates OpenDDS Doxygen documentation with ACE/TAO documentation built-in.\n");
  print("Extra arguments are passed to generate_doxygen.pl.\n");

  exit(0);
}
if (! -d $ARGV[0]) {
  die("ERROR: destination directory $ARGV[0] does not exist!\n");
}

# Set up paths
my $dest = abs_path(shift);
my @doxygen = ("perl", $ENV{"ACE_ROOT"} . "/bin/generate_doxygen.pl");
for (my $i = 0; $i <= $#ARGV; $i++) {
  push(@doxygen, $ARGV[$i]);
}
push(@doxygen, "-html_output");
my $ace_dest = "$dest/html/dds/ace_tao";
eval { mkpath($ace_dest) };
if ($@) {
  die("Couldn’t create $ace_dest: $@");
}

# Build ACE/TAO Documentation
chdir($ENV{"ACE_ROOT"});
push(@doxygen, $ace_dest);
system(@doxygen) == 0 or die "ERROR: ACE/TAO Doxygen failed ($?)\n";
pop(@doxygen);

# Find Tagfiles and Inject into dds.doxygen using $ace_tao_tagfiles
my $ace_tao_tagfiles = "";
my (@tagfiles)=();
find(\&check_if_tag, $ace_dest);
sub check_if_tag {
  push(@tagfiles, $File::Find::name) if (!-d && $File::Find::name =~ /.*\.tag$/);
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
close($f);

# Build OpenDDS Documentation
chdir($ENV{"DDS_ROOT"});
push(@doxygen, $dest);
system(@doxygen) == 0 or die "ERROR: DDS Doxygen failed ($?)\n";

# Delete bogus directories if any
opendir(my $root_dir, "$dest/html") or die "WARNING: Cannot open $dest/html ($!)\n";
my @files = readdir($root_dir);
foreach my $f (@files) {
  if ($f ne "dds" && $f ne "." && $f ne "..") {
    rmtree("$dest/html/$f");
  }
}
closedir($root_dir);
