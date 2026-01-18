#pragma once

namespace ida_re::api {
    struct mcp_tool_result_t {
        bool        m_success { false };
        json_t      m_data { };
        std::string m_error { };
    };

    struct mcp_tool_t {
        std::string m_name { };
        std::string m_description { };
        json_t      m_input_schema { };
    };

    // HTTP-based MCP client that connects to IDA plugin's HTTP server
    class c_mcp_client {
      public:
        c_mcp_client( )  = default;
        ~c_mcp_client( ) = default;

        void set_host( std::string_view host ) {
            m_host = host;
        }

        void set_port( int port ) noexcept {
            m_port = port;
        }

        [[nodiscard]] bool connect( );

        void disconnect( ) noexcept {
            m_connected.store( false, std::memory_order_release );
        }

        [[nodiscard]] bool is_connected( ) const noexcept {
            return m_connected.load( std::memory_order_acquire );
        }

        std::vector< mcp_tool_t > list_tools( );
        mcp_tool_result_t         call_tool( const std::string &name, const json_t &arguments );

        // convenience methods
        mcp_tool_result_t get_function_pseudocode( std::string_view address );
        mcp_tool_result_t get_function_assembly( std::string_view address );
        mcp_tool_result_t get_function_xrefs( std::string_view address );
        mcp_tool_result_t analyze_function( std::string_view address );
        mcp_tool_result_t list_functions( int limit = 100 );
        mcp_tool_result_t get_current_function( );
        mcp_tool_result_t rename_function( std::string_view address, std::string_view new_name );
        mcp_tool_result_t add_comment( std::string_view address, std::string_view comment, bool repeatable = false );
        mcp_tool_result_t get_database_info( );
        mcp_tool_result_t rename_local_variable( std::string_view address, std::string_view old_name, std::string_view new_name );
        mcp_tool_result_t set_variable_type( std::string_view address, std::string_view var_name, std::string_view type_str );
        mcp_tool_result_t add_function_comment( std::string_view address, std::string_view comment,
                                                std::optional< int > line_number = std::nullopt );
        mcp_tool_result_t get_function_local_variables( std::string_view address );

        [[nodiscard]] std::string_view get_last_error( ) const noexcept {
            return m_last_error;
        }

      private:
        json_t http_get( const std::string &path );
        json_t http_post( const std::string &path, const json_t &data );

        std::string         m_host { "127.0.0.1" };
        int                 m_port { 13120 };
        std::atomic< bool > m_connected { false };
        std::mutex          m_mutex { };
        std::string         m_last_error { };
    };

} // namespace ida_re::api
