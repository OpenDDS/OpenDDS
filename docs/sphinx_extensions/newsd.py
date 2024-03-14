#!/usr/bin/env python3

import re
import sys
from pathlib import Path
import io
from datetime import datetime, timezone
import textwrap
import unittest

from version_info import VersionInfo


# These sections are builtin and must appear in this order
main_sections = {
    'Additions': 100,
    'Platform Support and Dependencies': 95,
    'Deprecations': 90,
    'Removals': 80,
    'Security': 60,
    'Fixes': 40,
    'Documentation': 20,
    'Notes': 0,
}

news_directive_re = re.compile(r'\s*.. news-([\w-]+)(: (.*))?')
ghfile_re = re.compile(r':ghfile:')

newsd_path = Path(__file__).parent.parent / 'news.d'
releases_path = newsd_path / '_releases'

def loc_str(loc):
    if loc is None:
        path = 'unknown'
        lineno = 0
    else:
        path, lineno = loc
    return str(path) + ':' + str(lineno)


class PrintHelper:
    def __init__(self, file=sys.stdout):
        self.file = file if file is not None else io.StringIO()
        self.printed_blank_line = False
        self.avoid_rst = False
        self.show_rank = False

    def put(self, level, *args, sep=' ', decorate='', **kw):
        indent = '  ' * (level - 1)
        combined = sep.join(args)
        if combined.endswith('\n'):
            combined = combined[:-1]
        for line in combined.split('\n'):
            line = line.rstrip()
            blank = not bool(line)
            if blank and self.printed_blank_line:
                continue
            if len(line):
                if not line.startswith(' ') and decorate:
                    line += decorate
                line = indent + line
            if self.avoid_rst:
                ghfile_re.sub('')
            print(line, file=self.file, **kw)
            self.printed_blank_line = blank

    def put_level_separator(self):
        if not self.printed_blank_line:
            print(file=self.file)
            self.printed_blank_line = True

    def test_str(self):
        assert isinstance(self.file, io.StringIO)
        return self.file.getvalue()


class Node:
    def __init__(self, rank, prs, level):
        self.rank = rank
        self.prs = prs
        self.level = level

    def __lt__(self, other):
        if self.rank == other.rank:
            return list(self.prs) < list(other.prs)
        return self.rank > other.rank

    def empty(self):
        raise NotImplementedError()

    def print(self, h):
        raise NotImplementedError()

    def prs_str(self, h):
        if not self.prs:
            return ''
        elif h.avoid_rst:
            pre = '#'
            post = ''
        else:
            pre = ':ghpr:`'
            post = '`'
        return ' (' + ', '.join([pre + str(pr) + post for pr in self.prs]) + ')'

    def rank_str(self, h):
        # This doesn't appear in the actual release notes, only previews
        if not h.show_rank:
            return ''
        rv = '[Rank {}]'.format(self.rank)
        if self.level == 1:
            return '\n' + rv + '\n'
        else:
            return ' ' + rv


class Text(Node):
    def __init__(self, rank, prs, level, lines):
        super().__init__(rank, prs, level)
        self.lines = lines

    def empty(self):
        return len(self.lines) == 0

    def print(self, h):
        decorate = self.prs_str(h) + self.rank_str(h)
        for line in self.lines:
            h.put(self.level - 1, line, decorate=decorate)


class TestText(unittest.TestCase):
    def test_text(self):
        ph = PrintHelper(None)
        Text(0, set([1, 2]), 3, ['- Item\n  - SubItem\n- Additional Item']).print(ph)
        self.assertEqual(ph.test_str(), '''\
  - Item (:ghpr:`1`, :ghpr:`2`)
    - SubItem
  - Additional Item (:ghpr:`1`, :ghpr:`2`)
''')


class Section(Node):
    def __init__(self, rank=0, parent=None, name=None, last_start_loc=None):
        super().__init__(rank, set(), parent.level + 1 if parent is not None else 0)
        self.parent = parent
        self.name = name
        self.children = []
        self.sections = {}
        self.last_start_loc = last_start_loc

    def empty(self):
        for child in self.children:
            if not child.empty():
                return False
        return True

    def get_section(self, name, rank=0, loc=None, restricted=False):
        # Don't let fragments reorder main sections or create new ones.
        fixed_main_sections = restricted and self.level == 0
        if name in self.sections:
            # Already exists, adjust rank and return it
            child = self.sections[name]
            if not fixed_main_sections and rank > child.rank:
                child.rank = rank
            child.last_start_loc = loc
            return child
        if fixed_main_sections:
            raise KeyError(
                "Can't define a new main section named {} at {}".format(
                    repr(name), loc_str(loc)))
        child = Section(rank, self, name, last_start_loc=loc)
        self.children.append(child)
        self.sections[name] = child
        return child

    def add_text(self, prs, lines, rank=0):
        self.prs |= prs
        child = Text(rank, prs, self.level + 1, lines)
        self.children.append(child)
        return child

    def print(self, h):
        if self.empty():
            return
        h.put_level_separator()
        if self.level:
            if self.level == 1:
                h.put(0, self.name)
                h.put(0, '=' * len(self.name))
                h.put(0, self.rank_str(h))
            elif self.level > 1:
                h.put(self.level - 1, '-', self.name, decorate=self.rank_str(h))
            h.put_level_separator()
        for child in sorted(self.children):
            child.print(h)
        if self.level:
            h.put_level_separator()

    def print_all(self, file=sys.stdout):
        version_info = VersionInfo()
        if version_info.is_release:
            today = datetime.now(timezone.utc).date()
            print((
                'Released {}\n\n' +
                'Download :ghrelease:`this release on GitHub <{}>`.\n\n' +
                'Read `the documentation for this release on Read the Docs <https://opendds.readthedocs.io/en/{}>`__.'
            ).format(today.isoformat(), version_info.tag, version_info.tag.lower()), file=file)
        else:
            print('This version is currently still in development, so this list might change.', file=file)

        if self.empty():
            print('\nNothing news-worthy yet...\n', file=file)
        else:
            h = PrintHelper(file=file)
            h.show_rank = not version_info.is_release
            self.print(h)


class TestSection(unittest.TestCase):
    def test_section(self):
        root = Section()
        a = root.get_section('Section A')
        a.add_text(set([0]), ['- This is some text\n', '- This is a separate item\n'])
        aa = a.get_section('Section AA')
        aa.add_text(set([3]), ['- This is some text\n  - This is some more\n'])
        aa.add_text(set([1]), ['- This is some text  \n  - This is some more\n'])
        aa.add_text(set([5]), ['- This is some text\n  - This is some more\n    \n'])
        aa.add_text(set([50]), ['- (Should be second in Section AA)\n'], 9)
        aaa = aa.get_section('Section AAA (Should be first in Section AA)', 10)
        aaa.add_text(set([4]), ['- This is some text\n  - This is some more\n'])
        aab = aa.get_section('Section AAB')
        aab.add_text(set([10]), ['- This is some text\n'])
        aaba = aab.get_section('Section AABA')
        aaba.add_text(set([9]), ['- This is some text\n'])
        aac = aa.get_section('Section AAC')
        aac.add_text(set([1]), ['- This is some text\n'])
        b = root.get_section('Section B')
        b.add_text(set([12]), ['- This is some text\n'])
        root.get_section('This should be hidden') \
            .get_section('This should be hidden') \
            .get_section('This should be hidden')

        ph = PrintHelper(None)
        ph.show_rank = True
        root.print(ph)
        self.assertEqual(ph.test_str(), '''\

Section A
=========

[Rank 0]

- This is some text (:ghpr:`0`) [Rank 0]
- This is a separate item (:ghpr:`0`) [Rank 0]

- Section AA [Rank 0]

  - Section AAA (Should be first in Section AA) [Rank 10]

    - This is some text (:ghpr:`4`) [Rank 0]
      - This is some more

  - (Should be second in Section AA) (:ghpr:`50`) [Rank 9]
  - This is some text (:ghpr:`1`) [Rank 0]
    - This is some more

  - Section AAC [Rank 0]

    - This is some text (:ghpr:`1`) [Rank 0]

  - This is some text (:ghpr:`3`) [Rank 0]
    - This is some more
  - This is some text (:ghpr:`5`) [Rank 0]
    - This is some more

  - Section AAB [Rank 0]

    - Section AABA [Rank 0]

      - This is some text (:ghpr:`9`) [Rank 0]

    - This is some text (:ghpr:`10`) [Rank 0]

Section B
=========

[Rank 0]

- This is some text (:ghpr:`12`) [Rank 0]

''')


class ParseError(RuntimeError):
    def __init__(self, loc, *args):
        super().__init__('Parse error at {}: {}'.format(loc_str(loc), ' '.join(args)))


def parse(root, path):
    lineno = 0
    rank = 0
    section = root
    # Stack are tuples of rank and section to return to as sections end.
    stack = []
    prs = set()
    seen_prs = False
    lines = []
    with path.open() as f:
        for line in f:
            lineno += 1
            if line.startswith('#'):
                continue
            loc = (path, lineno)
            m = news_directive_re.match(line)
            if m:
                name = m.group(1)
                arg = m.group(3)
                if name == 'prs':
                    if seen_prs:
                        raise ParseError(loc, 'Can only have one news-prs in a fragment file')
                    try:
                        if arg != 'none':
                            prs = set([int(pr) for pr in arg.split(' ')])
                    except ValueError:
                        raise ParseError(loc,
                            'news-prs must be space-separated PR numbers or just "none"')
                    seen_prs = True
                elif name == 'start-section':
                    if not seen_prs:
                        raise ParseError(loc, 'Must have news-prs before any sections')
                    if lines:
                        section.add_text(prs, lines, rank)
                        lines = []
                    stack.append((rank, section))
                    section = section.get_section(arg, rank, loc=loc, restricted=True)
                    rank = 0
                elif name == 'end-section':
                    if len(stack) == 0:
                        raise ParseError(loc, 'news-end-section is missing a news-start-section')
                    if lines:
                        section.add_text(prs, lines, rank)
                        lines = []
                    rank, section = stack.pop()
                elif name == 'rank':
                    if lines:
                        section.add_text(prs, lines, rank)
                        lines = []
                    rank = int(arg)
                else:
                    raise ParseError(loc, 'Invalid directive:', name)
            elif section.level:
                lines.append(line)
            elif line.rstrip():
                raise ParseError(loc, 'Text outside a section')
    if len(stack) > 0:
        raise ParseError(section.last_start_loc, 'news-start-section', section.name,
            'is missing a news-end-section')


def get_fragments():
    fragments = []
    for fragment in newsd_path.iterdir():
        if not fragment.name.startswith('_'):
            fragments.append(fragment)
    return fragments


def parse_newsd():
    root = Section()

    for name, rank in main_sections.items():
        root.get_section(name, rank=rank)

    for fragment in get_fragments():
        parse(root, fragment)
    return root


def rst_title(title):
    astrisks = '*' * len(title)
    return '\n'.join((astrisks, title, astrisks)) + '\n'


def version_ref(version):
    return '.. _' + version.replace('.', '_') + ':\n'

def existing_release_notes():
    releases = []
    for p in releases_path.iterdir():
        m = re.match(r'v\d+\.\d+\.\d+\.rst', p.name)
        if m:
            releases.append(p)
    releases.sort(key=lambda p: p.stem[1:].split('.'), reverse=True)
    return releases


def print_all_news(file=sys.stdout):
    print(textwrap.dedent('''\
    ..
      This file is generated by newsd.py from rst files in news.d. The following
      means don't show subsections like "Additions" in local table of contents.

    :tocdepth: 2

    #############
    Release Notes
    #############

    These are all the recent releases of OpenDDS.
    '''), file=file)

    version_info = VersionInfo()
    if not version_info.is_release:
        print(version_ref(version_info.v_ver), file=file)
        print(rst_title(version_info.v_ver), file=file)
        parse_newsd().print_all(file=file)

    for f in existing_release_notes():
        print(version_ref(f.stem), rst_title(f.stem), f.read_text(), sep='\n', file=file)

    print(textwrap.dedent('''\
    **************
    Older Releases
    **************

    Older releases can be found in :ghfile:`NEWS.md`'''), file=file)


if __name__ == '__main__':
    parse_newsd().print_all()

# vim: expandtab:ts=4:sw=4
