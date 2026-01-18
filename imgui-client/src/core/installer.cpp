#include "vendor.hpp"

#include "installer.hpp"

#ifdef IDA_RE_PLATFORM_WINDOWS
#include <winreg.h>
#endif

namespace ida_re::core {
    c_plugin_installer::c_plugin_installer( ) { }

    std::filesystem::path c_plugin_installer::get_plugin_dir( ) {
#ifdef IDA_RE_PLATFORM_WINDOWS
        char appdata[ MAX_PATH ];
        if ( SUCCEEDED( SHGetFolderPathA( NULL, CSIDL_APPDATA, NULL, 0, appdata ) ) ) {
            return std::filesystem::path( appdata ) / "Hex-Rays" / "IDA Pro" / "plugins";
        }
        return "";
#else
        const char *home = std::getenv( "HOME" );
        if ( !home ) {
            struct passwd *pw = getpwuid( getuid( ) );
            home              = pw ? pw->pw_dir : "/tmp";
        }
        return std::filesystem::path( home ) / ".idapro" / "plugins";
#endif
    }

#ifdef IDA_RE_PLATFORM_WINDOWS
    std::vector< std::filesystem::path > c_plugin_installer::find_ida_from_registry( ) {
        std::vector< std::filesystem::path > paths;

        auto query_reg_path = []( HKEY root, const char *subkey, const char *value ) -> std::optional< std::filesystem::path > {
            HKEY hKey;
            if ( RegOpenKeyExA( root, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey ) != ERROR_SUCCESS ) {
                if ( RegOpenKeyExA( root, subkey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey ) != ERROR_SUCCESS ) {
                    return std::nullopt;
                }
            }

            char  buffer[ MAX_PATH ];
            DWORD size = sizeof( buffer );
            DWORD type;

            if ( RegQueryValueExA( hKey, value, NULL, &type, ( LPBYTE ) buffer, &size ) == ERROR_SUCCESS ) {
                RegCloseKey( hKey );
                if ( type == REG_SZ ) {
                    return std::filesystem::path( buffer );
                }
            }
            RegCloseKey( hKey );
            return std::nullopt;
        };

        std::vector< std::string > versions = { "9.1", "9.0", "8.4", "8.3" };
        for ( const auto &v : versions ) {
            std::string key  = "SOFTWARE\\Hex-Rays\\IDA Pro " + v;
            auto        path = query_reg_path( HKEY_LOCAL_MACHINE, key.c_str( ), "InstallDir" );
            if ( path && std::filesystem::exists( *path ) ) {
                paths.push_back( *path );
            }

            path = query_reg_path( HKEY_CURRENT_USER, key.c_str( ), "InstallDir" );
            if ( path && std::filesystem::exists( *path ) ) {
                paths.push_back( *path );
            }
        }

        // .idb file association
        auto idb_path = query_reg_path( HKEY_CLASSES_ROOT, "idb_auto_file\\shell\\open\\command", "" );
        if ( idb_path ) {
            std::string path_str = idb_path->string( );
            auto        pos      = path_str.find( ".exe" );
            if ( pos != std::string::npos ) {
                size_t                start    = ( path_str[ 0 ] == '"' ) ? 1 : 0;
                std::string           exe_path = path_str.substr( start, pos + 4 - start );
                std::filesystem::path ida_dir  = std::filesystem::path( exe_path ).parent_path( );
                if ( std::filesystem::exists( ida_dir ) ) {
                    paths.push_back( ida_dir );
                }
            }
        }

        // .i64 file association
        auto i64_path = query_reg_path( HKEY_CLASSES_ROOT, "i64_auto_file\\shell\\open\\command", "" );
        if ( i64_path ) {
            std::string path_str = i64_path->string( );
            auto        pos      = path_str.find( ".exe" );
            if ( pos != std::string::npos ) {
                size_t                start    = ( path_str[ 0 ] == '"' ) ? 1 : 0;
                std::string           exe_path = path_str.substr( start, pos + 4 - start );
                std::filesystem::path ida_dir  = std::filesystem::path( exe_path ).parent_path( );
                if ( std::filesystem::exists( ida_dir ) ) {
                    paths.push_back( ida_dir );
                }
            }
        }

        // App Paths
        auto app_path = query_reg_path( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ida64.exe", "Path" );
        if ( app_path && std::filesystem::exists( *app_path ) ) {
            paths.push_back( *app_path );
        }

        return paths;
    }
#endif

    std::vector< std::filesystem::path > c_plugin_installer::get_ida_search_paths( ) {
        std::vector< std::filesystem::path > paths;

#ifdef IDA_RE_PLATFORM_WINDOWS
        auto reg_paths = find_ida_from_registry( );
        paths.insert( paths.end( ), reg_paths.begin( ), reg_paths.end( ) );

        std::vector< std::string > versions = { "9.1", "9.0", "8.4", "8.3" };
        for ( const auto &v : versions ) {
            paths.push_back( std::filesystem::path( "C:\\Program Files\\IDA Pro " + v ) );
            paths.push_back( std::filesystem::path( "C:\\Program Files (x86)\\IDA Pro " + v ) );
            paths.push_back( std::filesystem::path( "C:\\IDA Pro " + v ) );
            paths.push_back( std::filesystem::path( "D:\\IDA Pro " + v ) );
        }

        const char *userprofile = std::getenv( "USERPROFILE" );
        if ( userprofile ) {
            for ( const auto &v : versions ) {
                paths.push_back( std::filesystem::path( userprofile ) / "IDA Pro" / v );
                paths.push_back( std::filesystem::path( userprofile ) / ( "IDA_Pro_" + v ) );
            }
        }
#elif defined( IDA_RE_PLATFORM_MACOS )
        std::vector< std::string > versions = { "9.1", "9.0", "8.4", "8.3" };
        for ( const auto &v : versions ) {
            paths.push_back( std::filesystem::path( "/Applications/IDA Pro " + v ) );
            paths.push_back( std::filesystem::path( "/Applications/IDA Pro " + v + ".app" ) );
        }
#else
        std::vector< std::string > versions = { "9.1", "9.0", "8.4", "8.3" };
        for ( const auto &v : versions ) {
            paths.push_back( std::filesystem::path( "/opt/ida-pro-" + v ) );
            paths.push_back( std::filesystem::path( "/opt/ida-" + v ) );
            paths.push_back( std::filesystem::path( "/opt/idapro-" + v ) );
            paths.push_back( std::filesystem::path( "/usr/local/ida-" + v ) );

            const char *home = std::getenv( "HOME" );
            if ( home ) {
                paths.push_back( std::filesystem::path( home ) / ( "ida-" + v ) );
                paths.push_back( std::filesystem::path( home ) / "ida" / v );
            }
        }
#endif

        return paths;
    }

    std::optional< std::filesystem::path > c_plugin_installer::find_ida_python( const std::filesystem::path &ida_dir ) {
        std::vector< std::filesystem::path > candidates;

#ifdef IDA_RE_PLATFORM_WINDOWS
        // IDA 8.3+ uses Python 3.10+, IDA 9.x uses Python 3.12
        candidates = {
            ida_dir / "python312" / "python.exe",
            ida_dir / "python311" / "python.exe",
            ida_dir / "python310" / "python.exe",
            ida_dir / "python3" / "python.exe",
        };
#elif defined( IDA_RE_PLATFORM_MACOS )
        candidates = {
            ida_dir / "ida64.app" / "Contents" / "MacOS" / "python3",
            ida_dir / "Contents" / "MacOS" / "python3",
            ida_dir / "idabin" / "python3",
            ida_dir / "python3",
        };
#else
        candidates = {
            ida_dir / "python3",
            ida_dir / "idabin" / "python3",
        };
#endif

        auto it = std::ranges::find_if( candidates, []( const auto &c ) {
            return std::filesystem::exists( c );
        } );
        return it != candidates.end( ) ? std::optional { *it } : std::nullopt;
    }

    std::vector< ida_installation_t > c_plugin_installer::detect_ida_installations( ) {
        std::vector< ida_installation_t > installations;
        std::set< std::string >           seen_paths;

        auto search_paths = get_ida_search_paths( );

        for ( const auto &path : search_paths ) {
            if ( !std::filesystem::exists( path ) )
                continue;

            std::string canonical;
            try {
                canonical = std::filesystem::canonical( path ).string( );
            } catch ( ... ) {
                canonical = path.string( );
            }

            if ( seen_paths.count( canonical ) )
                continue;
            seen_paths.insert( canonical );

            auto python = find_ida_python( path );
            if ( !python )
                continue;

            ida_installation_t inst;
            inst.m_install_dir = path;
            inst.m_python_path = *python;
            inst.m_plugin_dir  = get_plugin_dir( );

            std::string path_str = path.string( );
            std::regex  ver_regex( R"((\d+\.\d+))" );
            std::smatch match;
            if ( std::regex_search( path_str, match, ver_regex ) ) {
                inst.m_version = match[ 1 ].str( );
            } else {
                inst.m_version = "unknown";
            }

            installations.push_back( inst );
        }

        std::sort( installations.begin( ), installations.end( ), []( const auto &a, const auto &b ) {
            return a.m_version > b.m_version;
        } );

        return installations;
    }

    bool c_plugin_installer::install_plugin( const ida_installation_t &ida, const std::filesystem::path &plugin_source ) {
        if ( !std::filesystem::exists( plugin_source ) ) {
            m_last_error = "Plugin source not found: " + plugin_source.string( );
            return false;
        }

        try {
            std::filesystem::create_directories( ida.m_plugin_dir );
            std::filesystem::path dest = ida.m_plugin_dir / "ida_mcp_plugin.py";
            std::filesystem::copy_file( plugin_source, dest, std::filesystem::copy_options::overwrite_existing );
            return true;
        } catch ( const std::exception &e ) {
            m_last_error = e.what( );
            return false;
        }
    }

    bool c_plugin_installer::install_mcp_package( const ida_installation_t &ida ) {
        std::string cmd = "\"" + ida.m_python_path.string( ) + "\" -m pip install mcp";

#ifdef IDA_RE_PLATFORM_WINDOWS
        cmd += " > NUL 2>&1";
#else
        cmd += " > /dev/null 2>&1";
#endif

        int result = std::system( cmd.c_str( ) );
        if ( result != 0 ) {
            m_last_error = "pip install failed with code " + std::to_string( result );
            return false;
        }
        return true;
    }

    std::string c_plugin_installer::generate_claude_config( const ida_installation_t &ida ) {
        std::filesystem::path plugin_path = ida.m_plugin_dir / "ida_mcp_plugin.py";

        std::string python_str = ida.m_python_path.string( );
        std::string plugin_str = plugin_path.string( );

        auto escape = []( std::string s ) {
            std::string result;
            for ( char c : s ) {
                if ( c == '\\' )
                    result += '/';
                else
                    result += c;
            }
            return result;
        };

        std::string config = R"({
  "mcpServers": {
    "ida-re-assistant": {
      "command": ")" + escape( python_str )
                           + R"(",
      "args": ["-u", ")" + escape( plugin_str )
                           + R"("]
    }
  }
})";

        return config;
    }
} // namespace ida_re::core
