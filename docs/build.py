#!/usr/bin/env python3

'''\
Helper script for the Sphinx documentation
'''

import sys
import os
import venv
import webbrowser
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
    def __init__(self, venv_path, build_path, gh_links_commit=None):
        self.venv_path = Path(venv_path)
        self.abs_venv_path = self.venv_path.resolve()
        self.bin_path = self.abs_venv_path / 'bin'
        self.build_path = Path(build_path)
        self.abs_build_path = self.build_path.resolve()
        self.gh_links_commit = gh_links_commit
        self.done = set()

    def run(self, *cmd, cwd=abs_docs_path):
        env = os.environ.copy()
        env['VIRUTAL_ENV'] = str(self.abs_venv_path)
        env['PATH'] = str(self.bin_path) + os.pathsep + env['PATH']
        log('Running', repr(' '.join(cmd)), 'in', repr(str(cwd)))
        check_call(cmd, env=env, cwd=cwd)

    def rm_build(self):
        if self.build_path.is_dir():
            log('build.py: Removing existing {}...'.format(self.build_path))
            rmtree(self.build_path)

    def setup(self, force_new=False):
        sanity_path = self.bin_path / 'sphinx-build'
        install_deps = False
        if force_new or not self.venv_path.is_dir():
            log('Creating venv...')
            venv.create(self.venv_path, clear=True, with_pip=True)
            install_deps = True
        elif reqs_path.stat().st_mtime >= self.venv_path.stat().st_mtime:
            log('Requirements file was changed, updating dependencies.')
            install_deps = True
        elif not sanity_path.is_file():
            log('sphinx-build not found, need to install dependencies?')
            install_deps = True

        if install_deps:
            log('Install Dependencies...')
            self.run('python', '-m', 'pip', 'install', '-r', str(reqs_path))
            if not sanity_path.is_file():
                log('sphinx-build not found after installing dependencies', error=True)
                sys.exit(1)
            self.venv_path.touch()
            self.rm_build()

    def sphinx_build(self, builder, *args):
        args = list(args)
        if self.gh_links_commit is not None:
            args.append('-Dgithub_links_commitish=' + self.gh_links_commit)
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

    def do_strict(self):
        self.sphinx_build('dummy', '-W')
        self.sphinx_build('linkcheck')
        return None

    def do_html(self):
        self.sphinx_build('html')
        return self.abs_build_path / 'html/index.html'

    def do_pdf(self):
        self.sphinx_build('latexpdf')
        return self.abs_build_path / 'latex/opendds.pdf'

    def do_dash(self):
        self.do(['html'], because_of='dash')
        self.run('python', '-m', 'pip', 'install', 'doc2dash')
        self.run('doc2dash', 'html',
            '--name', 'OpenDDS',
            '--icon', str(abs_docs_path / 'logo_100_100.png'),
            '--force', '--enable-js',
            cwd=self.abs_build_path,
        )
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
    args = arg_parser.parse_args()

    doc_env = DocEnv(
        venv_path=args.venv, build_path=args.build,
        gh_links_commit=args.gh_links_commit,
    )
    doc_env.setup()
    doc_env.do(args.actions, open_result=args.open)

# vim: expandtab:ts=4:sw=4
