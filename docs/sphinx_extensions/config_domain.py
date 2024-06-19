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
from sphinx.util.docutils import SphinxDirective
from sphinx.roles import XRefRole

from custom_domain import CustomDomain, CustomDomainObject, ContextWrapper


class ConfigDomain(CustomDomain):
    name = 'cfg'
    label = 'config'
    logger = logging.getLogger(__name__)

    @classmethod
    def ctx(cls, env):
        return ContextWrapper(env, cls.name)


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

    def parse_sig(self, ctx, sig, options):
        name, arguments = parse(self._full_name,
            r'(' + id_re + r'(?:@' + id_re + r')?)(?:/(.*))?', sig, self)
        sec_name, sec_disc = parse_section_name(name)
        ctx.push(self, name, options, f'[{name}]',
            sec_name=sec_name, sec_disc=sec_name, arguments=arguments, props=[])
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


class DefaultCfgSecDirective(SphinxDirective):
    has_content = True
    name = 'default-cfg-sec'

    def run(self):
        ctx = ConfigDomain.ctx(self.env)

        count = len(self.content)
        if count == 0:
            del ctx.misc[self.name]
        elif count == 1:
            sec = self.content[0].strip()
            parse_section_name(sec, self)
            ctx.misc[self.name] = sec
        else:
            e = ValueError(f'Can not pass more than one line to default-sec')
            ConfigDomain.logger.warning(e, location=self.get_location())
            raise e
        return []

    @classmethod
    def get_default_section(cls, ctx):
        return ctx.misc.get(cls.name, 'common')


# cfg:prop ====================================================================

prop_name_re = id_re + r'(?:\.' + id_re + r')*'
prop_re = r'(?:\[(?P<sec>' + section_re + r')\])?(?P<prop_name>' + prop_name_re + r')'


def parse_prop_name(full_name, node=None):
    return parse(ConfigProp._full_name, prop_re, full_name, node,
        'sec', 'sec_name', 'sec_disc', 'prop_name')


def prop_text(sec_name, sec_disc, prop_name, insert=''):
    text = prop_name + insert
    if sec_name is not None:
        text = section_text(sec_name, sec_disc, ' ' + text)
    return text


# This should be equivalent to ConfigPair::canonicalize
def name_canonicalize(name):
    # Replace everything that's not a letter, number, or underscore
    name = re.sub(r'[^\w]', '_', name)

    # Convert CamelCase to camel_case
    name = re.sub(r'([A-Z][a-z])', r'_\1', name)
    name = re.sub(r'([a-z])([A-Z])', r'\1_\2', name)

    # Removed repeated underscores
    name = re.sub(r'_+', r'_', name)

    return name.strip('_').upper()


def key_canonicalize(sec_name, sec_args, prop_name):
    sec = name_canonicalize(sec_name)
    if sec_args:
        sec += '_' + sec_args
    return sec + '_' + name_canonicalize(prop_name)


def rescope(ctx, sec, sec_name, prop_name=None):
    scope = ctx.get_all_names()
    if scope:
        # We're somewhere inside a sec
        scope = scope + [None] * (2 - len(scope))
        if scope[0] == sec:
            sec_name = None
            if scope[1] == prop_name:
                prop_name = None
    elif sec == DefaultCfgSecDirective.get_default_section(ctx):
        sec_name = None

    return sec_name, prop_name


class ConfigPropRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> tuple[str, str]:
        ctx = ConfigDomain.ctx(env)
        target = target.strip()

        # Normalize target for the current scope
        sec, sec_name, sec_disc, prop_name = parse_prop_name(target, self)
        scope_kind = len(ctx.stack)
        if sec is not None:
            # [sec]prop anywhere
            pass
        elif scope_kind == 0 and sec is None:
            # prop outside a section
            default_section = DefaultCfgSecDirective.get_default_section(ctx)
            target = f'[{default_section}]{target}'
        elif scope_kind >= 1:
            # prop anywhere in a section
            prefix = ctx.get_full_name(0)
            target = f'{prefix}{target}'
        else:
            ConfigDomain.logger.warning(f'{repr(target)} is an invalid target here',
                location=self.get_location())

        if not has_explicit_title:
            # Reparse target and hide parts of title that are in the current scope
            sec, sec_name, sec_disc, prop_name = parse_prop_name(target, self)
            sec_name, _ = rescope(ctx, sec, sec_name)
            title = prop_text(sec_name, sec_disc, prop_name)

        return title, target


@ConfigDomain.add_type
class ConfigProp(CustomDomainObject):
    option_spec: OptionSpec = CustomDomainObject.option_spec.copy()
    option_spec.update({
        'required': directives.flag,
        'default': directives.unchanged,
    })

    our_name = 'prop'
    our_parent_required = True
    our_parent_type = ConfigSection
    our_ref_role_type = ConfigPropRefRole

    def get_index_text(self, name, full_name):
        sec, sec_name, sec_disc, prop = parse_prop_name(full_name)
        return f'{prop} (config property)'

    def parse_sig(self, ctx, sig, options):
        name, arguments = parse(self._full_name, r'(' + prop_name_re + r')(?:=(.*))?', sig, self)
        sec = ctx.get_full_name()
        ctx.push(self, name, options, f'{sec}{name}', arguments=arguments)
        sec_ctx = ctx.get(-2, 'props')
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
        try:
            key = key_canonicalize(
                ctx.get(-2, 'sec_name'), ctx.get(-2, 'arguments'), ctx.get_name())
            rst.append(f'| :ref:`Config store key <config-store-keys>`: ``{key}``',
                f'{__name__}', 1)
        except Exception as e:
            e = ValueError(f'Something went wrong with key_canonicalize: {e}')
            ConfigDomain.logger.warning(e, location=self.get_location())

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

value_sep = '='
value_sep_re = re.escape(value_sep)
value_re = r'(?:(?P<prop>' + prop_re + r')' + value_sep_re + r')?(?P<val_name>' + id_re + r')'


def parse_value_name(full_name, node=None):
    return parse(ConfigValue._full_name, value_re, full_name, node,
        'sec', 'sec_name', 'sec_disc', 'prop', 'prop_name', 'val_name')


def value_text(sec_name, sec_disc, prop_name, val_name):
    text = val_name
    if prop_name is not None:
        text = prop_text(sec_name, sec_disc, prop_name, value_sep + text)
    return text


class ConfigValueRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> tuple[str, str]:
        ctx = ConfigDomain.ctx(env)
        target = target.strip()

        # Normalize target for the current scope
        sec, sec_name, sec_disc, prop, prop_name, val_name = parse_value_name(target, self)
        scope_kind = len(ctx.stack)
        if sec is not None:
            # [sec]prop=val anywhere
            pass
        elif scope_kind == 0 and sec is None:
            # prop outside a section
            default_section = DefaultCfgSecDirective.get_default_section(ctx)
            target = f'[{default_section}]{target}'
        elif scope_kind >= 1 and prop is not None:
            # prop=val anywhere in a section
            target = ctx.get_full_name(0) + target
        elif scope_kind >= 2 and prop is None:
            # val anywhere in a prop
            target = ctx.get_full_name(1) + value_sep + target
        else:
            ConfigDomain.logger.warning(f'{repr(target)} is an invalid target here',
                location=self.get_location())

        if not has_explicit_title:
            # Reparse target and hide parts of title that are in the current scope
            sec, sec_name, sec_disc, prop, prop_name, val_name = parse_value_name(target, self)
            sec_name, prop_name = rescope(ctx, sec, sec_name, prop_name)
            title = value_text(sec_name, sec_disc, prop_name, val_name)

        return title, target


@ConfigDomain.add_type
class ConfigValue(CustomDomainObject):
    our_name = 'val'
    our_parent_required = True
    our_parent_type = ConfigProp
    our_ref_role_type = ConfigValueRefRole

    def run(self):
        # Values should probably never appear on the index or the page TOC
        self.options['no-contents-entry'] = ''
        self.options['no-index-entry'] = ''
        return super().run()

    def get_index_text(self, name, full_name):
        sec, sec_name, sec_disc, prop, prop_name, val_name = parse_value_name(full_name, self)
        return f'{val_name} (config value)'

    def parse_sig(self, ctx, sig, options):
        name_wo_brackets, name_w_brackets = parse(self._full_name,
            r'(' + id_re + r')|<(' + id_re + r')>', sig, self)
        brackets = bool(name_w_brackets)
        name = name_w_brackets if brackets else name_wo_brackets
        ctx.push(self, name, options, ctx.get_full_name() + value_sep + name)
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
    app.add_directive(DefaultCfgSecDirective.name, DefaultCfgSecDirective)

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

    def test_parse_prop_name(self):
        cases = {
            # sec, sec_name, sec_disc, prop_name
            'kn': (None, None, None, 'kn'),
            'kn.a.b.c': (None, None, None, 'kn.a.b.c'),
            '[sn]kn': ('sn', 'sn', None, 'kn'),
            '[sn]kn.a.b.c': ('sn', 'sn', None, 'kn.a.b.c'),
            '[sn@sd]kn': ('sn@sd', 'sn', 'sd', 'kn'),
            '[sn@sd]kn.a.b.c': ('sn@sd', 'sn', 'sd', 'kn.a.b.c'),
        }

        for ref, expected in cases.items():
            self.assertEqual(parse_prop_name(ref), expected, f'On prop {repr(ref)}')

    def test_parse_value_name(self):
        cases = {
            # sec, sec_name, sec_disc, prop, prop_name, val_name
            'vn': (None, None, None, None, None, 'vn'),
            'kn=vn': (None, None, None, 'kn', 'kn', 'vn'),
            'kn.a.b.c=vn': (None, None, None, 'kn.a.b.c', 'kn.a.b.c', 'vn'),
            '[sn]kn=vn': ('sn', 'sn', None, '[sn]kn', 'kn', 'vn'),
            '[sn]kn.a.b.c=vn': ('sn', 'sn', None, '[sn]kn.a.b.c', 'kn.a.b.c', 'vn'),
            '[sn@sd]kn=vn': ('sn@sd', 'sn', 'sd', '[sn@sd]kn', 'kn', 'vn'),
            '[sn@sd]kn.a.b.c=vn': ('sn@sd', 'sn', 'sd', '[sn@sd]kn.a.b.c', 'kn.a.b.c', 'vn'),
        }

        for ref, expected in cases.items():
            self.assertEqual(parse_value_name(ref), expected, f'On value {repr(ref)}')

    def test_key_canonicalize(self):
        cases = {
            ('CamelCase', None, 'key'): 'CAMEL_CASE_KEY',
            ('##CamelCase##', None, 'key'): 'CAMEL_CASE_KEY',
            ('~!abc.123_''_CamelCase/CAMELCase#$%', None, 'key'): 'ABC_123_CAMEL_CASE_CAMEL_CASE_KEY',
            ('sect', None, 'UseXTypes',): 'SECT_USE_X_TYPES',
            ('sect', None, 'UseXYZTypes',): 'SECT_USE_XYZ_TYPES',
            ('TheSection', None, 'TheKey'): 'THE_SECTION_THE_KEY',
            ('TheSection', '', 'TheKey'): 'THE_SECTION_THE_KEY',
            ('TheSection', '<instance-name>', 'TheKey'): 'THE_SECTION_<instance-name>_THE_KEY',
        }

        for args, expected in cases.items():
            self.assertEqual(key_canonicalize(*args), expected)
