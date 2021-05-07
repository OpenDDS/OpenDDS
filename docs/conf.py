# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import re
import sys

# Custom Values ---------------------------------------------------------------

def setup(app):
    app.add_config_value('is_release', True, True)


# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

needs_sphinx = '2.4'
master_doc = 'index'
primary_domain = 'cpp'
pygments_style = 'manni'

project = 'OpenDDS'
copyright = '2020, Object Computing, Inc.'
author = 'Object Computing, Inc.'

# Get Version Info
with open('../dds/Version.h') as f:
    version_file = f.read()

def get_version_prop(kind, macro):
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

version = get_version_prop(str, 'OPENDDS_VERSION')
release = version
is_release = get_version_prop(bool, 'OPENDDS_IS_RELEASE')


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'recommonmark',
    'sphinx.ext.ifconfig',
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
    'README.md',
    'android.md',
    'cmake.md',
    'dependencies.md',
    'docker.md',
    'ios.md',
    'migrating_to_topic_type_annotations.md',
    'multirepo.md',
    'qt.md',
    'OpenDDS.docset/**',
    '.venv',
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

numfig = True


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'alabaster'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['.']

html_theme_options = {
    'logo': 'logo.svg',
    'logo_name': True,
    'extra_nav_links': {
        'Main Website': 'https://opendds.org',
        'GitHub Repo': 'https://github.com/objectcomputing/OpenDDS',
    },
}

html_favicon = 'logo_32_32.ico'


# -- LaTeX (PDF) output ------------------------------------------------------

latex_logo = 'logo_276_186.png'
