# Framework for domains based on the builtin RST Domain in Sphinx.

from typing import Any, Iterator

from docutils.nodes import Element
from docutils.parsers.rst import directives

from sphinx.locale import _, __
from sphinx import addnodes
from sphinx.addnodes import desc_signature, pending_xref
from sphinx.directives import ObjectDescription
from sphinx.util.nodes import make_id, make_refnode, process_index_entry
from sphinx.util.typing import OptionSpec
from sphinx.domains import Domain
from sphinx.environment import BuildEnvironment
from sphinx.builders import Builder


class CustomDomainObject(ObjectDescription[str]):
    option_spec: OptionSpec = {
        'no-index-entry': directives.flag,
        'no-index': directives.flag, # This is implemented by Sphinx
        'no-contents-entry': directives.flag, # This is implemented by Sphinx
    }

    @classmethod
    def domain_name(cls):
        raise NotImplementedError()

    @classmethod
    def use_name(cls):
        raise NotImplementedError()

    @classmethod
    def index_discriminator(cls):
        raise NotImplementedError()

    @classmethod
    def parent_class(cls):
        '''Returns if the parent is required and the class
        '''
        return False, None

    def link_name(cls, name, parents):
        '''Returns the default text used when linking
        '''
        return name

    def handle_signature(self, sig: str, signode: desc_signature) -> str:
        '''Returns the header node for the defintion. It can be modifed to
        serperate things like arguments.
        '''
        name = sig.strip()
        signode['fullname'] = name
        signode += addnodes.desc_name(name, name)
        return name

    @classmethod
    def use_full_name(cls):
        return cls.domain_name() + ':' + cls.use_name()

    def get_context(self, key):
        return self.env.ref_context.setdefault(key, [])

    def get_parents(self):
        required, parent_class = self.parent_class()
        if parent_class is None:
            parents = []
        else:
            parents = self.get_context(parent_class.parents_key())
        if required and parent_class and not parents:
            domain = self.env.get_domain(self.domain_name())
            e = ValueError('Domain object is required to be in a ' + parent_class.use_full_name())
            domain.logger.warning(e, location=self.get_location())
            raise e
        return parents

    def get_index_text(self, objectname: str, name: str) -> str:
        return _('{} ({})').format(name, self.index_discriminator())

    def add_target_and_index(self, name: str, sig: str, signode: desc_signature) -> None:
        domain = self.env.get_domain(self.domain_name())

        # Create the reference target
        parents = self.get_parents()
        prefix_list = [self.objtype]
        prefix_list.extend([x[0] for x in parents])
        prefix = '-'.join(prefix_list)
        node_id = make_id(self.env, self.state.document, prefix, name)

        # Register reference target
        signode['ids'].append(node_id)
        self.state.document.note_explicit_target(signode)
        domain.note_object(self.objtype, self.link_name(name, parents), node_id, location=signode)
        if 'no-index-entry' not in self.options:
            indextext = self.get_index_text(self.objtype, name)
            if indextext:
                if parents:
                    parent_name = parents[-1][0]
                    parent = parents[-1][1]
                    parent_indextext = parent.get_index_text(parent_name, parent_name)
                    entry = process_index_entry(f'pair: {indextext}; {parent_indextext}', node_id)
                else:
                    entry = process_index_entry(indextext, node_id)
                self.indexnode['entries'].extend(entry)

    def _object_hierarchy_parts(self, sig_node: desc_signature) -> tuple[str, ...]:
        if 'fullname' not in sig_node:
            return ()
        parts = [x[0] for x in self.get_parents()]
        parts.append(sig_node['fullname'])
        return tuple(parts)

    def _toc_entry_name(self, sig_node: desc_signature) -> str:
        if not sig_node.get('_toc_parts'):
            return ''

        config = self.env.app.config
        objtype = sig_node.parent.get('objtype')
        *parents, name = sig_node['_toc_parts']
        return name


class CustomDomainParentObject(CustomDomainObject):
    @classmethod
    def parent_category(cls):
        raise NotImplementedError()

    @classmethod
    def parents_key(cls):
        return cls.domain_name() + ':' + cls.parent_category()

    def get_parents(self):
        return self.get_context(self.parents_key())

    def handle_signature(self, sig: str, signode: desc_signature) -> str:
        name = super().handle_signature(sig, signode)
        self._append_parents_in_before_content = (name, self)
        return name

    def before_content(self) -> None:
        self.get_parents().append(self._append_parents_in_before_content)

    def after_content(self) -> None:
        self.get_parents().pop()


class CustomDomain(Domain):
    name = None
    label = None
    logger = None

    object_types = {
    }
    directives = {
    }
    roles = {
    }
    initial_data: dict[str, dict[tuple[str, str], str]] = {
        'objects': {},  # fullname -> docname, objtype
    }

    @property
    def objects(self) -> dict[tuple[str, str], tuple[str, str]]:
        return self.data.setdefault('objects', {})  # (objtype, fullname) -> (docname, node_id)

    def note_object(self, objtype: str, name: str, node_id: str, location: Any = None) -> None:
        self.logger.debug('{} domain add {} object named {}'.format(self.name, objtype, repr(name)))
        if (objtype, name) in self.objects:
            docname, node_id = self.objects[objtype, name]
            self.logger.warning(__('duplicate description of %s %s, other instance in %s') %
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
                results.append((self.name + ':' + self.role_for_objtype(objtype),
                                make_refnode(builder, fromdocname, todocname, node_id,
                                             contnode, target + ' ' + objtype)))
        return results

    def get_objects(self) -> Iterator[tuple[str, str, str, str, str, int]]:
        for (typ, name), (docname, node_id) in self.data['objects'].items():
            yield name, name, typ, docname, node_id, 1
