eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Create generic permissions with no domains specified or other restrictions
# based on what identities have been generated with the identity CA
# (participant_01, etc.).
#
# Sign with sec-doc-manager.py after generating again.

use File::Basename;
use strict;

my $output_path = dirname(__FILE__);
my $identity_ca_path = "$output_path/../certs/identity";

my $template_before_subject = << "EOF";
<?xml version="1.0" encoding="utf-8"?>
<!-- Don't edit and sign these by hand, edit and run generate_generic_permissions.pl! -->
<dds xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://www.omg.org/spec/DDS-SECURITY/20170901/omg_shared_ca_permissions.xsd">
  <permissions>
    <grant name="TheGrant">
      <subject_name>
EOF
chop($template_before_subject); # Remove newline at end
my $template_after_subject = << "EOF";
</subject_name>
      <validity>
        <!-- Format is CCYY-MM-DDThh:mm:ss[Z|(+|-)hh:mm] in GMT -->
        <not_before>2015-09-15T01:00:00</not_before>
        <not_after>2025-09-15T01:00:00</not_after>
      </validity>
      <allow_rule>
        <domains>
          <id_range>
            <min>0</min>
          </id_range>
        </domains>
        <publish>
          <topics>
            <topic>*</topic>
          </topics>
        </publish>
        <subscribe>
          <topics>
            <topic>*</topic>
          </topics>
        </subscribe>
      </allow_rule>
      <default>DENY</default>
    </grant>
  </permissions>
</dds>
EOF

sub get_permissions_file_contents {
  my $subject_name = shift;
  return "${template_before_subject}${subject_name}${template_after_subject}";
};

sub get_participant_name {
  my $num = shift;
  return "test_participant_$num";
};

sub get_permissions_file_base_name {
  my $num = shift;
  my $name = get_participant_name($num);
  return "permissions_$name";
};

sub get_permissions_file_unsigned_name {
  my $num = shift;
  return get_permissions_file_base_name($num) . '.xml';
};

sub convert_subject_to_rfc4514_format {
  my $dn = shift;
  my @rdns = split(/\//, $dn);
  # It's observed that the first element can be an empty string.
  if ($rdns[0] eq "") {
    shift(@rdns);
  }
  return join(',', @rdns);
}

my %subjects = ();
open(my $identity_index, '<', "$identity_ca_path/index.txt")
  or die("Couldn't open $identity_ca_path: $!");
while (<$identity_index>) {
  my @fields = split('\t', $_);
  my $participant = $fields[3];
  my $dn = $fields[5];
  chop($dn); # Remove newline
  my $subject = convert_subject_to_rfc4514_format($dn);
  $subjects{$participant} = $subject;
}
close($identity_index);

while (my($participant, $subject) = each %subjects) {
  my $unsigned_path = "$output_path/" . get_permissions_file_unsigned_name($participant);
  print("Creating $unsigned_path\n");
  open(my $unsigned_file, '>', $unsigned_path)
    or die("Couldn't open $unsigned_path: $!");
  print $unsigned_file get_permissions_file_contents($subject);
  close($unsigned_file);
}
