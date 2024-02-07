# Sphinx Domain for Configuration Values
# Pass -vv to sphinx-build to get it to log the directives it's getting.

from __future__ import annotations

import sys
import re
from typing import Any
import re

from docutils import nodes

from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.util import logging
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
        return [groups[name] for name in ret]
    return m.groups()


# cfg:sec =====================================================================

section_re = r'(?P<sec_name>\w+)(?::(?P<sec_disc>\w+))?'


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
        return title, target


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
        name, arguments = parse(self._full_name, r'(\w+(?::\w+)?)(?:/(.*))?', sig, self)
        sec_name, sec_disc = parse_section_name(name)
        ctx.push(self, name, sec_name=sec_name, sec_disc=sec_name, arguments=arguments)
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

key_re = r'(?:(?P<sec>' + section_re + ')\.)?(?P<key_name>\w+)'


def parse_key_name(full_name, node=None):
    return parse(ConfigKey._full_name, key_re, full_name, node,
        'sec', 'sec_name', 'sec_disc', 'key_name')


def key_text(sec_name, sec_disc, key_name, insert=''):
    text = key_name + insert
    if sec_name is not None:
        text = section_text(sec_name, sec_disc, '.' + text)
    return text


class ConfigKeyRefRole(XRefRole):
    def process_link(self, env: BuildEnvironment, refnode: Element,
                     has_explicit_title: bool, title: str, target: str) -> tuple[str, str]:
        ctx = ContextWrapper(env, 'cfg')
        target = target.strip()

        # Normalize target for the current scope
        sec, sec_name, sec_disc, key_name = parse_key_name(target, self)
        scope_kind = len(ctx.stack)
        if sec is not None:
            # sec.key anywhere
            pass
        elif scope_kind >= 1:
            # key anywhere in a section
            prefix = ctx.get_full_name(0)
            target = f'{prefix}.{target}'
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
    our_name = 'key'
    our_parent_required = True
    our_parent_type = ConfigSection
    our_ref_role_type = ConfigKeyRefRole

    def get_index_text(self, name, full_name):
        sec, sec_name, sec_disc, key = parse_key_name(full_name)
        return f'{key} (config key)'

    def parse_sig(self, ctx, sig):
        name, arguments = parse(self._full_name, r'(\w+)(?:=(.*))?', sig, self)
        ctx.push(self, name, ctx.get_full_name() + f'.{name}', arguments=arguments)
        return (arguments,)

    def create_signode(self, ctx, name, signode, arguments):
        signode += addnodes.desc_name(name, name)
        if arguments is not None:
            text = '=' + arguments
            signode += addnodes.desc_addname(text, text)

    def transform_content(self, contentnode: addnodes.desc_content) -> None:
        ctx = self.get_context()

        # Insert the config store key at the top of the content
        p = nodes.paragraph()
        contentnode.insert(0, p)
        text = 'Config store key: '
        p += nodes.inline(text, text)
        text = ctx.get(-2, 'sec_name').upper() + '_'
        sec_args = ctx.get(-2, 'arguments')
        if sec_args:
            text += sec_args + '_'
        text += ctx.get_name().upper()
        p += nodes.literal(text, text)


# cfg:val =====================================================================

value_re = r'(?:(?P<key>' + key_re + ')\.)?(?P<val_name>\w+)'


def parse_value_name(full_name, node=None):
    return parse(ConfigValue._full_name, value_re, full_name, node,
        'sec', 'sec_name', 'sec_disc', 'key', 'key_name', 'val_name')


def value_text(sec_name, sec_disc, key_name, val_name):
    text = val_name
    if key_name is not None:
        text = key_text(sec_name, sec_disc, key_name, '.' + text)
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
            # sec.key.val anywhere
            pass
        elif scope_kind >= 1 and key is not None:
            # key.val anywhere in a section
            prefix = ctx.get_full_name(0)
            target = f'{prefix}.{target}'
        elif scope_kind >= 2 and key is None:
            # val anywhere in a key
            prefix = ctx.get_full_name(1)
            target = f'{prefix}.{target}'
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
        name_wo_brackets, name_w_brackets = parse(self._full_name, r'(\w+)|<(\w+)>', sig, self)
        brackets = bool(name_w_brackets)
        name = name_w_brackets if brackets else name_wo_brackets
        ctx.push(self, name, ctx.get_full_name() + f'.{name}')
        return (brackets,)

    def create_signode(self, ctx, name, signode, brackets):
        if brackets:
            signode += addnodes.desc_addname('<', '<')
        signode += addnodes.desc_name(name, name)
        if brackets:
            signode += addnodes.desc_addname('>', '>')


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_domain(ConfigDomain)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
