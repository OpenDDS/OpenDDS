@echo off
@REM

java -jar -Dapp.name=__APPNAME__ -Dopendds.xsd.file=%~dp0xsd/OpenDDSXMI.xsd %~dp0__APPNAME__.jar %*

@echo on