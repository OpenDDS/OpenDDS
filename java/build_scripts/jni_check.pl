eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Check if the shared lib has all the "JNIEXPORT" functions that Java expects

use strict;
use File::Temp qw/ :POSIX /;

my $lib_orig = shift;

my $nm;
my $pattern;
my @decorators;

if ($^O eq 'MSWin32') {
  $nm = 'dumpbin /exports';
  @decorators = (['', '.dll'], ['', 'd.dll'], ['lib', '.dll']);
} else {
  if ($^O eq 'solaris') {
    $nm = 'gnm -g -P';
  } else {
    $nm = 'nm -g -P';
  }
  $pattern = '^_?(Java_\\S+) T ';
  @decorators = (['lib', '.so'], ['lib', '.sl'], ['lib', '.a'], ['lib', '.so'],
                 ['lib', '.dylib'], ['lib', '.so'], ['lib', '.dll']);
}

my @path = split /[\/\\]/, $lib_orig;
my $lib_base = pop @path;
my $lib_dir = join '/', @path;
my $lib = $lib_dir . '/' . $lib_base;

my $idx = 0;
while (!-r $lib && $idx < scalar @decorators) {
  my $dec = $decorators[$idx++];
  $lib = $lib_dir . '/' . $dec->[0] . $lib_base . $dec->[1];
}

die "Can't find $lib_orig\n" unless -r $lib;

if ($^O eq 'MSWin32') {
  my $headers = `dumpbin /headers $lib`;
  (my $machine) = ($headers =~ /machine \((\w+)\)/);
  my $prefix = ($machine eq 'x64' ? '' : '_');
  my $suffix = ($machine eq 'x64' ? ' = @ILT\\+\\d+' : '@\\d+');
  $pattern = " $prefix(Java_\\S+)$suffix";
  $pattern .= '| (Java_\\S+)' if $machine eq 'x64';
}

my %exports;
open TEMP, "$nm $lib |" or die "Can't spawn $nm\n";
while (<TEMP>) {
  if (/$pattern/o) {
    $exports{defined $1 ? $1 : $2} = 1;
#    print "DBG: [$1] is exported.\n";
  }
}
close TEMP;

my $err = 0;
opendir DOT, '.' or die "Can't open . dir\n";
my @hfiles = grep /\.h$/, readdir DOT;
closedir DOT;
for my $hfile (@hfiles) {
  open H, $hfile or die "Can't open $hfile\n";
#  print "DBG: Reading $hfile\n";
  while (<H>) {
    if (/JNIEXPORT\s+\S+\s+\S+\s+(Java_\S+)/) {
#      print "DBG: Found declaration for [$1]\n";
      if (!exists $exports{$1}) {
        print "$0: error : $lib should be exporting [$1]\n";
        ++$err;
      }
    }
  }
  close H;
}

exit $err;
