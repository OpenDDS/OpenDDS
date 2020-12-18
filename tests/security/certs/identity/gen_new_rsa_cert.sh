#!/bin/bash
if [ $# -eq 0 ]
  then
    echo "Expecing certificate name / prefix as argument"
fi

openssl genrsa -out $1_private_key.pem 2048
openssl req -new -key $1_private_key.pem -out $1.csr
openssl ca -config identity_ca_openssl.cnf -days 3650 -in $1.csr -out $1_cert.pem
