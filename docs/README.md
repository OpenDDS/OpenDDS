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

## Building Sphinx Documentation

These steps assume a Unix system that has Python 3.

### 1. Create a Virutalenv to Isolate Python Dependencies (Optional):

It would be recommended to do this outside OpenDDS repo. Also `.venv` can
substituted for any valid directory name.

```
virtualenv -p /usr/bin/python3 .venv
source .venv/bin/activate
```

### 2. Install Sphinx and Other Dependencies

```
pip3 install -r requirements.txt
```

### 3. Build Website:

```
cd $DDS_ROOT/docs
make html
```

If it built successfully, the root of the website should be able to be found
at `_build/html/index.html`.

### 4. Build PDF (Optional):

Note this has additional LaTeX dependencies that haven't been documented yet.

```
make latexpdf
```

If it built successfully, the pdf file should be able to be found at
`_build/latex/opendds.pdf`.
