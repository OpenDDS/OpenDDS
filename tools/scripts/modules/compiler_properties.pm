package compiler_properties;

use warnings;
use strict;

use File::Temp ();

my @targets = qw/32b_x86 64b_x86 32b_arm 64b_arm/;
my @target_prop_names = map { "targets_$_" } @targets;

# Human Friendly Name, Value of __cplusplus
my @cpp_standards = (
  ["c++03", 199711],
  ["c++11", 201103],
  ["c++14", 201402],
  ["c++17", 201703],
  ["c++20", 202002],
);

sub cpp_standard_from_macro {
  my $macro_value = shift;

  my $std = "UNKNOWN";
  for my $pair (@cpp_standards) {
    if ($pair->[1] <= $macro_value) {
      $std = $pair->[0];
    }
  }
  return $std;
}

sub cpp_standard_to_macro {
  my $std = shift;

  for my $pair (@cpp_standards) {
    if ($pair->[0] eq $std) {
      return $pair->[1];
    }
  }
  print STDERR "WARNING: \"$std\" is an invalid standard\n";
  return 999999;
}

sub cpp_standard_at_least {
  my $macro_value = shift;
  my $std = shift;

  return ($macro_value >= cpp_standard_to_macro($std)) ? 1 : 0;
}

# Maps compiler_kind to how to detect and invoke the compiler.
my %compilers = (
  gcc => {
    cmd_re => qr/(?<!clan)g\+\+/,
    opts => '-E',
  },
  clang => {
    cmd_re => qr/clang\+\+/,
    opts => '-E',
  },
  msvc => {
    cmd_re => qr/cl(?!ang)/,
    opts => '/EP /Zc:__cplusplus',
  },
);

my @version_parts = qw/major_version minor_version patch_version/;

# All possible properties and how to get them.
# There are three kinds of ways to get properties which can be defined per
# compiler or for all compilers:
#   macro_value: base value off of the value of macro. Can be combined with
#     macro_value_re regex and/or macro_value_process function to refine the
#     result.
#   macro_defined: If given macro is defined, then the value of the property is
#     a 1 or else a 0.
#   get_value: function that is passed the hash of current property values and
#     returns the value. Can use the depends array of property names to define
#     properties that have a value first. This will loop if the dependencies are
#     circular, so be careful.
my @all_props = (
  {
    name => 'version',
    depends => \@version_parts,
    get_value => sub {
      my $props = shift;
      return join('.', map {$props->{$_}} @version_parts);
    },
  },
  {
    name => 'major_version',
    macro_value_re => qr/(\d+)/,
    for => {
      gcc => {
        macro_value => '__GNUC__',
      },
      clang => {
        macro_value => '__clang_major__',
      },
      msvc => {
        macro_value => '_MSC_VER',
        macro_value_re => qr/(\d+)\d{2}/,
      },
    },
  },
  {
    name => 'minor_version',
    macro_value_re => qr/(\d+)/,
    for => {
      gcc => {
        macro_value => '__GNUC_MINOR__',
      },
      clang => {
        macro_value => '__clang_minor__',
      },
      msvc => {
        macro_value => '_MSC_VER',
        macro_value_re => qr/\d+(\d{2})/,
        macro_value_process => sub {
          return int(shift);
        },
      },
    },
  },
  {
    name => 'patch_version',
    macro_value_re => qr/(\d+)/,
    for => {
      gcc => {
        macro_value => '__GNUC_PATCHLEVEL__',
      },
      clang => {
        macro_value => '__clang_patchlevel__',
      },
      msvc => {
        # cl doesn't seem to have this. It does have a build number in
        # _MSC_VER_FULL, but that is probably, not needed.
        get_value => sub { return '0'; },
      },
    },
  },
  {
    name => 'default_cpp_standard_macro_value',
    macro_value_re => qr/(\d+)L/,
    macro_value => '__cplusplus',
  },
  {
    name => 'default_cpp_standard',
    depends => ['default_cpp_standard_macro_value'],
    get_value => sub {
      my $props = shift;
      return cpp_standard_from_macro($props->{default_cpp_standard_macro_value});
    },
  },
  {
    name => 'targets_32b_x86',
    for => {
      gcc => {
        macro_defined => '__i386__',
      },
      clang => {
        macro_defined => '__i386__',
      },
      msvc => {
        macro_defined => '_M_IX86',
      },
    },
  },
  {
    name => 'targets_64b_x86',
    for => {
      gcc => {
        macro_defined => '__x86_64__',
      },
      clang => {
        macro_defined => '__x86_64__',
      },
      msvc => {
        macro_defined => '_M_X64',
      },
    },
  },
  {
    name => 'targets_32b_arm',
    for => {
      gcc => {
        macro_defined => '__arm__',
      },
      clang => {
        macro_defined => '__arm__',
      },
      msvc => {
        macro_defined => '_M_ARM',
      },
    },
  },
  {
    name => 'targets_64b_arm',
    for => {
      gcc => {
        macro_defined => '__aarch64__',
      },
      clang => {
        macro_defined => '__aarch64__',
      },
      msvc => {
        macro_defined => '_M_ARM64',
      },
    },
  },
  {
    name => 'targets',
    depends => \@target_prop_names,
    get_value => sub {
      my $props = shift;
      my $target = undef;
      for my $t (@targets) {
        my $name = "targets_$t";
        next unless ($props->{$name});
        if (defined($target)) {
          print STDERR "WARNING: Multiple targets matched\n";
        }
        $target = $t;
      }
      if (!defined($target)) {
        print STDERR "WARNING: Could not determine target\n";
        $target = "UNKNOWN";
      }
      return $target;
    }
  },
);

sub get_prop_by_name {
  my $name = shift;

  for my $prop (@all_props) {
    if ($prop->{name} eq $name) {
      return $prop;
    }
  }
  warn("No such prop $name");
}

sub eval_prop {
  my $props = shift;
  my $compiler_kind = shift;
  my $prop = shift;

  my $get_value = get_prop_hint($prop, $compiler_kind, 'get_value');
  if (defined($get_value) && !exists($props->{$prop->{name}})) {
    if (exists($prop->{depends})) {
      for my $name (@{$prop->{depends}}) {
        if (!exists($props->{$name})) {
          if (!eval_prop($props, $compiler_kind, get_prop_by_name($name))) {
            return 0;
          }
        }
      }
    }
    $props->{$prop->{name}} = &{\&{$get_value}}($props);
    return 1;
  }
  return 0;
}

sub get_prop_hint {
  my $prop = shift;
  my $compiler_kind = shift;
  my $key = shift;

  my $value = undef;
  if (exists($prop->{for}->{$compiler_kind}) &&
      exists($prop->{for}->{$compiler_kind}->{$key})) {
    $value = $prop->{for}->{$compiler_kind}->{$key};
  }
  elsif (exists($prop->{$key})) {
    $value = $prop->{$key};
  }
  return $value;
}

sub kind_from_command {
  my $compiler_command = shift;

  my $compiler_kind;
  for my $key (keys(%compilers)) {
    if ($compiler_command =~ /$compilers{$key}->{cmd_re}/i) {
      $compiler_kind = $key;
    }
  }
  return $compiler_kind;
}

sub get_props {
  my $compiler_command = shift;
  my $compiler_kind = shift;

  my %inserted_macros;
  my %props;

  my ($in_fd, $in_filename) = File::Temp::tempfile(
    'opendds-compiler-info-XXXXXX', SUFFIX => ".cpp");
  print $in_fd "valid: 1\n";
  for my $prop (@all_props) {
    my $macro_value = get_prop_hint($prop, $compiler_kind, 'macro_value');
    my $macro_defined = get_prop_hint($prop, $compiler_kind, 'macro_defined');
    if ($macro_value) {
      $inserted_macros{$prop->{name}} = $prop;
      print $in_fd "$prop->{name}: $macro_value\n";
    }
    elsif ($macro_defined) {
      $inserted_macros{$prop->{name}} = $prop;
      print $in_fd
        "#ifdef $macro_defined\n" .
        "$prop->{name}: 1\n" .
        "#else\n" .
        "$prop->{name}: 0\n" .
        "#endif\n";
    }
  }
  close($in_fd);

  my $command = "\"$compiler_command\" $compilers{$compiler_kind}->{opts} " .
    "\"$in_filename\" 2>&1";
  my $valid = 0;
  open(my $out_fd, "-|", $command) or die "ERROR: $!";
  while (my $line = <$out_fd>) {
    chomp($line);
    # print("compiler says: $line\n");
    if ($line =~ /^(\w+): (.*)$/) {
      my $name = $1;
      my $raw_value = $2;
      if ($name eq 'valid' && $raw_value eq "1") {
        $valid = 1;
      }
      for my $prop (values(%inserted_macros)) {
        if ($prop->{name} eq $name) {
          my $value;
          my $re = get_prop_hint($prop, $compiler_kind, 'macro_value_re');
          if ($re) {
            if ($raw_value =~ m/^$re$/) {
              $value = $1;
            } else {
              $value = "INVALID";
            }
          }
          else {
            $value = $raw_value;
          }
          my $macro_value_process =
            get_prop_hint($prop, $compiler_kind, 'macro_value_process');
          if (defined($macro_value_process)) {
              $value = &{\&{$macro_value_process}}($value);
          }
          $props{$prop->{name}} = $value;
          last;
        }
      }
    }
  }
  close($out_fd);
  unlink($in_filename)
    or warn "Unable to delete temporary file $in_filename: $!";

  die ("Invalid compiler output") if (!$valid);

  for my $prop (@all_props) {
    eval_prop(\%props, $compiler_kind, $prop);
  }

  return \%props;
}

sub props_in_order {
  my $props_hash = shift;

  my @props_array = ();
  for my $prop (@all_props) {
    my $name = $prop->{name};
    if (exists($props_hash->{$name})) {
      push(@props_array, {name => $name, value => $props_hash->{$name}});
    }
  }

  return \@props_array;
}

1;
