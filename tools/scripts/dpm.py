#!/usr/bin/env python3

import argparse
import os
import sys
from pathlib import Path
import subprocess

# sign governance and permissions

dpm_path = Path('./DPM')

class CertificateAuthority:
    def __init__(self, name):
        self.name = name
        self.path = dpm_path / 'ca' / name
        self.config_path = self.path / 'openssl.cnf'
        self.key_path = self.path / 'key.pem'
        self.cert_path = self.path / 'cert.pem'

    def init(self):
        csr_path = self.path / 'csr'

        os.makedirs(self.path)
        config_content = f'''
#
# OpenSSL example Certificate Authority configuration file.

####################################################################
[ ca ]
default_ca  = CA_default    # The default ca section

####################################################################
[ CA_default ]

dir    = {self.path}      # Where everything is kept
certs    = $dir/certs    # Where the issued certs are kept
crl_dir    = $dir/crl    # Where the issued crl are kept
database  = $dir/index.txt  # database index file.

new_certs_dir  = $dir

certificate  = {self.cert_path}      # The CA certificate
serial    = $dir/serial             # The current serial number
crlnumber  = $dir/crlnumber          # the current crl number
                  # must be commented out to leave a V1 CRL
crl    = $dir/crl.pem             # The current CRL
private_key  = {self.key_path}      # The private key
RANDFILE  = $dir/.rand            # private random number file

#x509_extensions  = usr_cert    # The extentions to add to the cert

# Comment out the following two lines for the "traditional"
# (and highly broken) format.
name_opt   = ca_default    # Subject Name options
cert_opt   = ca_default    # Certificate field options

# Extension copying option: use with caution.
# copy_extensions = copy

# Extensions to add to a CRL. Note: Netscape communicator chokes on V2 CRLs
# so this is commented out by default to leave a V1 CRL.
# crlnumber must also be commented out to leave a V1 CRL.
# crl_extensions  = crl_ext

default_days  = 365      # how long to certify for
default_crl_days= 30      # how long before next CRL
default_md  = sha256    # which md to use.
preserve  = no      # keep passed DN ordering

# A few difference way of specifying how similar the request should look
# For type CA, the listed attributes must be the same, and the optional
# and supplied fields are just that :-)
policy    = policy_match

# For the CA policy
[ policy_match ]
emailAddress    = optional
commonName    = supplied
organizationalUnitName  = optional
organizationName  = optional
stateOrProvinceName  = optional
countryName    = optional

# For the 'anything' policy
# At this point in time, you must list all acceptable 'object'
# types.
[ policy_anything ]
emailAddress    = optional
commonName    = supplied
organizationalUnitName  = optional
organizationName  = optional
localityName    = optional
stateOrProvinceName  = optional
countryName    = optional

[ req ]
prompt                  = no
#default_bits    = 1024
#default_keyfile   = privkey.pem
distinguished_name  = req_distinguished_name
#attributes    = req_attributes
req_extensions  = v3_ca  # The extentions to add to the self signed cert

[ v3_ca ]
basicConstraints        = critical, CA:TRUE
subjectKeyIdentifier    = hash
#authorityKeyIdentifier  = keyid:always, issuer:always
keyUsage                = critical, digitalSignature, keyCertSign

[ req_distinguished_name ]
countryName         = US
stateOrProvinceName = MO
localityName        = Saint Louis
0.organizationName  = Object Computing ({self.name} CA)
commonName          = Object Computing ({self.name} CA)
emailAddress        = info@objectcomputing.com
'''
        with open(self.config_path, 'w') as file:
            print(config_content, file=file)
        index_path = self.path / 'index.txt'
        index_path.touch()
        serial_path = self.path / 'serial'
        serial_path.write_text('01')
        subprocess.check_call(['openssl', 'ecparam', '-name', 'prime256v1', '-genkey', '-out', self.key_path])
        subprocess.check_call(['openssl', 'req', '-config', self.config_path, '-new', '-key', self.key_path, '-out', csr_path])
        subprocess.check_call(['openssl', 'x509', '-req', '-days', '3650', '-copy_extensions=copyall', '-in', csr_path, '-signkey', self.key_path, '-out', self.cert_path])

    def gencert(self, name, sn):
        identity_path = dpm_path / 'identity'
        path = identity_path / name
        if os.path.isdir(path):
            print(f'{path} already exists')
            return
        os.makedirs(path)
        key_path = path / 'key.pem'
        csr_path = path / 'csr'
        cert_path = path / 'cert.pem'
        subprocess.check_call(['openssl', 'ecparam', '-name', 'prime256v1', '-genkey', '-out', key_path])
        subprocess.check_call(['openssl', 'req', '-new', '-key', key_path, '-out', csr_path, '-subj', sn])
        subprocess.check_call(['openssl', 'ca', '-batch', '-config', self.config_path, '-days', '3650', '-in', csr_path, '-out', cert_path])

    def sign(self, path, name):
        signed_path = dpm_path / 'signed'
        os.makedirs(signed_path, exist_ok=True)
        out_path = signed_path / name

        subprocess.check_call(['openssl', 'smime', '-sign', '-in', path, '-text', '-out', out_path, '-signer', self.cert_path, '-inkey', self.key_path])

if __name__ == '__main__':
    argp = argparse.ArgumentParser()
    argp.add_argument('--init', action='store_true', help="initialize the DPM if it doesn't exist")
    argp.add_argument('--gencert', type=str, nargs=2, help='Create a new certificate from the given subject name')
    argp.add_argument('--sign', type=str, nargs=2, help='Path to a document to sign and the name of the signed document in the DPM')
    args = argp.parse_args()

    identity_ca = CertificateAuthority('identity')
    permissions_ca = CertificateAuthority('permissions')

    if args.init:
        if os.path.isdir(dpm_path):
            print('DPM directory already exists')
            sys.exit(0)
        os.makedirs(dpm_path)
        identity_ca.init()
        permissions_ca.init()
    elif args.gencert:
        name = args.gencert[0]
        sn = args.gencert[1]
        identity_ca.gencert(name, sn)
    elif args.sign:
        path = args.sign[0]
        name = args.sign[1]
        permissions_ca.sign(path, name)
    else:
        print('No command given')
