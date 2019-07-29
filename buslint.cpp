// buslint.cpp : Defines the entry point for the console application.
//

/*
 * Lint tool for Lumberyard Ebuses.
 *
 * Finds the most common mistake: forgetting to connect the bus.
 */

#include <iostream>
#include <vector>
#include <regex>
#include <sstream>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/xpressive/xpressive_static.hpp>

#define  WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace boost::xpressive;

constexpr int BUFFERSIZE = 100000; // maximum supported file size
char   definition_file_content[BUFFERSIZE] = { 0 };
char   implementation_file_content[BUFFERSIZE] = { 0 };

class Components
{
public:
    static const boost::xpressive::cregex bus_lines_re;
    static const cregex bus_name_re;
    static const std::regex header_extension;

    explicit Components(const char* path)
    {
        m_header = path;
    }

    DWORD ReadFileContent(const char* file_path, char* out_buffer)
    {
        out_buffer[0] = 0;

        HANDLE hFile = CreateFileA(file_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        DWORD lpNumberOfBytesRead = 0;
        DWORD bytesRead = 0;
        do
        {
            ReadFile(hFile, &out_buffer[bytesRead], BUFFERSIZE - 1, &lpNumberOfBytesRead, nullptr);
            bytesRead += lpNumberOfBytesRead;
        }
        while (lpNumberOfBytesRead > 0);

        out_buffer[++bytesRead] = 0; // null terminate the file
        return bytesRead;
    }

    bool Validate(std::string& problems)
    {
        // early bail out if the file isn't .h
        if (strstr(m_header, ".h") == nullptr)
        {
            return true;
        }

        std::vector<std::string> buses;
        buses.reserve(100);

        const DWORD definition_totalBytesRead = ReadFileContent(m_header, definition_file_content);

        std::vector<std::string> bus_lines;
        bus_lines.reserve(100);

        const char* c_start = &definition_file_content[0];
        const char* c_end = &definition_file_content[definition_totalBytesRead];
        cregex_iterator cur(c_start, c_end, bus_lines_re);
        const cregex_iterator end;

        for (; cur != end; ++cur)
        {
            const auto& bus_line_of_code = *cur;
            bus_lines.emplace_back(bus_line_of_code[0]);
        }

        for (const std::string& line : bus_lines)
        {
            cmatch bus_name_match;
            regex_search(line.c_str(), bus_name_match, bus_name_re);
            buses.emplace_back(bus_name_match[0]);
        }

        const std::string source = std::regex_replace(m_header, header_extension, ".cpp");
        ReadFileContent(source.c_str(), implementation_file_content);

        bool is_valid = true;
        for (const std::string& bus : buses)
        {
            const std::string bus_connect_string = bus + "::BusConnect";

            // either the header file or the source file needs to have BusConnect
            if (strstr(implementation_file_content, bus_connect_string.c_str()) == nullptr && strstr(definition_file_content, bus_connect_string.c_str()) == nullptr)
            {
                const char* beginning_to_bus = strstr(definition_file_content, bus.c_str()); // to count lines
                if (beginning_to_bus != nullptr)
                {
                    // figure the line number that will help the user figure out which bus isn't connected
                    int lines = 0;
                    char* definition_index = &definition_file_content[0];
                    while (definition_index != beginning_to_bus)
                    {
                        if (*definition_index == '\n')
                        {
                            ++lines;
                        }
                        ++definition_index;
                    }

                    problems += std::string(m_header) + "(" + std::to_string(lines + 1) + "): error: " + bus + " not connected.\n";
                }
                is_valid = false;
            }
        }

        return is_valid;
    }

private:
    const char* m_header = nullptr;
};

// this gets the entire bus line of code, ex, private MannequinAssetNotificationBus::Handler
const boost::xpressive::cregex Components::bus_lines_re = boost::xpressive::cregex::compile(R"(^(?!\s*(\/|\*).*).+(Bus::Handler|Bus::MultiHandler)(?!.*\/\/.?nolint).*$)",
    regex_constants::not_dot_newline | regex_constants::optimize);

// this gets the actual bus only, ex, MannequinAssetNotificationBus::Handler
const cregex Components::bus_name_re = +alnum >> "Bus::Handler" | "Bus::MultiHandler";

const std::regex Components::header_extension = std::regex("\\.h");

class App
{
public:
    explicit App(const std::vector<const char*>& componentList)
    {
        for (const char* comp : componentList)
        {
            m_components.emplace_back(comp);
        }
    }

    bool Validate()
    {
        bool overall = true;
        std::string problems;
        for (Components& comp : m_components)
        {
            if (!comp.Validate(problems))
            {
                overall = false;
            }
        }

        if (!overall)
        {
            problems = "Aw snap dawg!, you forgot to connect a bus! \n" + problems;
            std::cerr << problems << std::endl;
        }
        return overall;
    }

private:
    std::vector<Components> m_components;
};

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "buslint requires one or more files to parse" << std::endl;
        return 0;
    }

    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    LARGE_INTEGER Frequency;

    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&StartingTime);

    // read contents of files to parse from the file
    std::vector<const char*> files;
    files.reserve(1000);
    for (int arg_index = 1; arg_index < argc; ++arg_index)
    {
        files.push_back(argv[arg_index]);
    }

    App app(files);
    const auto result = app.Validate();

    QueryPerformanceCounter(&EndingTime);
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    ElapsedMicroseconds.QuadPart *= 1000000;
    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

    std::cout << "buslint took " << ElapsedMicroseconds.QuadPart / 1000.f << " ms" << std::endl;

    return result ? 0 : 1;
}

