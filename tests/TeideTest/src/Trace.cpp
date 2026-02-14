
#include "Trace.h"

#ifdef __linux__

#    include <cpptrace/basic.hpp>
#    include <cpptrace/cpptrace.hpp>
#    include <cpptrace/formatting.hpp>
#    include <cpptrace/utils.hpp>
#    include <spdlog/spdlog.h>
#    include <sys/wait.h>
#    include <unistd.h>

#    include <array>
#    include <csignal>
#    include <cstring>
#    include <filesystem>
#    include <iostream>
#    include <optional>
#    include <print>
#    include <span>

namespace
{
char* s_execPath = nullptr;
constexpr const char* s_resolveTraceEnv = "_CPPTRACE_RESOLVE_TRACE_";

struct Pipe
{
    explicit Pipe(int pid, std::span<const int, 2> pipe) : pid{pid}, writeEnd{pipe[1]}, readEnd{pipe[0]} {}

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    Pipe(Pipe&& other) noexcept : pid{other.pid}, writeEnd{other.writeEnd}, readEnd{other.readEnd} { other.Reset(); }
    Pipe& operator=(Pipe&& other) noexcept
    {
        CloseHandles();
        pid = other.pid;
        writeEnd = other.writeEnd;
        readEnd = other.readEnd;
        other.Reset();
        return *this;
    }

    ~Pipe() noexcept { CloseHandles(); }

    void Close() { *this = Pipe{}; }
    void ReadFromStdin() const { dup2(readEnd, STDIN_FILENO); }
    void Write(void* data, std::size_t size) const { write(writeEnd, data, size); }

private:
    explicit Pipe() = default;

    void Reset() noexcept
    {
        pid = -1;
        writeEnd = -1;
        readEnd = -1;
    }

    void CloseHandles() const noexcept
    {
        if (readEnd >= 0)
        {
            close(readEnd);
        }
        if (writeEnd >= 0)
        {
            close(writeEnd);
        }
        if (pid > 0)
        {
            // Wait for child
            waitpid(pid, nullptr, 0);
        }
    }

    int pid = -1;
    int writeEnd = -1;
    int readEnd = -1;
};

std::optional<Pipe> Run(std::span<char* const> command)
{
    // Setup pipe and spawn child
    auto input_pipe = std::array<int, 2>{};
    if (pipe(input_pipe.data()) == -1)
    {
        std::println("pipe() failed");
        return std::nullopt;
    }

    const pid_t pid = fork();
    if (pid == -1)
    {
        std::println(stderr, "fork() failed");
        return std::nullopt;
    }

    auto pipe = Pipe(pid, input_pipe);
    if (pid == 0)
    {
        // child
        pipe.ReadFromStdin();
        pipe.Close();

        execv(command[0], command.data());
        std::println(stderr, "execv() failed");
        _exit(1);
    }

    // parent
    return pipe;
}

void DoSignalSafeTrace(std::span<const cpptrace::frame_ptr> buffer)
{
    setenv(s_resolveTraceEnv, "1", 0); // NOLINT(concurrency-mt-unsafe)
    char* const command[] = {s_execPath, nullptr};
    const auto pipe = Run(command);
    if (!pipe)
    {
        return;
    }

    // Resolve to safe_object_frames and write those to the pipe
    for (cpptrace::frame_ptr ptr : buffer)
    {
        auto frame = cpptrace::safe_object_frame{};
        cpptrace::get_safe_object_frame(ptr, &frame);
        pipe->Write(&frame, sizeof(frame));
    }
}

void SignalHandler(int signo [[maybe_unused]], siginfo_t* info [[maybe_unused]], void* context [[maybe_unused]])
{
    // Print basic message
    std::println(stderr, "SIGSEGV occurred:");
    // Generate trace
    constexpr std::size_t N = 100;
    auto buffer = std::array<cpptrace::frame_ptr, N>{};
    std::size_t count = cpptrace::safe_generate_raw_trace(buffer.data(), N, 1);

    DoSignalSafeTrace(std::span(buffer).subspan(0, count));

    // Up to you if you want to exit or continue or whatever
    _exit(1);
}

[[noreturn]] void ResolveTrace()
{
    auto trace = cpptrace::object_trace{};
    while (true)
    {
        auto frame = cpptrace::safe_object_frame{};
        // fread used over read because a read() from a pipe might not read the full frame
        std::size_t res = fread(&frame, sizeof(frame), 1, stdin);
        if (res == 1)
        {
            trace.frames.push_back(frame.resolve());
        }
        else
        {
            if (res != 0)
            {
                std::println(stderr, "Unable to resolve callstack. Return code {} while reading from the pipe", res);
            }
            break;
        }
    }
    const auto formatter = //
        cpptrace::formatter{}
            .symbols(cpptrace::formatter::symbol_mode::full)
            .break_before_filename()
            .transform([](cpptrace::stacktrace_frame f) {
                f.filename = std::filesystem::path(f.filename).lexically_normal().string();
                f.symbol = cpptrace::prettify_symbol(f.symbol);
                if (f.symbol.length() > 150)
                {
                    f.symbol = cpptrace::prune_symbol(f.symbol);
                }
                return f;
            });
    formatter.print(trace.resolve());
    _exit(0);
}

} // namespace

void InitCpptrace(int argc [[maybe_unused]], char** argv)
{
    s_execPath = *argv;

    if (getenv(s_resolveTraceEnv)) // NOLINT(concurrency-mt-unsafe)
    {
        ResolveTrace();
    }

    // This is done for any dynamic-loading shenanigans
    auto buffer = std::array<cpptrace::frame_ptr, 10>{};
    cpptrace::safe_generate_raw_trace(buffer.data(), buffer.size());
    cpptrace::safe_object_frame frame = {};
    cpptrace::get_safe_object_frame(buffer[0], &frame);

    // Setup signal handler
    struct sigaction action = {{nullptr}};
    action.sa_flags = 0;
    action.sa_sigaction = &SignalHandler;
    if (sigaction(SIGSEGV, &action, nullptr) == -1)
    {
        perror("sigaction");
    }
}

#endif
