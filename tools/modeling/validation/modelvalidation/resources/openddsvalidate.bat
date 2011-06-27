@echo off
@REM

java -jar -Dapp.name=__APPNAME__ -Dopendds.xsd.file=%~dp0xsd/OpenDDSXMI.xsd %~dp0lib\__APPNAME__.jar %*

@echo on