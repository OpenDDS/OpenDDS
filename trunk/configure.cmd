@echo off
:: Win32 configure script wrapper for OpenDDS
:: Distributed under the OpenDDS License.
:: See: http://www.opendds.org/license.html

for %%x in (perl.exe) do set PERLPATH=%%~dp$PATH:x
if x%PERLPATH%==x (
  echo ERROR: perl.exe was not found.  This script requires ActiveState Perl.
  exit /b 1
)
set PERLPATH=
perl configure %*
if exist setenv.cmd call setenv.cmd
