#pragma once

namespace ida_re::core {
    struct app_config_t {
        // LLM settings
        std::string m_provider { "gemini" }; // claude, openai, gemini
        std::string m_claude_api_key { };
        std::string m_openai_api_key { };
        std::string m_gemini_api_key { };
        std::string m_model { "gemini-2.0-flash-exp" };
        int         m_max_tokens { 4096 };

        // IDA MCP settings
        std::string m_mcp_host { "127.0.0.1" };
        int         m_mcp_port { 13120 };

        // UI settings
        bool  m_auto_connect { false };
        float m_ui_scale { 1.0f };

        // Cache settings
        bool m_enable_cache { true };

        [[nodiscard]] static std::filesystem::path get_config_dir( ) {
#ifdef IDA_RE_PLATFORM_WINDOWS
            if ( const char *appdata = std::getenv( "APPDATA" ); appdata ) {
                return std::filesystem::path( appdata ) / "ida-re-assistant";
            }
            return ".";
#else
            if ( const char *home = std::getenv( "HOME" ); home ) {
                return std::filesystem::path( home ) / ".config" / "ida-re-assistant";
            }
            return ".";
#endif
        }

        [[nodiscard]] static std::filesystem::path get_config_path( ) {
            return get_config_dir( ) / "config.json";
        }

        [[nodiscard]] static std::filesystem::path get_cache_path( ) {
            return get_config_dir( ) / "analysis_cache.json";
        }

        bool load( ) {
            auto path = get_config_path( );
            if ( !std::filesystem::exists( path ) )
                return false;

            try {
                std::ifstream f( path );
                json_t        j = json_t::parse( f );

                m_provider       = j.value( "provider", "gemini" );
                m_claude_api_key = j.value( "claude_api_key", j.value( "api_key", "" ) ); // backward compat
                m_openai_api_key = j.value( "openai_api_key", "" );
                m_gemini_api_key = j.value( "gemini_api_key", "" );
                m_model          = j.value( "model", "gemini-2.0-flash-exp" );
                m_max_tokens     = j.value( "max_tokens", 4096 );
                m_mcp_host       = j.value( "mcp_host", "127.0.0.1" );
                m_mcp_port       = j.value( "mcp_port", 13120 );
                m_auto_connect   = j.value( "auto_connect", false );
                m_ui_scale       = j.value( "ui_scale", 1.0f );
                m_enable_cache   = j.value( "enable_cache", true );

                return true;
            } catch ( ... ) {
                return false;
            }
        }

        bool save( ) {
            auto path = get_config_path( );

            try {
                std::filesystem::create_directories( path.parent_path( ) );

                json_t j = {
                    {       "provider",       m_provider },
                    { "claude_api_key", m_claude_api_key },
                    { "openai_api_key", m_openai_api_key },
                    { "gemini_api_key", m_gemini_api_key },
                    {          "model",          m_model },
                    {     "max_tokens",     m_max_tokens },
                    {       "mcp_host",       m_mcp_host },
                    {       "mcp_port",       m_mcp_port },
                    {   "auto_connect",   m_auto_connect },
                    {       "ui_scale",       m_ui_scale },
                    {   "enable_cache",   m_enable_cache }
                };

                std::ofstream f( path );
                f << j.dump( 2 );
                return true;
            } catch ( ... ) {
                return false;
            }
        }
    };

} // namespace ida_re::core
