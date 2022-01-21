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


def get_art_name(root, build_path, source_path, test_path, command):
    # Find the relative path to the directory with the test's CMakeLists
    # file from source_path.
    rel_test_path = relative_to(test_path, build_path)
    if is_relative(rel_test_path):
        real_test_path = source_path / rel_test_path
        condition = 1
    elif not test_path.is_absolute():
        real_test_path = source_path / test_path
        condition = 2
    elif test_path.name == 'build':
        real_test_path = test_path.parent
        condition = 3
    else:
        real_test_path = test_path
        condition = 4
    cmakelists = real_test_path / 'CMakeLists.txt'
    if not cmakelists.is_file():
        raise FileNotFoundError(
            '"{}" was not found (test_path was "{}", rel_test_path was "{}", '
            'condition was {})'.format(
                cmakelists, test_path, rel_test_path, condition))
    cmakelists = str(relative_to(cmakelists, root).as_posix())

    # Normalize the command to something like what auto_run_tests prints
    command_parts = command.split(' ') if isinstance(command, str) else command
    command_parts = [s.strip('"') for s in command_parts[1:]]
    command_parts = [p for p in command_parts if p]
    try:
        # Remove -ExeSubDir DIR to make the output consistent
        index = command_parts.index('-ExeSubDir')
        command_parts.pop(index)
        command_parts.pop(index)
    except ValueError:
        # from index, just means there's no -ExeSubDir
        pass

    return '{} {}'.format(cmakelists, ' '.join(command_parts))


def generate_test_results(build_path, source_path, debug=False):
    testing_path = build_path.resolve() / 'Testing'
    if debug:
        print('testing_path:', testing_path)
    root = Path(os.environ['DDS_ROOT'])
    test_path_prefix = relative_to(source_path, root)
    if debug:
        print('test_path_prefix:', test_path_prefix)

    # Get first line in TAG file
    test_run_name = (testing_path / 'TAG').read_text().split('\n')[0].strip()
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

        results['art_name'] = get_art_name(
            root, build_path, source_path, Path(results['path']), results['command'])

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
            print(template.format(**results))


def list_tests(build_path, source_path, debug=False):
    test_info = json.loads(subprocess.check_output(
        ['ctest', '--show-only=json-v1'], cwd=str(build_path)).decode('utf-8'))
    root = Path(os.environ['DDS_ROOT'])
    for test in test_info['tests']:
        command = test['command']
        for prop in test['properties']:
            if prop['name'] == 'WORKING_DIRECTORY':
                path = Path(prop['value'])
        print(get_art_name(root, build_path, source_path, path, command))


if __name__ == "__main__":
    arg_parser = ArgumentParser(description=__doc__)
    arg_parser.add_argument('source_path', metavar='SOURCE_PATH', type=Path)
    arg_parser.add_argument('build_path', metavar='BUILD_PATH', type=Path)
    arg_parser.add_argument('--debug', action='store_true')
    arg_parser.add_argument('--list', action='store_true')
    args = arg_parser.parse_args()

    args.source_path = args.source_path.resolve()
    args.build_path = args.build_path.resolve()

    if args.list:
        list_tests(args.build_path, args.source_path)
    else:
        generate_test_results(args.build_path, args.source_path, args.debug)


# vim: expandtab:ts=4:sw=4
