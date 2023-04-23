#!/usr/bin/env python3

import sys
from pathlib import Path
import io
import subprocess

docs_path = Path(__file__).parent
sys.path.append(str((docs_path / 'sphinx_extensions').resolve()))

from newsd import parse_newsd, get_fragments
from version_info import VersionInfo


def insert_into_file(path, marker, to_insert):
    print('Inserting into', path.name, '...')
    with path.open('r') as f:
        lines = f.readlines()
    inserted = False
    with path.open('w') as f:
        insert = False
        for lineno, line in enumerate(lines):
            print(line, end='', file=f)
            if insert:
                print(to_insert, end='', file=f)
            insert = marker in line
            if insert:
                if inserted:
                    raise ValueError('Already inserted into ' + str(path))
                inserted = True
    if not inserted:
        raise ValueError(repr(marker) + ' not found in ' + str(path))


def mknews():
    version_info = VersionInfo()
    if not version_info.is_release:
        sys.exit('ERROR: Not a release')

    print('Generating news...')
    strio = io.StringIO()
    news = parse_newsd()
    if news.empty():
        sys.exit('ERROR: News is empty!')
    news.print_all(strio)
    new_news = strio.getvalue()
    print(new_news)

    insert_into_file(docs_path / 'news.rst', 'NEW NEWS GOES BELOW HERE', new_news)

    print('Generating Markdown news...')
    this_release_file = docs_path / 'this_release.rst'
    with this_release_file.open('w') as f:
        print(':orphan:\n', file=f)
        print(new_news, end='', file=f)
    subprocess.check_call(['./build.py', 'markdown'], cwd=str(docs_path))
    with (docs_path / '_build/markdown/this_release.md').open('r') as f:
        this_release_md = f.read()
    print(this_release_md)

    insert_into_file(docs_path / '../NEWS.md', '# OpenDDS Releases', this_release_md)

    print('Removing fragments...')
    for fragment in get_fragments():
        print(fragment)
        fragment.unlink()


if __name__ == '__main__':
    mknews()

# vim: expandtab:ts=4:sw=4
