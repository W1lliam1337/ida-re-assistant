#include "vendor.hpp"

#include "mcp_client.hpp"
#include <httplib.h>

namespace ida_re::api {
    bool c_mcp_client::connect( ) {
        std::lock_guard< std::mutex > lock( m_mutex );

        httplib::Client client( m_host, m_port );
        client.set_connection_timeout( 5 );
        client.set_read_timeout( 5 );

        auto res = client.Get( "/health" );
        if ( !res ) {
            m_last_error = "Cannot connect to IDA MCP server at " + m_host + ":" + std::to_string( m_port );
            m_connected  = false;
            return false;
        }

        if ( res->status != 200 ) {
            m_last_error = "Server returned status " + std::to_string( res->status );
            m_connected  = false;
            return false;
        }

        try {
            auto j = json_t::parse( res->body );
            if ( j.value( "status", "" ) == "ok" ) {
                m_connected = true;
                return true;
            }
        } catch ( ... ) { }

        m_last_error = "Invalid response from server";
        m_connected  = false;
        return false;
    }

    json_t c_mcp_client::http_get( const std::string &path ) {
        std::lock_guard< std::mutex > lock( m_mutex );

        httplib::Client client( m_host, m_port );
        client.set_connection_timeout( 10 );
        client.set_read_timeout( 30 );

        auto res = client.Get( path );
        if ( !res ) {
            m_last_error = "HTTP GET failed";
            m_connected  = false;
            return nullptr;
        }

        if ( res->status != 200 ) {
            m_last_error = "HTTP " + std::to_string( res->status );
            return nullptr;
        }

        try {
            return json_t::parse( res->body );
        } catch ( ... ) {
            m_last_error = "JSON parse error";
            return nullptr;
        }
    }

    json_t c_mcp_client::http_post( const std::string &path, const json_t &data ) {
        std::lock_guard< std::mutex > lock( m_mutex );

        httplib::Client client( m_host, m_port );
        client.set_connection_timeout( 10 );
        client.set_read_timeout( 60 );

        auto res = client.Post( path, data.dump( ), "application/json_t" );
        if ( !res ) {
            m_last_error = "HTTP POST failed";
            m_connected  = false;
            return nullptr;
        }

        if ( res->status != 200 ) {
            m_last_error = "HTTP " + std::to_string( res->status );
            return nullptr;
        }

        try {
            return json_t::parse( res->body );
        } catch ( ... ) {
            m_last_error = "JSON parse error";
            return nullptr;
        }
    }

    std::vector< mcp_tool_t > c_mcp_client::list_tools( ) {
        std::vector< mcp_tool_t > tools;

        auto resp = http_get( "/tools" );
        if ( resp.is_null( ) || !resp.contains( "tools" ) ) {
            return tools;
        }

        for ( auto &t : resp[ "tools" ] ) {
            mcp_tool_t tool;
            tool.m_name        = t.value( "name", "" );
            tool.m_description = t.value( "description", "" );
            if ( t.contains( "inputSchema" ) ) {
                tool.m_input_schema = t[ "inputSchema" ];
            }
            tools.push_back( tool );
        }

        return tools;
    }

    mcp_tool_result_t c_mcp_client::call_tool( const std::string &name, const json_t &arguments ) {
        mcp_tool_result_t result;

        json_t req = {
            {      "name",      name },
            { "arguments", arguments }
        };

        auto resp = http_post( "/call", req );

        if ( resp.is_null( ) ) {
            result.m_success = false;
            result.m_error   = m_last_error;
            return result;
        }

        if ( resp.contains( "error" ) ) {
            result.m_success = false;
            result.m_error   = resp[ "error" ].get< std::string >( );
            return result;
        }

        if ( resp.contains( "result" ) ) {
            result.m_data    = resp[ "result" ];
            result.m_success = !result.m_data.contains( "error" );
            if ( !result.m_success ) {
                result.m_error = result.m_data.value( "error", "Unknown error" );
            }
        } else {
            result.m_success = false;
            result.m_error   = "No result in response";
        }

        return result;
    }

    mcp_tool_result_t c_mcp_client::get_function_pseudocode( std::string_view address ) {
        return call_tool( "get_function_pseudocode", {
                                                         { "address", address }
        } );
    }

    mcp_tool_result_t c_mcp_client::get_function_assembly( std::string_view address ) {
        return call_tool( "get_function_assembly", {
                                                       { "address", address }
        } );
    }

    mcp_tool_result_t c_mcp_client::get_function_xrefs( std::string_view address ) {
        return call_tool( "get_function_xrefs", {
                                                    { "address", address }
        } );
    }

    mcp_tool_result_t c_mcp_client::analyze_function( std::string_view address ) {
        return call_tool( "analyze_function", {
                                                  { "address", address }
        } );
    }

    mcp_tool_result_t c_mcp_client::list_functions( int limit ) {
        return call_tool( "list_functions", {
                                                { "limit", limit }
        } );
    }

    mcp_tool_result_t c_mcp_client::get_current_function( ) {
        return call_tool( "get_current_function", { } );
    }

    mcp_tool_result_t c_mcp_client::rename_function( std::string_view address, std::string_view new_name ) {
        return call_tool( "rename_function", {
                                                 {  "address",  address },
                                                 { "new_name", new_name }
        } );
    }

    mcp_tool_result_t c_mcp_client::add_comment( std::string_view address, std::string_view comment, bool repeatable ) {
        return call_tool( "add_comment", {
                                             {    "address",    address },
                                             {    "comment",    comment },
                                             { "repeatable", repeatable }
        } );
    }

    mcp_tool_result_t c_mcp_client::get_database_info( ) {
        return call_tool( "get_database_info", { } );
    }

    mcp_tool_result_t c_mcp_client::rename_local_variable( std::string_view address, std::string_view old_name,
                                                           std::string_view new_name ) {
        return call_tool( "rename_local_variable", {
                                                       {  "address",  address },
                                                       { "old_name", old_name },
                                                       { "new_name", new_name }
        } );
    }

    mcp_tool_result_t c_mcp_client::set_variable_type( std::string_view address, std::string_view var_name, std::string_view type_str ) {
        return call_tool( "set_variable_type", {
                                                   {  "address",  address },
                                                   { "var_name", var_name },
                                                   { "type_str", type_str }
        } );
    }

    mcp_tool_result_t c_mcp_client::add_function_comment( std::string_view address, std::string_view comment,
                                                          std::optional< int > line_number ) {
        json_t args = {
            { "address", address },
            { "comment", comment }
        };

        if ( line_number.has_value( ) ) {
            args[ "line_number" ] = line_number.value( );
        }

        return call_tool( "add_function_comment", args );
    }

    mcp_tool_result_t c_mcp_client::get_function_local_variables( std::string_view address ) {
        return call_tool( "get_function_local_variables", {
                                                              { "address", address }
        } );
    }
} // namespace ida_re::api
