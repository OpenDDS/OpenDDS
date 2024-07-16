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
from sphinx.domains import Domain, ObjType
from sphinx.environment import BuildEnvironment
from sphinx.builders import Builder
from sphinx.roles import XRefRole


class ContextWrapper:
    def __init__(self, env, domain):
        self.ctx = env.ref_context.setdefault(f'{domain}-context', dict(
            stack=[],
            needs_push=True,
            misc={},
        ))

    def needs_push(self):
        return self.ctx['needs_push']

    def reset_needs_push(self):
        self.ctx['needs_push'] = True

    @property
    def stack(self):
        return self.ctx['stack']

    def push(self, obj, name, options, full_name=None, **kw):
        data = dict(
            obj=obj,
            name=name,
            options=options,
            full_name=name if full_name is None else full_name,
            index_text=None,
            **kw
        )
        self.stack.append(data)
        self.ctx['needs_push'] = False
        data['index_text'] = obj.get_index_text(name, full_name)
        return data

    def pop(self):
        self.stack.pop()
        self.reset_needs_push()

    def get_all_names(self):
        return [scope['name'] for scope in self.stack]

    def get(self, index, key=None):
        try:
            frame = self.stack[index]
        except IndexError:
            return None
        return frame if key is None else frame[key]

    def get_parent(self, index=-2):
        return self.get(index)

    def get_name(self, index=-1):
        return self.get(index, 'name')

    def get_full_name(self, index=-1):
        return self.get(index, 'full_name')

    def get_index_text(self, index=-1):
        return self.get(index, 'index_text')

    @property
    def misc(self):
        return self.ctx['misc']


class CustomDomainObject(ObjectDescription[str]):
    option_spec: OptionSpec = {
        'no-index-entry': directives.flag,
        'no-index': directives.flag, # This is implemented by Sphinx
        'no-contents-entry': directives.flag, # This is implemented by Sphinx
    }

    our_name = None
    our_index_discriminator = None
    our_parent_required = False
    our_parent_type = None
    our_ref_role_type = XRefRole
    our_role_name = None
    _full_name = None
    _domain_name = None

    def get_index_text(self, name, full_name):
        t = name
        if self.our_index_discriminator is None:
            t += f' ({self.index_discriminator})'
        return t

    def get_context(self):
        return ContextWrapper(self.env, self._domain_name)

    def check_parentage(self):
        domain = self.env.get_domain(self._domain_name)
        parent = self.get_context().get_parent()
        if self.our_parent_required and (parent is None or
                not isinstance(parent['obj'], self.our_parent_type)):
            e = ValueError(f'{self._full_name} must be in a {self.our_parent_type._full_name}')
            domain.logger.warning(e, location=self.get_location())
            raise e

    def parse_sig(self, ctx, sig, options):
        ctx.push(self, sig, options)
        return ()

    def create_signode(self, ctx, name, signode):
        signode += addnodes.desc_name(name, name)

    def handle_signature(self, sig: str, signode: desc_signature) -> str:
        domain = self.env.get_domain(self._domain_name)
        ctx = self.get_context()

        try:
            extra = self.parse_sig(ctx, sig.strip(), self.options)
        except Exception as e:
            domain.logger.exception('Exception in parse_sig:', location=self.get_location())
            raise e

        self.check_parentage()
        signode['fullname'] = ctx.get_full_name()
        name = ctx.get_name()
        try:
            self.create_signode(ctx, name, signode, *extra)
        except Exception as e:
            domain.logger.exception('Exception in create_signode:', location=self.get_location())
            raise e

        return name

    def _object_hierarchy_parts(self, signode: desc_signature) -> tuple[str, ...]:
        return tuple(self.get_context().get_all_names())

    def _toc_entry_name(self, signode: desc_signature) -> str:
        *parents, name = signode['_toc_parts']
        return name

    def add_target_and_index(self, name: str, sig: str, signode: desc_signature) -> None:
        domain = self.env.get_domain(self._domain_name)
        ctx = self.get_context()

        # Create the reference target
        prefix_list = [self._domain_name, self.objtype]
        prefix_list.extend(ctx.get_all_names())
        prefix_list.pop()
        prefix = '-'.join(prefix_list)
        node_id = make_id(self.env, self.state.document, prefix, name)

        # Register reference target
        signode['ids'].append(node_id)
        self.state.document.note_explicit_target(signode)
        full_name = ctx.get_full_name()
        domain.note_object(self.objtype, full_name, node_id, location=signode)
        if 'no-index-entry' not in self.options:
            indextext = ctx.get_index_text()
            if indextext:
                parent = ctx.get_parent()
                if parent:
                    parent_indextext = parent['index_text']
                    entry = process_index_entry(f'pair: {indextext}; {parent_indextext}', node_id)
                else:
                    entry = process_index_entry(indextext, node_id)
                self.indexnode['entries'].extend(entry)

    def before_content(self) -> None:
        self.get_context().reset_needs_push()

    def our_after_content(self, ctx):
        pass

    def after_content(self) -> None:
        ctx = self.get_context()
        self.our_after_content(ctx)
        ctx.pop()


class CustomDomain(Domain):
    name = None
    label = None
    logger = None

    def __init_subclass__(cls):
        cls.object_types = {
        }
        cls.directives = {
        }
        cls.roles = {
        }
        cls.initial_data = {
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

    @classmethod
    def add_type(cls, Type):
        if Type.our_name is None:
            raise ValueError(f'{Type.__name__} is missing our_name')
        if Type.our_parent_required and Type.our_parent_type is None:
            raise ValueError(
                f'{Type.__name__} has our_parent_required = True, but our_parent_type = None')

        Type._full_name = f'{cls.name}:{Type.our_name}'
        Type._domain_name = cls.name
        cls.directives[Type.our_name] = Type
        if Type.our_ref_role_type is not None:
            cls.roles[Type.our_name] = Type.our_ref_role_type()
        if Type.our_role_name is None:
            role_name = Type.our_name
        else:
            role_name = Type.our_role_name
        obj_type = ObjType(Type.our_name.replace(':', '-'), role_name)
        cls.object_types[Type.our_name] = obj_type

        return Type
