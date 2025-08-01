#!/usr/bin/env python3

# Generates the combinations of Android NDK revisions, API levels, and other
# information to build.

import sys
from enum import Enum
import re
from functools import cmp_to_key
import argparse

# Start of matrix data ========================================================
# This is what needs to be kept up to date

default_default_flags = dict(
  # TODO: os='ubuntu-latest',
  os='ubuntu-22.04',
  use_security=False,
  use_java=False,
  use_toolchain=False,
  target_api=30,
)


all_archs = dict(
  # Before 21, Android was 32-bit only
  arm=dict(min_api=16, max_api=99999),
  arm64=dict(min_api=21,max_api=99999),
  x86=dict(min_api=16,max_api=99999),
  x86_64=dict(min_api=21,max_api=99999),
)


def define_ndks():
  # Define NDK versions, their min and max API Levels, and if they have
  # nicknames.
  Ndk('r28c', 21, 30, latest_stable=True)
  Ndk('r27c', 21, 30, latest_lts=True)
  Ndk('r26d', 21, 30)
  Ndk('r25c', 19, 30)
  Ndk('r23c', 16, 30)
  Ndk('r21e', 16, 29)
  Ndk('r18b', 16, 28)


# API Level 20 was Android Wear-only and 25 isn't in any NDK
skip_apis = set([20, 25])


def default_arch(api):
  # Before 21, Android was 32-bit only
  return "arm64" if api >= 21 else "arm"


def get_matrices():

  def comprehensive(matrix, ndk, extras=False):
    # Build all APIs using NDK directly with security and Java on max and min
    # APIs.
    matrix.add_ndk(ndk, 'all',
      flags_on_edges=dict(
        use_security=extras,
        use_java=extras,
      ),
    )

  # DOC Group master branch ---------------------------------------------------
  doc_group_master_matrix = Matrix(
    'doc_group_master', mark='M',
    url='https://github.com/DOCGroup/ACE_TAO',
    ace_tao='doc_group_master',
  )
  comprehensive(doc_group_master_matrix, 'latest_stable', extras=True)
  comprehensive(doc_group_master_matrix, 'latest_beta', extras=True)
  doc_group_master_matrix.add_ndk('latest_lts', 'minmax')
  doc_group_master_matrix.add_ndk('r23c', 'minmax')
  doc_group_master_matrix.add_ndk('r18b', 'min',
    default_flags=dict(
      use_toolchain=True,
      use_java=True,
      # Make sure API<24 work because of NetworkCallback
      target_api=16,
    ),
  )
  doc_group_master_matrix.add_ndk('r18b', 'max',
    default_flags=dict(
      use_toolchain=True,
    ),
  )
  # # Make sure everything works on macOS.
  # doc_group_master_matrix.add_ndk('latest_stable', 'max',
  #   default_flags=dict(
  #     use_java=True,
  #     use_security=True,
  #     os='macos-latest',
  #   ),
  # )

  # DOC Group master ace6_tao2 branch -----------------------------------------
  doc_group_ace6_tao2_matrix = Matrix(
    'doc_group_ace6_tao2', mark='6',
    url='https://github.com/DOCGroup/ACE_TAO/tree/ace6tao2',
    ace_tao='doc_group_ace6_tao2',
  )
  comprehensive(doc_group_ace6_tao2_matrix, 'latest_stable')
  doc_group_ace6_tao2_matrix.add_ndk('r21e', 'minmax')
  doc_group_ace6_tao2_matrix.add_ndk('r18b', 'minmax',
    default_flags=dict(
      use_toolchain=True,
    ),
  )

  return [
    doc_group_master_matrix,
    doc_group_ace6_tao2_matrix,
  ]

# End of matrix data ==========================================================
# Below is supposed to be generic code that uses the data

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument('--get-ndk-major', metavar='NDK')
arg_parser.add_argument('--get-ndk-minor', metavar='NDK')
arg_parser.add_argument('--debug', action='store_true')
args = arg_parser.parse_args()


def get_api_range(api_range):
  rv = set()
  if api_range:
    rv = set(range(api_range[0], api_range[1] + 1)) - skip_apis
  return rv


class Ndk:
  all_ndks = {}
  valid_nicknames = ['latest_stable', 'latest_lts', 'latest_beta']

  def __init__(self, name, min_api, max_api, latest_stable=False, latest_lts=False):
    self.name = name
    self.as_tuple = convert_ndk(name)
    self.min_api = min_api
    self.max_api = max_api
    self.ndk_only = self.as_tuple[0] >= 19
    self.latest_stable = latest_stable
    self.latest_lts = latest_lts
    self.latest_beta = 'beta' in name
    self.all_ndks[name] = self
    self.nicknames = \
      [nickname for nickname in self.valid_nicknames if getattr(self, nickname)]

  @classmethod
  def get(cls, name):
    if name in cls.all_ndks:
      return cls.all_ndks[name]
    # Find by nickname
    for ndk in cls.all_ndks.values():
      if name in ndk.nicknames:
        return ndk
    if name in cls.valid_nicknames:
      return None
    raise ValueError("Couldn't find a NDK named or nicknamed {}".format(repr(name)))

  def all_apis(self):
    return list(get_api_range((self.min_api, self.max_api)))

  def all_apis_for_arch(self, arch):
    arch_info = all_archs[arch]
    return list(get_api_range((
      max(self.min_api, arch_info['min_api']),
      min(self.max_api, arch_info['max_api']))))

  def apis_by_name(self, name):
    if name == 'all':
      return self.all_apis()
    elif name == 'minmax':
      return [self.min_api, self.max_api]
    elif name == 'min':
      return [self.min_api]
    elif name == 'max':
      return [self.max_api]
    else:
      raise ValueError("Invalid API list name: {}".format(repr(name)))

  def __str__(self):
    return self.name


class Build:
  '''Represents a single build for a particular Android target with a
  particular set of options.
  '''

  builtin_properties = ['name', 'arch', 'ndk', 'api']

  def __init__(self, ndk, api, arch=None, flags={}):
    self.ndk = ndk
    self.api = api
    self.arch = default_arch(api) if arch is None else arch
    self.flags = flags
    self.all_properties = self.builtin_properties + list(self.flags.keys())

  def __str__(self):
    result = '{}-{}-{}'.format(self.ndk, self.arch, self.api)
    def append(result, s):
      return '{}-{}'.format(result, s.replace('_', '-'))
    for k, v in self.flags.items():
      if k.startswith('use_'):
        flag = k[4:]
      elif k in ('target_api',):
        flag = '{}-{}'.format(k, v)
      elif k in default_default_flags and isinstance(v, str):
        if k == 'os' and v.endswith('-latest'):
          flag = v[:-7]
        else:
          flag = v
      else:
        flag = k
      if k in default_default_flags:
        if v != default_default_flags[k]:
          result = append(result, flag)
      elif isinstance(v, str):
        result = append(result, v)
    return result

  def __repr__(self):
    return '<Build {}>'.format(str(self))

  def case_format(self, format_str, flag_convert):
    format_flags = {}
    for k, v in self.flags.items():
      format_flags[k] = flag_convert(v)
    return format_str.format(
      name=str(self), arch=self.arch, ndk=self.ndk, api=self.api,
      **format_flags)


class Matrix:
  '''Represents information about a group of builds.

  For now builds are grouped into matrices based on which ACE/TAO they use.
  '''

  def __init__(self, name, mark=None, url=None, **default_flags):
    self.name = name
    self.mark = name[0] if mark is None else mark
    self.url = url
    self.ndks = []
    self.builds = []
    self.builds_by_ndk = {}
    self.apis = []
    self.default_flags = default_default_flags.copy()
    self.default_flags.update(default_flags)

  def archs_for_build(self, ndk, api):
    archs = set()
    for build in matrix.builds_by_ndk[ndk]:
      if build.api == api:
        archs.add(build.arch)
    return archs

  def has_build_for(self, ndk, api):
    if ndk in self.builds_by_ndk:
      for build in self.builds_by_ndk[ndk]:
        if build.api == api:
          return True
    return False

  def add_ndk(self, ndk, *args, api_range=None, default_flags={}, flags_on_edges={}):
    '''Adds a build or set of builds for an NDK.

    `ndk` can be an `Ndk` object, the name of the NDK ("r23") or a nickname
    ("latest_stable").
    `args` Are the Android API Levels to build. They can be a single API level
    numbers, or a `str` nickname of a single or series of API levels associated
    with the NDK. The nicknames are:
      "min": The minimum API level supported by the NDK.
      "max": The maximum API level supported by the NDK.
      "minmax": Equivalent to `"min", "max"`.
      "all": All the API levels supported by the NDK.
    These are combined with `api_range` if also passed.
    `api_range` is a tuple of two `int`s for a range of API levels to use.
    These are combined with `args` if also passed.
    `default_flags` are the flags to use on the builds.
    `flags_on_edges` are the flags to override `default_flags` on the builds
    with minimum and maximum API levels.
    '''

    tmp_ndk = Ndk.get(ndk)
    if tmp_ndk is None:
      print("Skipping {} {} {}: No such NDK at the moment".format(ndk, args, api_range));
      return
    ndk = tmp_ndk
    name = str(ndk)
    new_ndk = name not in self.ndks
    if new_ndk:
      self.ndks.append(name)

    # See if any args are API list names, else assume they are API numbers
    apis = []
    for arg in args:
      if isinstance(arg, str):
        apis.extend(ndk.apis_by_name(arg))
      else:
        apis.append(arg)

    # Turn args and api_range into a proper list of APIs
    api_list = list(set(apis) | get_api_range(api_range))
    api_list.sort(reverse=True)
    self.apis = list(set(self.apis) | set(api_list))
    self.apis.sort()
    last = len(api_list) - 1

    # Create Specific Builds
    builds = []
    for i, api in enumerate(api_list):
      flags = self.default_flags.copy()
      flags.update(default_flags)
      if i == 0 or i == last:
        flags.update(flags_on_edges)
      builds.append(Build(name, api, flags=flags))
    self.builds.extend(builds)
    if new_ndk:
      self.builds_by_ndk[name] = []
    self.builds_by_ndk[name].extend(builds)


def shell_value(value):
  if isinstance(value, bool):
    return "true" if value else "false"
  elif isinstance(value, str):
    return '"{}"'.format(value)
  elif isinstance(value, int):
    return str(value)
  else:
    raise TypeError('Unexpected Type: ' + repr(type(value)))


def fill_line(line, char, length = 80):
  if line:
    if (len(line) + 2 > length):
      return line
    line += ' '
    line += char * (length - len(line))
    return line
  return char * length


def github(matrices, file, **kw):
  for matrix in matrices:
    comment(file, Kind.GhActions, fill_line(matrix.name, '='))
    for ndk in matrix.ndks:
      comment(file, Kind.GhActions, fill_line(ndk, '-'))
      for build in matrix.builds_by_ndk[ndk]:
        first = True
        for prop in build.all_properties:
          if first:
            print('          - ', end='', file=file)
            first = False
          else:
            print('            ', end='',  file=file)
          print(build.case_format(prop + ': {' + prop + '}', shell_value), file=file)


ndk_regex = re.compile(r'^r(\d+)([a-z]?)(?:-beta(\d+))?$')
def convert_ndk(r):
  m = ndk_regex.match(r)
  if m is None:
    raise ValueError(r)
  r = m.groups()
  return (
    int(r[0]),
    0 if not r[1] else ord(r[1]) - ord('a'),
    None if r[2] is None else int(r[2]),
  )


if args.get_ndk_major is not None:
  print(convert_ndk(args.get_ndk_major)[0])
  sys.exit(0)


if args.get_ndk_minor is not None:
  print(convert_ndk(args.get_ndk_minor)[1])
  sys.exit(0)


def compare_ndk(a, b):
  a = convert_ndk(a)
  b = convert_ndk(b)
  if a[0] == b[0]:
    if a[1] == b[1]:
      if a[2] is None and b[2] is None:
        return 0
      if a[2] is None and b[2] is not None:
        return 1
      elif a[2] is not None and b[2] is None:
        return -1
      elif a[2] > b[2]:
        return 1
      elif a[2] < b[2]:
        return -1
      else:
        return 0
    else:
      return 1 if a[1] > b[1] else -1
  else:
    return 1 if a[0] > b[0] else -1


def sort_ndks(ndks):
  return sorted(ndks, key=cmp_to_key(compare_ndk), reverse=True)


def readme(matrices, file, **kw):
  def print_row(cells):
    print('|', ' | '.join(cells), '|', file=file)

  # Get All APIs
  apis = []
  for matrix in matrices:
    for api in matrix.apis:
      if api not in apis:
        apis.append(api)
  apis.sort()

  # Get All NDKs
  ndks = set()
  for matrix in matrices:
    for ndk in matrix.ndks:
      ndks |= {ndk}
  ndks = sort_ndks(list(ndks))

  # Print Legend
  for matrix in matrices:
    name = '`{}`'.format(matrix.name)
    if matrix.url is not None:
      name = '[{}]({})'.format(name, matrix.url)
    print('`{}` = {}'.format(matrix.mark, name), file=file)

  # Print Table Header
  print_row([''] + list(map(str, apis)))
  print_row(['---'] * (len(apis) + 1))
  # Print Rows
  for ndk in ndks:
    cells = [ndk]
    for api in apis:
      marks = []
      for matrix in matrices:
        if matrix.has_build_for(ndk, api):
          marks.append('`{}`'.format(matrix.mark))
      cells.append(','.join(marks) if marks else '-')
    print_row(cells)


settings_export = re.compile(r'^(#)?export (\w+)=([^#]*)$')
def default_settings(matrices, file, **kw):
  for line in kw['before']:
    line = line.rstrip('\n')
    m = settings_export.match(line)
    if m:
      commented, name, value = m.groups()
      if not commented:
        commented = ''
      if name == 'ndk':
        value = str(Ndk.get('latest_stable'))
      elif name in default_default_flags:
        value = default_default_flags[name]
        if commented and isinstance(value, bool):
          value = shell_value(not value)
        else:
          value = shell_value(value)
      line = '{}export {}={}'.format(commented, name, value)
    print(line, file=file)


def bash_array(l):
  return '(' + ' '.join([f"'{e}'" for e in l]) + ')'


def known_apis(matrices, file, **kw):
  ndks = list(reversed(Ndk.all_ndks.values()))
  print('known_archs=' + bash_array(all_archs.keys()), file=file)
  print('known_ndks=' + bash_array([ndk.name for ndk in ndks]), file=file)
  print('known_ndks_ndk_only=' + bash_array([ndk.name for ndk in ndks if ndk.ndk_only]), file=file)
  for ndk in ndks:
    apis = bash_array(ndk.all_apis())
    print(f'known_apis_{ndk.name}={apis}', file=file)
    for arch in all_archs.keys():
      apis = bash_array(ndk.all_apis_for_arch(arch))
      print(f'known_apis_{ndk.name}_{arch}={apis}', file=file)


class Kind(Enum):
  GhActions = ('../../../.github/workflows/android-matrix.yml', '# {}', github, {})
  ReadMe = ('README.md', '<!-- {} -->', readme, {})
  DefaultSettings = ('default.settings.sh', '# {}', default_settings, {
    'pass_before': True,
  })
  KnownApis = ('known_apis.sh', '# {}', known_apis, {})


def get_comment(kind, *args, **kw):
  kind = Kind(kind)
  sep = kw['sep'] if 'sep' in kw else ' '
  return kind.value[1].format(sep.join(args))


def comment(file, kind, *args, **kw):
  print(get_comment(kind, *args, **kw), file=file, **kw)


def read_file(kind):
  kind = Kind(kind)
  begin = get_comment(kind, 'BEGIN MATRIX') + '\n'
  end = get_comment(kind, 'END MATRIX') + '\n'
  mode = 0
  before = []
  after = []
  filename = kind.value[0]
  with open(filename) as file:
    for line in file:
      if mode == 0:
        before.append(line)
        if line == begin:
          mode = 1
      elif mode == 1:
        if line == end:
          after.append(line)
          mode = 2
      else:
        after.append(line)
  return (filename, before, after)


class NullContext:
  def __enter__(self):
    pass
  def __exit__(self, *args):
    pass


define_ndks()


matrices = get_matrices()
for matrix in matrices:
  if args.debug:
    print(matrix.name)
    for ndk in matrix.ndks:
      print(' ', ndk)
      for build in matrix.builds_by_ndk[ndk]:
        print('   ', str(build))


for kind in Kind:
  opts = {
    'pass_before': False,
  }
  opts.update(kind.value[3])
  filename, before, after = read_file(kind)
  if args.debug:
    file_context = NullContext()
    file = sys.stdout
    print(fill_line(str(kind), '#'))
  else:
    file_context = file = open(filename, 'w')
  with file_context:
    if opts['pass_before']:
      opts['before'] = before
    else:
      for line in before:
        print(line, end='', file=file)
      comment(file, kind, 'This part is generated by matrix.py')
    kind.value[2](matrices, file, **opts)
    for line in after:
      print(line, end='', file=file)
