#!/usr/bin/env python3

'''\
Convert CTest/CDash XML results to fake auto_run_tests output. See the
following URL for information about the XML file format:
https://public.kitware.com/Wiki/CDash:XML

Requires that ctest was run with "--no-compress-output -T Test".
'''

import sys
import os
import xml.etree.ElementTree
from pathlib import Path
from argparse import ArgumentParser

template = '''\

==============================================================================
auto_run_tests: {art_name}
The CMake name of this test is "{cmake_name}"
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


def fix_ctest_path(abs_source_path, path):
    '''Work around ctest putting C_ instead of C: in the path
    '''
    drive = abs_source_path.drive
    if drive and path.upper().startswith(drive[0].upper() + '_'):
        path = path[2:]
    return path


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
        output_text = output_node.text
        if output_node.get('encoding') is not None or \
                output_node.get('compression') is not None:
            sys.exit('ERROR: Test output in XML file is not usable, ' +
                'pass --no-compress-output to ctest')

        status = get_named_measurement(test_node, 'Completion Status')
        if status == "Missing Configuration":
            sys.exit('ERROR: Build has a configuration and ctest needs to know it. ' +
                'Pass --cmake-build-cfg with the config if using auto_run_tests.pl. ' +
                'Pass --build-config with the config if using ctest directly')

        abs_source_path = source_path.resolve()

        results = dict(
            cmake_name=test_node.findtext('./Name'),
            path=fix_ctest_path(abs_source_path, test_node.findtext('./Path')),
            passed=test_node.get('Status') == "passed",
            exec_time=get_named_measurement(test_node, 'Execution Time'),
            exit_value=get_named_measurement(test_node, 'Exit Value'),
            output=output_text,
            command=get_named_measurement(test_node, 'Command Line'),
        )

        # Find the relative path to the directory with the test's CMakeLists
        # file from source_path.
        abs_test_path = Path(results['path'])
        if not abs_test_path.is_absolute():
            abs_test_path = abs_source_path / abs_test_path
        if abs_test_path.name == 'build':
            abs_test_path = abs_test_path.parent
        cmakelists = abs_test_path / 'CMakeLists.txt'
        if not cmakelists.is_file():
            raise FileNotFoundError('"{}" was not found'.format(cmakelists))
        test_path = test_path_prefix / relative_to(abs_test_path, source_path)

        command_parts = [s.strip('"') for s in results['command'].split(' ')[1:]]
        results['art_name'] = '{}/{}'.format(test_path, ' '.join(command_parts))
        # Exit Value isn't included if the test passed
        results['art_result'] = 0 if results['passed'] else results['exit_value']
        results['art_time'] = time=int(results['exec_time'])

        if debug:
            copy = results.copy()
            del copy['output']
            del copy['cmake_name']
            print(results['cmake_name'])
            for k, v in copy.items():
                print('   ', k, '=', repr(v))
        else:
            print(template.format(**results))

if __name__ == "__main__":
    arg_parser = ArgumentParser(description=__doc__)
    arg_parser.add_argument('source_path', metavar='SOURCE_PATH', type=Path)
    arg_parser.add_argument('build_path', metavar='BUILD_PATH', type=Path)
    arg_parser.add_argument('--debug', action='store_true', default=False)
    args = arg_parser.parse_args()
    generate_test_results(args.build_path, args.source_path, args.debug)

# vim: expandtab:ts=4:sw=4
