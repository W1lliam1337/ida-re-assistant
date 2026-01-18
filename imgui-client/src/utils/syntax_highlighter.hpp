#pragma once

#include <imgui.h>

namespace ida_re::utils {
    class c_syntax_highlighter {
      public:
        c_syntax_highlighter( );

        // Render text with syntax highlighting
        void render_text( const std::string &text, bool show_line_numbers = true );
        void render_assembly( const std::string &text, bool show_line_numbers = true );
        void render_markdown( const std::string &text );

        // Color scheme
        struct color_scheme_t {
            ImVec4 text_default { };
            ImVec4 keyword { };
            ImVec4 type { };
            ImVec4 comment { };
            ImVec4 string { };
            ImVec4 number { };
            ImVec4 preprocessor { };
            ImVec4 function { };
            ImVec4 line_number { };
            ImVec4 line_number_bg { };
        };

        void set_color_scheme( const color_scheme_t &scheme ) noexcept {
            m_colors = scheme;
        }

        [[nodiscard]] const color_scheme_t &get_color_scheme( ) const noexcept {
            return m_colors;
        }

        // Predefined themes
        [[nodiscard]] static color_scheme_t ios_dark_theme( );
        [[nodiscard]] static color_scheme_t ios_light_theme( );
        [[nodiscard]] static color_scheme_t vaporwave_theme( );

        void set_dark_mode( bool dark ) {
            m_dark_mode = dark;
            set_color_scheme( dark ? ios_dark_theme( ) : ios_light_theme( ) );
        }

        [[nodiscard]] bool is_dark_mode( ) const noexcept {
            return m_dark_mode;
        }

      private:
        enum class e_token_type {
            e_default,
            e_keyword,
            e_type,
            e_comment,
            e_string,
            e_number,
            e_preprocessor,
            e_function
        };

        struct token_t {
            e_token_type m_type { };
            std::string  m_text { };
        };

        std::vector< token_t > tokenize_line( std::string_view line );
        bool                   is_keyword( std::string_view word );
        bool                   is_type( std::string_view word );
        bool                   is_number( std::string_view word );

        color_scheme_t                    m_colors { };
        std::unordered_set< std::string > m_keywords { };
        std::unordered_set< std::string > m_types { };
        bool                              m_dark_mode { true };
    };

} // namespace ida_re::utils
