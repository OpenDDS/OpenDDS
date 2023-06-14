# Sphinx Domain for CMake
# Based on the builtin RST Domain in Sphinx
# Pass -vv to sphinx-build to get it to log the directives it's getting.

from __future__ import annotations

import sys
import re
from typing import Any, Iterator, cast

from docutils.nodes import Element
from docutils.parsers.rst import directives

from sphinx import addnodes
from sphinx.addnodes import desc_signature, pending_xref
from sphinx.application import Sphinx
from sphinx.builders import Builder
from sphinx.directives import ObjectDescription
from sphinx.domains import Domain, ObjType
from sphinx.environment import BuildEnvironment
from sphinx.locale import _, __
from sphinx.roles import XRefRole
from sphinx.util import logging
from sphinx.util.nodes import make_id, make_refnode
from sphinx.util.typing import OptionSpec

logger = logging.getLogger(__name__)


class CMakeMarkup(ObjectDescription[str]):
    option_spec: OptionSpec = {
        'noindex': directives.flag,
        'noindexentry': directives.flag,
        'nocontentsentry': directives.flag,
    }

    def handle_signature(self, sig: str, signode: desc_signature) -> str:
        name = sig.strip()
        signode['fullname'] = name
        signode += addnodes.desc_name(name, name)
        return name

    def add_target_and_index(self, name: str, sig: str, signode: desc_signature) -> None:
        node_id = make_id(self.env, self.state.document, self.objtype, name)
        signode['ids'].append(node_id)
        self.state.document.note_explicit_target(signode)

        domain = cast(CMakeDomain, self.env.get_domain('cmake'))
        domain.note_object(self.objtype, name, node_id, location=signode)

        if 'noindexentry' not in self.options:
            indextext = self.get_index_text(self.objtype, name)
            if indextext:
                self.indexnode['entries'].append(('single', indextext, node_id, '', None))

    def get_index_text(self, objectname: str, name: str) -> str:
        return ''

    def _object_hierarchy_parts(self, sig_node: desc_signature) -> tuple[str, ...]:
        if 'fullname' not in sig_node:
            return ()
        parts = list(self.env.ref_context.get('cmake:functions', ()))
        parts.append(sig_node['fullname'])
        return tuple(parts)

    def _toc_entry_name(self, sig_node: desc_signature) -> str:
        if not sig_node.get('_toc_parts'):
            return ''

        config = self.env.app.config
        objtype = sig_node.parent.get('objtype')
        *parents, name = sig_node['_toc_parts']
        return name


class CMakeFunction(CMakeMarkup):
    def get_index_text(self, objectname: str, name: str) -> str:
        return _('%s (CMake function)') % name

    def before_content(self) -> None:
        if self.names:
            functions = self.env.ref_context.setdefault('cmake:functions', [])
            functions.append(self.names[0])

    def after_content(self) -> None:
        functions = self.env.ref_context.setdefault('cmake:functions', [])
        if functions:
            functions.pop()


class CMakeFunctionArgument(CMakeMarkup):
    option_spec: OptionSpec = CMakeMarkup.option_spec.copy()
    option_spec.update({
        'type': directives.unchanged,
    })

    def handle_signature(self, sig: str, signode: desc_signature) -> str:
        try:
            name, arguments = re.split(r'\s+', sig.strip())
        except ValueError:
            name, arguments = sig, None

        signode['fullname'] = name.strip()
        signode += addnodes.desc_name(name, name)
        if arguments:
            signode += addnodes.desc_annotation(' ' + arguments, ' ' + arguments)
        if self.options.get('type'):
            text = ' (%s)' % self.options['type']
            signode += addnodes.desc_annotation(text, text)
        return name

    def add_target_and_index(self, name: str, sig: str, signode: desc_signature) -> None:
        domain = cast(CMakeDomain, self.env.get_domain('cmake'))

        function_name = self.current_function
        if function_name:
            prefix = '-'.join([self.objtype, function_name])
            objname = f'{function_name}({name})'
        else:
            prefix = self.objtype
            objname = name

        node_id = make_id(self.env, self.state.document, prefix, name)
        signode['ids'].append(node_id)
        self.state.document.note_explicit_target(signode)
        domain.note_object(self.objtype, objname, node_id, location=signode)

        if function_name:
            key = name[0].upper()
            pair = [_('%s (CMake function)') % function_name, _('%s (CMake function argument)') % name]
            self.indexnode['entries'].append(('pair', '; '.join(pair), node_id, '', key))
        else:
            key = name[0].upper()
            text = _('%s (CMake function argument)') % name
            self.indexnode['entries'].append(('single', text, node_id, '', key))

        if 'noindexentry' not in self.options:
            indextext = self.get_index_text(self.objtype, name)
            if indextext:
                self.indexnode['entries'].append(('single', indextext, node_id, '', None))

    @property
    def current_function(self) -> str:
        functions = self.env.ref_context.get('cmake:functions')
        if functions:
            return functions[-1]
        else:
            return ''


class CMakeVariable(CMakeMarkup):
    def get_index_text(self, objectname: str, name: str) -> str:
        return _('%s (CMake variable)') % name


class CMakeProperty(CMakeMarkup):
    def get_index_text(self, objectname: str, name: str) -> str:
        return _('%s (CMake property)') % name


class CMakeTarget(CMakeMarkup):
    def get_index_text(self, objectname: str, name: str) -> str:
        return _('%s (CMake target)') % name


class CMakeDomain(Domain):
    name = 'cmake'
    label = 'CMake'

    object_types = {
        'func': ObjType(_('func'), 'func'),
        'func:arg': ObjType(_('func-arg'), 'func'),
        'var': ObjType(_('var'), 'var'),
        'prop': ObjType(_('prop'), 'prop'),
        'tgt': ObjType(_('tgt'), 'tgt'),
    }
    directives = {
        'func': CMakeFunction,
        'func:arg': CMakeFunctionArgument,
        'var': CMakeVariable,
        'prop': CMakeProperty,
        'tgt': CMakeTarget,
    }
    roles = {
        'func': XRefRole(),
        'var': XRefRole(),
        'prop': XRefRole(),
        'tgt': XRefRole(),
    }
    initial_data: dict[str, dict[tuple[str, str], str]] = {
        'objects': {},  # fullname -> docname, objtype
    }

    @property
    def objects(self) -> dict[tuple[str, str], tuple[str, str]]:
        return self.data.setdefault('objects', {})  # (objtype, fullname) -> (docname, node_id)

    def note_object(self, objtype: str, name: str, node_id: str, location: Any = None) -> None:
        logger.debug('CMake domain add {} object named {}'.format(objtype, repr(name)))
        if (objtype, name) in self.objects:
            docname, node_id = self.objects[objtype, name]
            logger.warning(__('duplicate description of %s %s, other instance in %s') %
                           (objtype, name, docname), location=location)

        self.objects[objtype, name] = (self.env.docname, node_id)

    def clear_doc(self, docname: str) -> None:
        for (typ, name), (doc, _node_id) in list(self.objects.items()):
            if doc == docname:
                del self.objects[typ, name]

    def merge_domaindata(self, docnames: list[str], otherdata: dict[str, Any]) -> None:
        # XXX check duplicates
        for (typ, name), (doc, node_id) in otherdata['objects'].items():
            if doc in docnames:
                self.objects[typ, name] = (doc, node_id)

    def resolve_xref(self, env: BuildEnvironment, fromdocname: str, builder: Builder,
                     typ: str, target: str, node: pending_xref, contnode: Element,
                     ) -> Element | None:
        objtypes = self.objtypes_for_role(typ)
        for objtype in objtypes:
            result = self.objects.get((objtype, target))
            if result:
                todocname, node_id = result
                return make_refnode(builder, fromdocname, todocname, node_id,
                                    contnode, target + ' ' + objtype)
        return None

    def resolve_any_xref(self, env: BuildEnvironment, fromdocname: str, builder: Builder,
                         target: str, node: pending_xref, contnode: Element,
                         ) -> list[tuple[str, Element]]:
        results: list[tuple[str, Element]] = []
        for objtype in self.object_types:
            result = self.objects.get((objtype, target))
            if result:
                todocname, node_id = result
                results.append(('cmake:' + self.role_for_objtype(objtype),
                                make_refnode(builder, fromdocname, todocname, node_id,
                                             contnode, target + ' ' + objtype)))
        return results

    def get_objects(self) -> Iterator[tuple[str, str, str, str, str, int]]:
        for (typ, name), (docname, node_id) in self.data['objects'].items():
            yield name, name, typ, docname, node_id, 1


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_domain(CMakeDomain)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
