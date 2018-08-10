#!/bin/bash
if [ $# -eq 0 ]; then
  echo "Expecing document name as argument."
  exit -1;
fi

if [ "$DDS_ROOT" == "" ]; then
  echo "DDS_ROOT environment variable not set."
  exit -1;
fi

PREFIX=`echo ${1} | rev | cut -d '/' -f 1 | cut -d '.' -f 2- | rev`

if [ "$PREFIX" == "" ]; then
  echo "Unable to determine document prefix."
  exit -1;
fi

if [ ! -f ${1} ]; then
  echo "File '${1}' doesn't exist."
  exit -1;
fi

openssl smime -sign -in ${1} -text -out ${PREFIX}_signed.p7s -signer ${DDS_ROOT}/tests/security/certs/permissions/permissions_ca_cert.pem -inkey ${DDS_ROOT}/tests/security/certs/permissions/permissions_ca_private_key.pem

