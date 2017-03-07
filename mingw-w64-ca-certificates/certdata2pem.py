#!/usr/bin/python
# vim:set et sw=4:
#
# certdata2pem.py - splits certdata.txt into multiple files
#
# Copyright (C) 2009 Philipp Kern <pkern@debian.org>
# Copyright (C) 2013 Kai Engert <kaie@redhat.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301,
# USA.

import base64
import os.path
import re
import sys
import textwrap
import urllib
import subprocess

objects = []

def printable_serial(obj):
  return ".".join(map(lambda x:str(ord(x)), obj['CKA_SERIAL_NUMBER']))

# Dirty file parser.
in_data, in_multiline, in_obj = False, False, False
field, type, value, obj = None, None, None, dict()
for line in open('certdata.txt', 'r'):
    # Ignore the file header.
    if not in_data:
        if line.startswith('BEGINDATA'):
            in_data = True
        continue
    # Ignore comment lines.
    if line.startswith('#'):
        continue
    # Empty lines are significant if we are inside an object.
    if in_obj and len(line.strip()) == 0:
        objects.append(obj)
        obj = dict()
        in_obj = False
        continue
    if len(line.strip()) == 0:
        continue
    if in_multiline:
        if not line.startswith('END'):
            if type == 'MULTILINE_OCTAL':
                line = line.strip()
                for i in re.finditer(r'\\([0-3][0-7][0-7])', line):
                    value += chr(int(i.group(1), 8))
            else:
                value += line
            continue
        obj[field] = value
        in_multiline = False
        continue
    if line.startswith('CKA_CLASS'):
        in_obj = True
    line_parts = line.strip().split(' ', 2)
    if len(line_parts) > 2:
        field, type = line_parts[0:2]
        value = ' '.join(line_parts[2:])
    elif len(line_parts) == 2:
        field, type = line_parts
        value = None
    else:
        raise NotImplementedError('line_parts < 2 not supported.\n' + line)
    if type == 'MULTILINE_OCTAL':
        in_multiline = True
        value = ""
        continue
    obj[field] = value
if len(obj.items()) > 0:
    objects.append(obj)

# Build up trust database.
trustmap = dict()
for obj in objects:
    if obj['CKA_CLASS'] != 'CKO_NSS_TRUST':
        continue
    key = obj['CKA_LABEL'] + printable_serial(obj)
    trustmap[key] = obj
    print " added trust", key

# Build up cert database.
certmap = dict()
for obj in objects:
    if obj['CKA_CLASS'] != 'CKO_CERTIFICATE':
        continue
    key = obj['CKA_LABEL'] + printable_serial(obj)
    certmap[key] = obj
    print " added cert", key

def obj_to_filename(obj):
    label = obj['CKA_LABEL'][1:-1]
    label = label.replace('/', '_')\
        .replace(' ', '_')\
        .replace(':', '_')\
        .replace('(', '=')\
        .replace(')', '=')\
        .replace(',', '_')
    label = re.sub(r'\\x[0-9a-fA-F]{2}', lambda m:chr(int(m.group(0)[2:], 16)), label)
    serial = printable_serial(obj)
    return label + "_" + serial

def write_cert_ext_to_file(f, oid, value, public_key):
    f.write("[p11-kit-object-v1]\n")
    f.write("label: ");
    f.write(tobj['CKA_LABEL'])
    f.write("\n")
    f.write("class: x-certificate-extension\n");
    f.write("object-id: " + oid + "\n")
    f.write("value: \"" + value + "\"\n")
    f.write("modifiable: false\n");
    f.write(public_key)

trust_types = {
  "CKA_TRUST_DIGITAL_SIGNATURE": "digital-signature",
  "CKA_TRUST_NON_REPUDIATION": "non-repudiation",
  "CKA_TRUST_KEY_ENCIPHERMENT": "key-encipherment",
  "CKA_TRUST_DATA_ENCIPHERMENT": "data-encipherment",
  "CKA_TRUST_KEY_AGREEMENT": "key-agreement",
  "CKA_TRUST_KEY_CERT_SIGN": "cert-sign",
  "CKA_TRUST_CRL_SIGN": "crl-sign",
  "CKA_TRUST_SERVER_AUTH": "server-auth",
  "CKA_TRUST_CLIENT_AUTH": "client-auth",
  "CKA_TRUST_CODE_SIGNING": "code-signing",
  "CKA_TRUST_EMAIL_PROTECTION": "email-protection",
  "CKA_TRUST_IPSEC_END_SYSTEM": "ipsec-end-system",
  "CKA_TRUST_IPSEC_TUNNEL": "ipsec-tunnel",
  "CKA_TRUST_IPSEC_USER": "ipsec-user",
  "CKA_TRUST_TIME_STAMPING": "time-stamping",
  "CKA_TRUST_STEP_UP_APPROVED": "step-up-approved",
}

legacy_trust_types = {
  "LEGACY_CKA_TRUST_SERVER_AUTH": "server-auth",
  "LEGACY_CKA_TRUST_CODE_SIGNING": "code-signing",
  "LEGACY_CKA_TRUST_EMAIL_PROTECTION": "email-protection",
}

legacy_to_real_trust_types = {
  "LEGACY_CKA_TRUST_SERVER_AUTH": "CKA_TRUST_SERVER_AUTH",
  "LEGACY_CKA_TRUST_CODE_SIGNING": "CKA_TRUST_CODE_SIGNING",
  "LEGACY_CKA_TRUST_EMAIL_PROTECTION": "CKA_TRUST_EMAIL_PROTECTION",
}

openssl_trust = {
  "CKA_TRUST_SERVER_AUTH": "serverAuth",
  "CKA_TRUST_CLIENT_AUTH": "clientAuth",
  "CKA_TRUST_CODE_SIGNING": "codeSigning",
  "CKA_TRUST_EMAIL_PROTECTION": "emailProtection",
}

for tobj in objects:
    if tobj['CKA_CLASS'] == 'CKO_NSS_TRUST':
        key = tobj['CKA_LABEL'] + printable_serial(tobj)
        print "producing trust for " + key
        trustbits = []
        distrustbits = []
        openssl_trustflags = []
        openssl_distrustflags = []
        legacy_trustbits = []
        legacy_openssl_trustflags = []
        for t in trust_types.keys():
            if tobj.has_key(t) and tobj[t] == 'CKT_NSS_TRUSTED_DELEGATOR':
                trustbits.append(t)
                if t in openssl_trust:
                    openssl_trustflags.append(openssl_trust[t])
            if tobj.has_key(t) and tobj[t] == 'CKT_NSS_NOT_TRUSTED':
                distrustbits.append(t)
                if t in openssl_trust:
                    openssl_distrustflags.append(openssl_trust[t])

        for t in legacy_trust_types.keys():
            if tobj.has_key(t) and tobj[t] == 'CKT_NSS_TRUSTED_DELEGATOR':
                real_t = legacy_to_real_trust_types[t]
                legacy_trustbits.append(real_t)
                if real_t in openssl_trust:
                    legacy_openssl_trustflags.append(openssl_trust[real_t])
            if tobj.has_key(t) and tobj[t] == 'CKT_NSS_NOT_TRUSTED':
                raise NotImplementedError, 'legacy distrust not supported.\n' + line

        fname = obj_to_filename(tobj)
        try:
            obj = certmap[key]
        except:
            obj = None

        # optional debug code, that dumps the parsed input to files
        #fulldump = "dump-" + fname
        #dumpf = open(fulldump, 'w')
        #dumpf.write(str(obj));
        #dumpf.write(str(tobj));
        #dumpf.close();

        is_legacy = 0
        if tobj.has_key('LEGACY_CKA_TRUST_SERVER_AUTH') or tobj.has_key('LEGACY_CKA_TRUST_EMAIL_PROTECTION') or tobj.has_key('LEGACY_CKA_TRUST_CODE_SIGNING'):
            is_legacy = 1
            if obj == None:
                raise NotImplementedError, 'found legacy trust without certificate.\n' + line

            legacy_fname = "legacy-default/" + fname + ".crt"
            f = open(legacy_fname, 'w')
            f.write("# alias=%s\n"%tobj['CKA_LABEL'])
            f.write("# trust=" + " ".join(legacy_trustbits) + "\n")
            if legacy_openssl_trustflags:
                f.write("# openssl-trust=" + " ".join(legacy_openssl_trustflags) + "\n")
            f.write("-----BEGIN CERTIFICATE-----\n")
            f.write("\n".join(textwrap.wrap(base64.b64encode(obj['CKA_VALUE']), 64)))
            f.write("\n-----END CERTIFICATE-----\n")
            f.close()

            if tobj.has_key('CKA_TRUST_SERVER_AUTH') or tobj.has_key('CKA_TRUST_EMAIL_PROTECTION') or tobj.has_key('CKA_TRUST_CODE_SIGNING'):
                legacy_fname = "legacy-disable/" + fname + ".crt"
                f = open(legacy_fname, 'w')
                f.write("# alias=%s\n"%tobj['CKA_LABEL'])
                f.write("# trust=" + " ".join(trustbits) + "\n")
                if openssl_trustflags:
                    f.write("# openssl-trust=" + " ".join(openssl_trustflags) + "\n")
                f.write("-----BEGIN CERTIFICATE-----\n")
                f.write("\n".join(textwrap.wrap(base64.b64encode(obj['CKA_VALUE']), 64)))
                f.write("\n-----END CERTIFICATE-----\n")
                f.close()

            # don't produce p11-kit output for legacy certificates
            continue

        pk = ''
        cert_comment = ''
        if obj != None:
            # must extract the public key from the cert, let's use openssl
            cert_fname = "cert-" + fname
            fc = open(cert_fname, 'w')
            fc.write("-----BEGIN CERTIFICATE-----\n")
            fc.write("\n".join(textwrap.wrap(base64.b64encode(obj['CKA_VALUE']), 64)))
            fc.write("\n-----END CERTIFICATE-----\n")
            fc.close();
            pk_fname = "pubkey-" + fname
            fpkout = open(pk_fname, "w")
            dump_pk_command = ["openssl", "x509", "-in", cert_fname, "-noout", "-pubkey"]
            subprocess.call(dump_pk_command, stdout=fpkout)
            fpkout.close()
            with open (pk_fname, "r") as myfile:
                pk=myfile.read()
            # obtain certificate information suitable as a comment
            comment_fname = "comment-" + fname
            fcout = open(comment_fname, "w")
            comment_command = ["openssl", "x509", "-in", cert_fname, "-noout", "-text"]
            subprocess.call(comment_command, stdout=fcout)
            fcout.close()
            sed_command = ["sed", "--in-place", "s/^/#/", comment_fname]
            subprocess.call(sed_command)
            with open (comment_fname, "r") as myfile:
                cert_comment=myfile.read()

        fname += ".tmp-p11-kit"
        f = open(fname, 'w')

        if obj != None:
            is_distrusted = False
            has_server_trust = False
            has_email_trust = False
            has_code_trust = False

            if tobj.has_key('CKA_TRUST_SERVER_AUTH'):
                if tobj['CKA_TRUST_SERVER_AUTH'] == 'CKT_NSS_NOT_TRUSTED':
                    is_distrusted = True
                elif tobj['CKA_TRUST_SERVER_AUTH'] == 'CKT_NSS_TRUSTED_DELEGATOR':
                    has_server_trust = True

            if tobj.has_key('CKA_TRUST_EMAIL_PROTECTION'):
                if tobj['CKA_TRUST_EMAIL_PROTECTION'] == 'CKT_NSS_NOT_TRUSTED':
                    is_distrusted = True
                elif tobj['CKA_TRUST_EMAIL_PROTECTION'] == 'CKT_NSS_TRUSTED_DELEGATOR':
                    has_email_trust = True

            if tobj.has_key('CKA_TRUST_CODE_SIGNING'):
                if tobj['CKA_TRUST_CODE_SIGNING'] == 'CKT_NSS_NOT_TRUSTED':
                    is_distrusted = True
                elif tobj['CKA_TRUST_CODE_SIGNING'] == 'CKT_NSS_TRUSTED_DELEGATOR':
                    has_code_trust = True

            if is_distrusted:
                trust_ext_oid = "1.3.6.1.4.1.3319.6.10.1"
                trust_ext_value = "0.%06%0a%2b%06%01%04%01%99w%06%0a%01%04 0%1e%06%08%2b%06%01%05%05%07%03%04%06%08%2b%06%01%05%05%07%03%01%06%08%2b%06%01%05%05%07%03%03"
                write_cert_ext_to_file(f, trust_ext_oid, trust_ext_value, pk)

            trust_ext_oid = "2.5.29.37"
            if has_server_trust:
                if has_email_trust:
                    if has_code_trust:
                        # server + email + code
                        trust_ext_value = "0%2a%06%03U%1d%25%01%01%ff%04 0%1e%06%08%2b%06%01%05%05%07%03%04%06%08%2b%06%01%05%05%07%03%01%06%08%2b%06%01%05%05%07%03%03"
                    else:
                        # server + email
                        trust_ext_value = "0 %06%03U%1d%25%01%01%ff%04%160%14%06%08%2b%06%01%05%05%07%03%04%06%08%2b%06%01%05%05%07%03%01"
                else:
                    if has_code_trust:
                        # server + code
                        trust_ext_value = "0 %06%03U%1d%25%01%01%ff%04%160%14%06%08%2b%06%01%05%05%07%03%01%06%08%2b%06%01%05%05%07%03%03"
                    else:
                        # server
                        trust_ext_value = "0%16%06%03U%1d%25%01%01%ff%04%0c0%0a%06%08%2b%06%01%05%05%07%03%01"
            else:
                if has_email_trust:
                    if has_code_trust:
                        # email + code
                        trust_ext_value = "0 %06%03U%1d%25%01%01%ff%04%160%14%06%08%2b%06%01%05%05%07%03%04%06%08%2b%06%01%05%05%07%03%03"
                    else:
                        # email
                        trust_ext_value = "0%16%06%03U%1d%25%01%01%ff%04%0c0%0a%06%08%2b%06%01%05%05%07%03%04"
                else:
                    if has_code_trust:
                        # code
                        trust_ext_value = "0%16%06%03U%1d%25%01%01%ff%04%0c0%0a%06%08%2b%06%01%05%05%07%03%03"
                    else:
                        # none
                        trust_ext_value = "0%18%06%03U%1d%25%01%01%ff%04%0e0%0c%06%0a%2b%06%01%04%01%99w%06%0a%10"

            # no 2.5.29.37 for neutral certificates
            if (is_distrusted or has_server_trust or has_email_trust or has_code_trust):
                write_cert_ext_to_file(f, trust_ext_oid, trust_ext_value, pk)

            pk = ''
            f.write("\n")

            f.write("[p11-kit-object-v1]\n")
            f.write("label: ");
            f.write(tobj['CKA_LABEL'])
            f.write("\n")
            if is_distrusted:
                f.write("x-distrusted: true\n")
            elif has_server_trust or has_email_trust or has_code_trust:
                f.write("trusted: true\n")
            else:
                f.write("trusted: false\n")

            # requires p11-kit >= 0.23.4
            f.write("nss-mozilla-ca-policy: true\n")
            f.write("modifiable: false\n");

            f.write("-----BEGIN CERTIFICATE-----\n")
            f.write("\n".join(textwrap.wrap(base64.b64encode(obj['CKA_VALUE']), 64)))
            f.write("\n-----END CERTIFICATE-----\n")
            f.write(cert_comment)
            f.write("\n")

        else:
            f.write("[p11-kit-object-v1]\n")
            f.write("label: ");
            f.write(tobj['CKA_LABEL']);
            f.write("\n")
            f.write("class: certificate\n")
            f.write("certificate-type: x-509\n")
            f.write("modifiable: false\n");
            f.write("issuer: \"");
            f.write(urllib.quote(tobj['CKA_ISSUER']));
            f.write("\"\n")
            f.write("serial-number: \"");
            f.write(urllib.quote(tobj['CKA_SERIAL_NUMBER']));
            f.write("\"\n")
            if (tobj['CKA_TRUST_SERVER_AUTH'] == 'CKT_NSS_NOT_TRUSTED') or (tobj['CKA_TRUST_EMAIL_PROTECTION'] == 'CKT_NSS_NOT_TRUSTED') or (tobj['CKA_TRUST_CODE_SIGNING'] == 'CKT_NSS_NOT_TRUSTED'):
              f.write("x-distrusted: true\n")
            f.write("\n\n")
        f.close()
        print " -> written as '%s', trust = %s, openssl-trust = %s, distrust = %s, openssl-distrust = %s" % (fname, trustbits, openssl_trustflags, distrustbits, openssl_distrustflags)
