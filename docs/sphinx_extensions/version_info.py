#!/usr/bin/env python3

import re
from pathlib import Path
import configparser


opendds_root_path = Path(__file__).parent.parent.parent

with (opendds_root_path / 'dds/Version.h').open() as f:
    version_file = f.read()


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

        ini = configparser.ConfigParser(interpolation=None)
        ini.read(opendds_root_path / 'acetao.ini')
        for sec in [ini[sn] for sn in ini.sections()]:
            for k in sec.keys():
                name = f'{sec.name}_{k}'.replace('-', '_').replace('.', '_')
                setattr(self, name, sec[k])


if __name__ == '__main__':
    for k, v in vars(VersionInfo()).items():
        print(k, '=', repr(v))

# vim: expandtab:ts=4:sw=4
