import re
from pathlib import Path

opendds_root_path = Path(__file__).parent.parent.parent


class VersionInfo:
    def __init__(self):
        with (opendds_root_path / 'dds/Version.h').open() as f:
            version_file = f.read()

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
                raise RuntimeError('Unexpected kind: ' + repr(kind))
            m = re.search(r'#define {} {}'.format(macro, regex), version_file)
            if m:
                return cast(m[1])
            raise RuntimeError('Could not find ' + macro)

        self.version = get(str, 'version')
        self.is_release = get(bool, 'is_release')
        self.tag = None
        # if self.is_release:
        vparts = {p: get(int, p + '_version') for p in ('major', 'minor', 'micro')}
        if vparts['major'] >= 4:
            fmt_str = 'v{major}.{minor}.{micro}'
        else:
            fmt_str = 'DDS-{major}.{minor}'
            if vparts['micro'] > 0:
                fmt_str += '.{micro}'
        self.tag = fmt_str.format(**vparts)


if __name__ == '__main__':
    print(vars(VersionInfo()))

# vim: expandtab:ts=4:sw=4
