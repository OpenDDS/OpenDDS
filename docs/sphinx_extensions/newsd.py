#!/usr/bin/env python3

import re
import sys
from pathlib import Path
import io
from datetime import datetime, timezone

from version_info import VersionInfo


news_directive_re = re.compile(r'\s*.. news-(\w+)(: (.*))?')
ghfile_re = re.compile(r':ghfile:')


class PrintHelper:
    def __init__(self, file=sys.stdout):
        self.file = file if file is not None else io.StringIO()
        self.printed_blank_line = False
        self.avoid_rst = False
        self.show_rank = False

    def pr_list(self, prs):
        if prs:
            if self.avoid_rst:
                pre = '#'
                post = ''
            else:
                pre = ':ghpr:`'
                post = '`'
            return ' (' + ', '.join([pre + str(pr) + post for pr in prs]) + ')'
        return ''

    def put(self, level, *args, sep=' ', after_first_line='', **kw):
        indent = '  ' * (level - 1)
        first = True
        combined = sep.join(args)
        if combined.endswith('\n'):
            combined = combined[:-1]
        for line in combined.split('\n'):
            blank = not bool(line)
            if blank and self.printed_blank_line:
                continue
            line = indent + line
            if after_first_line and first:
                line += after_first_line
                first = False
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
        assert self.file.getvalue() == expected


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


class Text(Node):
    def __init__(self, rank, prs, level, lines):
        super().__init__(rank, prs, level)
        self.lines = lines

    def empty(self):
        return len(self.lines) == 0

    def print(self, h):
        first = True
        for line in self.lines:
            if first:
                append = h.pr_list(self.prs)
                if h.show_rank:
                    append += ' [Rank {}]'.format(self.rank)
                h.put(self.level - 1, line, after_first_line=append)
                first = False
            else:
                h.put(self.level - 1, line.rstrip())


def test_text():
    ph = PrintHelper(None)
    Text(0, set([1, 2]), 3, ['- Item\n  - SubItem\n']).print(ph)
    ph.test('''\
  - Item (:ghpr:`1`, :ghpr:`2`)
    - SubItem
''')


class Section(Node):
    def __init__(self, rank=0, parent=None, name=None):
        super().__init__(rank, set(), parent.level + 1 if parent is not None else 0)
        self.parent = parent
        self.name = name
        self.children = []
        self.sections = {}

    def empty(self):
        for child in self.children:
            if not child.empty():
                return False
        return True

    def get_section(self, name, rank=0):
        if name in self.sections:
            child = self.sections[name]
            if rank > child.rank:
                child.rank = rank
            return child
        child = Section(rank, self, name)
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
            if self.level > 1:
                end = ''
                if h.show_rank:
                    end = '[Rank {}]'.format(self.rank)
                h.put(self.level - 1, '- ', end='')
            h.put(0, self.name)
            if self.level == 1:
                h.put(0, '=' * len(self.name))
                if h.show_rank:
                    h.put(0, '\n[Rank {}]'.format(self.rank))
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
    root.get_section('Section A').add_text(set([0]), ['- This is some text\n'])
    a = root.get_section('Section A')
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
    root.print(ph)
    ph.test('''\

Section A
=========

- This is some text (:ghpr:`0`)

- Section AA

  - Section AAA (Should be first in Section AA)

    - This is some text (:ghpr:`4`)
      - This is some more

  - (Should be second in Section AA) (:ghpr:`50`)
  - This is some text (:ghpr:`1`)
    - This is some more

  - Section AAC

    - This is some text (:ghpr:`1`)

  - This is some text (:ghpr:`3`)
    - This is some more
  - This is some text (:ghpr:`5`)
    - This is some more

  - Section AAB

    - Section AABA

      - This is some text (:ghpr:`9`)

    - This is some text (:ghpr:`10`)

Section B
=========

- This is some text (:ghpr:`12`)

''')


class ParseError(RuntimeError):
    def __init__(self, path, lineno, *args):
        super().__init__('Parse error on {}:{}:'.format(str(path), lineno, ' '.join(args)))


def parse(root, path):
    lineno = 0
    section = root
    stack = []
    rank = 0
    prs = set()
    lines = []
    with path.open() as f:
        for line in f:
            lineno += 1
            if line.startswith('#'):
                continue
            m = news_directive_re.match(line)
            if m:
                name = m.group(1)
                arg = m.group(3)
                if name == 'prs':
                    if prs:
                        raise ParseError(path, lineno, 'prs was already set')
                    prs = set([int(pr) for pr in arg.split(' ')])
                elif name == 'push':
                    stack.append((rank, section))
                    section = section.get_section(arg, rank)
                    lines = []
                elif name == 'pop':
                    if len(stack) == 0:
                        raise ParseError(path, lineno, 'news-pop is missing a news-push')
                    if lines:
                        section.add_text(prs, lines, rank)
                    rank, section = stack.pop()
                    lines = []
                elif name == 'rank':
                    rank = int(arg)
                else:
                    raise ParseError(path, lineno, 'Invalid directive:', name)
            elif section.level:
                lines.append(line)
            elif line.rstrip():
                raise ParseError(path, lineno, 'Text outside a section')
    if len(stack) > 0:
        raise ParseError(path, lineno, 'news-push is missing a news-pop')


def get_fragments():
    fragments = []
    for fragment in (Path(__file__).parent.parent / 'news.d').iterdir():
        if not fragment.name.startswith('_'):
            fragments.append(fragment)
    return fragments


def parse_newsd():
    root = Section()

    # Set ranks of sections, so they go the expected order, at least by
    # default.
    root.get_section('Additions', rank=100)
    root.get_section('Deprecations', rank=90)
    root.get_section('Removals', rank=80)
    root.get_section('Security', rank=60)
    root.get_section('Fixes', rank=40)
    root.get_section('Documentation', rank=20)
    root.get_section('Notes', rank=0)

    for fragment in get_fragments():
        parse(root, fragment)
    return root


# Since we can, always do testing.
test_text()
test_section()


if __name__ == '__main__':
    parse_newsd().print_all()

# vim: expandtab:ts=4:sw=4
