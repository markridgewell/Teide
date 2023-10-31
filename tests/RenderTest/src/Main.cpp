
#include "RenderTest.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <initializer_list>

class ArgParser
{
public:
    explicit ArgParser(int argc, char** argv) : m_args{std::span(argv, static_cast<std::size_t>(argc)).subspan<1>()} {}

    auto GetOption(std::initializer_list<std::string_view> aliases) -> bool
    {
        TEIDE_ASSERT(aliases.size() > 0);
        const auto containsAlias = [&](std::string_view arg) { return std::ranges::find(aliases, arg) != aliases.end(); };
        return std::ranges::any_of(m_args, containsAlias);
    };

    template <class T>
    auto GetArg(std::initializer_list<std::string_view> aliases, std::optional<T> defaultValue = std::nullopt) -> T
    {
        TEIDE_ASSERT(aliases.size() > 0);
        const auto containsAlias = [&](std::string_view arg) { return std::ranges::find(aliases, arg) != aliases.end(); };

        const auto arg = std::ranges::find_if(m_args, containsAlias);
        if (arg == m_args.end())
        {
            if (defaultValue)
            {
                return defaultValue.value();
            }
            m_errorString += "Missing required argument ";
            m_errorString += *aliases.begin();
            m_errorString += '\n';
            return {};
        }

        const auto nextArg = arg + 1;
        if (nextArg == m_args.end() || std::string_view(*nextArg).starts_with("-"))
        {
            m_errorString += "Missing argument to ";
            m_errorString += *arg;
            m_errorString += '\n';
            return {};
        }
        return *nextArg;
    };

    bool HasErrors() const { return !m_errorString.empty(); }
    const std::string& GetErrors() const { return m_errorString; }

private:
    std::span<const char* const> m_args;
    std::string m_errorString;
};

int main(int argc, char** argv)
{
    using namespace std::filesystem;

    auto parser = ArgParser(argc, argv);

    if (parser.GetOption({"-s", "--sw-render"}))
    {
        Teide::EnableSoftwareRendering();
    }

    RenderTest::SetUpdateReferences(parser.GetOption({"-u", "--update-refs"}));
    RenderTest::SetReferenceDir(parser.GetArg<path>({"-r", "--reference-dir"}));
    RenderTest::SetOutputDir(parser.GetArg<path>({"-o", "--output-dir"}, current_path()) / "RenderTestOutput");

    if (parser.GetOption({"-v", "--verbose"}))
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Verbose logging enabled");
    }

    if (parser.HasErrors())
    {
        std::cerr << parser.GetErrors();
        return 1;
    }

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
