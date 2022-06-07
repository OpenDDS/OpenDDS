#!/usr/bin/env python3

'''\
Mock the HTTP server hosting downloads for OpenDDS on objectcomputing.com.
'''

from pathlib import Path
import http.server
import socketserver
from os import chdir
from argparse import ArgumentParser


def Version(string):
    parts = [int(part) for part in string.split('.')]
    if len(parts) not in (2, 3):
        raise ValueError('Invalid number of fields: ' + repr(ver))
    if len(parts) == 2:
        parts.append(0)
    return parts


def add_version(versions, latest, major, minor, micro):
    versions.append({
        'major': major, 'minor': minor, 'micro': micro,
        'latest': latest == [major, minor, micro],
        'latest_non_micro': latest[0:2] == [major, minor] and micro == 0
    })


def fake_filenames(paths, path_base, version, extra, source=True):
    suffixes = []
    if source:
        suffixes += [
            '.md5',
            '.sha256',
            '.tar.gz',
            '.zip',
        ]
    if extra:
        suffixes += [
            '-doxygen.tar.gz',
            '-doxygen.zip',
            '.pdf',
        ]
    fmt = 'OpenDDS-{major}.{minor}'
    if version['micro'] > 0:
        fmt += '.{micro}'
    filename_base = fmt.format(**version)
    paths.extend([path_base / (filename_base + suffix) for suffix in suffixes])


def generate_fake_files(serve_root, latest):
    downloads = serve_root / 'downloads/OpenDDS'
    previous = downloads / 'previous-releases'
    previous.mkdir(parents=True, exist_ok=True)

    versions = []
    for major in range(1, latest[0]):
        add_version(versions, latest, major, 0, 0)
    for minor in range(0, latest[1]):
        add_version(versions, latest, latest[0], minor, 0)
    for micro in range(0, latest[2] + 1):
        add_version(versions, latest, latest[0], latest[1], micro)

    paths = [downloads / 'OpenDDS-latest.pdf']
    for version in versions:
        if version['latest']:
            fake_filenames(paths, downloads, version, version['latest_non_micro'])
        else:
            is_micro = version['micro'] == 0
            fake_filenames(paths, previous, version, is_micro and not version['latest_non_micro'])
            if is_micro and version['latest_non_micro']:
                fake_filenames(paths, downloads, version, True, False)

    for p in paths:
        print('Touching', p)
        p.touch()


def main(serve_root, fake_latest_version):
    if fake_latest_version:
        generate_fake_files(serve_root, fake_latest_version)

    chdir(serve_root)
    port = 8000
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", port), http.server.SimpleHTTPRequestHandler) as httpd:
        print("serving at port", port)
        httpd.serve_forever()


if __name__ == '__main__':
    arg_parser = ArgumentParser(description=__doc__)
    arg_parser.add_argument('--fake-version', '-v', metavar='FAKE_LATEST_VERSION',
        help='Fake this version (and previous ones) of OpenDDS using empty files',
        type=Version
    )
    arg_parser.add_argument('--serve-root', '-r', metavar='SERVE_ROOT', default='.',
        help='Where to put the files and serve it from',
        type=Path,
    )

    args = arg_parser.parse_args()
    main(args.serve_root, args.fake_version)
