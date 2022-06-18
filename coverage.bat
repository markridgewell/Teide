@echo off
setlocal
SET PRESET=%1
set GTEST_FILTER=%2
OpenCppCoverage --export_type html:coverage --modules "*.exe" --sources "Source\\" --excluded_sources "Test\\" --excluded_sources "build" --cover_children --quiet -- ctest -V --preset %PRESET%
endlocal
