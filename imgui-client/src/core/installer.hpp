#pragma once

namespace ida_re::core {
    struct ida_installation_t {
        std::filesystem::path m_install_dir { };
        std::filesystem::path m_python_path { };
        std::filesystem::path m_plugin_dir { };
        std::string           m_version { };
    };

    class c_plugin_installer {
      public:
        c_plugin_installer( );

        std::vector< ida_installation_t > detect_ida_installations( );
        bool                              install_plugin( const ida_installation_t &ida, const std::filesystem::path &plugin_source );
        bool                              install_mcp_package( const ida_installation_t &ida );
        std::string                       generate_claude_config( const ida_installation_t &ida );

        [[nodiscard]] std::string_view get_last_error( ) const noexcept {
            return m_last_error;
        }

      private:
        std::filesystem::path                  get_plugin_dir( );
        std::vector< std::filesystem::path >   get_ida_search_paths( );
        std::optional< std::filesystem::path > find_ida_python( const std::filesystem::path &ida_dir );
#ifdef IDA_RE_PLATFORM_WINDOWS
        std::vector< std::filesystem::path > find_ida_from_registry( );
#endif

        std::string m_last_error { };
    };

} // namespace ida_re::core
