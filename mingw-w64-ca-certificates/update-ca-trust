#!/bin/sh

#set -vx

# At this time, while this script is trivial, we ignore any parameters given.
# However, for backwards compatibility reasons, future versions of this script must 
# support the syntax "update-ca-trust extract" trigger the generation of output 
# files in $DEST.

DEST=@PREFIX@/etc/pki/ca-trust/extracted

# OpenSSL PEM bundle that includes trust flags
# (BEGIN TRUSTED CERTIFICATE)
@PREFIX@/bin/p11-kit.exe extract --format=openssl-bundle --filter=certificates --overwrite --comment $DEST/openssl/ca-bundle.trust.crt
@PREFIX@/bin/p11-kit.exe extract --format=pem-bundle --filter=ca-anchors --overwrite --comment --purpose server-auth $DEST/pem/tls-ca-bundle.pem
@PREFIX@/bin/p11-kit.exe extract --format=pem-bundle --filter=ca-anchors --overwrite --comment --purpose email $DEST/pem/email-ca-bundle.pem
@PREFIX@/bin/p11-kit.exe extract --format=pem-bundle --filter=ca-anchors --overwrite --comment --purpose code-signing $DEST/pem/objsign-ca-bundle.pem
@PREFIX@/bin/p11-kit.exe extract --format=java-cacerts --filter=ca-anchors --overwrite --purpose server-auth $DEST/java/cacerts
