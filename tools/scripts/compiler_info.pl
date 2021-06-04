use warnings;
use strict;

use File::Temp ();

my @targets = qw/32b_x86 64b_x86 32b_arm 64b_arm/;

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
  my $macro_value = shift;
  for my $pair (@cpp_standards) {
    if ($pair->[0] eq $std) {
      return $pair->[1];
    }
  }
  print STDERR "WARNING: \"$std\" is an invalid standard\n";
  return 0;
}

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
    opts => '/EP',
  },
);

my @version_parts = qw/major_version minor_version patch_version/;

my %info_with_values;
my %inserted_macros;
my @all_info = (
  {
    name => 'version',
    depends => \@version_parts,
    get_value => sub {
      return join('.', map {$info_with_values{$_}->{value}} @version_parts);
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
      return cpp_standard_from_macro(
        $info_with_values{default_cpp_standard_macro_value}->{value});
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
    get_value => sub {
      my $target = undef;
      for my $t (@targets) {
        my $name = "targets_$t";
        next unless ($info_with_values{$name}->{value});
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

my $compiler_command = shift;
if (!defined($compiler_command)) {
  die("Must pass compiler command to inspect");
}
my $compiler_kind;
for my $key (keys(%compilers)) {
  if ($compiler_command =~ /$compilers{$key}->{cmd_re}/i) {
    $compiler_kind = $key;
  }
}
if (!defined($compiler_kind)) {
  die("Could not determine compiler kind of command");
}
my $what = shift;

sub get_info_prop {
  my $info = shift;
  my $key = shift;

  my $value = undef;
  if (exists($info->{for}->{$compiler_kind}) && exists($info->{for}->{$compiler_kind}->{$key})) {
    $value = $info->{for}->{$compiler_kind}->{$key};
  }
  elsif (exists($info->{$key})) {
    $value = $info->{$key};
  }
  return $value;
}

sub set_value {
  my $info = shift;
  my $value = shift;
  $info->{value} = $value;
  $info_with_values{$info->{name}} = $info;
}

my ($tmp_fd, $tmp_filename) = File::Temp::tempfile(
  'opendds-compiler-info-XXXXXX', SUFFIX => ".cpp");

for my $info (@all_info) {
  my $macro_value = get_info_prop($info, 'macro_value');
  my $macro_defined = get_info_prop($info, 'macro_defined');
  if ($macro_value) {
    $inserted_macros{$info->{name}} = $info;
    print $tmp_fd "$info->{name}: $macro_value\n";
  }
  elsif ($macro_defined) {
    $inserted_macros{$info->{name}} = $info;
    print $tmp_fd
      "#ifdef $macro_defined\n" .
      "$info->{name}: 1\n" .
      "#else\n" .
      "$info->{name}: 0\n" .
      "#endif\n";
  }
}

my $command = "\"$compiler_command\" $compilers{$compiler_kind}->{opts} \"$tmp_filename\" 2>&1";
open(my $out_fd, "-|", $command) or die "ERROR: $!";
while (my $line = <$out_fd>) {
  chomp($line);
  # print("compiler says: $line\n");
  if ($line =~ /^(\w+): (.*)$/) {
    my $name = $1;
    my $raw_value = $2;
    for my $info (values(%inserted_macros)) {
      if ($info->{name} eq $name) {
        my $value;
        my $re = get_info_prop($info, 'macro_value_re');
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
        my $macro_value_process = get_info_prop($info, 'macro_value_process');
        if (defined($macro_value_process)) {
            $value = &{\&{$macro_value_process}}($value);
        }
        set_value($info, $value);
        last;
      }
    }
  }
}
close($out_fd);
unlink($tmp_filename)
  or warn "Unable to delete temporary file $tmp_filename: $!";

for my $info (@all_info) {
  my $get_value = get_info_prop($info, 'get_value');
  if (defined($get_value)) {
    my $depends_met = !exists($info->{depends});
    if (!$depends_met) {
      $depends_met = 1;
      for my $name (@{$info->{depends}}) {
        if (!exists($info_with_values{$name})) {
          $depends_met = 0;
          last;
        }
      }
    }
    if ($depends_met) {
      set_value($info, &{\&{$get_value}}());
    }
  }
}

if (defined($what)) {
  print("$info_with_values{$what}->{value}\n");
}
else {
  for my $info (@all_info) {
    print("$info->{name}: $info->{value}\n") if exists($info->{value});
  }
}
