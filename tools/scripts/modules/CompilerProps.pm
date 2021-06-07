package CompilerProps;

# Code for extracting properties from a given supported compiler. First part of
# the module is the utility info and code. Last part is the CompilerProps
# OO interface that a user will use.
# See compiler_props.pl for command line interface to this module.

use warnings;
use strict;

use File::Temp ();

use FindBin;
use lib "$FindBin::Bin";

use OsProps;

my @archs = qw/32b_x86 64b_x86 32b_arm 64b_arm/;
my @oses = qw/linux android windows ios macos/;

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

sub android_compiler {
  # This function should closely mirror the logic of
  # ACE/include/makeinclude/platform_android.GNU
  my $os_props = shift;
  my $abi = shift // 'armeabi-v7a';
  my $ndk = shift;
  my $api = shift;

  my $cross_compile;
  if ($abi eq 'armeabi-v7a' || $abi eq 'neon' || $abi eq 'armeabi-v7a-with-neon') {
    $cross_compile = 'armv7a-linux-androideabi';
  }
  elsif ($abi eq 'arm64-v8a') {
    $cross_compile = 'aarch64-linux-android';
  }
  elsif ($abi eq 'x86') {
    $cross_compile = 'i686-linux-android';
  }
  elsif ($abi eq 'x86_64') {
    $cross_compile = 'x86_64-linux-android';
  }
  else {
    die("ERROR: Invalid android_abi: \"$abi\"");
  }

  my $command = undef;
  if (defined($ndk)) {
    # NDK method is based on an absolute path of the NDK
    die("ERROR: android_ndk also requires android_api") unless (defined($api));

    my $bin = undef;
    my $glob = $os_props->path($ndk, 'toolchains', 'llvm', 'prebuilt', '');
    foreach my $path (<$glob*>) {
      my $p = "$path$os_props->{slash}bin";
      $bin = $p if (-d $p);
    }

    if (defined($bin)) {
      my $path = $os_props->path($bin, "${cross_compile}${api}-clang++");
      $command = $path if (-x $path);
    }
    die("ERROR: Could not find Android compiler in NDK: \"$ndk\"") unless (defined($command));
  }
  else { # Standalone Toolchain
    # Standalone toolchain method is based on the compilers being on the PATH
    # Fallback to gcc if the NDK too old to be clang-based
    for my $c ('clang++', 'g++') {
      my $cmd = "$cross_compile-$c";
      print("$cmd\n");
      if ($os_props->which($cmd)) {
        $command = $cmd;
      }
      elsif ($cross_compile eq 'armv7a-linux-androideabi') {
        # Fallback to ARMv5 if ARMv7 isn't there.
        # For details about this see armv7a note in platform_android.GNU
        $cmd = "arm-linux-androideabi-$c";
        print("$cmd\n");
        if ($os_props->which($cmd)) {
          $command = $cmd;
        }
      }
    }
    die("ERROR: Could not find standalone toolchain Android compiler on PATH")
      unless (defined($command));
  }
  return $command;
}

my @version_parts = qw/major_version minor_version patch_version/;

sub prop_from_bool_props {
  my $props = shift;
  my $prop = shift;
  my $bool_props = shift;

  my $value = undef;
  for my $bool_prop (@{$bool_props}) {
    next unless ($props->{$bool_prop});
    if (defined($value)) {
      warn("Multiple properties matched for $prop");
    }
    $value = $bool_prop;
  }
  if (!defined($value)) {
    warn("Could not determine value for $prop");
    return undef;
  }
  return $value;
}

# This is a list of all the possible properties and how to get them.
# There are three ways to get properties and they can be defined per compiler
# or for all compilers:
#   macro_value: base the value off of the value of a C preprocessor macro. Can
#     be combined with macro_value_re (regex) and/or
#     macro_value_process (function) to refine the result.
#   macro_if: If the given macro expression is true, then the value of the
#     property is a 1 or else a 0.
#   get_value: function that is passed the hash of current property values and
#     returns the value. Can use the depends array ref of property names to
#     define properties that must be defined first. This will loop if the
#     dependencies are circular, so be careful.
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
        macro_if => 'defined(__i386__)',
      },
      clang => {
        macro_if => 'defined(__i386__)',
      },
      msvc => {
        macro_if => 'defined(_M_IX86)',
      },
    },
  },
  {
    name => '64b_x86',
    for => {
      gcc => {
        macro_if => 'defined(__x86_64__)',
      },
      clang => {
        macro_if => 'defined(__x86_64__)',
      },
      msvc => {
        macro_if => 'defined(_M_X64)',
      },
    },
  },
  {
    name => '32b_arm',
    for => {
      gcc => {
        macro_if => 'defined(__arm__)',
      },
      clang => {
        macro_if => 'defined(__arm__)',
      },
      msvc => {
        macro_if => 'defined(_M_ARM)',
      },
    },
  },
  {
    name => '64b_arm',
    for => {
      gcc => {
        macro_if => 'defined(__aarch64__)',
      },
      clang => {
        macro_if => 'defined(__aarch64__)',
      },
      msvc => {
        macro_if => 'defined(_M_ARM64)',
      },
    },
  },
  {
    name => 'arch',
    depends => \@archs,
    get_value => sub {
      my $props = shift;

      return prop_from_bool_props($props, 'arch', \@archs);
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
      my $ver = $props->{msc_ver};
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
      my $ver = $props->{msc_ver};
      $ver =~ s/^(\d+)\d$/$1/;
      return $table{$ver};
    },
  },
  {
    name => 'linux',
    alt_prop_id => 'is_linux', # linux is a macro in GCC
    macro_if => 'defined(__linux__) && !defined(__ANDROID__)',
  },
  {
    name => 'android',
    macro_if => 'defined(__ANDROID__)',
  },
  {
    name => 'windows',
    macro_if => 'defined(_WIN32)',
  },
  {
    name => 'ios',
    macro_if => 'TARGET_OS_IOS',
  },
  {
    name => 'macos',
    macro_if => 'TARGET_OS_MAC && !TARGET_OS_IPHONE',
  },
  {
    name => 'os',
    depends => \@oses,
    get_value => sub {
      my $props = shift;

      return prop_from_bool_props($props, 'os', \@oses);
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
  print $in_fd
    "$magic\n" .
    "#ifdef __APPLE__\n" .
    "#  include <TargetConditionals.h>\n" .
    "#else\n" .
    "#  define TARGET_OS_IOS 0\n" .
    "#  define TARGET_OS_MAC 0\n" .
    "#  define TARGET_OS_IPHONE 0\n" .
    "#endif\n";
  for my $prop (@all_props) {
    my $macro_value = get_prop_hint($prop, $compiler_kind, 'macro_value');
    my $macro_if = get_prop_hint($prop, $compiler_kind, 'macro_if');
    # Allow an alternative name if something is wrong of the actual property
    # name, like "linux" on Linux GCC is a builtin defined to 1.
    my $alt_prop_id = get_prop_hint($prop, $compiler_kind, 'alt_prop_id');
    my $prop_id = $alt_prop_id // $prop->{name};
    if ($macro_value) {
      $inserted_macros{$prop_id} = $prop;
      print $in_fd "$prop_id: $macro_value\n";
    }
    elsif ($macro_if) {
      $inserted_macros{$prop_id} = $prop;
      print $in_fd
        "#if $macro_if\n" .
        "$prop_id: 1\n" .
        "#else\n" .
        "$prop_id: 0\n" .
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
      print STDERR ("compiler said: $line\n") if (length($line));
    }
    die("Stopped");
  }

  if ($debug) {
    for my $line (@lines) {
      print("compiler said: $line\n") if (length($line));
    }
  }

  # Look for Inserted Macros
  for my $line (@lines) {
    if ($line =~ /^(\w+): (.*)$/) {
      my $prop_id = $1; # Might not be the same as name
      my $raw_value = $2;
      my $prop = $inserted_macros{$prop_id};
      if (defined($prop)) {
        my $value;
        my $re = get_prop_hint($prop, $compiler_kind, 'macro_value_re');
        if ($re) {
          if ($raw_value =~ m/^$re$/) {
            $value = $1;
          }
        }
        else {
          $value = $raw_value;
        }
        if (defined($value)) {
          my $macro_value_process =
            get_prop_hint($prop, $compiler_kind, 'macro_value_process');
          if (defined($macro_value_process)) {
            $value = &{\&{$macro_value_process}}($value);
          }
          if (defined($value)) {
            $props{$prop->{name}} = $value;
          }
        }
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

sub version_parse {
  my $str = shift;
  if ($str =~ /^(\d+)(?:\.(\d+)(?:\.(\d+))?)?$/) {
    return (int($1), int($2 // 0), int($3 // 0));
  }
  die("ERROR: Invalid version string: \"$str\"");
}

# Below is the Public Object-Oriented Interface

sub new {
  my $class = shift;
  my $command = shift;
  my $args = shift;
  my $debug = $args->{debug} // 0;
  my $os_props = $args->{os_props} // OsProps->new();

  if (exists($args->{android_abi}) && !defined($command)) {
    $command = android_compiler($os_props,
      $args->{android_abi}, $args->{android_ndk}, $args->{android_api});
  }

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
  $self->{os_props} = $os_props;

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

sub version_at_least {
  my $self = shift;
  my $cmp_str = shift;

  my $maj = int($self->require('major_version'));
  my $min = int($self->require('minor_version'));
  my $pat = int($self->{patch_version} // 0);

  my ($cmp_maj, $cmp_min, $cmp_pat) = version_parse($cmp_str);

  return 1 if ($maj > $cmp_maj);
  return 0 if ($maj < $cmp_maj);

  return 1 if ($min > $cmp_min);
  return 0 if ($min < $cmp_min);

  return $pat >= $cmp_pat ? 1 : 0;
}

1;
