eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Create generic permissions with no domains specified or other restrictions
# based on what identities have been generated with the identity CA
# (participant_01, etc.).

use File::Basename;
use strict;

my $output_path = dirname(__FILE__);
my $identity_ca_path = "$output_path/../certs/identity";
my $permissions_ca_path = "$output_path/../certs/permissions";

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

sub get_permissions_file_signed_name {
  my $num = shift;
  return get_permissions_file_base_name($num) . '_signed.p7s';
};

my %subjects = ();
open(my $identity_index, '<', "$identity_ca_path/index.txt")
  or die("Couldn't open $identity_ca_path: $!");
while (<$identity_index>) {
  my @fields = split('\t', $_);
  my $participant = $fields[3];
  my $subject = $fields[5];
  chop($subject); # Remove newline
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

  my $signed_path = "$output_path/" . get_permissions_file_signed_name($participant);
  print("Signing $unsigned_path to make $signed_path\n");
  my $exit_status = system("openssl smime -sign " .
    "-in $unsigned_path -text -out $signed_path " .
    "-signer $permissions_ca_path/permissions_ca_cert.pem " .
    "-inkey $permissions_ca_path/permissions_ca_private_key.pem");
  if ($exit_status) {
    print("OpenSSL Failed!\n");
    exit($exit_status);
  }
}
