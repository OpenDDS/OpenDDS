#!/usr/bin/env python3

'''\
Helper script for the Sphinx documentation
'''

import sys
import os
import venv
import webbrowser
import itertools
from subprocess import check_call
from shutil import rmtree
from argparse import ArgumentParser
from pathlib import Path

docs_path = Path(__file__).parent
abs_docs_path = docs_path.resolve()
reqs_path = abs_docs_path / 'requirements.txt'
default_venv_path = docs_path / '.venv'
default_build_path = docs_path / '_build'


def log(*args, **kwargs):
    error = kwargs.pop('error', False)
    f = sys.stderr if error else sys.stdout
    prefix = 'build.py: '
    if error:
        prefix += 'ERROR: '
    print(prefix, end='', file=f)
    print(*args, **kwargs, file=f, flush=True)


class DocEnv:
    def __init__(self, venv_path, build_path,
            gh_links_commit=None, conf_defines=None, debug=False):
        self.venv_path = Path(venv_path)
        self.abs_venv_path = self.venv_path.resolve()
        self.bin_path = self.abs_venv_path / 'bin'
        self.build_path = Path(build_path)
        self.abs_build_path = self.build_path.resolve()
        self.conf_defines = []
        if conf_defines is not None:
            self.conf_defines.extend(conf_defines)
        if gh_links_commit is not None:
            self.conf_defines.append('github_links_commitish=' + gh_links_commit)
        self.debug = debug
        self.done = set()

    def run(self, *cmd, cwd=abs_docs_path):
        env = os.environ.copy()
        env['VIRUTAL_ENV'] = str(self.abs_venv_path)
        env['PATH'] = str(self.bin_path) + os.pathsep + env['PATH']
        if os.environ.get('GITHUB_ACTIONS', 'false') == 'true':
            # Github actions doesn't act as a TTY:
            # https://github.com/actions/runner/issues/241
            # This should force at least sphinx-builder to use color for the
            # link check so it's easier to see errors.
            env['FORCE_COLOR'] = 'true'
        log('Running', repr(' '.join(cmd)), 'in', repr(str(cwd)))
        check_call(cmd, env=env, cwd=cwd)

    def rm_build(self):
        if self.build_path.is_dir():
            log('build.py: Removing existing {}...'.format(self.build_path))
            rmtree(self.build_path)

    def setup(self, force_new=False):
        install_deps = False
        if force_new or not self.venv_path.is_dir():
            log('Creating venv...')
            venv.create(self.venv_path, clear=True, with_pip=True)
            install_deps = True
        elif reqs_path.stat().st_mtime >= self.venv_path.stat().st_mtime:
            log('Requirements file was changed, updating dependencies.')
            install_deps = True

        if install_deps:
            log('Install Dependencies...')
            self.run('python', '-m', 'pip', 'install', '-r', str(reqs_path))
            self.venv_path.touch()
            self.rm_build()

    def sphinx_build(self, builder, *args, defines=[]):
        args = list(args)
        for define in itertools.chain(self.conf_defines, defines):
            args.append('-D' + define)
        if self.debug:
            args.append('-vv')
        self.run('sphinx-build', '-M', builder, '.', str(self.abs_build_path), *args)

    def do(self, actions, because_of=None, open_result=False):
        for action in actions:
            log('Doing', action, ('needed by ' + because_of) if because_of else '')
            if action in self.done:
                log(action, 'already done')
                continue
            result_path = getattr(self, 'do_' + action)()
            self.done |= {action,}
            if open_result:
                if result_path is None:
                    log('Can\'t open', action, 'result')
                else:
                    uri = result_path.as_uri()
                    log('Opening', uri)
                    webbrowser.open(uri)

    @classmethod
    def all_actions(cls):
        return [k[3:] for k, v in vars(cls).items() if k.startswith('do_')]

    def do_test(self):
        self.run('python3', '-m', 'unittest', 'discover', '--verbose',
            '--start-directory', 'sphinx_extensions',
            '--pattern', '*.py',
            cwd=abs_docs_path)
        return None

    def do_strict(self):
        self.do(['test'], because_of='strict')
        self.sphinx_build('dummy', '-W')
        return None

    def do_linkcheck(self):
        self.sphinx_build('linkcheck', defines=['gen_all_omg_spec_links=False'])
        return None

    def do_html(self):
        self.sphinx_build('html')
        return self.abs_build_path / 'html/index.html'

    def do_singlehtml(self):
        self.sphinx_build('singlehtml')
        return self.abs_build_path / 'singlehtml/index.html'

    def do_pdf(self):
        self.sphinx_build('latexpdf')
        return self.abs_build_path / 'latex/opendds.pdf'

    def do_dash(self):
        self.do(['html'], because_of='dash')
        self.run('doc2dash', 'html',
            '--name', 'OpenDDS',
            '--icon', str(abs_docs_path / 'logo_100_100.png'),
            '--force', '--enable-js',
            cwd=self.abs_build_path,
        )
        return None

    def do_markdown(self):
        self.sphinx_build('markdown', '-Dsmartquotes_action=')
        return None


if __name__ == '__main__':
    arg_parser = ArgumentParser(description=__doc__)
    arg_parser.add_argument('actions', nargs='+', choices=DocEnv.all_actions())
    arg_parser.add_argument('-o', '--open',
        action='store_true',
        help='Open result after building'
    )
    arg_parser.add_argument('-c', '--gh-links-commit',
        metavar='COMMITISH',
        help='What commit, branch, or tag to use for file links to the GitHub repo. ' +
            'Default depends on the current state of the git repo.'
    )
    arg_parser.add_argument('--build',
        metavar='PATH', type=Path, default=default_build_path,
        help='Where to place the results. Default is %(default)s'
    )
    arg_parser.add_argument('--venv',
        metavar='PATH', type=Path, default=default_venv_path,
        help='Where to place the Python virtual environment. Default is %(default)s'
    )
    arg_parser.add_argument('-D',
        metavar='NAME=VALUE',
        dest='conf_defines',
        action='append',
        help='Passed to sphinx-build to override conf.py values.'
    )
    arg_parser.add_argument('-d', '--debug', action='store_true')
    args = arg_parser.parse_args()

    doc_env = DocEnv(
        venv_path=args.venv, build_path=args.build,
        gh_links_commit=args.gh_links_commit,
        conf_defines=args.conf_defines,
        debug=args.debug,
    )
    doc_env.setup()
    doc_env.do(args.actions, open_result=args.open)

# vim: expandtab:ts=4:sw=4
