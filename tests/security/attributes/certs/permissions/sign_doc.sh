#!/bin/bash
if [ $# -eq 0 ]
  then
    echo "Expecing document name / prefix as argument"
fi

PREFIX=`echo ${1} | cut -d '.' -f -1`

openssl smime -sign -in ${1} -text -out ${PREFIX}_signed.p7s -signer ${DDS_ROOT}/tests/security/attributes/certs/permissions/permissions_ca_cert.pem -inkey ${DDS_ROOT}/tests/security/attributes/certs/permissions/permissions_ca_private_key.pem

