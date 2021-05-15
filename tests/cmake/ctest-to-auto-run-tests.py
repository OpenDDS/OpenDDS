#!/usr/bin/env python3

# Convert CTest/CDash XML results to fake auto_run_tests output.
# Info about the XML file format: https://public.kitware.com/Wiki/CDash:XML

import sys
import xml.etree.ElementTree
from pathlib import Path
from argparse import ArgumentParser

template = '''\
auto_run_tests: {art_name}
The CMake name of this test is "{cmake_name}"
{output}
auto_run_tests_finished: {art_name} Time:{art_time}s Result:{art_result}
'''

test_path_prefix = 'tests/cmake/'

def get_named_measurement(test_node, name):
    for node in test_node.findall('./Results/NamedMeasurement'):
        if node.get('name') == name:
            value = node.findtext('./Value')
            if node.get('type') == "numeric/double":
                value = float(value)
            return value
    return None

def generate_test_results(build_path, debug=False):
    testing_path = build_path / 'Testing'

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
        if output_node.get('encoding') is not None or output_node.get('compression') is not None:
            sys.exit('ERROR: Test output in XML file is not usable, ' +
                'pass --no-compress-output to ctest')

        results = dict(
            cmake_name=test_node.findtext('./Name'),
            path=test_node.findtext('./Path'),
            passed=test_node.get('Status') == "passed",
            exec_time=get_named_measurement(test_node, 'Execution Time'),
            exit_value=get_named_measurement(test_node, 'Exit Value'),
            output=output_text,
            command=get_named_measurement(test_node, 'Command Line'),
        )
        test_path = Path(test_path_prefix) / results['path']
        results['art_name'] = '{} {}'.format(test_path, results['command'])
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
    arg_parser = ArgumentParser(
        description='Convert CTest/CDash XML results to fake auto_run_tests output.')
    arg_parser.add_argument('build_paths', metavar='BUILD_PATH', type=Path, nargs='+')
    arg_parser.add_argument('--debug', action='store_true', default=False)
    args = arg_parser.parse_args()
    for build_path in args.build_paths:
        generate_test_results(build_path, args.debug)
