
#include "RenderTest.h"

#include <algorithm>
#include <filesystem>
#include <initializer_list>

class ArgParser
{
public:
    explicit ArgParser(int argc, char** argv) : m_argc{argc}, m_argv{argv} {}

    auto GetOption(std::initializer_list<std::string_view> aliases) -> bool
    {
        assert(aliases.size() > 0);
        for (int i = 1; i < m_argc; ++i)
        {
            const std::string_view arg = m_argv[i];
            if (std::ranges::contains(aliases, arg))
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
        for (int i = 1; i < m_argc; ++i)
        {
            const std::string_view arg = m_argv[i];
            if (std::ranges::contains(aliases, arg))
            {
                if (i < m_argc - 1)
                {
                    return m_argv[i + 1];
                }
                else
                {
                    m_errorString += "Missing argument to ";
                    m_errorString += arg;
                    m_errorString += '\n';
                    return {};
                }
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
    int m_argc;
    char** m_argv;
    std::string m_errorString;
};

int main(int argc, char** argv)
{
    using namespace std::filesystem;

    auto parser = ArgParser(argc, argv);

    RenderTest::SetUpdateReferences(parser.GetOption({"-u", "--update-refs"}));
    RenderTest::SetReferenceDir(parser.GetArg<path>({"-r", "--reference-dir"}));
    RenderTest::SetOutputDir(parser.GetArg<path>({"-o", "--output-dir"}, current_path()) / "RenderTestOutput");

    if (parser.HasErrors())
    {
        std::cerr << parser.GetErrors();
        return 1;
    }

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
