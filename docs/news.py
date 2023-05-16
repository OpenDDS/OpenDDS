#!/usr/bin/env python3

'''\
This script helps with managing the news.
'''

import os
import sys
from pathlib import Path
import subprocess
from argparse import ArgumentParser
import re

docs_path = Path(__file__).parent
sys.path.append(str((docs_path / 'sphinx_extensions').resolve()))

from newsd import parse_newsd, get_fragments, print_all_news, releases_path
from version_info import VersionInfo


def run(*argv):
    subprocess.check_call(argv, cwd=str(docs_path))


def release(args):
    print(args)
    version_info = VersionInfo()
    if not args.solo_test and not version_info.is_release:
        sys.exit('ERROR: Not a release')
    version_entry = '## Version '
    news_md_title = version_entry + version_info.ver + ' of OpenDDS'

    news_md_path = docs_path / '../NEWS.md'
    news_rst_path = releases_path / (version_info.v_ver + '.rst')
    release_notes_path = docs_path / 'gh-release-notes.md'

    needs_changes = False
    print('Checking files...')
    if not news_rst_path.is_file():
        print('  Missing ' + news_rst_path.name)
        needs_changes = True
    if not release_notes_path.is_file():
        print('  Missing ' + release_notes_path.name)
        needs_changes = True
    with news_md_path.open('r') as f:
        for line in f.readlines():
            line = line.rstrip()
            if line.startswith(version_entry):
                if line != news_md_title:
                    print('  Missing entry in NEWS.md for ' + version_info.ver)
                    needs_changes = True
                break
    news = parse_newsd()
    if news.empty():
        if not args.mock and needs_changes:
            sys.exit('ERROR: news.d is empty!')
    elif not args.mock:
        needs_changes = True

    if needs_changes:
        if not args.remedy:
            sys.exit('ERROR: --remedy needs to be passed to process the news')

        news.print_all()
        with news_rst_path.open('w') as f:
            news.print_all(file=f)

        print('Generating Markdown news...')
        run('./build.py', 'markdown')
        with (docs_path / '_build/markdown/this-release.md').open('r') as f:
            this_release_md = f.read()
        this_release_md = re.sub(r'\n\n+', r'\n\n', this_release_md)
        this_release_md = re.sub(r'^#', r'###', this_release_md, flags=re.MULTILINE)
        this_release_md = news_md_title + '\n\n' + this_release_md
        print(this_release_md)
        # That was for NEWS.md, now generate GitHub release notes
        release_notes, dl_count = re.subn(r'^Download \[this release.*$', '', this_release_md, flags=re.MULTILINE)
        if not args.solo_test and dl_count != 1:
            sys.exit('ERROR: Expected 1 download lines in release notes, got ' + str(dl_count))
        release_notes = re.sub(r'\n\n+', r'\n\n', release_notes)
        with (docs_path / 'gh-release-notes.md').open('w') as f:
            print(release_notes, file=f)

        print('Inserting Markdown news...')
        omit = False
        with news_md_path.open('r') as f:
            lines = []
            for line in f.readlines():
                line = line.rstrip()
                if line.startswith(version_entry):
                    if line == news_md_title:
                        print('Removing existing entry')
                        omit = True
                    elif omit:
                        omit = False
                if not omit:
                    lines.append(line)
        with news_md_path.open('w') as f:
            inserted = False
            for line in lines:
                if line.startswith(version_entry) and not inserted:
                    print(this_release_md, file=f)
                    inserted = True
                print(line, file=f)
    else:
        print('  No changes needed!')

        if not args.solo_test:
            print('Removing fragments...')
            for fragment in get_fragments():
                print(fragment)
                fragment.unlink()

    return 0


def preview(args):
    parse_newsd().print_all()
    return 0


def preview_all(args):
    print_all_news()
    return 0


if __name__ == '__main__':
    arg_parser = ArgumentParser(description=__doc__)
    subparsers = arg_parser.add_subparsers(required=True)

    arg_parser_release = subparsers.add_parser('release')
    arg_parser_release.add_argument('--remedy', action='store_true',
        help='Allow changes to the news')
    arg_parser_release.add_argument('--mock', action='store_true',
        help='Allow empty news.d')
    arg_parser_release.add_argument('--solo-test', action='store_true',
        help='Allow a non-release test that does not remove the fragments')
    arg_parser_release.set_defaults(func=release)

    arg_parser_preview = subparsers.add_parser('preview')
    arg_parser_preview.set_defaults(func=preview)

    arg_parser_preview = subparsers.add_parser('preview-all')
    arg_parser_preview.set_defaults(func=preview_all)

    args = arg_parser.parse_args()
    sys.exit(args.func(args))

# vim: expandtab:ts=4:sw=4
