package TYPESUPPORTHelper;

use strict;
use CommandHelper;
use File::Basename;
our @ISA = qw(CommandHelper);

sub get_output {
  my($self, $file, $flags) = @_;

  my $dir = '';
  $flags = '' unless defined $flags;
  if ($flags =~ /-o +(\S+)/) {
    $dir = "$1/";
  }

  my @out = ();
  if ($flags =~ /-SI/) {
    return \@out;
  }

  my $tsidl = $file;
  $tsidl =~ s/\.idl$/TypeSupport.idl/;
  push(@out, $dir . basename($tsidl));

  my $deps;
  if ($flags =~ /-Wb,java/) {

    my $i2j = CommandHelper::get('idl2jni_files');
    $i2j->do_cached_parse($file, $flags);

    my $tsfile = $tsidl;
    $tsfile =~ s/\\/\//g;

    my $tsinfo = $i2j->get_typesupport_info($tsfile);
    if (defined $tsinfo) {
      my @types = split /;/, $tsinfo;
      foreach my $type (@types) {
        my @parts = split /::/, $type;
        my $tsimpl = $dir . join('/', @parts) . 'TypeSupportImpl.java';
        push(@out, $tsimpl);
        my $peer_base = pop @parts;
        my $peer = $dir . join('/', @parts);
        $peer .= '/' if length $peer;
        $peer .= '_' . $peer_base . 'TypeSupportTAOPeer.java';
        $deps->{'java_files'}->{$tsimpl} = [$peer];
      }
    }
  }

  if ($flags =~ /-Gface/) {
    my $name = basename($file, '.idl');
    foreach my $suffix ('_TS.hpp', '_TS.cpp') {
      push(@out, $dir . $name . $suffix);
    }
  }

  # check for list context for compatibility with older MPC
  return wantarray ? (\@out, $deps) : \@out;
}

sub get_outputexts {
  return ['\\.idl', '\\.java']; # these are regular expressions
}

1;
