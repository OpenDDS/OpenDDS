import re
import os
import logging
from pathlib import Path

try:
    # If we don't derive from this class, we will can get double output because
    # Sphinx buffers warnings and errors while reading.
    from sphinx.util.logging import WarningStreamHandler as BaseStreamHandler
except ModuleNotFoundError:
    from logging import StreamHandler as BaseStreamHandler


opendds_root_path = Path(__file__).resolve().parent.parent.parent
github_actions = os.environ.get('GITHUB_ACTIONS', 'false') == 'true'
link_check = os.environ.get('LINK_CHECK', 'false') == 'true'
no_source_loc = ('docs/index.rst', 1)
replace_files = {
    None: no_source_loc,
    'docs/news.rst': no_source_loc,
    'docs/this-release.rst': no_source_loc,
}


def create_gha_annotation(kind, message,
      title=None, file=None, col=None, endColumn=None, line=None, endLine=None,
      for_link_check=False, replace_files={}):
    if not github_actions or (link_check and not for_link_check):
        return None

    # See if we should replace the location
    if file:
        file = str(file)
    file, line = replace_files.get(file, (file, line))

    if kind not in ('debug', 'notice', 'warning', 'error'):
        raise ValueError('{kind!r} is an invalid kind')
    if kind == 'debug':
        return None
    # Except for debug (which we will ignore), all the params are the same:
    # https://docs.github.com/en/actions/writing-workflows/choosing-what-your-workflow-does/workflow-commands-for-github-actions#setting-an-error-message
    params = dict(
        title=title,
        file=file,
        col=col,
        endColumn=endColumn,
        line=line,
        endLine=endLine,
    )
    param_list = [f'{k}={v}' for k, v in params.items() if v is not None]
    param_str = ''
    if param_list:
        param_str += ' ' + ','.join(param_list)
    return f'::{kind}{param_str}::{message}'


class GhaAnnotationHandler(BaseStreamHandler):
    '''
    Used to make Sphinx loggers output GitHub Action annotations.
    '''

    def __init__(self, level=logging.WARNING, stream=None, replace_files={}):
        super().__init__(stream)
        self.setLevel(level)
        self.replace_files = replace_files

    def emit(self, record):
        try:
            # Use the raw getMessage to avoid location and the ERROR:/WARNING: prefixes.
            message = logging.LogRecord.getMessage(record)
            file = None
            line = None
            location = getattr(record, 'location', None)
            if location:
                match = re.fullmatch(r'(.*):(\d+)', location)
                if match:
                    file = Path(match.group(1)).resolve()
                    try:
                        file = file.relative_to(opendds_root_path)
                    except ValueError:
                        pass
                    line = int(match.group(2))

            # Sphinx and docutils don't distinguish between a warning and an
            # error in the log level, so just say it's all errors.
            msg = create_gha_annotation('error', message, title=record.name,
                file=file, line=line, replace_files=self.replace_files)
            if msg:
                self.stream.write(msg + self.terminator)
                self.flush()
        except RecursionError:
            raise
        except Exception:
            self.handleError(record)


# Based on https://gist.github.com/rene-d/9e584a7dd2935d0f461904b9f2950007
class AnsiColors:
    Red = "\033[0;31m"
    Purple = "\033[0;35m"
    Yellow = "\033[1;33m"
    End = "\033[0m"
    # cancel SGR codes if we don't write to a terminal
    if not (__import__("sys").stdout.isatty() or github_actions):
        for _ in dir():
            if isinstance(_, str) and _[0] != "_":
                locals()[_] = ""
    else:
        # set Windows console in VT mode
        if __import__("platform").system() == "Windows":
            kernel32 = __import__("ctypes").windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
            del kernel32
