#!/usr/bin/env python3

import re
from pathlib import Path


opendds_root_path = Path(__file__).parent.parent.parent

with (opendds_root_path / 'dds/Version.h').open() as f:
    version_file = f.read()

with (opendds_root_path / 'configure').open() as f:
    configure_file = f.read()


class VersionInfo:

    @staticmethod
    def get(kind, name):
        macro = 'OPENDDS_' + name.upper()
        if kind is bool:
            regex = r'([01])'
            cast = lambda v: bool(int(v))
        elif kind is int:
            regex = r'(\d+)'
            cast = int
        elif kind is str:
            regex = r'"(.*)"'
            cast = lambda v: v
        else:
            raise TypeError('Unexpected kind: ' + repr(kind))
        m = re.search(r'#define {} {}'.format(macro, regex), version_file)
        if m:
            return cast(m[1])
        raise KeyError('Could not find ' + macro)

    @staticmethod
    def configure_get(kind, name):
        if kind is bool:
            regex = r'([01])'
            cast = lambda v: bool(int(v))
        elif kind is int:
            regex = r'(\d+)'
            cast = int
        elif kind is str:
            regex = r'["\'](.*)["\']'
            cast = lambda v: v
        else:
            raise TypeError('Unexpected kind: ' + repr(kind))
        m = re.search(r'my \${} = {};'.format(name, regex), configure_file)
        if m:
            return cast(m[1])
        raise KeyError('Could not find ' + name)

    @staticmethod
    def ace_tao_ver(ace_ver):
        parts = ace_ver.split('.')
        tao_ver = '.'.join([str(int(parts[0]) - 4)] + parts[1:3])
        return f'ACE {ace_ver}/TAO {tao_ver}'

    def __init__(self):
        self.version = self.get(str, 'version')
        self.is_release = self.get(bool, 'is_release')
        self.tag = None
        vparts = {p: self.get(int, p + '_version') for p in ('major', 'minor', 'micro')}
        self.ver = '{major}.{minor}.{micro}'.format(**vparts)
        self.v_ver = 'v' + self.ver
        if vparts['major'] >= 4:
            self.tag = self.v_ver
        else:
            fmt_str = 'DDS-{major}.{minor}'
            if vparts['micro'] > 0:
                fmt_str += '.{micro}'
            self.tag = fmt_str.format(**vparts)

        self.ace6tao2_version = self.ace_tao_ver(self.configure_get(str, 'doc_tao2_version'))
        self.ace7tao3_version = self.ace_tao_ver(self.configure_get(str, 'doc_tao3_version'))


if __name__ == '__main__':
    print(vars(VersionInfo()))

# vim: expandtab:ts=4:sw=4
