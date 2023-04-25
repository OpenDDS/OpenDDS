#!/usr/bin/env python3

'''\
This script helps with managing the news.
'''

import os
import sys
from pathlib import Path
import io
import subprocess
from argparse import ArgumentParser
import re

docs_path = Path(__file__).parent
sys.path.append(str((docs_path / 'sphinx_extensions').resolve()))

from newsd import parse_newsd, get_fragments, news_directive_re
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


def run(*argv):
    subprocess.check_call(argv, cwd=str(docs_path))


def release(args):
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
    run('./build.py', 'markdown')
    with (docs_path / '_build/markdown/this_release.md').open('r') as f:
        this_release_md = f.read()
    print(this_release_md)

    insert_into_file(docs_path / '../NEWS.md', '# OpenDDS Releases', this_release_md)

    print('Removing fragments...')
    for fragment in get_fragments():
        print(fragment)
        fragment.unlink()


def insert_pr_num(args):
    if 'GITHUB_ACTIONS' not in os.environ:
        sys.exit('ERROR: Can only run on GitHub Actions')

    pr_num = args.pr_num

    inserted = False
    for fragment in get_fragments():
        print('Looking in', fragment, '...')
        with fragment.open('r') as f:
            lines = f.readlines()
        with fragment.open('w') as f:
            for lineno, line in enumerate(lines):
                line = line.rstrip()
                m = news_directive_re.match(line)
                if m and m.group(1) == 'prs':
                    print('  Found news-prs on line ', lineno)
                    if 'this' in m.group(2):
                        print('  Found "this", replacing with', pr_num)
                        print(m.group(0).replace('this', str(pr_num)), file=f)
                        inserted = True
                    else:
                        print(line, file=f)
                else:
                    print(line, file=f)

    if inserted:
        print('Pushing changes...')
        run('git', 'config', 'user.name', 'github-actions[bot]')
        run('git', 'config', 'user.email', 'github-actions[bot]@users.noreply.github.com')
        run('git', 'commit', '--all', '--message',
            'Insert PR Number into News Fragment(s) [skip actions]')
        run('git', 'push')
    else:
        print('No changes to push')


def preview(args):
    parse_newsd().print_all()


if __name__ == '__main__':
    arg_parser = ArgumentParser(description=__doc__)
    subparsers = arg_parser.add_subparsers(required=True)

    arg_parser_insert_pr_num = subparsers.add_parser('insert-pr-num')
    arg_parser_insert_pr_num.add_argument('pr_num', type=int, metavar='PR_NUM')
    arg_parser_insert_pr_num.set_defaults(func=insert_pr_num)

    arg_parser_release = subparsers.add_parser('release')
    arg_parser_release.set_defaults(func=release)

    arg_parser_preview = subparsers.add_parser('preview')
    arg_parser_preview.set_defaults(func=preview)

    args = arg_parser.parse_args()
    args.func(args)

# vim: expandtab:ts=4:sw=4
