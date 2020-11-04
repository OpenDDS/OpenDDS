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

These step assume you have Python 3 installed on your system. Perform these
steps inside this directory, `DDS_ROOT/docs`.

#### 1. Create a Python Virtual Environment to Isolate Dependencies (Optional):

*You should only have to do this once.*

If you choose to ignore this step and the next one, pip will install
dependencies as user-wide packages, which may interfere with other Python
applications and packages.

See [here](https://packaging.python.org/guides/installing-using-pip-and-virtual-environments/#creating-a-virtual-environment)
for more information about Python virtual environments.

Linux and macOS:

```
python3 -m venv .venv
```

Windows:

```
py -m venv .venv
```

#### 2. Source Virtual Environment (Optional):

If you have a virtual environment to source.

Linux and macOS:

```
source .venv/bin/activate
```

Windows:

```
.\.venv\Scripts\activate
```

When you are done you can use `deactivate` to restore your shell or command
prompt environment.

#### 3. Install Sphinx and Other Dependencies

*You should only have to do this once.*

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

This has additional dependencies on LaTeX that are documented
[here](https://www.sphinx-doc.org/en/master/usage/builders/index.html#sphinx.builders.latex.LaTeXBuilder).

```
make latexpdf
```

If it built successfully, the PDF file should be able to be found at
`_build/latex/opendds.pdf`.
