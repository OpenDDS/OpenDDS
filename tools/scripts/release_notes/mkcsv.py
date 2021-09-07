#!/usr/bin/env python3

import sys
import json
import csv

if len(sys.argv) != 2:
  sys.exit('Invalid number of arguments. Usage is mkcsv.py JSON')

with open(sys.argv[1]) as f:
  prs = json.load(f)

pr_link_template = '=HYPERLINK("https://github.com/objectcomputing/OpenDDS/pull/{0}","#{0}")'
writer = csv.writer(sys.stdout)
for pr in prs:
  writer.writerow([
    pr['mergedAt'],
    pr_link_template.format(pr['number']),
    pr['title'],
    'no', '', '', '',
    pr['login'],
    '',
  ])
