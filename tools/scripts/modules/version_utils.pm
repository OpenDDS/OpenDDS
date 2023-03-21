package version_utils;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(
  parse_version
  version_greater_equal
  version_not_equal
  version_greater
  version_lesser
  version_cmp
);

sub parse_version {
  my $version = shift;
  my %result = ();
  my $field = qr/0|[1-9]\d*/;
  my $metafield = qr/(?:$field|\d*[a-zA-Z-][0-9a-zA-Z-]*)/;
  if ($version =~ /^($field)\.($field)(?:\.($field))?(?:-($metafield(?:\.$metafield)*))?$/) {
    $result{major} = $1;
    $result{minor} = $2;
    $result{micro} = $3 || "0";
    $result{metadata} = $4 || "";
    my $metadata_maybe = $result{metadata} ? "-$result{metadata}" : "";

    $result{series_string} = "$result{major}.$result{minor}";
    $result{release_string} = "$result{series_string}.$result{micro}";
    $result{string} = "$result{release_string}$metadata_maybe";
    if ($result{micro} eq "0") {
      $result{tag_string} = $result{series_string};
    } else {
      $result{tag_string} = $result{release_string};
    }

    # For Version Comparison
    my @metadata_fields = split(/\./, $result{metadata});
    $result{metadata_fields} = \@metadata_fields;
  }
  return \%result;
}

sub normalize_args {
  my $left = shift();
  my $right = shift();

  if (!ref($left)) {
    $left = parse_version($left);
  }
  if (!ref($right)) {
    $right = parse_version($right);
  }

  return $left, $right;
}

# Compare Versions According To Semver
sub version_greater_equal {
  my ($left, $right) = normalize_args(@_);

  # Compare X.Y.Z fields
  if ($left->{major} > $right->{major}) {
    return 1;
  }
  elsif ($left->{major} < $right->{major}) {
    return 0;
  }
  if ($left->{minor} > $right->{minor}) {
    return 1;
  }
  elsif ($left->{minor} < $right->{minor}) {
    return 0;
  }
  if ($left->{micro} > $right->{micro}) {
    return 1;
  }
  elsif ($left->{micro} < $right->{micro}) {
    return 0;
  }

  # If they are equal in the normal fields, compare the metadata fields, which
  # are the dot-delimited fields after "-". See
  # https://semver.org/#spec-item-11 for an explanation.
  my @lfields = @{$left->{metadata_fields}};
  my @rfields = @{$right->{metadata_fields}};
  my $llen = scalar(@lfields);
  my $rlen = scalar(@rfields);
  return 1 if ($llen == 0 && $rlen > 0);
  return 0 if ($llen > 0 && $rlen == 0);
  my $mlen = $llen > $rlen ? $llen : $rlen;
  for (my $i = 0; $i < $mlen; $i += 1) {
    my $morel = $i < $llen;
    my $morer = $i < $rlen;
    return 1 if ($morel && !$morer);
    return 0 if (!$morel && $morer);
    my $li = $lfields[$i];
    my $lnum = $li =~ /^\d+$/ ? 1 : 0;
    my $ri = $rfields[$i];
    my $rnum = $ri =~ /^\d+$/ ? 1 : 0;
    return 1 if (!$lnum && $rnum);
    return 0 if ($lnum && !$rnum);
    if ($lnum) {
      return 1 if $li > $ri;
      return 0 if $li < $ri;
    }
    else {
      return 1 if $li gt $ri;
      return 0 if $li lt $ri;
    }
  }
  return 1;
}

sub version_not_equal {
  my ($left, $right) = normalize_args(@_);
  return $left->{string} ne $right->{string};
}

sub version_greater {
  my ($left, $right) = normalize_args(@_);
  return version_greater_equal($left, $right) && version_not_equal($left, $right);
}

sub version_lesser {
  my ($left, $right) = normalize_args(@_);
  return !version_greater_equal($left, $right);
}

sub version_cmp {
  my ($left, $right) = normalize_args(@_);

  return 0 if $left->{string} eq $right->{string};
  return version_lesser($left, $right) ? -1 : 1;
}

1;
