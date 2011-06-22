@echo off
@REM

java -jar -Dopendds.xsd.file=%~dp0xsd/OpenDDSXMI.xsd %~dp0__APPNAME__.jar %*

@echo on