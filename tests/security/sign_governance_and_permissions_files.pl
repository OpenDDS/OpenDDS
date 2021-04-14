#!/usr/bin/env perl

use Cwd;
use strict;
my $scriptdir= getcwd;
my $govdir = <$scriptdir/governance>;
my $permdir = <$scriptdir/permissions>;
my $ca_cert = <$scriptdir/certs/opendds_identity_ca_cert.pem>;
my $ca_private_key = <$scriptdir/certs/opendds_identity_ca_private_key.pem>;

#gensigned ($govdir);
gensigned ($permdir);

sub gensigned {
    my $dirname = shift;
    my @files = <$dirname/*.xml>;

    foreach my $f (@files) {
        my $filename_wo_extension = $f;
        $filename_wo_extension =~ s{.*/}{}; # remove path
        $filename_wo_extension =~ s{\.[^.]+$}{}; # remove .xml
        #my $filename_wo_extension = `basename $f .xml`;
        chomp $filename_wo_extension;
        my $outfile = ${dirname} . '/signed/' . ${filename_wo_extension} . '.p7s';
        my $openssl = "openssl smime -sign -in $f -text -out $outfile -signer $ca_cert -inkey $ca_private_key";
        system($openssl);
    }
}
