#!/usr/bin/env bash

# Generates cert chains for given URLs

set -euox pipefail

d=data
grep _URL ../src/secrets.h | awk -F\" '{print $2}' | while read -r url; do
  echo "Fetching cert for $url"
  # strip url down to host
  host=$(echo $url | awk -F/ '{print $3}')
  openssl s_client -showcerts -connect ${host}:443 </dev/null > ${d}/${host}_chain.pem
  ./extract-cert-chain.py ${d}/${host}_chain.pem ${d}/${host}_chain_extracted.pem
  cat - <<EOF >../src/certs.h
  #ifndef CERTS_H
  #define CERTS_H

  const char rootCACerts[] = R"EOF(
  $(cat ${d}/*_chain_extracted.pem)
  )EOF";

  #endif // CERTS_H
EOF
done
