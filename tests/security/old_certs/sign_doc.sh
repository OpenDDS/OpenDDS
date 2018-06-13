#!/bin/bash
set -e

if [ $# -eq 0 ]
  then
    echo "Expecing document name / prefix as argument"
    exit 1
fi

PREFIX=`echo ${1} | cut -d '.' -f -1`

openssl smime -sign -in ${1} -text -out ${PREFIX}_signed.p7s -signer ${DDS_ROOT}/tests/security/certs/opendds_identity_ca_cert.pem -inkey ${DDS_ROOT}/tests/security/certs/opendds_identity_ca_private_key.pem

