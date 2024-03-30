# Sphinx Domain for Configuration Values
# Pass -d to build.py to get it to log the directives it's getting.

from __future__ import annotations

import sys
import re
from typing import Any
import re
import unittest

from docutils import nodes
from docutils.parsers.rst import directives
from docutils.statemachine import ViewList

from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.util import logging
from sphinx.util.nodes import nested_parse_with_titles
from sphinx.roles import XRefRole

from custom_domain import CustomDomain, CustomDomainObject, ContextWrapper


class ConfigDomain(CustomDomain):
    name = 'cfg'
    label = 'config'
    logger = logging.getLogger(__name__)


def parse(what, regex, string, node, *ret):
    m = re.fullmatch(regex, string)
    if not m:
        if node is None:
            return None
        else:
            e = ValueError(f'{repr(string)} isn\'t a valid value for a {what}')
            ConfigDomain.logger.warning(e, location=node.get_location())
            raise e
    if ret:
        groups = m.groupdict()
        return tuple([groups[name] for name in ret])
    return m.groups()


id_re = r'[\$\w-]+'


# cfg:sec =====================================================================

section_re = r'(?P<sec_name>' + id_re + r')(?:@(?P<sec_disc>' + id_re + r'))?'


def parse_section_name(full_name, node=None):
    return parse(ConfigSection._full_name, section_re, full_name, node,
        'sec_name', 'sec_disc')


def section_text(sec_name, sec_disc, insert=''):
    text = f'[{sec_name}]{insert}'
    if sec_disc:
        text += f' ({sec_disc})'
    return text


class ConfigSectionRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> tuple[str, str]:
        target = target.strip()
        if not has_explicit_title:
            sec_name, sec_disc = parse_section_name(target, self)
            title = section_text(sec_name, sec_disc)
        return title, f'[{target}]'


@ConfigDomain.add_type
class ConfigSection(CustomDomainObject):
    our_name = 'sec'
    our_ref_role_type = ConfigSectionRefRole

    def get_index_text(self, name, full_name):
        sec_name, sec_disc = parse_section_name(name)
        if sec_disc:
            sec_disc += ' '
        else:
            sec_disc = ''
        return f'{sec_name} ({sec_disc}config section)'

    def parse_sig(self, ctx, sig):
        name, arguments = parse(self._full_name,
            r'(' + id_re + r'(?:@' + id_re + r')?)(?:/(.*))?', sig, self)
        sec_name, sec_disc = parse_section_name(name)
        ctx.push(self, name, f'[{name}]',
            sec_name=sec_name, sec_disc=sec_name, arguments=arguments, keys=[])
        return (sec_name, sec_disc, arguments)

    def create_signode(self, ctx, name, signode, sec_name, sec_disc, arguments):
        signode += addnodes.desc_addname('[', '[')
        signode += addnodes.desc_name(sec_name, sec_name)
        if arguments:
            text = '/' + arguments
            signode += addnodes.desc_addname(text, text)
        signode += addnodes.desc_addname(']', ']')
        if sec_disc:
            text = f' ({sec_disc})'
            signode += addnodes.desc_annotation(text, text)


# cfg:key =====================================================================

key_name_re = id_re + r'(?:\.' + id_re + r')*'
key_re = r'(?:\[(?P<sec>' + section_re + r')\])?(?P<key_name>' + key_name_re + r')'


def parse_key_name(full_name, node=None):
    return parse(ConfigKey._full_name, key_re, full_name, node,
        'sec', 'sec_name', 'sec_disc', 'key_name')


def key_text(sec_name, sec_disc, key_name, insert=''):
    text = key_name + insert
    if sec_name is not None:
        text = section_text(sec_name, sec_disc, text)
    return text


# This should be equivalent to ConfigPair::canonicalize
def key_canonicalize(*parts):
    key = '_'.join(filter(None, parts))

    # Replace everything that's not a letter, number, or underscore
    key = re.sub(r'[^\w]', '_', key)

    # Convert CamelCase to camel_case
    key = re.sub(r'([A-Z][a-z])', r'_\1', key)
    key = re.sub(r'([a-z])([A-Z])', r'\1_\2', key)

    # Removed repeated underscores
    key = re.sub(r'_+', r'_', key)

    return key.strip('_').upper()


class ConfigKeyRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> tuple[str, str]:
        ctx = ContextWrapper(env, 'cfg')
        target = target.strip()

        # Normalize target for the current scope
        sec, sec_name, sec_disc, key_name = parse_key_name(target, self)
        scope_kind = len(ctx.stack)
        if sec is not None:
            # [sec]key anywhere
            pass
        elif scope_kind == 0 and sec is None:
            # Assume key outside section should be in [common]
            target = f'[common]{target}'
        elif scope_kind >= 1:
            # key anywhere in a section
            prefix = ctx.get_full_name(0)
            target = f'{prefix}{target}'
        else:
            ConfigDomain.logger.warning(f'{repr(target)} is an invalid target here',
                location=self.get_location())

        if not has_explicit_title:
            # Reparse target and hide parts of title that are in the current scope
            sec, sec_name, sec_disc, key_name = parse_key_name(target, self)
            scope = ctx.get_all_names()
            scope = scope + [None] * (2 - len(scope))
            if scope[0] == sec:
                sec_name = None

            title = key_text(sec_name, sec_disc, key_name)

        return title, target


@ConfigDomain.add_type
class ConfigKey(CustomDomainObject):
    option_spec: OptionSpec = CustomDomainObject.option_spec.copy()
    option_spec.update({
        'required': directives.flag,
        'default': directives.unchanged,
    })

    our_name = 'key'
    our_parent_required = True
    our_parent_type = ConfigSection
    our_ref_role_type = ConfigKeyRefRole

    def get_index_text(self, name, full_name):
        sec, sec_name, sec_disc, key = parse_key_name(full_name)
        return f'{key} (config key)'

    def parse_sig(self, ctx, sig):
        name, arguments = parse(self._full_name, r'(' + key_name_re + r')(?:=(.*))?', sig, self)
        sec = ctx.get_full_name()
        ctx.push(self, name, f'{sec}{name}', arguments=arguments)
        sec_ctx = ctx.get(-2, 'keys')
        if sec_ctx is not None:
            sec_ctx.append((name, self.get_location()))
        return (arguments,)

    def create_signode(self, ctx, name, signode, arguments):
        signode += addnodes.desc_name(name, name)
        if arguments is not None:
            text = '=' + arguments
            signode += addnodes.desc_addname(text, text)

    def transform_content(self, contentnode: addnodes.desc_content) -> None:
        ctx = self.get_context()

        # Insert this stuff at the top
        p = nodes.paragraph()
        contentnode.insert(0, p)
        rst = ViewList()

        # Config store key
        key = key_canonicalize(ctx.get(-2, 'sec_name'), ctx.get(-2, 'arguments'), ctx.get_name())
        rst.append(f'| **Config store key**: ``{key}``', f'{__name__}', 1)

        # :required: flag
        required = 'required' in self.options
        if required:
            rst.append(f'| **Required**', f'{__name__}', 1)

        # :default: flag
        default = self.options.get('default')
        if default:
            if required:
                e = ValueError(f'A {self._full_name} shouldn\'t be both default and required')
                ConfigDomain.logger.warning(e, location=self.get_location())
            rst.append(f'| **Default:** {default}\n', f'{__name__}', 1)

        nested_parse_with_titles(self.state, rst, p)


# cfg:val =====================================================================

value_re = r'(?:(?P<key>' + key_re + r'):)?(?P<val_name>' + id_re + r')'


def parse_value_name(full_name, node=None):
    return parse(ConfigValue._full_name, value_re, full_name, node,
        'sec', 'sec_name', 'sec_disc', 'key', 'key_name', 'val_name')


def value_text(sec_name, sec_disc, key_name, val_name):
    text = val_name
    if key_name is not None:
        text = key_text(sec_name, sec_disc, key_name, ':' + text)
    return text


class ConfigValueRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> tuple[str, str]:
        ctx = ContextWrapper(env, 'cfg')
        target = target.strip()

        # Normalize target for the current scope
        sec, sec_name, sec_disc, key, key_name, val_name = parse_value_name(target, self)
        scope_kind = len(ctx.stack)
        if sec is not None:
            # [sec]key=val anywhere
            pass
        elif scope_kind == 0 and sec is None:
            # key:val outside section
            target = f'[common]{target}'
        elif scope_kind >= 1 and key is not None:
            # key:val anywhere in a section
            prefix = ctx.get_full_name(0)
            target = f'{prefix}{target}'
        elif scope_kind >= 2 and key is None:
            # val anywhere in a key
            prefix = ctx.get_full_name(1)
            target = f'{prefix}:{target}'
        else:
            ConfigDomain.logger.warning(f'{repr(target)} is an invalid target here',
                location=self.get_location())

        if not has_explicit_title:
            # Reparse target and hide parts of title that are in the current scope
            sec, sec_name, sec_disc, key, key_name, val_name = parse_value_name(target, self)
            scope = ctx.get_all_names()
            scope = scope + [None] * (3 - len(scope))
            if scope[0] == sec:
                sec_name = None
                if scope[1] == key_name:
                    key_name = None

            title = value_text(sec_name, sec_disc, key_name, val_name)

        return title, target


@ConfigDomain.add_type
class ConfigValue(CustomDomainObject):
    our_name = 'val'
    our_parent_required = True
    our_parent_type = ConfigKey
    our_ref_role_type = ConfigValueRefRole

    def run(self):
        # Values should probably never appear on the index or the page TOC
        self.options['no-contents-entry'] = ''
        self.options['no-index-entry'] = ''
        return super().run()

    def get_index_text(self, name, full_name):
        sec, sec_name, sec_disc, key, key_name, val_name = parse_value_name(full_name, self)
        return f'{val_name} (config value)'

    def parse_sig(self, ctx, sig):
        name_wo_brackets, name_w_brackets = parse(self._full_name,
            r'(' + id_re + r')|<(' + id_re + r')>', sig, self)
        brackets = bool(name_w_brackets)
        name = name_w_brackets if brackets else name_wo_brackets
        ctx.push(self, name, ctx.get_full_name() + f':{name}')
        return (brackets,)

    def create_signode(self, ctx, name, signode, brackets):
        if brackets:
            signode += addnodes.desc_addname('<', '<')
        signode += addnodes.desc_name(name, name)
        if brackets:
            signode += addnodes.desc_addname('>', '>')


# setup =======================================================================

def setup(app: Sphinx) -> dict[str, Any]:
    app.add_domain(ConfigDomain)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }

class TestConfigDomain(unittest.TestCase):

    def test_parse_section_name(self):
        cases = {
            # sec_name, sec_disc
            'sn': ('sn', None),
            'sn@sd': ('sn', 'sd'),
        }

        for ref, expected in cases.items():
            self.assertEqual(parse_section_name(ref), expected)

    def test_parse_key_name(self):
        cases = {
            # sec, sec_name, sec_disc, key_name
            'kn': (None, None, None, 'kn'),
            'kn.a.b.c': (None, None, None, 'kn.a.b.c'),
            '[sn]kn': ('sn', 'sn', None, 'kn'),
            '[sn]kn.a.b.c': ('sn', 'sn', None, 'kn.a.b.c'),
            '[sn@sd]kn': ('sn@sd', 'sn', 'sd', 'kn'),
            '[sn@sd]kn.a.b.c': ('sn@sd', 'sn', 'sd', 'kn.a.b.c'),
        }

        for ref, expected in cases.items():
            self.assertEqual(parse_key_name(ref), expected, f'On key {repr(ref)}')

    def test_parse_key_name(self):
        cases = {
            # sec, sec_name, sec_disc, key, key_name, val_name
            'vn': (None, None, None, None, None, 'vn'),
            'kn:vn': (None, None, None, 'kn', 'kn', 'vn'),
            'kn.a.b.c:vn': (None, None, None, 'kn.a.b.c', 'kn.a.b.c', 'vn'),
            '[sn]kn:vn': ('sn', 'sn', None, '[sn]kn', 'kn', 'vn'),
            '[sn]kn.a.b.c:vn': ('sn', 'sn', None, '[sn]kn.a.b.c', 'kn.a.b.c', 'vn'),
            '[sn@sd]kn:vn': ('sn@sd', 'sn', 'sd', '[sn@sd]kn', 'kn', 'vn'),
            '[sn@sd]kn.a.b.c:vn': ('sn@sd', 'sn', 'sd', '[sn@sd]kn.a.b.c', 'kn.a.b.c', 'vn'),
        }

        for ref, expected in cases.items():
            self.assertEqual(parse_value_name(ref), expected, f'On value {repr(ref)}')

    def test_key_canonicalize(self):
        cases = {
            ('~!abc.123__CamelCase/CAMELCase#$%',): 'ABC_123_CAMEL_CASE_CAMEL_CASE',
            ('CamelCase',): 'CAMEL_CASE',
            ('TheSection', None, 'TheKey'): 'THE_SECTION_THE_KEY',
            ('TheSection', '', 'TheKey'): 'THE_SECTION_THE_KEY',
            ('TheSection', 'TheInstance', 'TheKey'): 'THE_SECTION_THE_INSTANCE_THE_KEY',
            ('##CamelCase##',): 'CAMEL_CASE',
            ('UseXTypes',): 'USE_X_TYPES',
            ('UseXYZTypes',): 'USE_XYZ_TYPES',
        }

        for args, expected in cases.items():
            self.assertEqual(key_canonicalize(*args), expected)
