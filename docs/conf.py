# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import os
import re
import logging
from pathlib import Path

from docutils.transforms import Transform
from docutils import nodes

from sphinx.util.logging import NAMESPACE as sphinx_root_logger

docs_path = Path(__file__).parent
opendds_root_path = docs_path.parent
ext = (docs_path / 'sphinx_extensions').resolve()
sys.path.append(str(ext))
github_links_root_path = str(opendds_root_path)

import newsd
from mpc_lexer import MpcLexer
from version_info import VersionInfo
from links import add_omg_spec
from opendds_logging import GhaAnnotationHandler, create_gha_annotation, replace_files


# Make Sphinx output GitHub Actions annotations
logging.getLogger(sphinx_root_logger).addHandler(
    GhaAnnotationHandler(replace_files=replace_files))


# Custom Values ---------------------------------------------------------------

class GlobalSubstitutions(Transform):
    default_priority = 200

    def apply(self):
        config = self.document.settings.env.config
        global_substitutions = config['global_substitutions']
        to_handle = set(global_substitutions.keys()) - set(self.document.substitution_defs)
        for ref in self.document.traverse(nodes.substitution_reference):
            refname = ref['refname']
            if refname in to_handle:
                try:
                    text = str(global_substitutions[refname])
                    ref.replace_self(nodes.Text(text, text))
                except:
                    pass


autotab = 'autotab.js'


def setup(app):
    app.add_config_value('global_substitutions', vars(opendds_version_info), True)
    app.add_config_value('is_release', False, True)
    app.add_lexer('mpc', MpcLexer)
    app.add_transform(GlobalSubstitutions)
    app.add_js_file(autotab)

    add_omg_spec(app, 'DDS', '1.4')
    add_omg_spec(app, 'DDSI-RTPS', '2.3', our_name='rtps', display_name='RTPS')
    add_omg_spec(app, 'DDS-SECURITY', '1.1', our_name='sec', display_name='DDS Security')
    add_omg_spec(app, 'DDS-XTypes', '1.3', our_name='xtypes')
    add_omg_spec(app, 'IDL', '4.2')
    add_omg_spec(app, 'CPP', '1.3', our_name='cpp03', display_name='IDL to C++03')
    add_omg_spec(app, 'CPP11', '1.5', display_name='IDL to C++11')
    add_omg_spec(app, 'I2JAV', '1.3', our_name='java', display_name='IDL to Java')


# -- Project information -----------------------------------------------------

needs_sphinx = '2.4'
master_doc = 'index'
primary_domain = 'cpp'
pygments_style = 'manni'
nitpicky = True

project = 'OpenDDS'
author = 'OpenDDS Foundation'
copyright = author
github_links_repo = 'OpenDDS/OpenDDS'
github_main_branch = 'master'
github_repo = 'https://github.com/' + github_links_repo
rtd_base = 'https://opendds.readthedocs.io/en/'

# Get Version Info
opendds_version_info = VersionInfo()
release = opendds_version_info.version
is_release = opendds_version_info.is_release
if is_release:
    github_links_release_tag = opendds_version_info.tag
ace6tao2_version = opendds_version_info.ace6tao2_version
ace8tao4_version = opendds_version_info.ace8tao4_version
if opendds_version_info.release_year:
    copyright = f'{opendds_version_info.release_year} {copyright}'

# Handle News Generation
try:
    # Generate news for all releases
    with (docs_path / 'news.rst').open('w') as f:
        newsd.print_all_news(file=f)

    # Generate news used for NEWS.md and Markdown release notes for GitHub
    with (docs_path / 'this-release.rst').open('w') as f:
        print(':orphan:\n', file=f)
        newsd.parse_newsd().print_all(file=f)
except newsd.ParseError as e:
    anno = create_gha_annotation(
        'error', e.message, title='news', file=e.path, line=e.lineno, replace_files=replace_files)
    if anno:
        print(anno)
    raise


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    # Custom ones
    'links',
    'cmake_domain',
    'config_domain',

    # Official ones
    'sphinx.ext.ifconfig',
    'sphinx.ext.todo',
    'sphinx.ext.intersphinx',

    # Other ones
    'sphinx_copybutton',
    'sphinx_markdown_builder',
    'sphinx_inline_tabs',
    'sphinxcontrib.svgbob'
]

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
]

source_suffix = {
    '.rst': 'restructuredtext',
}

numfig = False

highlight_language = 'none'

linkcheck_retries = 3
linkcheck_ignore = [
    # Linkcheck doesn't work with GitHub anchors
    r'^https?://github\.com/.*#.+$',
]

intersphinx_mapping = {
    'sphinx': ('https://www.sphinx-doc.org/en/master', None),
    'cmake': ('https://cmake.org/cmake/help/latest', None),
}

manpages_url = 'https://manpages.debian.org/{page}({section})'


# -- Options for Markdown output ---------------------------------------------
# This builder is just used to generate the release notes for GitHub

# These options point the markdown to the Sphinx on RTD. This way we can refer
# to things in the Sphinx in the news and it will work in RTD and in the GitHub
# release notes.
markdown_http_base = rtd_base
if is_release:
    markdown_http_base += opendds_version_info.tag.lower()
else:
    markdown_http_base += os.getenv('MD_RTD_BRANCH', github_main_branch)
markdown_uri_doc_suffix = '.html'


# -- Options for HTML output -------------------------------------------------

logo = 'logo_with_name.svg'

html_static_path = [
    logo,
    autotab,
]

html_css_files = [
    'custom.css',
]

html_theme = 'furo'
# See documentation for the theme here:
#   https://pradyunsg.me/furo/

html_title = project + ' ' + release

html_theme_options = {
    'light_logo': logo,
    'dark_logo': logo,
    'sidebar_hide_name': True, # Logo has the name in it
    # furo doesn't support a view source link for some reason, force edit
    # button to do that.
    'source_edit_link': github_repo + '/blob/' + github_main_branch + '/docs/{filename}?plain=1',
}

# Change the sidebar to include fixed links
#   https://pradyunsg.me/furo/customisation/sidebar/#making-changes
html_context = {
    'sidebar_links': {
        'Main Website': 'https://opendds.org',
        'GitHub Repo': github_repo,
    }
}
templates_path = [str(ext / 'templates')]
html_sidebars = {
    '**': [
        'sidebar/brand.html',
        'sidebar-links.html',
        'sidebar/search.html',
        'sidebar/scroll-start.html',
        'sidebar/navigation.html',
        'sidebar/ethical-ads.html',
        'sidebar/scroll-end.html',
        'sidebar/variant-selector.html',
    ]
}

html_favicon = 'logo_32_32.ico'


# -- LaTeX (PDF) output ------------------------------------------------------

latex_logo = 'logo_276_186.png'

latex_elements = {
    # This seems to fix "LaTeX Error: Too deeply nested." on local PDF builds
    # https://stackoverflow.com/questions/57945414
    'preamble': r'''
\usepackage{enumitem}
\setlistdepth{20}
\renewlist{itemize}{itemize}{20}
\renewlist{enumerate}{enumerate}{20}
\setlist[itemize]{label=$\cdot$}
\setlist[itemize,1]{label=\textbullet}
\setlist[itemize,2]{label=--}
\setlist[itemize,3]{label=*}
''',
}

# vim: expandtab:ts=4:sw=4
