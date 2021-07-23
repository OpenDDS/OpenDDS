# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import re
import sys
from pathlib import Path

docs_path = Path(__file__).parent
opendds_root_path = docs_path.parent
sys.path.append(str((docs_path / 'sphinx_extensions').resolve()))
github_links_root_path = str(opendds_root_path)


# Custom Values ---------------------------------------------------------------

def setup(app):
    app.add_config_value('is_release', False, True)


# -- Project information -----------------------------------------------------

needs_sphinx = '2.4'
master_doc = 'index'
primary_domain = 'cpp'
pygments_style = 'manni'
nitpicky = True

project = 'OpenDDS'
copyright = '2021, Object Computing, Inc.'
author = 'Object Computing, Inc.'
github_links_repo = 'objectcomputing/OpenDDS'

# Get Version Info
with (opendds_root_path / 'dds/Version.h').open() as f:
    version_file = f.read()

def get_version_prop(kind, macro):
    macro = 'OPENDDS_' + macro.upper()
    if kind is bool:
        regex = r'([01])'
        cast = lambda v: bool(int(v))
    elif kind is int:
        regex = r'(\d+)'
        cast = int
    elif kind is str:
        regex = r'"(.*)"'
        cast = lambda v: v
    else:
        raise RuntimeError('Unexpected kind: ' + repr(kind))
    m = re.search(r'#define {} {}'.format(macro, regex), version_file)
    if m:
        return cast(m[1])
    raise RuntimeError('Could not find ' + macro)

version = get_version_prop(str, 'version')
release = version
is_release = get_version_prop(bool, 'is_release')
if is_release:
    vparts = {p: get_version_prop(int, p + '_version') for p in
        ('major', 'minor', 'micro')}
    github_links_release_tag = 'DDS-{major}.{minor}'.format(**vparts)
    if vparts['micro']:
        github_links_release_tag += '.' + vparts['micro']


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.ifconfig',
    'github_links',
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    '_build',
    'Thumbs.db',
    '.DS_Store',
    'history/**',
    'design/**',
    'OpenDDS.docset/**',
    '.venv',
    'sphinx_extensions/**',
]

source_suffix = {
    '.rst': 'restructuredtext',
}

numfig = True


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'alabaster'
# See documentation for alabaster here:
#   https://alabaster.readthedocs.io/en/latest/customization.html

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['.']

html_theme_options = {
    'logo': 'logo.svg',
    'logo_name': True,
    'extra_nav_links': {
        'Main Website': 'https://opendds.org',
        'GitHub Repo': 'https://github.com/' + github_links_repo,
    },
    'fixed_sidebar': True,
}

html_favicon = 'logo_32_32.ico'


# -- LaTeX (PDF) output ------------------------------------------------------

latex_logo = 'logo_276_186.png'

# vim: expandtab:ts=4:sw=4
