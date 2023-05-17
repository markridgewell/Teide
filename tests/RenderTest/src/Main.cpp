
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
        assert(aliases.size() > 0);
        for (const std::string_view arg : m_args)
        {
            if (std::ranges::find(aliases, arg) != aliases.end())
            {
                return true;
            }
        }
        return false;
    };

    template <class T>
    auto GetArg(std::initializer_list<std::string_view> aliases, std::optional<T> defaultValue = std::nullopt) -> T
    {
        assert(aliases.size() > 0);
        for (unsigned i = 0; i < m_args.size(); ++i)
        {
            const std::string_view arg = m_args[i];
            if (std::ranges::find(aliases, arg) != aliases.end())
            {
                if (i < m_args.size() - 1)
                {
                    return m_args[i + 1];
                }

                m_errorString += "Missing argument to ";
                m_errorString += arg;
                m_errorString += '\n';
                return {};
            }
        }
        if (!defaultValue)
        {
            m_errorString += "Missing required argument ";
            m_errorString += *aliases.begin();
            m_errorString += '\n';
            return {};
        }
        return defaultValue.value();
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
