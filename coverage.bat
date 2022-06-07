@echo off
setlocal
SET PRESET=%1
set GTEST_FILTER=%2
OpenCppCoverage --export_type html:coverage --modules "*.exe" --sources source --cover_children --quiet -- ctest -V --preset %PRESET%
endlocal
