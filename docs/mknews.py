#!/usr/bin/env python3

import sys
from pathlib import Path
import io

docs_path = Path(__file__).parent
sys.path.append(str((docs_path / 'sphinx_extensions').resolve()))

from newsd import parse_newsd, get_fragments
from version_info import VersionInfo


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

    print('Inserting news...')
    news_file = docs_path / 'news.rst'
    with news_file.open('r') as f:
        lines = f.readlines()
    with news_file.open('w') as f:
        insert = False
        for lineno, line in enumerate(lines):
            print(line, end='', file=f)
            if insert:
                print(new_news, end='', file=f)
            insert = 'NEW NEWS GOES BELOW HERE' in line

    print('Removing fragments...')
    for fragment in get_fragments():
        print(fragment)
        fragment.unlink()


if __name__ == '__main__':
    mknews()

# vim: expandtab:ts=4:sw=4
