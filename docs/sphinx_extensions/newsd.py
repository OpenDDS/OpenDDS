#!/usr/bin/env python3

import re
import sys
from pathlib import Path
import io
from datetime import datetime, timezone

from version_info import VersionInfo


main_sections = {
    'Additions': 100,
    'Deprecations': 90,
    'Removals': 80,
    'Security': 60,
    'Fixes': 40,
    'Documentation': 20,
    'Notes': 0,
}

news_directive_re = re.compile(r'\s*.. news-([\w-]+)(: (.*))?')
ghfile_re = re.compile(r':ghfile:')


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
            blank = not bool(line)
            if blank and self.printed_blank_line:
                continue
            if len(line) and not line.startswith(' ') and decorate:
                line += decorate
            line = indent + line
            if self.avoid_rst:
                ghfile_re.sub('')
            print(line, file=self.file, **kw)
            self.printed_blank_line = blank

    def put_level_seperator(self):
        if not self.printed_blank_line:
            print(file=self.file)
            self.printed_blank_line = True

    def test(self, expected):
        assert isinstance(self.file, io.StringIO)
        got = self.file.getvalue()
        if got != expected:
            print('ERROR: internal test failed, expected:', expected, 'but got:', got, sep='\n', file=sys.stderr)
            raise AssertionError()


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


def test_text():
    ph = PrintHelper(None)
    Text(0, set([1, 2]), 3, ['- Item\n  - SubItem\n- Additional Item']).print(ph)
    ph.test('''\
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
        fixed_main_sections = restricted and self.level == 0
        if name in self.sections:
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
        h.put_level_seperator()
        if self.level:
            if self.level == 1:
                h.put(0, self.name)
                h.put(0, '=' * len(self.name))
                h.put(0, self.rank_str(h))
            elif self.level > 1:
                h.put(self.level - 1, '-', self.name, decorate=self.rank_str(h))
            h.put_level_seperator()
        for child in sorted(self.children):
            child.print(h)
        if self.level:
            h.put_level_seperator()

    def print_all(self, file=sys.stdout):
        version_info = VersionInfo()

        title = version_info.v_ver
        astrisks = '*' * len(title)
        print(astrisks, title, astrisks, sep='\n', file=file)
        print(file=file)
        if version_info.is_release:
            today = datetime.now(timezone.utc).date()
            print((
                'Released {}\n\n' +
                'Download :ghrelease:`this release on GitHub <{}>`.\n\n' +
                'Read `the documenation for this release on Read the Docs <https://opendds.readthedocs.io/en/{}>`__.'
            ).format(today.isoformat(), version_info.tag, version_info.tag.lower()), file=file)
        else:
            print('This version is currently still in development, so this list might change.', file=file)

        if self.empty():
            print('\nNothing news-worthy yet...\n', file=file)
        else:
            h = PrintHelper(file=file)
            h.show_rank = not version_info.is_release
            self.print(h)


def test_section():
    root = Section()
    a = root.get_section('Section A')
    a.add_text(set([0]), ['- This is some text\n', '- This is a seperate item\n'])
    aa = a.get_section('Section AA')
    aa.add_text(set([3]), ['- This is some text\n  - This is some more\n'])
    aa.add_text(set([1]), ['- This is some text\n  - This is some more\n'])
    aa.add_text(set([5]), ['- This is some text\n  - This is some more\n'])
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
    ph.test('''\

Section A
=========

[Rank 0]

- This is some text (:ghpr:`0`) [Rank 0]
- This is a seperate item (:ghpr:`0`) [Rank 0]

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


def get_pr(string):
    if string == 'this':
        return 0
    return int(string)


def parse(root, path):
    lineno = 0
    section = root
    stack = []
    rank = 0
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
                            prs = set([get_pr(pr) for pr in arg.split(' ')])
                    except ValueError:
                        raise ParseError(loc,
                            'news-prs must be space seperated PR numbers or just "none"')
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
    for fragment in (Path(__file__).parent.parent / 'news.d').iterdir():
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


# Since we can, always do testing.
test_text()
test_section()


if __name__ == '__main__':
    parse_newsd().print_all()

# vim: expandtab:ts=4:sw=4
