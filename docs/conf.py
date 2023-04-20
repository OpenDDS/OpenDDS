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

from mpc_lexer import MpcLexer
from newsd import parse_newsd
from version_info import VersionInfo

# Custom Values ---------------------------------------------------------------

def setup(app):
    app.add_config_value('is_release', False, True)
    app.add_lexer('mpc', MpcLexer)


# -- Project information -----------------------------------------------------

needs_sphinx = '2.4'
master_doc = 'index'
primary_domain = 'cpp'
pygments_style = 'manni'
nitpicky = True

project = 'OpenDDS'
copyright = '2023, OpenDDS Foundation'
author = 'OpenDDS Foundation'
github_links_repo = 'OpenDDS/OpenDDS'

# Get Version Info
version_info = VersionInfo()
release = version_info.version
is_release = version_info.is_release
if is_release:
    github_links_release_tag = version_info.tag

# Generate WIP News if this isn't a release
wip_news = 'wip_news.rst'
if not is_release:
    newsd = parse_newsd()
    with (docs_path / wip_news).open('w') as f:
        newsd.print_all(file=f)


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.ifconfig',
    'links',
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
    'news.d/**',
    wip_news,
]

source_suffix = {
    '.rst': 'restructuredtext',
}

numfig = True

highlight_language = 'none'

linkcheck_ignore = [
    # Linkcheck fails to work with GitHub anchors
    r'^https?://github\.com/.*#.+$',
    # Returns 403 for some reason
    r'^https?://docs\.github\.com/.*$',
    # TODO: This needs to be fixed
    r'^http://download\.opendds\.org/modeling/eclipse_44/',
]


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
    'logo': 'logo_with_name.svg',
    'logo_name': False,
    'extra_nav_links': {
        'Main Website': 'https://opendds.org',
        'GitHub Repo': 'https://github.com/' + github_links_repo,
    },
    'fixed_sidebar': False,
    'page_width': '1100px',
    'body_max_width': '1100px',
}

html_favicon = 'logo_32_32.ico'


# -- LaTeX (PDF) output ------------------------------------------------------

latex_logo = 'logo_276_186.png'

# vim: expandtab:ts=4:sw=4
