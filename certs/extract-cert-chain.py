#!/usr/bin/env python3
 
import sys
import re

def extract_certificates(input_file, output_file):
    with open(input_file, 'r') as f:
        data = f.read()

    cert_pattern = re.compile(
        r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----',
        re.DOTALL
    )

    certs = cert_pattern.findall(data)
    if not certs:
        print("No certificates found in the input file.")
        return

    with open(output_file, 'w') as f_out:
        for cert in certs:
            f_out.write(cert + '\n\n')

    print(f"Extracted {len(certs)} certificates saved to {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python extract_certs.py <openssl_output.txt> <output_pem_file.pem>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    extract_certificates(input_file, output_file)
