from docutils import nodes
from docutils.parsers.rst.states import Struct
from docutils.utils import unescape
import subprocess
from pathlib import Path
import re

url_base = 'https://github.com'


def get_commitish(app):
    if app.config.github_links_commitish is None:
        if app.config.github_links_release_tag is not None:
            app.config.github_links_commitish = app.config.github_links_release_tag
        else:
            try:
                app.config.github_links_commitish = str(subprocess.check_output(
                    ['git', 'rev-parse', 'HEAD']), "utf-8").strip()
            except:
                app.config.github_links_commitish = 'master'

    return app.config.github_links_commitish


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


def link_node(rawtext, lineno, inliner, title, parse_title, url, options):
    node = nodes.reference(rawtext, '' if parse_title else title, refuri=url, **options)
    messages = []
    if parse_title:
        messages = parse_rst(node, title, lineno, inliner)
    return ([node], messages)


ghfile_arg_re = re.compile(r'^([^#]+)(#[^#]+)?$')


def ghfile_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo

    explicit_title, title, target = process_title_target(text)

    # Seperate path and possible URL fragment
    m = ghfile_arg_re.match(target)
    if not m:
        return rst_error(rawtext, text, lineno, inliner,
            '{} is an invalid target', repr(target))
    path = m.group(1)
    fragment = m.group(2) if m.group(2) is not None else ''

    # Check if the path exists locally
    local_path = Path(app.config.github_links_root_path) / path
    if not local_path.exists():
        return rst_error(rawtext, text, lineno, inliner,
            '"{}" doesn\'t exist. Checked for existence of "{}"',
                path, str(local_path)),

    kind = 'tree' if local_path.is_dir() else 'blob'
    url = '/'.join([url_base, repo, kind, get_commitish(app), path]) + fragment
    if explicit_title:
        options['classes'] = [c for c in options['classes'] if c != 'custom_literal']
    else:
        options['classes'] = ['custom_literal']
    return link_node(rawtext, lineno, inliner, title, explicit_title, url, options)


# Turns :ghissue:`213` into the equivalent of:
#   `Issue #213 on GitHub <https://github.com/objectcomputing/OpenDDS/issues/213>`_
def ghissue_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo
    explicit_title, title, target = process_title_target(
        text, 'Issue #{} on GitHub'.format(text))
    return link_node(rawtext, lineno, inliner,
        title, explicit_title,
        '{}/{}/issues/{}'.format(url_base, repo, target),
        options)


# Turns :ghpr:`1` into the equivalent of:
#   `Pull Request #1 on GitHub <https://github.com/objectcomputing/OpenDDS/pull/1>`_
def ghpr_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo
    explicit_title, title, target = process_title_target(
        text, 'Pull Request #{} on GitHub'.format(text))
    return link_node(rawtext, lineno, inliner,
        title, explicit_title,
        '{}/{}/pull/{}'.format(url_base, repo, target),
        options)


# If this is a release, turns :ghrelease:`Release Text` into "Release Text"
# that is a link to the GitHub release page to the release. If it's not a
# release then it turns into an invalid link. Because of this, the paragraph
# this is part of should be conditional.
def ghrelease_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo
    if app.config.github_links_release_tag is None:
        url = ''
        text = '(Invalid Release Link)'
    else:
        url = '{}/{}/releases/tag/{}'.format(
            url_base, repo, app.config.github_links_release_tag)
    node = nodes.reference(rawtext, text, refuri=url, **options)
    return ([node], [])


def setup(app):
    app.add_config_value('github_links_repo', None, 'env', types=[str])
    app.add_config_value('github_links_commitish', None, 'env', types=[str])
    app.add_config_value('github_links_release_tag', None, 'env', types=[str])
    app.add_config_value('github_links_root_path', None, 'env', types=[str])

    app.add_role('ghfile', ghfile_role)
    app.add_role('ghissue', ghissue_role)
    app.add_role('ghpr', ghpr_role)
    app.add_role('ghrelease', ghrelease_role)

# vim: expandtab:ts=4:sw=4
