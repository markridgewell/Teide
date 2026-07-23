
#pragma once

#include <cpptrace/forward.hpp>

int TracedMain(int argc, char** argv);

void PrintTrace(const cpptrace::stacktrace& trace);
