// buslint.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <filesystem>
#include <vector>
#include <tuple>
#include <regex>
#include <fstream>


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
        const std::regex e( "[A-z]*Bus::Handler" );

        std::sregex_iterator next( definition.begin(), definition.end(), e );
        const std::sregex_iterator end;
        while ( next != end )
        {
            std::smatch sm = *next;
            buses.push_back( sm.str() );
            ++next;
        }

        std::string impl;
        {
            std::ifstream source( m_source );
            impl = std::string( ( std::istreambuf_iterator<char>( source ) ), std::istreambuf_iterator<char>() );
        }

        impl += definition;
        for ( const auto& bus : buses )
        {
            if ( !std::regex_search( impl, std::regex( bus + "::BusConnect" ) ) )
            {
                return{ false, "Aw snap dawg!, you forgot to connect the bus " + bus + " in " + m_source };
            }
        }

        return{ true, "" };
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
        for ( auto& comp : m_components )
        {
            auto result = comp.Validate();
            if ( !std::get<0>( result ) )
            {
                std::cerr << std::get<1>( result ) << std::endl;
                return false;
            }
        }
        return true;
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

    std::vector<std::string> components{};
    for ( int i = 1; i < argc; i++ )
    {
        components.emplace_back( argv[i] );
    }

    App app( components );
    const auto result = app.Validate();
    return result ? 0 : 1;
}

