// buslint.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <filesystem>
#include <vector>
#include <tuple>
#include <regex>
#include <fstream>
#include <sstream>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/xpressive/xpressive_static.hpp>

class Components
{
public:
    explicit Components( const std::string& path )
    {
        m_header = path;
        m_source = std::regex_replace( path, std::regex( "\\.h" ), ".cpp" );
    }

    std::tuple<bool, std::string> Validate()
    {
        std::vector<std::string> buses{};

        std::string definition;
        {
            std::ifstream header( m_header );
            definition = std::string( std::istreambuf_iterator<char>( header ), std::istreambuf_iterator<char>() );
        }

        using namespace boost::xpressive;
        // this gets the entire bus line of code, ex, private MannequinAssetNotificationBus::Handler
        const sregex bus_lines_re = sregex::compile( R"(^(?!\s*(\/|\*).*).+(Bus::Handler|Bus::MultiHandler)(?!.*\/\/.?nolint).*$)", regex_constants::not_dot_newline | regex_constants::optimize );

        std::vector<std::string> bus_lines{};
        sregex_iterator cur( definition.begin(), definition.end(), bus_lines_re );
        const sregex_iterator end;

        for ( ; cur != end; ++cur )
        {
            const auto &bus_line_of_code = *cur;
            bus_lines.emplace_back( bus_line_of_code[0] );
        }

        // this gets the actual bus only, ex, MannequinAssetNotificationBus::Handler
        const sregex bus_name_re = +alnum >> "Bus::Handler" | "Bus::MultiHandler";

        for ( auto& line : bus_lines )
        {
            smatch bus_name_match;
            regex_search( line, bus_name_match, bus_name_re );
            buses.emplace_back( bus_name_match[0] );
        }

        std::string impl;
        {
            std::ifstream source( m_source );
            impl = std::string( ( std::istreambuf_iterator<char>( source ) ), std::istreambuf_iterator<char>() );
        }

        std::string problems;
        bool is_valid = true;
        for ( const auto& bus : buses )
        {
            if ( impl.find( bus + "::BusConnect" ) == std::string::npos && definition.find( bus + "::BusConnect" ) == std::string::npos )
            {
                problems += "Aw snap dawg!, you forgot to connect the bus " + bus + " in " + m_source + "\n";
                is_valid = false;
            }
        }

        return{ is_valid, problems };
    }

private:
    std::string m_header;
    std::string m_source;
};

class App
{
public:
    explicit App( std::vector<std::string> componentList )
    {
        for ( const auto& comp : componentList )
        {
            m_components.emplace_back( comp );
        }
    }

    bool Validate()
    {
        bool overall = true;
        std::string problems;
        for ( auto& comp : m_components )
        {
            auto result = comp.Validate();
            if ( !std::get<0>( result ) )
            {
                overall = false;
                problems += std::get<1>( result );
            }
        }

        if ( !overall ) std::cerr << problems << std::endl;
        return overall;
    }

private:
    std::vector<Components> m_components;
};

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        std::cout << "buslint requires one or more files to parse" << std::endl;
        return 0;
    }

    // read contents of files to parse from the file
    std::ifstream t( argv[1] );
    const std::string str( ( std::istreambuf_iterator<char>( t ) ), std::istreambuf_iterator<char>() );

    std::istringstream ss( str );

    std::vector<std::string> components{};

    std::string one_line;
    while ( getline( ss, one_line, ' ' ) )
    {
        components.emplace_back( one_line );
    }

    App app( components );
    const auto result = app.Validate();
    return result ? 0 : 1;
}

