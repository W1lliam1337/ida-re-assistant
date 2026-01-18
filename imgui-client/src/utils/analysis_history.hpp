#pragma once

namespace ida_re::utils {
    struct analysis_entry_t {
        std::string                           m_function_address { };
        std::string                           m_function_name { };
        std::string                           m_analysis_type { }; // "general", "vulnerability", "name_suggestion"
        std::string                           m_result { };
        std::chrono::system_clock::time_point m_timestamp { };
        std::string                           m_provider { }; // "Claude", "OpenAI", "Gemini"

        std::string get_formatted_time( ) const;
    };

    class c_analysis_history {
      public:
        c_analysis_history( );

        // Add new entry
        void add_entry( const analysis_entry_t &entry );

        // Get all entries
        [[nodiscard]] const std::vector< analysis_entry_t > &get_entries( ) const noexcept {
            return m_entries;
        }

        // Get entries for specific function
        [[nodiscard]] std::vector< analysis_entry_t > get_entries_for_function( std::string_view address ) const;

        // Clear all history
        void clear( ) noexcept {
            m_entries.clear( );
        }

        // Persistence
        bool load_from_file( std::string_view filepath );
        bool save_to_file( std::string_view filepath ) const;

        // Get default history file path
        [[nodiscard]] static std::string get_default_history_path( );

      private:
        std::vector< analysis_entry_t > m_entries { };
        const size_t                    m_max_entries { 1000 }; // Limit history size
    };

} // namespace ida_re::utils
