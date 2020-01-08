eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;

eval('use JSON::PP;');
if ($@) {
  print STDERR "is_topic_type: JSON::PP not installed, skipping test and returning 0.\n";
  exit(0);
}

# The expected results of is_topic_type()
# dn and no_dn refers to running opendds_idl with --default-nested and
# --no-default-nested respectively
my %expected = (
  "NonAnnotatedStruct"                             => {not_found => 1, dn => 0, no_dn => 1},
  "TopicStruct"                                    => {not_found => 1, dn => 1, no_dn => 1},
  "NestedStruct"                                   => {not_found => 1, dn => 0, no_dn => 0},
  "NestedTrueStruct"                               => {not_found => 1, dn => 0, no_dn => 0},
  "NestedFalseStruct"                              => {not_found => 1, dn => 1, no_dn => 1},
  "NonAnnotatedModule/NonAnnotatedStruct"          => {not_found => 1, dn => 0, no_dn => 1},
  "NonAnnotatedModule/TopicStruct"                 => {not_found => 1, dn => 1, no_dn => 1},
  "NonAnnotatedModule/NestedStruct"                => {not_found => 1, dn => 0, no_dn => 0},
  "NonAnnotatedModule/NestedTrueStruct"            => {not_found => 1, dn => 0, no_dn => 0},
  "NonAnnotatedModule/NestedFalseStruct"           => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedModule/NonAnnotatedStruct"         => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedModule/TopicStruct"                => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedModule/NestedStruct"               => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedModule/NestedTrueStruct"           => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedModule/NestedFalseStruct"          => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedTrueModule/NonAnnotatedStruct"     => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedTrueModule/TopicStruct"            => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedTrueModule/NestedStruct"           => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedTrueModule/NestedTrueStruct"       => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedTrueModule/NestedFalseStruct"      => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedFalseModule/NonAnnotatedStruct"    => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedFalseModule/TopicStruct"           => {not_found => 1, dn => 1, no_dn => 1},
  "DefaultNestedFalseModule/NestedStruct"          => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedFalseModule/NestedTrueStruct"      => {not_found => 1, dn => 0, no_dn => 0},
  "DefaultNestedFalseModule/NestedFalseStruct"     => {not_found => 1, dn => 1, no_dn => 1},
);

sub subtest {
  my $status = 0;
  my $mode = shift;

  my $itl_file = "$mode/is_topic_type.itl";
  open my $f, '<', $itl_file or die "Can't open itl file $itl_file: $!";
  local $/;
  my $itl_text = <$f>;
  close $f;

  # Check the Types in the ITL
  my $itl = decode_json($itl_text);
  my @types = @{$itl->{'types'}};
  foreach my $type (@types) {
    die "There's a type with no name" unless exists $type->{'name'};
    $type->{'name'} =~ /IDL:(.*):.*/;
    my $name = $1;
    die "Type \"$name\" was not expected at all" unless exists $expected{$name};
    die "Type \"$name\" does not have \"note\"" unless exists $type->{'note'};
    die "Type \"$name\" does not have \"is_dcps_data_type\"" unless exists $type->{'note'}->{'is_dcps_data_type'};

    $expected{$name}{not_found} = 0;
    my $expected_value = $expected{$name}{$mode};
    my $itl_value = $type->{'note'}->{'is_dcps_data_type'} ? 1 : 0;
    if ($expected_value != $itl_value) {
      my $exp = $expected_value ? "NOT " : "";
      my $act = $itl_value ? " NOT" : "";
      print STDERR "ERROR in $mode: Expected \"$name\" ${exp}to be a topic type but it was${act}.\n";
      $status = 1;
    }
  }
  # Make sure we found everything in %expected
  while (my ($k, $v) = each %expected) {
    if ($v->{not_found}) {
      print STDERR "ERROR in $mode: $k was not found in ITL\n";
      $status = 1;
    }
    $v->{not_found} = 1;
  }
  return $status;
}

my $status = 0;
$status |= subtest('dn');
$status |= subtest('no_dn');

exit $status;
