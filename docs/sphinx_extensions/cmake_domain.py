# Sphinx Domain for CMake
# Pass -vv to sphinx-build to get it to log the directives it's getting.

from __future__ import annotations

import sys
import re
from typing import Any

from sphinx import addnodes
from sphinx.addnodes import desc_signature
from sphinx.application import Sphinx
from sphinx.util import logging

from custom_domain import CustomDomain, CustomDomainObject


class CMakeDomain(CustomDomain):
    name = 'cmake'
    label = 'CMake'
    logger = logging.getLogger(__name__)


@CMakeDomain.add_type
class CMakeFunction(CustomDomainObject):
    our_name = 'func'
    our_index_discriminator = 'CMake function'


@CMakeDomain.add_type
class CMakeFunctionArgument(CustomDomainObject):
    our_name = 'func:arg'
    our_index_discriminator = 'CMake function argument'
    our_parent_required = True
    our_parent_type = CMakeFunction
    # Use the :func: role for this
    our_role_name = 'func'
    our_ref_role_type = None

    def parse_sig(self, ctx, sig, options):
        try:
            name, arguments = re.split(r'\s+', sig)
        except ValueError:
            name, arguments = sig, None
        ctx.push(self, name, options, ctx.get_full_name() + f'({name})')
        return arguments,

    def create_signode(self, ctx, name, signode, arguments):
        signode += addnodes.desc_name(name, name)
        if arguments:
            signode += addnodes.desc_annotation(' ' + arguments, ' ' + arguments)


@CMakeDomain.add_type
class CMakeVariable(CustomDomainObject):
    our_name = 'var'
    our_index_discriminator = 'CMake variable'


@CMakeDomain.add_type
class CMakeProperty(CustomDomainObject):
    our_name = 'prop'
    our_index_discriminator = 'CMake property'


@CMakeDomain.add_type
class CMakeTarget(CustomDomainObject):
    our_name = 'tgt'
    index_discriminator = 'CMake target'


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_domain(CMakeDomain)

    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
