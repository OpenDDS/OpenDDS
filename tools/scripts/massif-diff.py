#!/usr/bin/env python3

"""\
This script can help you analyze massif memory profiler output. It can clean up
the output by:

- Effectively applying the --alloc-fn massif option after the fact to attribute
  memory for "malloc-like" functions to the caller. It has some ACE and OpenDDS
  functions predefined (see combine list in source).
- Remove entire functions and their memory if desired, but currently doesn't
  (see remove list in source).
- Truncate the number of snapshots in a large massif file if there are more
  than what's necessary. This might be needed if the script is running out of
  memory.

This is shown as a "cleaned" output separate from the original.

After that it will take the cleaned up output and separate it into two new
massif outputs based on if a frame gained or lost allocated memory between
snapshots. This visualizes the allocation and deallocation of memory.

Finally the original and new output is all opened in the graphical massif
visualizer (assuming it's installed) in one window. This output could also be
opened in ms_print.
"""

import sys
import os
import re
from argparse import ArgumentParser, RawTextHelpFormatter
import subprocess
import signal
import linecache
import tracemalloc
import gc

root_prop_re = re.compile(r'^([^:]+): (.*)$')
section_prop_re = re.compile(r'^([^=]+)=(.*)$')
node_re = re.compile(r'^(\s*)n(\d+): (\d+) (.*)$')
header = '#-----------'
section_total = 'mem_heap_B'

# Treat these like allocators and merge their children with the parent. This
# uses "in" so it's a simple search, not regex.
combine = [
    'ACE_New_Allocator::malloc(unsigned long)',
    'OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow',
    'OpenDDS::DCPS::Cached_Allocator_With_Overflow',
    'OpenDDS::DCPS::PoolAllocationBase',
    'OpenDDS::DCPS::RcHandle',
]

# Remove these frames and their children from the snapshots entirely. This uses
# "in" so it's a simple search, not regex. Keep this in case it's needed again.
remove = [
    # 'ACE_Data_Block::ACE_Data_Block('
]


debug = False


def memory_stats(message, key_type='lineno', limit=10):
    print(f'Getting memory stats {message}, please wait...')

    _, peak_blocks = tracemalloc.get_traced_memory()

    snapshot = tracemalloc.take_snapshot()
    snapshot = snapshot.filter_traces((
        tracemalloc.Filter(False, "<frozen importlib._bootstrap>"),
        tracemalloc.Filter(False, "<unknown>"),
    ))
    top_stats = snapshot.statistics(key_type)

    print("  Top %s lines" % limit)
    for index, stat in enumerate(top_stats[:limit], 1):
        frame = stat.traceback[0]
        print("  #%s: %s:%s: %.1f KiB" % (index, frame.filename, frame.lineno, stat.size / 1024))
        line = linecache.getline(frame.filename, frame.lineno).strip()
        if line:
            print('      %s' % line)

    other = top_stats[limit:]
    if other:
        size = sum(stat.size for stat in other)
        print("  %s other: %.1f KiB" % (len(other), size / 1024))
    total = sum(stat.size for stat in top_stats)
    print("  Total allocated size: %.1f KiB" % (total / 1024))

    peak_size = peak_blocks / 1024
    print(f'  Peak so far has been {peak_size:0.2f} KiB')


class Deduplicater:
    def __init__(self):
        self.data = {}

    def __call__(self, value):
        return self.data.setdefault(value, value)


class Node:
    def __init__(self, orig_level=0, parent=None, text=None, copy=None):
        self.orig_level = orig_level if copy is None else copy.orig_level
        self.parent = parent if copy is None else copy.parent
        self.text = text if copy is None else copy.text
        # TBD: This dict copy takes up good chunk of time and memory and
        # probably isn't always necessary, but replacing this with a simple
        # copy on write wrapper used up more space.
        self.props = {} if copy is None else copy.props.copy()
        self.total = 0 if copy is None else copy.total
        self.value = None if copy is None else copy.value

        self.nodes = []
        if copy is not None:
            for node in copy.nodes:
                self.nodes.append(Node(copy=node))

    def is_root(self):
        return self.orig_level == 0

    def is_section(self):
        return self.orig_level == 1

    def is_heap_node(self):
        return self.orig_level == 2

    def path(self):
        if self.is_root():
            return (None,)
        elif self.is_section():
            return (None, None)
        elif self.is_heap_node():
            return (None, None, None)
        return self.parent.path() + (self.text,)

    def print(self, file, level=0):
        if level == 0:  # Root
            for k, v in self.props.items():
                print(f'{k}: {v}', file=file)
        elif level == 1:  # Section (aka snapshot)
            print(header, file=file)
            print(self.text, file=file)
            print(header, file=file)
            for k, v in self.props.items():
                if k == section_total:
                    v = self.value
                print(f'{k}={v}', file=file)
        else:  # Heap Node (aka frame)
            s = ' ' * (level - 2)
            count = len(self.nodes)
            print(f'{s}n{count}: {self.value} {self.text}', file=file)
        level += 1
        for node in self.nodes:
            node.print(file, level)

    def __repr__(self):
        return repr(self.text)

    def should(self, match_list):
        if self.is_root():
            return False
        for i in match_list:
            if i in self.text:
                return True
        return False

    def sort_nodes(self):
        if not self.is_root():
            self.nodes.sort(key=lambda n: n.total, reverse=True)

    def process(self, method, *args):
        if self.nodes:
            old_nodes = self.nodes
            self.nodes = []
            for child in old_nodes:
                self.nodes.extend(method(child, *args))

    def clean(self):
        if self.should(remove):
            return []
        self.process(Node.clean)
        self.sort_nodes()
        if self.should(combine):
            return self.nodes
        self.value = self.total
        if not self.is_root():
            return [self]

    def sum_values(self):
        if self.nodes:
            return sum([n.sum_values() for n in self.nodes])
        return self.value

    def zero(self):
        self.value = 0
        self.total = 0
        for node in self.nodes:
            node.zero()

    def diff(self, allocs, dedup, nodes_by_path=None):
        if nodes_by_path is None:
            nodes_by_path = {}

        path = dedup(self.path())
        if path in nodes_by_path:
            last = nodes_by_path[path][-1]
            nodes_by_path[path].append(self)
            # Temporarily put in "missing" nodes
            our_nodes = set([n.text for n in self.nodes])
            for node in last.nodes:
                if node.text not in our_nodes:
                    copy = Node(copy=node)
                    copy.zero()
                    self.nodes.append(copy)
            change = self.total - last.total
        else:
            last = None
            nodes_by_path[path] = [self]
            change = self.total

        if not allocs:
            change = -change
        self.value = 0 if change < 0 else change

        self.process(Node.diff, allocs, dedup, nodes_by_path)

        if self.is_section():
            self.value = self.sum_values()

        if self.value == 0 and last is not None and last.value == 0:
            # Remove nodes with 0 total size
            if self.is_section() or self.is_heap_node():
                return [self]
            return []
        elif not self.is_root():
            return [self]


def add_node(stack, level, text):
    del stack[level:]
    parent = stack[-1]
    node = Node(level, parent, text)
    parent.nodes.append(node)
    stack.append(node)
    return node


def parse(in_file, start, max_count, skip_every, skip_all):
    root = Node()
    section = root
    stack = [root]
    section_header = False
    section_name = None
    skip_section = False
    dedup = Deduplicater()
    sections_seen = 0
    sections_saved = 0
    sections_skipped = 0
    sections_period = 0
    first_period = True

    for line_no, line in enumerate(in_file):
        line_no += 1
        line = line.rstrip()
        valid = False

        if section_header:  # Start or end of section/snapshot header
            if line == header:
                section_header = False
                valid = True

            elif section_name is None:  # New section/snapshot
                section_name = line
                reached_max_count = max_count is not None and sections_saved >= max_count
                in_range = sections_seen >= start and not reached_max_count
                skip_period = not first_period and sections_period < skip_every
                if in_range and not (skip_all or skip_period):
                    section = add_node(stack, 1, section_name)
                    sections_saved += 1
                    sections_period = 0
                    first_period = False
                else:
                    # Skip this section/snapshot
                    skip_section = True
                    sections_skipped += 1
                    sections_period += 1

                if debug:
                    print('SECTION', section_name, '(skipped)' if skip_section else '')
                sections_seen += 1
                valid = True

        elif skip_section:
            # We're skipping this section until we see a header
            if line == header:
                section_header = True
                section_name = None
                skip_section = False
            valid = True

        else:
            if section_name is None:
                if m := root_prop_re.match(line):
                    key, value = m.groups()
                    if debug:
                        print('ROOT PROP', key, value)
                    root.props[key] = value
                    valid = True

            else:
                if m := node_re.match(line):
                    space, child_count, total, text = m.groups()
                    text = dedup(text)
                    level = len(space) + 2
                    node = add_node(stack, level, text)
                    node.total = int(total)
                    valid = True

                elif m := section_prop_re.match(line):
                    key, value = m.groups()
                    if key == section_total:
                        section.total = int(value)
                    section.props[key] = value
                    if debug:
                        print('SECTION PROP', key, value)
                    valid = True

            if not valid and line == header:
                section_header = True
                section_name = None
                valid = True

        if not valid:
            sys.exit(f'unknown on line {line_no}: {line}')

    print(sections_seen, 'snapshots seen')
    print(sections_saved, 'snapshots saved')
    print(sections_skipped, 'snapshots skipped')

    return root


def write_diff(filename, root, allocs, dedup):
    out_filename = filename + ('.allocs' if allocs else '.frees')
    print('Creating', out_filename)

    root = Node(copy=root)
    root.diff(allocs, dedup)
    with open(out_filename, 'w') as out_file:
        root.print(out_file)

    del root

    return out_filename


if __name__ == '__main__':
    arg_parser = ArgumentParser(description=__doc__, formatter_class=RawTextHelpFormatter)
    arg_parser.add_argument('input', help='Massif main output file (massif.out.<pid>)')
    arg_parser.add_argument(
        '--start', '-s', type=int, default=0, help='Start at this snapshot, default is 0'
    )
    arg_parser.add_argument(
        '--count',
        '-c',
        type=int,
        default=None,
        help='Max number of snapshots, default is unlimited',
    )
    arg_parser.add_argument(
        '--skip-every',
        '-e',
        type=int,
        default=0,
        help='Skip this many snapshots after parsing a snapshot, default is 0',
    )
    arg_parser.add_argument(
        '--just-count', action='store_true', help='Just parse the input and count the number of sections'
    )
    arg_parser.add_argument(
        '--no-allocs', action='store_false', dest='allocs', help="Don't make allocation output"
    )
    arg_parser.add_argument(
        '--no-frees', action='store_false', dest='frees', help="Don't make deallocation output"
    )
    arg_parser.add_argument(
        '--no-vis', action='store_false', dest='vis', help="Don't start massif-visualizer"
    )
    arg_parser.add_argument('--debug', action='store_true')
    arg_parser.add_argument('--memory-stats', action='store_true')
    args = arg_parser.parse_args()
    debug = args.debug

    if args.memory_stats:
        tracemalloc.start()

    print('Parsing', args.input)
    with open(args.input) as in_file:
        orig = parse(in_file, args.start, args.count, args.skip_every, args.just_count)

    if args.just_count:
        print('Skipping processing')
        sys.exit(0)

    if args.memory_stats:
        memory_stats('after parse')

    # Create a "cleaned" copy to rewrite unwanted nodes.
    cleaned_filename = args.input + '.cleaned'
    print('Creating', cleaned_filename)
    cleaned = Node(copy=orig)
    cleaned.clean()
    with open(cleaned_filename, 'w') as out_file:
        cleaned.print(out_file)

    # Force cleanup of unneeded data
    del orig
    gc.collect()
    if args.memory_stats:
        memory_stats('after clean')

    # Generate allocs and frees, set files to open in visualizer
    dedup = Deduplicater()
    filenames = [args.input]
    if args.allocs:
        filenames.append(write_diff(args.input, cleaned, True, dedup=dedup))
    if args.frees:
        filenames.append(write_diff(args.input, cleaned, False, dedup=dedup))
    filenames.append(cleaned_filename)

    # Force cleanup of unneeded data
    del cleaned
    del dedup
    gc.collect()
    if args.memory_stats:
        memory_stats('after final cleanup')

    # Run visualizer so it's separate from this process.
    if args.vis:
        import subprocess, signal

        subprocess.Popen(
            ['massif-visualizer'] + filenames,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True,
            preexec_fn=(lambda: signal.signal(signal.SIGHUP, signal.SIG_IGN)),
        )
