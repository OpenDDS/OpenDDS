#!/usr/bin/env python3

'''\
Convert CTest/CDash XML results to fake auto_run_tests output. See the
following URL for information about the XML file format:
https://public.kitware.com/Wiki/CDash:XML

Requires that ctest was run with "-T Test".
Might require "--no-compress-output" depending on how CMake embeds the test
console output into the XML.
'''

import sys
import os
import xml.etree.ElementTree
from pathlib import Path
from argparse import ArgumentParser
from base64 import b64decode
import zlib
import subprocess
import json
import re


py_source_dir = Path(__file__).resolve().parent
opendds_root = Path(os.environ['DDS_ROOT'])
template = '''\

==============================================================================
auto_run_tests: {art_name}
The CMake name of this test is "{cmake_name}"
The reported command was: {command}
The following is the actual output:
{output}
auto_run_tests_finished: {art_name} Time:{art_time}s Result:{art_result}
'''


def get_named_measurement(test_node, name):
    for node in test_node.findall('./Results/NamedMeasurement'):
        if node.get('name') == name:
            value = node.findtext('./Value')
            if node.get('type') == "numeric/double":
                value = float(value)
            return value
    return None


def relative_to(a, b):
    return Path(os.path.relpath(a.resolve(), start=b.resolve()))


def is_relative(path):
    return '..' not in path.parts


def fix_ctest_path(source_path, path):
    '''Work around ctest putting C_ instead of C: in the path
    '''
    drive = source_path.drive
    if drive and path.upper().startswith(drive[0].upper() + '_'):
        path = path[2:]
    return path


def cmake_list(value):
    return value.split(';')


source_dir_re = re.compile(r'# Source directory: (.*)')


def get_cmakelists_from_testfile(testfile):
    with testfile.open() as f:
        for line in f:
            line = line.rstrip('\n\r')
            m = source_dir_re.fullmatch(line)
            if m:
                cmakelists = Path(m.group(1)) / 'CMakeLists.txt'
                break
    if not cmakelists.is_file():
        raise FileNotFoundError('CMakeLists {} from {} is invalid'.format(cmakelists, testfile))
    return cmakelists


dump_line_re = re.compile(r'.*?DUMP\[(?P<name>\w+)\]: (?P<value>.*)')


def dump_ctest_info(cmake, build_path):
    # This uses the scripts cmake sets up for ctest to run to get the CMakeList
    # files for the tests.

    lines = subprocess.check_output([cmake, '-P', str(py_source_dir / 'dump_ctest_info.cmake')],
        cwd=str(build_path)).decode('utf-8').splitlines()
    tests = {}
    stack = []
    for index, line in enumerate(lines):
        line = line.rstrip('\n\r')
        m = dump_line_re.fullmatch(line)
        if m:
            name = m['name']
            value = m['value']
            if name == 'START_SUBDIRS':
                stack.append(dict(
                    test_file=value,
                    cmakelists=get_cmakelists_from_testfile(Path(value)),
                ))
            elif name == 'TEST':
                test_name, *cmd = cmake_list(value)
                if test_name in tests:
                    raise ValueError('Multiple tests called ' + test_name)
                tests[test_name] = dict(
                    cmakelists=stack[-1]['cmakelists'],
                    cmd=cmd,
                )
            elif name == 'SET_TESTS_PROPERTIES':
                pass  # Nothing to do with this at the moment
            elif name == 'END_SUBDIRS':
                current = stack.pop()
                if current['test_file'] != value:
                    raise ValueError(
                        'Got END_SUBDIRS {}, but {} is what was at the top of the stack!'.format(
                            value, current['test_file']))
            else:
                raise ValueError('Unexpected info name: {}'.format(name))
        else:
            raise ValueError('Unexpected line in ctest info dump: {}'.format(repr(line)))
    if stack:
        raise ValueError('Reached end of output, but stack is not empty: {}'.format(stack))

    return tests


def get_art_name(test_info):
    cmakelists = relative_to(test_info['cmakelists'], opendds_root).as_posix()
    command_parts = [p for p in test_info['cmd'][1:] if p]
    try:
        # Remove -ExeSubDir DIR to make the output consistent
        index = command_parts.index('-ExeSubDir')
        command_parts.pop(index)
        command_parts.pop(index)
    except ValueError:
        # from index, just means there's no -ExeSubDir
        pass

    return '{} {}'.format(cmakelists, ' '.join(command_parts))


def generate_test_results(tests, build_path, source_path, art_output, debug=False):
    testing_path = build_path.resolve() / 'Testing'
    if debug:
        print('testing_path:', testing_path)
    test_path_prefix = relative_to(source_path, opendds_root)
    if debug:
        print('test_path_prefix:', test_path_prefix)

    # Get first line in TAG file
    tag_path = testing_path / 'TAG'
    if not tag_path.is_file():
        sys.exit('ERROR: {} not found, please run the tests: '
            'ctest -T Test --no-compress-output'.format(tag_path))
    test_run_name = tag_path.read_text().split('\n')[0].strip()
    if debug:
        print('test_run_name =', test_run_name)

    # Iterate over Test nodes
    test_xml_path = testing_path / test_run_name / 'Test.xml'
    root_node = xml.etree.ElementTree.parse(str(test_xml_path)).getroot()
    for test_node in root_node.findall('./Testing/Test'):
        output_node = test_node.find('./Results/Measurement/Value')
        encoding = output_node.get('encoding')
        compression = output_node.get('compression')
        decoded_bytes = None
        output_text = None
        if encoding == 'base64':
            decoded_bytes = b64decode(output_node.text)
        elif encoding is None:
            decoded_bytes = output_node.text
        if decoded_bytes is not None:
            if compression == 'gzip':
                output_text = zlib.decompress(decoded_bytes).decode()
            elif encoding is None:
                output_text = decoded_bytes
        if output_text is None:
            sys.exit(('ERROR: Test output in XML file is not usable, '
                'encoding is {} and compression is {}. '
                'Pass --no-compress-output to ctest').format(
                    repr(encoding), repr(compression)))

        status = get_named_measurement(test_node, 'Completion Status')
        if status == "Missing Configuration":
            sys.exit('ERROR: Build has a configuration and ctest needs to know it. '
                'Pass --cmake-build-cfg with the config if using auto_run_tests.pl. '
                'Pass --build-config with the config if using ctest directly')

        results = dict(
            cmake_name=test_node.findtext('./Name'),
            path=fix_ctest_path(source_path, test_node.findtext('./Path')),
            passed=test_node.get('Status') == "passed",
            exec_time=get_named_measurement(test_node, 'Execution Time'),
            exit_value=get_named_measurement(test_node, 'Exit Value'),
            output=output_text,
            command=get_named_measurement(test_node, 'Command Line'),
        )

        results['art_name'] = get_art_name(tests[results['cmake_name']])

        # Exit Value isn't included if the test passed
        results['art_result'] = 0 if results['passed'] else results['exit_value']
        results['art_time'] = int(results['exec_time'])

        if debug:
            copy = results.copy()
            del copy['output']
            del copy['cmake_name']
            print(results['cmake_name'])
            for k, v in copy.items():
                print('   ', k, '=', repr(v))
        else:
            print(template.format(**results), file=art_output)


# Like a normal file, stdout itself can be used as a context manger, but
# like a normal file __exit__ will close it.
class StdoutContextManager:

    def __enter__(self):
        return sys.stdout

    def __exit__(self, *args):
        pass


if __name__ == "__main__":
    arg_parser = ArgumentParser(description=__doc__)
    arg_parser.add_argument('source_path', metavar='SOURCE_PATH', type=Path)
    arg_parser.add_argument('build_path', metavar='BUILD_PATH', type=Path)
    arg_parser.add_argument('--debug', action='store_true')
    arg_parser.add_argument('--list', action='store_true')
    arg_parser.add_argument('--art-output', metavar='ART_OUTPUT_FILE', type=Path)
    arg_parser.add_argument('--cmake', metavar='CMAKE_CMD', default='cmake')
    args = arg_parser.parse_args()

    args.source_path = args.source_path.resolve()
    args.build_path = args.build_path.resolve()

    tests = dump_ctest_info(args.cmake, args.build_path)

    if args.list:
        for name, info in tests.items():
            print(get_art_name(info))
    else:
        if args.art_output is not None:
            art_output_cm = args.art_output.resolve().open('w')
        else:
            art_output_cm = StdoutContextManager();

        with art_output_cm as art_output:
            generate_test_results(tests, args.build_path, args.source_path, art_output, args.debug)


# vim: expandtab:ts=4:sw=4
