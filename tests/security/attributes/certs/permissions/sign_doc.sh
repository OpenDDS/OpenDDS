#!/bin/bash
if [ $# -eq 0 ]
  then
    echo "Expecing document name / prefix as argument"
fi

PREFIX=`echo ${1} | cut -d '.' -f -1`

openssl smime -sign -in ${1} -text -out ${PREFIX}_signed.p7s -signer permissions_ca_cert.pem -inkey permissions_ca_private_key.pem

