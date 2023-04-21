from docutils import nodes
from docutils.parsers.rst.states import Struct
from docutils.utils import unescape
import subprocess
from pathlib import Path
import re

gh_url_base = 'https://github.com'
omg_url_base = 'https://issues.omg.org'


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


def setup(app):
    app.add_config_value('github_links_repo', None, 'env', types=[str])
    app.add_config_value('github_links_commitish', None, 'env', types=[str])
    app.add_config_value('github_links_release_tag', None, 'env', types=[str])
    app.add_config_value('github_links_root_path', None, 'env', types=[str])

    app.add_role('ghfile', ghfile_role)
    app.add_role('ghissue', ghissue_role)
    app.add_role('ghpr', ghpr_role)
    app.add_role('ghrelease', ghrelease_role)

    app.add_role('omgissue', omgissue_role)

# vim: expandtab:ts=4:sw=4
