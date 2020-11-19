#!/bin/bash
if [ $# -eq 0 ]
  then
    echo "Expecing certificate name / prefix as argument"
fi

openssl ecparam -name prime256v1 -genkey -out $1_private_key.pem
openssl req -new -key $1_private_key.pem -out $1.csr
openssl ca -config identity_ca_openssl.cnf -days 3650 -in $1.csr -out $1_cert.pem
