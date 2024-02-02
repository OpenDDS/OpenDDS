# Sphinx Domain for CMake
# Pass -vv to sphinx-build to get it to log the directives it's getting.

from __future__ import annotations

import sys
import re
from typing import Any

from sphinx import addnodes
from sphinx.addnodes import desc_signature
from sphinx.application import Sphinx
from sphinx.domains import ObjType
from sphinx.locale import _, __
from sphinx.roles import XRefRole
from sphinx.util import logging

from custom_domain import CustomDomain, CustomDomainObject, CustomDomainParentObject


class CMakeDomainObject(CustomDomainObject):
    @classmethod
    def domain_name(cls):
        return 'cmake'


class CMakeFunction(CMakeDomainObject, CustomDomainParentObject):
    @classmethod
    def use_name(cls):
        return 'func'

    @classmethod
    def index_discriminator(cls):
        return 'CMake function'

    @classmethod
    def parent_category(cls):
        return 'function'


class CMakeFunctionArgument(CMakeDomainObject):
    @classmethod
    def use_name(cls):
        return 'func:arg'

    @classmethod
    def index_discriminator(cls):
        return 'CMake function argument'

    @classmethod
    def parent_class(cls):
        return True, CMakeFunction

    def link_name(cls, name, parents):
        return f'{parents[-1][0]}({name})'

    def handle_signature(self, sig: str, signode: desc_signature) -> str:
        try:
            name, arguments = re.split(r'\s+', sig.strip())
        except ValueError:
            name, arguments = sig, None

        signode['fullname'] = name.strip()
        signode += addnodes.desc_name(name, name)
        if arguments:
            signode += addnodes.desc_annotation(' ' + arguments, ' ' + arguments)
        return name


class CMakeVariable(CMakeDomainObject):
    @classmethod
    def use_name(cls):
        return 'var'

    @classmethod
    def index_discriminator(cls):
        return 'CMake variable'


class CMakeProperty(CMakeDomainObject):
    @classmethod
    def use_name(cls):
        return 'prop'

    @classmethod
    def index_discriminator(cls):
        return 'CMake property'


class CMakeTarget(CMakeDomainObject):
    @classmethod
    def use_name(cls):
        return 'tgt'

    @classmethod
    def index_discriminator(cls):
        return 'CMake target'


class CMakeDomain(CustomDomain):
    name = 'cmake'
    label = 'CMake'
    logger = logging.getLogger(__name__)

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


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_domain(CMakeDomain)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
