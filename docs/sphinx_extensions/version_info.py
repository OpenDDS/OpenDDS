#!/usr/bin/env python3

import re
from pathlib import Path


opendds_root_path = Path(__file__).parent.parent.parent

with (opendds_root_path / 'dds/Version.h').open() as f:
    version_file = f.read()


class VersionInfo:

    @staticmethod
    def get(kind, macro):
        macro = 'OPENDDS_' + macro.upper()
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


if __name__ == '__main__':
    print(vars(VersionInfo()))

# vim: expandtab:ts=4:sw=4
