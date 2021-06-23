#!/usr/bin/env python3

import csv
import sys
import re
from pathlib import Path

# Find News file
news_name = 'NEWS.md'
possible_dirs = ['../../..', '.']
possible_files = [p for p in [Path(p) / news_name for p in possible_dirs] if p.is_file()]
if len(possible_files) != 1:
  sys.exit("Could not find {} in {}".format(news_name, ', '.join(possible_dirs)))
news_path = possible_files[0]

# Get PRs from News file
pr_re = re.compile('#(\d+)')
news_prs = set()
with open(news_path) as f:
    for line in f:
        found_prs = pr_re.findall(line)
        if found_prs:
            news_prs |= set(found_prs)

# Get PRs from Spreadsheet
prs = []
with open(sys.argv[1]) as f:
    reader = csv.reader(f)
    rows = iter(reader)
    next(rows) # Skip Row
    for row in rows:
        # print(' '.join(['[{}]'.format(c) for c in row]))
        if row[4] in ('yes', 'maybe'):
            # Number, Name, Notes
            prs.append((row[1], row[2], row[8]))

# Print Missing PRs
for pr in reversed(prs):
    num = pr[0]
    if num not in news_prs:
        what = pr[1]
        if pr[2]:
            what += ' ' + pr[2]
        print('-', what, '(#{})'.format(num))
