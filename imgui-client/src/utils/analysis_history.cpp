#include "vendor.hpp"
#include "analysis_history.hpp"

#include <fstream>
#include <iomanip>

namespace ida_re::utils {
    std::string analysis_entry_t::get_formatted_time( ) const {
        const auto time_t_val = std::chrono::system_clock::to_time_t( m_timestamp );
        std::tm    tm;
        IDA_RE_LOCALTIME( &tm, &time_t_val );

        std::ostringstream oss;
        oss << std::put_time( &tm, "%Y-%m-%d %H:%M:%S" );
        return oss.str( );
    }

    c_analysis_history::c_analysis_history( ) { }

    void c_analysis_history::add_entry( const analysis_entry_t &entry ) {
        m_entries.insert( m_entries.begin( ), entry ); // Add to front (newest first)

        // Limit size
        if ( m_entries.size( ) > m_max_entries ) {
            m_entries.resize( m_max_entries );
        }
    }

    std::vector< analysis_entry_t > c_analysis_history::get_entries_for_function( std::string_view address ) const {
        auto matches = m_entries
                     | std::views::filter( [ & ]( const auto &e ) { return e.m_function_address == address; } );
        return { matches.begin( ), matches.end( ) };
    }


    std::string c_analysis_history::get_default_history_path( ) {
#ifdef IDA_RE_PLATFORM_WINDOWS
        const char *appdata = std::getenv( "APPDATA" );
        if ( appdata ) {
            std::filesystem::path path = std::filesystem::path( appdata ) / "ida-re-assistant" / "analysis_history.json";
            return path.string( );
        }
        return "analysis_history.json";
#else
        const char *home = std::getenv( "HOME" );
        if ( home ) {
            std::filesystem::path path = std::filesystem::path( home ) / ".config" / "ida-re-assistant" / "analysis_history.json";
            return path.string( );
        }
        return "analysis_history.json";
#endif
    }

    bool c_analysis_history::save_to_file( std::string_view filepath ) const {
        try {
            std::filesystem::path path( filepath );
            std::filesystem::create_directories( path.parent_path( ) );

            json_t j = json_t::array( );

            for ( const auto &entry : m_entries ) {
                auto time_since_epoch = entry.m_timestamp.time_since_epoch( );
                auto millis           = std::chrono::duration_cast< std::chrono::milliseconds >( time_since_epoch ).count( );

                j.push_back( {
                    { "function_address", entry.m_function_address },
                    {    "function_name",    entry.m_function_name },
                    {    "analysis_type",    entry.m_analysis_type },
                    {           "result",           entry.m_result },
                    {        "timestamp",                 millis },
                    {         "provider",         entry.m_provider }
                } );
            }

            std::ofstream file{ std::string( filepath ) };
            file << j.dump( 2 );
            return true;
        } catch ( ... ) {
            return false;
        }
    }

    bool c_analysis_history::load_from_file( std::string_view filepath ) {
        try {
            const auto path_str = std::string( filepath );
            if ( !std::filesystem::exists( path_str ) )
                return false;

            std::ifstream file{ path_str };
            json_t          j = json_t::parse( file );

            m_entries.clear( );

            for ( const auto &item : j ) {
                analysis_entry_t entry;
                entry.m_function_address = item.value( "function_address", "" );
                entry.m_function_name    = item.value( "function_name", "" );
                entry.m_analysis_type    = item.value( "analysis_type", "general" );
                entry.m_result           = item.value( "result", "" );
                entry.m_provider         = item.value( "provider", "Unknown" );

                int64_t millis  = item.value( "timestamp", 0LL );
                entry.m_timestamp = std::chrono::system_clock::time_point( std::chrono::milliseconds( millis ) );

                m_entries.push_back( entry );
            }

            return true;
        } catch ( ... ) {
            return false;
        }
    }
} // namespace ida_re::utils
