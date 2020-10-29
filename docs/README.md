# OpenDDS Documentation

OpenDDS Documentation that doesn't belong in other places, like the Doxygen
comments in the code or the OpenDDS Developer's Guide.

- [Changelogs](history)
- [Developer Guidelines](guidelines.md)
- [Detailed List of OpenDDS Dependencies](dependencies.md)
- [Using OpenDDS in a CMake Project](cmake.md)
- [Building OpenDDS for Android](android.md)
- [Building Qt Applications included with OpenDDS](qt.md)
- [Design Files for OpenDDS](design)

## Sphinx Documentation

The [Sphinx](https://www.sphinx-doc.org/en/master/)-based documentation is
hosted on [Read the Docs](readthedocs.org) located
[here](https://opendds.readthedocs.io/en/latest/). It can also be built
locally. To do this follow the steps in the following section.

### Building

These steps assume a Unix system that has Python 3.

#### 1. Create a Virutalenv to Isolate Python Dependencies (Optional):

*You should only have to do this once.*

It would be recommended to do this outside the OpenDDS repo. Also `.venv` can
substituted for any valid non-existing directory path.

```
virtualenv -p /usr/bin/python3 .venv
```

#### 2. Source Virutalenv (Optional):

If you have a virutalenv to source.

```
source .venv/bin/activate
```

#### 3. Install Sphinx and Other Dependencies

*You should only have to do this once.*

This and following steps should be done inside this directory
(`DDS_ROOT/docs`).

```
pip3 install -r requirements.txt
```

#### 4. Build Website:

```
make html
```

If it built successfully, the root of the website should be able to be found
at `_build/html/index.html`.

#### 5. Build PDF (Optional):

Note this has additional LaTeX dependencies that haven't been documented yet.

```
make latexpdf
```

If it built successfully, the PDF file should be able to be found at
`_build/latex/opendds.pdf`.
