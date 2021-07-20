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
  "NonAnnotatedStruct"                             => {not_found => 1, dn => 0, no_dn => 1, f => 'is_topic_type'},
  "TopicStruct"                                    => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "TopicStarStruct"                                => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "TopicDdsStruct"                                 => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "TopicOpenDdsStruct"                             => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "TopicInvalidStruct"                             => {not_found => 1, dn => 0, no_dn => 1, f => 'is_topic_type'},
  "NestedStruct"                                   => {not_found => 1, dn => 0, no_dn => 0, f => 'is_topic_type'},
  "NestedTrueStruct"                               => {not_found => 1, dn => 0, no_dn => 0, f => 'is_topic_type'},
  "NestedFalseStruct"                              => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "NonAnnotatedModule/NonAnnotatedStruct"          => {not_found => 1, dn => 0, no_dn => 1, f => 'is_topic_type'},
  "NonAnnotatedModule/TopicStruct"                 => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "NonAnnotatedModule/NestedStruct"                => {not_found => 1, dn => 0, no_dn => 0, f => 'is_topic_type'},
  "NonAnnotatedModule/NestedTrueStruct"            => {not_found => 1, dn => 0, no_dn => 0, f => 'is_topic_type'},
  "NonAnnotatedModule/NestedFalseStruct"           => {not_found => 1, dn => 1, no_dn => 1, f => 'is_topic_type'},
  "DefaultNestedModule/NonAnnotatedStruct"         => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedModule/TopicStruct"                => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
  "DefaultNestedModule/NestedStruct"               => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedModule/NestedTrueStruct"           => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedModule/NestedFalseStruct"          => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
  "DefaultNestedTrueModule/NonAnnotatedStruct"     => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedTrueModule/TopicStruct"            => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
  "DefaultNestedTrueModule/NestedStruct"           => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedTrueModule/NestedTrueStruct"       => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedTrueModule/NestedFalseStruct"      => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
  "DefaultNestedFalseModule/NonAnnotatedStruct"    => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
  "DefaultNestedFalseModule/TopicStruct"           => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
  "DefaultNestedFalseModule/NestedStruct"          => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedFalseModule/NestedTrueStruct"      => {not_found => 1, dn => 0, no_dn => 0, f => 'default_nested_is_topic_type'},
  "DefaultNestedFalseModule/NestedFalseStruct"     => {not_found => 1, dn => 1, no_dn => 1, f => 'default_nested_is_topic_type'},
);

sub subtest {
  my $status = 0;
  my $mode = shift;
  my $fname = shift;

  my $itl_file = "$mode/$fname.itl";
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
    if ($v->{not_found} && $v->{f} eq $fname) {
      print STDERR "ERROR in $mode: $k was not found in ITL\n";
      $status = 1;
    }
    $v->{not_found} = 1;
  }
  return $status;
}

my $status = 0;
$status |= subtest('dn', 'is_topic_type');
$status |= subtest('no_dn', 'is_topic_type');
$status |= subtest('dn', 'default_nested_is_topic_type');
$status |= subtest('no_dn', 'default_nested_is_topic_type');

exit $status;
