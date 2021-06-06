package CompilerProps;

use warnings;
use strict;

use File::Temp ();

my @archs = qw/32b_x86 64b_x86 32b_arm 64b_arm/;

# Human Friendly Name, Value of __cplusplus
my @cpp_stds = (
  ["c++03", 199711],
  ["c++11", 201103],
  ["c++14", 201402],
  ["c++17", 201703],
  ["c++20", 202002],
);

sub cpp_std_from_macro {
  my $macro_value = shift;

  my $std = undef;
  for my $pair (@cpp_stds) {
    if ($pair->[1] <= $macro_value) {
      $std = $pair->[0];
    }
  }
  return $std;
}

sub cpp_std_to_macro {
  my $std = shift;

  for my $pair (@cpp_stds) {
    if ($pair->[0] eq $std) {
      return $pair->[1];
    }
  }
  die("\"$std\" is an unreconized C++ standard\n")
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
#     properties that must be defined first. This will loop if the dependencies
#     are circular, so be careful.
my @all_props = (
  {
    name => 'command',
  },
  {
    name => 'kind',
  },
  {
    name => 'accepts_std',
    get_value => sub { return 1; },
    for => {
      msvc => {
        get_value => sub { return 0; },
      },
    },
  },
  {
    name => 'version',
    depends => \@version_parts,
    for => {
      msvc => {
        depends => ['major_version', 'minor_version'],
      },
    },
    get_value => sub {
      my $props = shift;

      return join('.', map {$props->{$_}} grep {exists $props->{$_}} @version_parts);
    },
  },
  {
    name => 'msc_ver',
    for => {
      msvc => {
        macro_value => '_MSC_VER',
        macro_value_re => qr/(\d{3,})/,
      },
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
        depends => ['msc_ver'],
        get_value => sub {
          my $props = shift;

          $props->{msc_ver} =~ /^(\d+)\d{2}$/;
          return $1;
        },
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
        depends => ['msc_ver'],
        get_value => sub {
          my $props = shift;

          $props->{msc_ver} =~ /^\d+(\d{2})$/;
          return int($1); # Get rid of leading zeros
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
    },
  },
  {
    name => 'default_cpp_std_macro_value',
    macro_value_re => qr/(\d+)L/,
    macro_value => '__cplusplus',
  },
  {
    name => 'default_cpp_std',
    depends => ['default_cpp_std_macro_value'],
    get_value => sub {
      my $props = shift;

      return cpp_std_from_macro($props->{default_cpp_std_macro_value});
    },
  },
  {
    name => '32b_x86',
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
    name => '64b_x86',
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
    name => '32b_arm',
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
    name => '64b_arm',
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
    name => 'arch',
    depends => \@archs,
    get_value => sub {
      my $props = shift;

      my $arch = undef;
      for my $a (@archs) {
        next unless ($props->{$a});
        if (defined($arch)) {
          warn("Multiple archs matched");
        }
        $arch = $a;
      }
      if (!defined($arch)) {
        warn("Could not determine arch");
        $arch = "UNKNOWN";
      }
      return $arch;
    }
  },
  {
    name => 'vs_cmake_arch',
    depends => ['arch'],
    for => {
      msvc => {
        get_value => sub {
          my $props = shift;

          my %table = (
            '32b_x86' => 'Win32',
            '64b_x86' => 'x64',
            '32b_arm' => 'ARM',
            '64b_arm' => 'ARM64',
          );
          return $table{$props->{arch}};
        },
      },
    },
  },
  {
    name => 'vs_cmake_gen',
    depends => ['msc_ver'],
    get_value => sub {
      my $props = shift;

      my %table = (
        '150' => 'Visual Studio 9 2008',
        '160' => 'Visual Studio 10 2010',
        '170' => 'Visual Studio 11 2012',
        '180' => 'Visual Studio 12 2013',
        '190' => 'Visual Studio 14 2015',
        '191' => 'Visual Studio 15 2017',
        '192' => 'Visual Studio 16 2019',
      );
      my $ver = $props->{version};
      $ver =~ s/^(\d+)\d$/$1/;
      return $table{$ver};
    },
  },
  {
    name => 'vs_mpc_type',
    depends => ['msc_ver'],
    get_value => sub {
      my $props = shift;

      my %table = (
        '131' => 'vc71',
        '140' => 'vc8',
        '150' => 'vc9',
        '160' => 'vc10',
        '170' => 'vc11',
        '180' => 'vc12',
        '190' => 'vc14',
        '191' => 'vs2017',
        '192' => 'vs2019',
      );
      my $ver = $props->{version};
      $ver =~ s/^(\d+)\d$/$1/;
      return $table{$ver};
    },
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
  return undef;
}

sub eval_prop {
  my $props = shift;
  my $compiler_kind = shift;
  my $prop = shift;

  my $get_value = get_prop_hint($prop, $compiler_kind, 'get_value');
  my $depends = get_prop_hint($prop, $compiler_kind, 'depends');
  if (defined($get_value) && !exists($props->{$prop->{name}})) {
    if (defined($depends)) {
      for my $name (@{$depends}) {
        if (!exists($props->{$name})) {
          if (!eval_prop($props, $compiler_kind, get_prop_by_name($name))) {
            return 0;
          }
        }
      }
    }
    my $value = &{\&{$get_value}}($props);
    if (defined($value)) {
      $props->{$prop->{name}} = $value;
      return 1;
    }
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
      if (defined($compiler_kind)) {
        warn "compiler matched $key, but already matched $compiler_kind!";
      }
      $compiler_kind = $key;
    }
  }
  return $compiler_kind;
}

sub get_props {
  my $compiler_command = shift;
  my $compiler_kind = shift;
  my $debug = shift;

  my %inserted_macros;
  my %props = (
    command => $compiler_command,
    kind => $compiler_kind,
  );

  # Make the Input File
  my ($in_fd, $in_filename) = File::Temp::tempfile(
    'opendds-compiler-info-XXXXXX', SUFFIX => ".cpp");
  my $magic = "opendds-compiler-props-magic";
  print $in_fd "$magic\n";
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

  # Run the Compiler on Input File
  my $command = "\"$compiler_command\" $compilers{$compiler_kind}->{opts} " .
    "\"$in_filename\" 2>&1";
  if ($debug) {
    print("Running command: $command\n");
  }
  my $valid = 0;
  open(my $out_fd, "-|", $command) or die "ERROR: $!";
  my @lines;
  while (my $line = <$out_fd>) {
    chomp($line);
    $valid = 1 if ($line eq $magic);
    push(@lines, $line);
  }
  close($out_fd);
  unlink($in_filename)
    or warn("Unable to delete temporary file $in_filename: $!");
  if (!$valid) {
    print STDERR
      "ERROR: Invalid compiler output from: $command\n" .
      "This is the output:\n";
    for my $line (@lines) {
      print STDERR ("compiler said: $line\n");
    }
    die("Stopped");
  }

  if ($debug) {
    for my $line (@lines) {
      print("compiler said: $line\n");
    }
  }

  # Look for Inserted Macros
  for my $line (@lines) {
    if ($line =~ /^(\w+): (.*)$/) {
      my $name = $1;
      my $raw_value = $2;
      my $prop = $inserted_macros{$name};
      if (defined($prop)) {
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
      }
    }
  }

  # Evaluate Remaining Properties that can be Evaluated
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

# Below is the Public Object-Oriented Interface

sub new {
  my $class = shift;
  my $command = shift;
  my $args = shift;
  my $debug = $args->{debug} // 0;

  print("Compiler is \"$command\"\n") if ($debug);

  my $kind = $args->{kind};
  if (!defined($kind)) {
    $kind = kind_from_command($command);
    if (!defined($kind)) {
      die("ERROR: Could not determine compiler kind of command: $command");
    }
  }
  elsif (!exists($compilers{$kind})) {
    die("ERROR: Invalid compiler kind: \"$kind\"");
  }
  print("Compiler kind is \"$kind\"\n") if ($debug);

  my $self = get_props($command, $kind, $debug);
  $self->{debug} = $debug;

  return bless($self, $class);
}

sub log {
  my $self = shift;
  my $indent = shift || '';

  my $props_array = props_in_order($self);
  for my $prop (@{$props_array}) {
    print("$indent$prop->{name}: $prop->{value}\n");
  }
}

sub require {
  my $self = shift;
  my $prop = shift;

  die("ERROR: property \"$prop\" " . (get_prop_by_name($prop) ?
    "was not defined for this compiler" : "is invalid")) unless exists($self->{$prop});
  return $self->{$prop};
}

sub default_cpp_std_is_at_least {
  my $self = shift;
  my $std = shift;

  return
    ($self->require('default_cpp_std_macro_value') >= cpp_std_to_macro($std)) ? 1 : 0;
}

1;
