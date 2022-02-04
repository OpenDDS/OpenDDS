#!/bin/bash
if [ $# -eq 0 ]; then
  echo "Expecing document name as argument."
  exit -1;
fi

if [ "$DDS_ROOT" == "" ]; then
  echo "DDS_ROOT environment variable not set."
  exit -1;
fi

inpath="$1"
outpath="${inpath%.xml}_signed.p7s"

if [ ! -f ${1} ]; then
  echo "File '${1}' doesn't exist."
  exit -1;
fi

openssl smime -sign -in "${inpath}" -text -out "${outpath}" -signer ${DDS_ROOT}/tests/security/certs/permissions/permissions_ca_cert.pem -inkey ${DDS_ROOT}/tests/security/certs/permissions/permissions_ca_private_key.pem
