#!/bin/bash
# Generate self-signed CA and server certificates for Mosquitto TLS
# Run this script once, then the certs will be mounted into Docker

set -e

CERT_DIR="$(dirname "$0")"
cd "$CERT_DIR"

DAYS=365
SUBJ_CA="/CN=IoT Project CA"
SUBJ_SERVER="/CN=mosquitto"

echo "=== Generating CA key and certificate ==="
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days $DAYS -out ca.crt -subj "$SUBJ_CA"

echo "=== Generating server key and CSR ==="
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr -subj "$SUBJ_SERVER"

echo "=== Signing server certificate with CA ==="
# SAN extension so the cert is valid for 'mosquitto' (Docker hostname) and localhost
cat > server_ext.cnf <<EOF
[v3_req]
subjectAltName = DNS:mosquitto, DNS:localhost, IP:127.0.0.1, IP:192.168.1.134
EOF

openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days $DAYS -sha256 -extfile server_ext.cnf -extensions v3_req

# Cleanup intermediate files
rm -f server.csr server_ext.cnf ca.srl

echo ""
echo "=== Certificates generated ==="
echo "  CA cert:     $CERT_DIR/ca.crt"
echo "  Server cert: $CERT_DIR/server.crt"
echo "  Server key:  $CERT_DIR/server.key"
echo ""
echo "Copy ca.crt content into your Secrets.h as the CA_CERT string."
