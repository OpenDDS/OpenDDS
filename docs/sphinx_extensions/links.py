import subprocess
from pathlib import Path
import re
from urllib.request import urlopen
from urllib.error import URLError
import shutil

from docutils import nodes
from docutils.parsers.rst import directives
from docutils.parsers.rst.states import Struct
from docutils.utils import unescape

from sphinx.util.docutils import SphinxDirective
from sphinx.util.typing import OptionSpec
from sphinx.util import logging

import fitz # PyMuPDF

gh_url_base = 'https://github.com'
omg_url_base = 'https://issues.omg.org'
omg_spec_section_re = re.compile(r'^(\d(\.\d+)*) (.*)')
logger = logging.getLogger(__name__)
docs_path = Path(__file__).parent.parent


def get_config(inliner):
    return inliner.document.settings.env.app.config


def get_commitish(config):
    if config.github_links_commitish is None:
        if config.github_links_release_tag is not None:
            config.github_links_commitish = config.github_links_release_tag
        else:
            try:
                config.github_links_commitish = str(subprocess.check_output(
                    ['git', 'rev-parse', 'HEAD']), "utf-8").strip()
            except:
                config.github_links_commitish = 'master'

    return config.github_links_commitish


def rst_error(rawtext, text, lineno, inliner, message, *fmtargs, **fmtkwargs):
    error = inliner.reporter.error(message.format(*fmtargs, **fmtkwargs), line=lineno)
    return [inliner.problematic(text, rawtext, error)], [error]


title_target_re = re.compile(r'^(.+?)\s*<(.*?)>$')


def process_title_target(text, implied_title=None):
    m = title_target_re.match(text)
    if m:
        return True, m[1], m[2]
    return False, text if implied_title is None else implied_title, text


def parse_rst(parent, text, lineno, inliner):
    context = Struct(
        document=inliner.document,
        reporter=inliner.reporter,
        language=inliner.language)
    processed, messages = inliner.parse(unescape(text), lineno, context, parent)
    parent += processed
    return messages


def text_node(rawtext, lineno, text, options):
    return ([nodes.inline(rawtext, text, **options)], [])


def link_node(rawtext, lineno, inliner, title, parse_title, url, options):
    node = nodes.reference(rawtext, '' if parse_title else title, refuri=url, **options)
    messages = []
    if parse_title:
        messages = parse_rst(node, title, lineno, inliner)
    return ([node], messages)


def append(all_values, values):
    if all_values:
        all_values[0].extend(values[0])
        all_values[1].extend(values[1])
    else:
        all_values.append(values[0])
        all_values.append(values[1])


ghfile_arg_re = re.compile(r'^([^#]+)(#[^#]+)?$')


def ghfile_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    config = get_config(inliner)
    explicit_title, title, target = process_title_target(text)

    # Seperate path and possible URL fragment
    m = ghfile_arg_re.match(target)
    if not m:
        return rst_error(rawtext, text, lineno, inliner,
            '{} is an invalid target', repr(target))
    path = m.group(1)
    fragment = m.group(2) if m.group(2) is not None else ''

    # Check if the path exists locally
    local_path = Path(config.github_links_root_path) / path
    if not local_path.exists():
        return rst_error(rawtext, text, lineno, inliner,
            '"{}" doesn\'t exist. Checked for existence of "{}"',
                path, str(local_path)),

    # Create the main link
    kind = 'tree' if local_path.is_dir() else 'blob'
    url = '/'.join([gh_url_base, config.github_links_repo, kind, get_commitish(config), path]) + fragment
    rv = []
    main_link = link_node(rawtext, lineno, inliner,
        title if explicit_title else '', explicit_title, url, options)
    if not explicit_title:
        main_link[0][0].append(nodes.literal(rawtext, text, **options))
    append(rv, main_link)

    # Create a secondary link to preview HTML files
    if path.endswith('.html'):
        append(rv, text_node(rawtext, lineno, ' ', options))
        append(rv, link_node(rawtext, lineno, inliner,
            '(View as HTML)', False,
            'https://htmlpreview.github.io/?' + url,
            options))
    return rv


# Turns :ghissue:`213` into the equivalent of:
#   `Issue #213 <https://github.com/OpenDDS/OpenDDS/issues/213>`_
def ghissue_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    config = get_config(inliner)
    explicit_title, title, target = process_title_target(
        text, 'Issue #{}'.format(text))
    return link_node(rawtext, lineno, inliner,
        title, explicit_title,
        '{}/{}/issues/{}'.format(gh_url_base, config.github_links_repo, target),
        options)


# Turns :ghpr:`1` into the equivalent of:
#   `PR #1 <https://github.com/OpenDDS/OpenDDS/pull/1>`_
def ghpr_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    config = get_config(inliner)
    explicit_title, title, target = process_title_target(
        text, 'PR #{}'.format(text))
    return link_node(rawtext, lineno, inliner,
        title, explicit_title,
        '{}/{}/pull/{}'.format(gh_url_base, config.github_links_repo, target),
        options)


# If this is a release, turns :ghrelease:`Release Text` into "Release Text"
# that is a link to the GitHub release page to the release. If it's not a
# release then it turns into an invalid link. Because of this, the paragraph
# this is part of should be conditional.
def ghrelease_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    config = get_config(inliner)
    explicit_title, title, target = process_title_target(text)
    if not explicit_title and config.github_links_release_tag is None:
        url = ''
        title = '(Invalid Release Link)'
    else:
        url = '{}/{}/releases/tag/{}'.format(
            gh_url_base, config.github_links_repo,
            target if explicit_title else config.github_links_release_tag)
    node = nodes.reference(rawtext, title, refuri=url, **options)
    return ([node], [])


def acetaorel_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    config = get_config(inliner)
    info = vars(config.opendds_version_info)
    explicit_title, title, target = process_title_target(text)
    if not explicit_title:
        ace_ver = info[target + '_version']
        parts = ace_ver.split('.')
        tao_ver = '.'.join([str(int(parts[0]) - 4)] + parts[1:3])
        title = f'ACE {ace_ver}/TAO {tao_ver}'
    return link_node(rawtext, lineno, inliner,
        title, explicit_title, info[target + '_url'], options)


def omgissue_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    explicit_title, title, target = process_title_target(
        text, 'OMG Issue {}'.format(text))
    rv = []
    append(rv, link_node(rawtext, lineno, inliner,
        title, explicit_title,
        '{}/issues/{}'.format(omg_url_base, target),
        options))
    append(rv, text_node(rawtext, lineno, ' ', options))
    append(rv, link_node(rawtext, lineno, inliner,
        '(Member Link)', False,
        '{}/browse/{}'.format(omg_url_base, target),
        options))
    return rv


def add_omg_spec(app, slug, version, our_name=None, display_name=None):
    '''To be used in conf.py to declare sepcs based on links like https://www.omg.org/spec/DDS/1.4.
    slug is the OMG name in their URL.
    version must also match the URL.
    our_name is the name to use with :omgspec:.
    display_name is the name of the spec to be used in output.
    '''

    our_name = slug.lower() if our_name is None else our_name
    omg_specs = app.config.omg_specs
    if our_name in omg_specs:
        raise KeyError('Already a spec named ' + our_name)
    display_name = slug.replace('-', ' ') if display_name is None else display_name

    # Get the PDF if we don't have it
    dir_path = docs_path / Path('_build') / 'omg-specs'
    dir_path.mkdir(parents=True, exist_ok=True)
    pdf_path = dir_path / '{}-{}.pdf'.format(slug, version)
    url = 'https://www.omg.org/spec/{}/{}'.format(slug, version)
    pdf_url = url + '/PDF'
    if not pdf_path.is_file():
        logger.info('Downloading spec %s from %s', our_name, pdf_url)
        try:
            with urlopen(pdf_url) as res, pdf_path.open('wb') as pdf_file:
                shutil.copyfileobj(res, pdf_file)
        except Exception as e:
            logger.warning("Couldn't download pdf %s: %s", pdf_url, repr(e))
            pdf_path.unlink(missing_ok=True)
            pdf_path = None

    # Process PDF's Table of Contents
    root = dict(subsections=None)
    sections_by_number = {}
    sections_by_title = {}
    if pdf_path:
        doc = fitz.open(pdf_path)
        root = dict(subsections=[])
        section_stack = []
        last_section = root
        # See https://pymupdf.readthedocs.io/en/latest/document.html#Document.get_toc
        for level, title, page, dest in doc.get_toc(simple=False):
            assert page >= 1

            # We only have level numbers, so we have to recreate the structure
            # of the sections by using last_section and section_stack to keep
            # track what the different levels mean.
            if level > len(section_stack):
                section_stack.append(last_section)
            elif level < len(section_stack):
                del section_stack[level - len(section_stack):]

            kind = dest['kind']
            # PDFs have two kinds of internal links. One is named and the other
            # is page and coordinate based.
            # See https://pdfobject.com/pdf/pdf_open_parameters_acro8.pdf for URL syntax
            if kind == fitz.LINK_GOTO:
                loc = 'page={}&view=FitH,{}'.format(page, dest['to'].y)
            elif kind == fitz.LINK_NAMED:
                if 'name' in dest:
                    loc = dest['name']
                else:
                    loc = dest['nameddest']
            else:
                continue

            # Sections can be referenced by section number, which is preferred
            # or by part of or the whole title. See omgspec_role for why.
            section = dict(title=title, loc=loc, subsections=[], level=level)
            section_stack[-1]['subsections'].append(section)
            last_section = section
            m = omg_spec_section_re.match(title)
            if m:
                sections_by_number[m.group(1)] = section
            sections_by_title[title] = section

    omg_specs[our_name] = dict(
        display_name=display_name,
        url=url,
        pdf_url=pdf_url,
        version=version,
        sections=root['subsections'],
        sections_by_number=sections_by_number,
        sections_by_title=sections_by_title,
    )


def section_link(spec, section):
    return spec['pdf_url'] + '#' + section['loc']


def omgspec_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    config = get_config(inliner)
    explicit_title, title, target = process_title_target(text)
    args = target.split(':', 1)
    if len(args) == 1:
        spec_name = args[0]
        section_key = None
    elif len(args) == 2:
        spec_name = args[0]
        section_key = args[1]
    else:
        return rst_error(rawtext, text, lineno, inliner,
            'omgspec target must be of the form SPEC[:SECTION], not {}', repr(target))

    spec = config.omg_specs.get(spec_name)
    if spec is None:
        return rst_error(rawtext, text, lineno, inliner,
            '{} is not a valid omgspec spec name, must be one of: {}',
            repr(spec_name), ', '.join(config.omg_specs.keys()))

    section = None
    if section_key is None or spec['sections'] is None:
        # Either no section was specified or we couldn't download the PDF for
        # some reason.
        url = spec['url']
    else:
        # Here we check it as a section number first, then as a partial or
        # whole title. The whole title includes the section number, so we could
        # just check the title, but doing that can match part of an earlier
        # section number/title. Ex: 1.2.3 would match 1.1.1.2.3
        section = spec['sections_by_number'].get(section_key)
        if section is None:
            for section_title, sect in spec['sections_by_title'].items():
                if section_key in section_title:
                    section = sect
                    break
        if section is None:
            return rst_error(rawtext, text, lineno, inliner,
                '{} is not a valid section in the {} spec', repr(section_key), spec_name)
        url = section_link(spec, section)

    if not explicit_title:
        title = '{display_name} v{version}'.format(**spec)
        if section_key is not None:
            if section is None:
                # If we got here then we couldn't download the PDF for some
                # reason, so just use the target/section_key in the title.
                title += ' ' + section_key
            else:
                title += ' ' + section['title']

    return link_node(rawtext, lineno, inliner, title, explicit_title, url, options)


class OmgSpecsDirective(SphinxDirective):
    option_spec: OptionSpec = {
        'debug-links': directives.flag,
    }

    def spec_sections(self, spec, node, sections):
        if sections is None:
            node += nodes.inline('', '(No section info, PDF download failed)')
        else:
            section_list = nodes.bullet_list()
            for section in sections:
                section_node = nodes.list_item()
                p = nodes.paragraph()
                p += nodes.reference('', section['title'], refuri=section_link(spec, section))
                p += nodes.inline('', ' ({})'.format(section['level']))
                self.spec_sections(spec, p, section['subsections'])
                section_node += p
                section_list += section_node
            node += section_list

    def run(self):
        specs_node = nodes.bullet_list()
        for spec_name, spec in self.env.app.config.omg_specs.items():
            spec_node = nodes.list_item()
            p = nodes.paragraph()
            p += nodes.reference('',
                spec['display_name'] + ' ' + spec['version'], refuri=spec['url'])
            p += nodes.inline('', ' (')
            p += nodes.literal('', spec_name)
            p += nodes.inline('', ')')
            spec_node += p
            if 'debug-links' in self.options and not self.env.app.config.gen_all_omg_spec_links:
                self.spec_sections(spec, spec_node, spec['sections'])
            specs_node += spec_node
        return [specs_node]


def setup(app):
    app.add_config_value('github_links_repo', None, 'env', types=[str])
    app.add_config_value('github_links_commitish', None, 'env', types=[str])
    app.add_config_value('github_links_release_tag', None, 'env', types=[str])
    app.add_config_value('github_links_root_path', None, 'env', types=[str])
    app.add_role('ghfile', ghfile_role)
    app.add_role('ghissue', ghissue_role)
    app.add_role('ghpr', ghpr_role)
    app.add_role('ghrelease', ghrelease_role)

    app.add_config_value('opendds_version_info', None, 'env')
    app.add_role('acetaorel', acetaorel_role)

    app.add_config_value('omg_specs', {}, 'env', types=[dict])
    app.add_config_value('gen_all_omg_spec_links', True, 'env', types=[bool])
    app.add_role('omgissue', omgissue_role)
    app.add_role('omgspec', omgspec_role)
    app.add_directive("omgspecs", OmgSpecsDirective)

# vim: expandtab:ts=4:sw=4
