#!/usr/bin/env python3

import sys
import os
import subprocess
import io
import re
import configparser
from pathlib import Path
from tempfile import NamedTemporaryFile
from datetime import datetime, timedelta, timezone
from argparse import ArgumentParser

from lxml import etree


test_sec_path = Path(__file__).resolve().parent
root_path = test_sec_path.parent.parent
certs_path = test_sec_path / 'certs'

schema_path = test_sec_path / 'schema'
governance_schema_path = schema_path / 'omg_shared_ca_governance.xsd'
schema_parser = etree.XMLParser(recover=True)
read_schema = lambda p: etree.XMLSchema(etree.XML(p.read_text().encode('utf-8'), parser=schema_parser))
governance_schema = read_schema(governance_schema_path)
permissions_schema_path = schema_path / 'omg_shared_ca_permissions.xsd'
permissions_schema = read_schema(permissions_schema_path)

xml_ext = '.xml'
p7s_ext = '.p7s'
exts = {xml_ext, p7s_ext}


def parse_iso8601(s: str) -> datetime:
    dt = datetime.fromisoformat(s)
    if dt.tzinfo is None:
        return dt.replace(tzinfo=timezone.utc)
    return dt


old_on_purpose = parse_iso8601('2010-09-15 01:00:00+00:00')
year = timedelta(days=356)
now = datetime.now(timezone.utc)
update_if_before = now + year
update_to = now + 10 * year


class Status:
    def __init__(self):
        self.fix = False
        self.got_error = False
        self.verbose = False

    def log(self, *args, unfixable_error=False, **kw):
        if self.fix and not unfixable_error:
            prefix = 'FIX:'
        else:
            prefix = 'ERROR:'
            self.got_error = True
        print(prefix, *args, **kw, file=sys.stderr)

    def exit_status(self):
        return 1 if self.got_error else 0


status = Status()


def ignored_by_git(path: Path, cwd: Path) -> bool:
    result = subprocess.run(
        ['git', 'check-ignore', str(path)], stdout=subprocess.DEVNULL, cwd=cwd)
    if result.returncode == 0:
        return True
    elif result.returncode != 1:
        status.log(f'git check-ignore returned {result.returncode} for {path}', unfixable_error=True)
        return True
    return False


def find_files(dir: Path, exts, *, found = None):
    if found is None:
        found = {}

    # Ignore dir if it's a submodule
    if (dir / '.git').is_file():
        return

    sec_doc_json = dir / 'sec-doc.json'
    if sec_doc_json.is_file():
        return

    for path in dir.iterdir():
        if path.name.startswith('.'):
            continue

        if path.is_file() and path.suffix in exts and not ignored_by_git(path, dir):
            found.setdefault(path.suffix, []).append(path)
        elif path.is_dir() and not ignored_by_git(path, dir):
            find_files(path, exts, found=found)

    return found

def openssl(subcmd, *args,
        quiet=False, want_printed=False,
        check=True, get_stderr=None, get_stdout=None, stderr_on_fail=False):
    if want_printed:
        check = False
        get_stdout = True
        stderr_on_fail = True
    if stderr_on_fail:
        get_stderr = True
    if quiet:
        check = False
        if get_stdout is None:
            get_stdout = True
        if get_stderr is None:
            get_stderr = True
    cmd = ['openssl', subcmd] + [str(a) for a in args]
    if status.verbose:
        print('Running', ' '.join(cmd))
    result = subprocess.run(cmd,
        stdout=subprocess.PIPE if get_stdout else None,
        stderr=subprocess.PIPE if get_stderr else None,
    )
    if status.verbose:
        print(' '.join(cmd), 'exited with status', result.returncode)
    if stderr_on_fail and result.returncode != 0:
        print(result.stderr.decode(), file=sys.stderr)
    if check:
        result.check_returncode()
    return result


def cert_expires(cert: Path) -> datetime:
    result = openssl('x509', '-enddate', '-noout', '-in', cert, want_printed=True, check=True)
    if result.returncode == 0:
        # should be like: notAfter=Jun 10 04:11:03 2028 GMT
        not_after = result.stdout.decode().strip().split('=', 1)[1]
        return datetime.strptime(not_after, '%b %d %H:%M:%S %Y %Z')


var_re = re.compile(r"\$(\w+)")


class CertAuth:
    def __init__(self, cnf_path: Path):
        self.cnf_path = cnf_path
        self.config = configparser.ConfigParser(inline_comment_prefixes=('#', ';'))
        self.config.SECTCRE = re.compile(r"\[\s*(?P<header>[^]]+?)\s*\]")
        self.config.read(self.cnf_path)
        self.ca_section_name = self.config['ca']['default_ca']
        self.ca_section = self.config[self.ca_section_name]

    def get_config_value(self, key: str) -> str:
        value = self.ca_section[key]
        return var_re.sub(lambda m: self.get_config_value(m[1]), value)

    def get_config_path_value(self, key: str) -> Path:
        p = Path(self.get_config_value(key))
        if not p.is_absolute():
            p = self.cnf_path.parent / p
        return p.resolve()

    def certificate(self) -> Path:
        return self.get_config_path_value('certificate')

    def private_key(self) -> Path:
        return self.get_config_path_value('private_key')

    def expires(self) -> datetime:
        return cert_expires(self.certificate())

    def sign_file(self, unsigned_path: Path, signed_path: Path):
        openssl('smime', '-sign', '-in', unsigned_path, '-text', '-out', signed_path,
            '-signer', self.certificate(), '-inkey', self.private_key())

    def verify_file(self, signed_path: Path) -> bool:
        return openssl('smime', '-verify', '-in', signed_path, '-CAfile', self.certificate(), quiet=True).returncode == 0

    def signed_certificate(self, cert_path: Path) -> bool:
        return openssl('verify', '-CAfile', self.certificate(), cert_path, quiet=True).returncode == 0


enc = 'utf-8'


def extract_p7s_contents(path):
    with NamedTemporaryFile(mode='r', encoding=enc) as xml_file:
        openssl('smime', '-verify', '-noverify', '-in', path, '-inform', 'SMIME',
            '-out', xml_file.name, '-text', check=True, quiet=not status.verbose)
        return xml_file.read()


class XmlWrapper:
    def __init__(self, path):
        self.path = path
        self.is_signed = path.suffix == p7s_ext
        self._other_path = None

        if status.verbose:
            print(f'Reading {path}...')
        self.xml_contents = extract_p7s_contents(self.path) if self.is_signed else self.path.read_text()
        self.root = etree.fromstring(self.xml_contents.encode(enc))
        self.tree = etree.ElementTree(self.root)

    def write(self):
        if self.is_signed:
            raise ValueError("Can't write signed!")
        self.path.write_bytes(etree.tostring(
            self.tree, encoding=enc,
            # For some reason, lxml uses double quotes here
            doctype=f'<?xml version="1.0" encoding="{enc}"?>',
            pretty_print=True))

    def _signed(self, signed: bool):
        if signed != self.is_signed:
            raise ValueError('Can\'t use on ' + ('' if self.is_signed else 'un') + 'signed file')
        return self

    @property
    def signed_path(self) -> Path:
        self._signed(False)
        if self._other_path is not None:
            return self._other_path
        return self.path.with_suffix(p7s_ext)

    @signed_path.setter
    def signed_path(self, path: Path) -> Path:
        self._signed(False)._other_path = path

    @property
    def unsigned_path(self) -> Path:
        self._signed(True)
        if self._other_path is not None:
            return self._other_path
        stem = re.sub(r'[-_]signed$', '', self.path.stem, re.I) or self.path.stem
        return self.path.parent / (stem + xml_ext)

    @unsigned_path.setter
    def unsigned_path(self, path: Path) -> Path:
        self._signed(True)._other_path = path

    def is_permissions(self):
        return permissions_schema.validate(self.tree)

    def is_governance(self):
        return governance_schema.validate(self.tree)

    def get_not_afters(self):
        return [(e, parse_iso8601(e.text)) for e in self.root.xpath('//not_after') if e.text]

    def sign(self, ca: CertAuth):
        ca.sign_file(self._signed(False).path, self.signed_path)

    def __repr__(self):
        return str(self.path)


def sort_xml_and_p7s(known_files, ext):
    permissions = {}
    governance = {}
    for path in known_files[ext]:
        xml_wrap = XmlWrapper(path)
        if xml_wrap.is_permissions():
            permissions[path] = xml_wrap
        elif xml_wrap.is_governance():
            governance[path] = xml_wrap
        elif status.verbose:
            print('ignored', ext, path)
    return permissions, governance


def compare_xml_and_p7s(all_xml, all_p7s):
    missing_xml = set()
    for p7s_path, signed_wrap in all_p7s.items():
        unsigned_wrap = all_xml.get(signed_wrap.unsigned_path)
        if unsigned_wrap is None:
            status.log(p7s_path, "doesn't have the expected unsigned file",
                signed_wrap.unsigned_path)
            missing_unsigned.add(signed_wrap)
        else:
            unsigned_wrap.signed_path = p7s_path

    needs_signed = set()
    for xml_path, xml_wrap in all_xml.items():
        signed_wrap = all_p7s.get(xml_wrap.signed_path)
        if signed_wrap is None:
            status.log(xml_path, "doesn't have expected signed file")
            needs_signed.add(xml_wrap)
            continue

        if xml_wrap.xml_contents != signed_wrap.xml_contents:
            status.log(xml_path, "doesn't match", signed_wrap.path)
            needs_signed.add(xml_wrap)

    return missing_xml, needs_signed


def update_permissions_xml_not_before(permissions_xml, permissions_needs_signed):
    for path, xml_wrap in permissions_xml.items():
        update = False
        for not_after_e, not_after_dt in xml_wrap.get_not_afters():
            if not_after_dt > old_on_purpose and not_after_dt < update_if_before:
                expires_in = not_after_dt - now
                if expires_in > timedelta():
                    msg = f'expires in {expires_in}'
                else:
                    msg = f'expired {-expires_in} ago'
                status.log('<not_after> in', path, msg)
                if status.fix:
                    not_after_e.text = update_to.isoformat()
                    update = True

        if update:
            xml_wrap.write()
            permissions_needs_signed.add(xml_wrap)


def sign_documents(signed, needs_signed, ca):
    for xml_wrap in needs_signed:
        signed_path = xml_wrap.signed_path
        if signed_path in signed:
            if not ca.verify_file(signed_path):
                status.log('Need to sign', xml_wrap.path, ', but', signed_path,
                    'was not signed by CA we were going to use', unfixable_error=True)
                continue

        if status.fix:
            xml_wrap.sign(ca)


def main(args):
    status.fix = args.fix
    status.verbose = args.verbose

    identity_ca = CertAuth(certs_path / 'identity/identity_ca_openssl.cnf')
    permissions_ca = CertAuth(certs_path / 'permissions/permissions_ca_openssl.cnf')

    # Find all the files we're interested in
    ext_strs = ', '.join(exts)
    print(f'Looking for {ext_strs} files...')
    known_files = find_files(root_path, exts)

    # Sort the XML and p7s files we found
    permissions_xml, governance_xml = sort_xml_and_p7s(known_files, xml_ext)
    permissions_p7s, governance_p7s = sort_xml_and_p7s(known_files, p7s_ext)

    # See if p7s files have an original XML and make sure contents are the same
    missing_permissions_xml, permissions_needs_signed = compare_xml_and_p7s(permissions_xml, permissions_p7s)
    missing_governance_xml, governance_needs_signed = compare_xml_and_p7s(governance_xml, governance_p7s)
    # TODO: Handle missing XML?

    # Update not_before in permissions if expires soon or is already passed
    update_permissions_xml_not_before(permissions_xml, permissions_needs_signed)

    # Sign missing and updated files
    if status.fix:
        sign_documents(permissions_p7s, permissions_needs_signed, permissions_ca)
        sign_documents(governance_p7s, governance_needs_signed, identity_ca)

    return status.exit_status()


if __name__ == '__main__':
    arg_parse = ArgumentParser()
    arg_parse.add_argument('--fix', '-f', action='store_true')
    arg_parse.add_argument('--verbose', action='store_true')
    sys.exit(main(arg_parse.parse_args()))
