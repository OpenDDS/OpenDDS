@echo off
:: Author: Adam Mitz (mitza@ociweb.com)
:: Purpose: This fixes files that have Unix line endings on Windows systems
:: Preconditions: Run from a completely clean svn checkout (no unversioned
:: files present) and CYGWIN_ROOT should be set to c:\cygwin or something.
if not "%DDS_ROOT%" == "%CD%" (
        echo This script must be run from DDS_ROOT
        goto :EOF
)
echo if (/[^^\r]\n$/) {print "$ARGV: $_\n"; exit 1} > ../detect.pl
del ..\files.txt > NUL 2>&1
for /r %%f in (*.h *.cpp *.inl *.mp? *.txt *.idl *.pl *. domain_id* *.conf *.ini *.mw?) do (
        echo %%f | findstr \.svn > NUL
        if ERRORLEVEL 1 (
                %CYGWIN_ROOT%\bin\perl -n ..\detect.pl %%f
                if ERRORLEVEL 1 (
                        svn ps svn:eol-style CRLF %%f
                        %CYGWIN_ROOT%\bin\unix2dos %%f
                        echo %%f >> ..\files.txt
                )
        )
)
if not exist ..\files.txt (
        echo No files found with bad line endings
        del ..\detect.pl
        goto :EOF
)
echo Check the files with svn stat in another window and then
pause
svn ci -m"fixing line endings..."
for /f "usebackq" %%f in (`type ..\files.txt`) do (
        svn ps svn:eol-style native %%f
)
echo Check the files with svn stat in another window and then
pause
svn ci -m"...done fixing line endings"
del ..\files.txt ..\detect.pl
