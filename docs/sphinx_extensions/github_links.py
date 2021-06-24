from docutils import nodes
import subprocess
from pathlib import Path

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


# Turns :ghfile:`README.md` into the equivalent of ``README.md`` that is a link
# to that file on GitHub. It will try to point to the most specific version of
# the file:
# - If the OpenDDS is a release it will calculate the release tag and use that.
# - Else if the OpenDDS is in a git repository it will use the commit hash.
# - Else it will use `master`.
def ghfile_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo
    local_path = Path(app.config.github_links_root_path) / text
    if not local_path.exists():
        msg = inliner.reporter.error(
            '"{}" doesn\'t exist. Checked for existence of "{}".'.format(
                text, str(local_path)),
            line=lineno)
        prb = inliner.problematic(rawtext, rawtext, msg)
        return [prb], [msg]
    kind = 'tree' if local_path.is_dir() else 'blob'
    url = '/'.join([url_base, repo, kind, get_commitish(app), text])
    options['classes'] = ['custom_literal']
    node = nodes.reference(rawtext, text, refuri=url, **options)
    return ([node], [])


# Turns :ghissue:`213` into the equivalent of:
#   `Issue #213 on GitHub <https://github.com/objectcomputing/OpenDDS/issues/213>`_
def ghissue_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo
    url = '{}/{}/issues/{}'.format(url_base, repo, text)
    text = 'Issue #{} on GitHub'.format(text)
    node = nodes.reference(rawtext, text, refuri=url, **options)
    return ([node], [])


# Turns :ghpr:`1` into the equivalent of:
#   `Pull Request #1 on GitHub <https://github.com/objectcomputing/OpenDDS/pull/1>`_
def ghpr_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
    app = inliner.document.settings.env.app
    repo = app.config.github_links_repo
    url = '{}/{}/pull/{}'.format(url_base, repo, text)
    text = 'Pull Request #{} on GitHub'.format(text)
    node = nodes.reference(rawtext, text, refuri=url, **options)
    return ([node], [])


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
