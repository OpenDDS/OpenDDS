import re

from pygments.lexer import *
from pygments.token import *

class MpcLexer(RegexLexer):
    name = 'Make Project Creator'
    aliases = ['mpc']
    filenames = ['*.mwc', '*.mbp', '*.mpc']
    mimetypes = []

    tokens = {
        'common': [
            (r'//.*', Comment),
            (words((
                'project', 'workspace',
                'compile_flags',
                'Define_Custom', 'Modify_Custom',
                'exename', 'sharedname', 'staticname', 'buildflags', 'dependent_upon', 'dllout', 'libout',
                'exeout', 'managed', 'no_pch', 'pch_header', 'pch_source', 'postbuild', 'prebuild', 'postclean',
                'recurse', 'version', 'macros', 'libpaths', 'recursive_libpaths', 'includes', 'libs',
                'recursive_includes', 'lit_libs', 'pure_libs', 'after', 'custom_only', 'dynamicflags',
                'staticflags', 'verbatim', 'specific', 'expand', 'conditional', 'requires', 'avoids', 'webapp',
                'dependent_upon', 'generates_source', 'subtype',
                'automatic', 'automatic_in', 'automatic_out', 'command', 'commandflags', 'dependent',
                'dependent_libs', 'inputext', 'keyword', 'libpath', 'output_option', 'output_follows_input',
                'pch_postrule', 'postcommand', 'pre_extension', 'source_pre_extension',
                'inline_pre_extension', 'header_pre_extension', 'template_pre_extension',
                'resource_pre_extension', 'documentation_pre_extension', 'generic_pre_extension',
                'pre_filename', 'source_pre_filename', 'inline_pre_filename', 'header_pre_filename',
                'template_pre_filename', 'resource_pre_filename', 'documentation_pre_filename',
                'generic_pre_filename', 'pre_dirname', 'source_pre_dirname', 'inline_pre_dirname',
                'header_pre_dirname', 'template_pre_dirname', 'resource_pre_dirname',
                'documentation_pre_dirname', 'generic_pre_dirname', 'source_outputext',
                'inline_outputext', 'header_outputext', 'template_outputext', 'resource_outputext',
                'documentation_outputext', 'generic_outputext',
                'feature', 'prop', 'else', 'associate', 'exclude', 'cmdline', 'Release', 'Debug',
            ), prefix=r'\b', suffix=r'\b'), Keyword),
        ],
        'value': [
            (r'\\\n', Name.Class),
            (r'$', Whitespace, '#pop'),
            include('common'),
            (r'\$\w+', Name.Variable),
            (r'\$\(\w+\)', Name.Variable),
            (r'\s+', Whitespace),
            (r'\S+', String),
        ],
        'block_common': [
            (r'(\w+)(\s*)(\+?=)(\s*)', bygroups(Name.Variable, Whitespace, Operator, Whitespace), 'value'),
            include('common'),
            (r'\s+', Whitespace),
            (r'(\+?=)(\s*)', bygroups(Operator, Whitespace), 'value'),
            ('}', Punctuation, '#pop'),
        ],
        'nested_block': [
            include('block_common'),
            (r'(\S+)$', bygroups(String)),
        ],
        'block': [
            (r'(\w+)(\s*)({)', bygroups(Name.Function, Whitespace, Punctuation), 'nested_block'),
            include('block_common'),
            (r'{', Punctuation, 'nested_block'),
            ('}', Punctuation, '#pop'),
        ],
        'root': [
            include('common'),
            (r'(\s*)(\()([A-Za-z0-9_*]+)(\))(\s*)',
                bygroups(Whitespace, Punctuation, Name.Class, Punctuation, Whitespace)),
            (r'[:,]', Punctuation),
            (r'\s+', Whitespace),
            (r'\w+', Name.Class),
            (r'({)', Punctuation, 'block'),
        ],
    }
