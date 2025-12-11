#!/usr/bin/env python3

import re
import configparser
import time
import datetime
from pathlib import Path


opendds_root_path = Path(__file__).parent.parent.parent

with (opendds_root_path / 'dds/Version.h').open() as f:
    version_h_file = f.read()

with (opendds_root_path / 'VERSION.txt').open() as f:
    version_txt_file = f.read()


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
        m = re.search(r'#define {} {}'.format(macro, regex), version_h_file)
        if m:
            return cast(m[1])
        raise KeyError('Could not find ' + macro)

    def __init__(self):
        # Read values from Version.h
        self.version = self.get(str, 'version')
        self.is_release = self.get(bool, 'is_release')
        self.tag = None
        vparts = {p: self.get(int, p + '_version') for p in ('major', 'minor', 'micro')}
        self.ver = '{major}.{minor}.{micro}'.format(**vparts)
        self.v_ver = 'v' + self.ver
        self.tag = self.v_ver

        # Read values from acetao.ini
        ini = configparser.ConfigParser(interpolation=None)
        ini.read(opendds_root_path / 'acetao.ini')
        for sec in [ini[sn] for sn in ini.sections()]:
            for k in sec.keys():
                name = f'{sec.name}_{k}'.replace('-', '_').replace('.', '_')
                setattr(self, name, sec[k])

        # Get release date from VERSION.txt
        self.release_date_str = None
        self.release_date = None
        if self.is_release:
            m = re.search(r'released (\w+ \d+ \d+)', version_txt_file)
            if not m:
                raise ValueError('Could not find release date in VERSION.txt')
            self.release_date_str = m[1]
            self.release_date = datetime.date(*(time.strptime(self.release_date_str, '%b %d %Y')[0:3]))


if __name__ == '__main__':
    for k, v in vars(VersionInfo()).items():
        print(k, '=', repr(v))

# vim: expandtab:ts=4:sw=4
