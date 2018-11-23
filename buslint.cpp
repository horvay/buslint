// buslint.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <filesystem>
#include <vector>
#include <tuple>
#include <regex>
#include <fstream>
#include <sstream>


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
            definition = std::string( ( std::istreambuf_iterator<char>( header ) ), std::istreambuf_iterator<char>() );
        }

        // this regex will get the entire line with the bus connect, but not including //nolint and lines that start with //
        const std::regex bus_lines_reg( R"(^(?!\s*((\/\/)|(\*))).*[A-z,0-9,_]*(Bus::Handler)\s*?(?!.*\/\/.?nolint))" );
        // this extracts out the name and handler for the bus from the bus line above ex: YourMotherBus::Handler
        const std::regex bus_reg( "[A-z,0-9,_]*Bus::Handler" );

        std::sregex_iterator next( definition.begin(), definition.end(), bus_lines_reg );
        const std::sregex_iterator end;
        while ( next != end )
        {
            std::smatch sm = *next;

            const auto bus_line = sm.str();
            std::smatch bus_match;
            if ( std::regex_search( bus_line, bus_match, bus_reg ) )
                buses.push_back( bus_match[0] );

            ++next;
        }

        std::string impl;
        {
            std::ifstream source( m_source );
            impl = std::string( ( std::istreambuf_iterator<char>( source ) ), std::istreambuf_iterator<char>() );
        }

        impl += definition;
        std::string problems;
        bool is_valid = true;
        for ( const auto& bus : buses )
        {
            if ( impl.find( bus + "::BusConnect" ) == std::string::npos )
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

