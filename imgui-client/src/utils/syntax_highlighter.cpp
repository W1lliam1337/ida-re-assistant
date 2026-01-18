#include "vendor.hpp"

#include "syntax_highlighter.hpp"

namespace ida_re::utils {
    c_syntax_highlighter::c_syntax_highlighter( ) {
        // C/C++ keywords
        m_keywords = { "if",       "else",      "while",  "for",      "do",       "switch",  "case",     "default", "break",     "continue",
                       "return",   "goto",      "struct", "union",    "enum",     "typedef", "sizeof",   "const",   "volatile",  "static",
                       "extern",   "register",  "auto",   "inline",   "restrict", "class",   "public",   "private", "protected", "virtual",
                       "override", "namespace", "using",  "template", "typename", "new",     "delete",   "this",    "nullptr",   "true",
                       "false",    "try",       "catch",  "throw",    "operator", "friend",  "explicit", "mutable", "constexpr" };

        // C/C++ types
        m_types = { "void",     "int",       "char",     "short",     "long",   "float",   "double",  "unsigned", "signed",
                    "bool",     "wchar_t",   "size_t",   "ptrdiff_t", "int8_t", "int16_t", "int32_t", "int64_t",  "uint8_t",
                    "uint16_t", "uint32_t",  "uint64_t", "BYTE",      "WORD",   "DWORD",   "QWORD",   "BOOL",     "HANDLE",
                    "HWND",     "HINSTANCE", "LPVOID",   "LPCSTR",    "LPSTR",  "LPCWSTR", "LPWSTR" };

        // Default to iOS dark theme
        m_colors = ios_dark_theme( );
    }

    c_syntax_highlighter::color_scheme_t c_syntax_highlighter::ios_dark_theme( ) {
        return {
            ImVec4( 0.95f, 0.95f, 0.97f, 1.0f ), // text_default - white
            ImVec4( 1.00f, 0.42f, 0.77f, 1.0f ), // keyword - pink
            ImVec4( 0.42f, 0.77f, 1.00f, 1.0f ), // type - blue
            ImVec4( 0.50f, 0.60f, 0.55f, 1.0f ), // comment - muted green
            ImVec4( 1.00f, 0.58f, 0.42f, 1.0f ), // string - orange
            ImVec4( 0.85f, 0.85f, 0.42f, 1.0f ), // number - yellow
            ImVec4( 1.00f, 0.62f, 0.40f, 1.0f ), // preprocessor - orange
            ImVec4( 0.42f, 0.90f, 0.77f, 1.0f ), // function - cyan
            ImVec4( 0.50f, 0.50f, 0.55f, 1.0f ), // line_number - gray
            ImVec4( 0.05f, 0.05f, 0.08f, 0.3f )  // line_number_bg - dark translucent
        };
    }

    c_syntax_highlighter::color_scheme_t c_syntax_highlighter::ios_light_theme( ) {
        return {
            ImVec4( 0.10f, 0.10f, 0.10f, 1.0f ), // text_default - dark text
            ImVec4( 0.69f, 0.13f, 0.53f, 1.0f ), // keyword - magenta/purple
            ImVec4( 0.11f, 0.40f, 0.65f, 1.0f ), // type - blue
            ImVec4( 0.40f, 0.50f, 0.40f, 1.0f ), // comment - muted green
            ImVec4( 0.77f, 0.22f, 0.17f, 1.0f ), // string - red/brown
            ImVec4( 0.10f, 0.40f, 0.80f, 1.0f ), // number - blue
            ImVec4( 0.50f, 0.35f, 0.15f, 1.0f ), // preprocessor - brown
            ImVec4( 0.30f, 0.30f, 0.70f, 1.0f ), // function - purple/blue
            ImVec4( 0.55f, 0.55f, 0.55f, 1.0f ), // line_number - gray
            ImVec4( 0.92f, 0.92f, 0.92f, 0.5f )  // line_number_bg - light translucent
        };
    }

    c_syntax_highlighter::color_scheme_t c_syntax_highlighter::vaporwave_theme( ) {
        return {
            ImVec4( 0.90f, 0.85f, 0.95f, 1.0f ), // text_default
            ImVec4( 0.90f, 0.40f, 1.00f, 1.0f ), // keyword - purple
            ImVec4( 0.40f, 0.90f, 1.00f, 1.0f ), // type - cyan
            ImVec4( 0.60f, 0.60f, 0.70f, 1.0f ), // comment
            ImVec4( 1.00f, 0.70f, 0.80f, 1.0f ), // string - pink
            ImVec4( 0.00f, 0.90f, 0.90f, 1.0f ), // number - cyan
            ImVec4( 1.00f, 0.60f, 0.70f, 1.0f ), // preprocessor
            ImVec4( 0.70f, 0.30f, 0.90f, 1.0f ), // function - purple
            ImVec4( 0.50f, 0.45f, 0.55f, 1.0f ), // line_number
            ImVec4( 0.10f, 0.05f, 0.15f, 0.3f )  // line_number_bg
        };
    }

    bool c_syntax_highlighter::is_keyword( std::string_view word ) {
        return m_keywords.find( std::string( word ) ) != m_keywords.end( );
    }

    bool c_syntax_highlighter::is_type( std::string_view word ) {
        return m_types.find( std::string( word ) ) != m_types.end( );
    }

    bool c_syntax_highlighter::is_number( std::string_view word ) {
        if ( word.empty( ) )
            return false;

        // Check for hex (0x...)
        if ( word.size( ) > 2 && word[ 0 ] == '0' && ( word[ 1 ] == 'x' || word[ 1 ] == 'X' ) ) {
            for ( size_t i = 2; i < word.size( ); ++i ) {
                if ( !std::isxdigit( word[ i ] ) )
                    return false;
            }
            return true;
        }

        // Check for decimal
        bool has_digit = false;
        for ( char c : word ) {
            if ( std::isdigit( c ) ) {
                has_digit = true;
            } else if ( c != '.' && c != 'f' && c != 'F' && c != 'l' && c != 'L' && c != 'u' && c != 'U' ) {
                return false;
            }
        }
        return has_digit;
    }

    std::vector< c_syntax_highlighter::token_t > c_syntax_highlighter::tokenize_line( std::string_view line ) {
        std::vector< token_t > tokens;
        size_t                 i = 0;

        while ( i < line.size( ) ) {
            // Skip whitespace but preserve it
            if ( std::isspace( line[ i ] ) ) {
                size_t start = i;
                while ( i < line.size( ) && std::isspace( line[ i ] ) )
                    i++;
                tokens.push_back( { e_token_type::e_default, std::string( line.substr( start, i - start ) ) } );
                continue;
            }

            // Comments
            if ( i + 1 < line.size( ) && line[ i ] == '/' && line[ i + 1 ] == '/' ) {
                tokens.push_back( { e_token_type::e_comment, std::string( line.substr( i ) ) } );
                break;
            }

            if ( i + 1 < line.size( ) && line[ i ] == '/' && line[ i + 1 ] == '*' ) {
                size_t start  = i;
                i            += 2;
                while ( i + 1 < line.size( ) && !( line[ i ] == '*' && line[ i + 1 ] == '/' ) )
                    i++;
                if ( i + 1 < line.size( ) )
                    i += 2;
                tokens.push_back( { e_token_type::e_comment, std::string( line.substr( start, i - start ) ) } );
                continue;
            }

            // Preprocessor
            if ( line[ i ] == '#' ) {
                tokens.push_back( { e_token_type::e_preprocessor, std::string( line.substr( i ) ) } );
                break;
            }

            // Strings
            if ( line[ i ] == '"' || line[ i ] == '\'' ) {
                char   quote = line[ i ];
                size_t start = i;
                i++;
                while ( i < line.size( ) && line[ i ] != quote ) {
                    if ( line[ i ] == '\\' && i + 1 < line.size( ) )
                        i++;
                    i++;
                }
                if ( i < line.size( ) )
                    i++;
                tokens.push_back( { e_token_type::e_string, std::string( line.substr( start, i - start ) ) } );
                continue;
            }

            // Identifiers and keywords
            if ( std::isalpha( line[ i ] ) || line[ i ] == '_' ) {
                size_t start = i;
                while ( i < line.size( ) && ( std::isalnum( line[ i ] ) || line[ i ] == '_' ) )
                    i++;
                std::string word = std::string( line.substr( start, i - start ) );

                e_token_type type = e_token_type::e_default;
                if ( is_keyword( word ) ) {
                    type = e_token_type::e_keyword;
                } else if ( is_type( word ) ) {
                    type = e_token_type::e_type;
                } else {
                    // Check if followed by '(' - likely a function
                    size_t j = i;
                    while ( j < line.size( ) && std::isspace( line[ j ] ) )
                        j++;
                    if ( j < line.size( ) && line[ j ] == '(' ) {
                        type = e_token_type::e_function;
                    }
                }

                tokens.push_back( { type, word } );
                continue;
            }

            // Numbers
            if ( std::isdigit( line[ i ] )
                 || ( line[ i ] == '0' && i + 1 < line.size( ) && ( line[ i + 1 ] == 'x' || line[ i + 1 ] == 'X' ) ) ) {
                size_t start = i;
                if ( line[ i ] == '0' && i + 1 < line.size( ) && ( line[ i + 1 ] == 'x' || line[ i + 1 ] == 'X' ) ) {
                    i += 2;
                    while ( i < line.size( ) && std::isxdigit( line[ i ] ) )
                        i++;
                } else {
                    while ( i < line.size( ) && ( std::isdigit( line[ i ] ) || line[ i ] == '.' ) )
                        i++;
                    if ( i < line.size( ) && ( line[ i ] == 'f' || line[ i ] == 'F' || line[ i ] == 'l' || line[ i ] == 'L' ) )
                        i++;
                }
                tokens.push_back( { e_token_type::e_number, std::string( line.substr( start, i - start ) ) } );
                continue;
            }

            // Other characters (operators, punctuation)
            tokens.push_back( { e_token_type::e_default, std::string( 1, line[ i ] ) } );
            i++;
        }

        return tokens;
    }

    void c_syntax_highlighter::render_text( const std::string &text, bool show_line_numbers ) {
        std::istringstream stream( text );
        std::string        line;
        int                line_num = 1;

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );

        while ( std::getline( stream, line ) ) {
            if ( show_line_numbers ) {
                // Draw line number background
                ImVec2      pos         = ImGui::GetCursorScreenPos( );
                ImDrawList *draw_list   = ImGui::GetWindowDrawList( );
                float       line_height = ImGui::GetTextLineHeight( );

                char num_str[ 8 ];
                snprintf( num_str, sizeof( num_str ), "%4d", line_num );

                float num_width = ImGui::CalcTextSize( num_str ).x + 16;
                draw_list->AddRectFilled( pos, ImVec2( pos.x + num_width, pos.y + line_height ),
                                          ImGui::GetColorU32( m_colors.line_number_bg ) );

                // Draw line number
                ImGui::TextColored( m_colors.line_number, "%s", num_str );
                ImGui::SameLine( );
                ImGui::Text( "  " );
                ImGui::SameLine( );
            }

            // Tokenize and render line
            auto tokens = tokenize_line( line );
            for ( const auto &token : tokens ) {
                ImVec4 color;
                switch ( token.m_type ) {
                    case e_token_type::e_keyword :
                        color = m_colors.keyword;
                        break;
                    case e_token_type::e_type :
                        color = m_colors.type;
                        break;
                    case e_token_type::e_comment :
                        color = m_colors.comment;
                        break;
                    case e_token_type::e_string :
                        color = m_colors.string;
                        break;
                    case e_token_type::e_number :
                        color = m_colors.number;
                        break;
                    case e_token_type::e_preprocessor :
                        color = m_colors.preprocessor;
                        break;
                    case e_token_type::e_function :
                        color = m_colors.function;
                        break;
                    default :
                        color = m_colors.text_default;
                        break;
                }

                ImGui::TextColored( color, "%s", token.m_text.c_str( ) );
                ImGui::SameLine( );
            }

            ImGui::NewLine( );
            line_num++;
        }

        ImGui::PopStyleVar( );
    }

    void c_syntax_highlighter::render_assembly( const std::string &text, bool show_line_numbers ) {
        std::istringstream stream( text );
        std::string        line;
        int                line_num = 1;

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 2 ) );
        ImGui::PushFont( ImGui::GetIO( ).Fonts->Fonts[ 0 ] ); // Use monospace font

        while ( std::getline( stream, line ) ) {
            if ( show_line_numbers ) {
                ImGui::TextColored( m_colors.line_number, "%4d", line_num );
                ImGui::SameLine( );
            }

            // Check for comment
            size_t      comment_pos  = line.find( ';' );
            std::string code_part    = ( comment_pos != std::string::npos ) ? line.substr( 0, comment_pos ) : line;
            std::string comment_part = ( comment_pos != std::string::npos ) ? line.substr( comment_pos ) : "";

            // Render leading whitespace
            size_t first_non_space = code_part.find_first_not_of( " \t" );
            if ( first_non_space != std::string::npos && first_non_space > 0 ) {
                std::string spaces = code_part.substr( 0, first_non_space );
                ImGui::TextUnformatted( spaces.c_str( ), spaces.c_str( ) + spaces.size( ) );
                ImGui::SameLine( );
                code_part = code_part.substr( first_non_space );
            }

            // Parse code part manually to preserve spacing
            size_t pos        = 0;
            bool   first_word = true;

            while ( pos < code_part.size( ) ) {
                // Skip spaces
                size_t word_start = code_part.find_first_not_of( " \t,", pos );
                if ( word_start == std::string::npos )
                    break;

                // Find word end
                size_t word_end = code_part.find_first_of( " \t,;", word_start );
                if ( word_end == std::string::npos )
                    word_end = code_part.size( );

                std::string word  = code_part.substr( word_start, word_end - word_start );
                ImVec4      color = m_colors.text_default;

                if ( first_word ) {
                    color      = m_colors.keyword;
                    first_word = false;
                } else if ( word.find( "0x" ) == 0 || word.find( "0X" ) == 0 ) {
                    color = m_colors.number;
                } else if ( word.find( '[' ) != std::string::npos || word.find( ']' ) != std::string::npos ) {
                    color = m_colors.type;
                } else if ( word.find( "ptr" ) != std::string::npos ) {
                    color = m_colors.type;
                } else if ( !word.empty( )
                            && ( word[ 0 ] == 'r' || word[ 0 ] == 'e' || word.find( "sp" ) != std::string::npos
                                 || word.find( "bp" ) != std::string::npos ) ) {
                    color = ImVec4( 0.8f, 0.6f, 0.9f, 1.0f );
                }

                ImGui::TextColored( color, "%s", word.c_str( ) );
                ImGui::SameLine( );

                // Render spacing/punctuation between words
                if ( word_end < code_part.size( ) ) {
                    size_t punct_end = code_part.find_first_not_of( " \t,", word_end );
                    if ( punct_end == std::string::npos )
                        punct_end = code_part.size( );
                    std::string spacing = code_part.substr( word_end, punct_end - word_end );
                    if ( !spacing.empty( ) ) {
                        ImGui::TextUnformatted( spacing.c_str( ), spacing.c_str( ) + spacing.size( ) );
                        ImGui::SameLine( );
                    }
                    pos = punct_end;
                } else {
                    pos = word_end;
                }
            }

            // Render comment
            if ( !comment_part.empty( ) ) {
                ImGui::TextColored( m_colors.comment, "%s", comment_part.c_str( ) );
            }

            ImGui::NewLine( );
            line_num++;
        }

        ImGui::PopFont( );
        ImGui::PopStyleVar( );
    }

    void c_syntax_highlighter::render_markdown( const std::string &text ) {
        std::istringstream stream( text );
        std::string        line;
        bool               in_code_block = false;
        std::string        code_block_content;

        // Colors based on theme
        ImVec4 bold_color   = m_dark_mode ? ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) : ImVec4( 0.0f, 0.0f, 0.0f, 1.0f );
        ImVec4 italic_color = m_dark_mode ? ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ) : ImVec4( 0.2f, 0.2f, 0.2f, 1.0f );
        ImVec4 code_bg      = m_dark_mode ? ImVec4( 0.15f, 0.15f, 0.15f, 1.0f ) : ImVec4( 0.90f, 0.90f, 0.90f, 1.0f );

        while ( std::getline( stream, line ) ) {
            // Check for code block markers
            if ( line.find( "```" ) == 0 ) {
                if ( in_code_block ) {
                    // End of code block - render accumulated code
                    if ( !code_block_content.empty( ) ) {
                        ImGui::PushStyleColor( ImGuiCol_ChildBg, code_bg );
                        ImGui::BeginChild( "##code_block", ImVec2( 0, 0 ), true, ImGuiWindowFlags_AlwaysAutoResize );
                        render_text( code_block_content, false );
                        ImGui::EndChild( );
                        ImGui::PopStyleColor( );
                        code_block_content.clear( );
                    }
                    in_code_block = false;
                } else {
                    // Start of code block
                    in_code_block = true;
                }
                continue;
            }

            if ( in_code_block ) {
                // Accumulate code block lines
                code_block_content += line + "\n";
                continue;
            }

            // Parse inline markdown
            size_t pos           = 0;
            bool   first_element = true;
            while ( pos < line.length( ) ) {
                // Check for **bold**
                size_t bold_start = line.find( "**", pos );
                if ( bold_start != std::string::npos ) {
                    // Render text before bold
                    if ( bold_start > pos ) {
                        std::string before = line.substr( pos, bold_start - pos );
                        if ( !first_element )
                            ImGui::SameLine( 0, 0 );
                        ImGui::TextUnformatted( before.c_str( ), before.c_str( ) + before.size( ) );
                        first_element = false;
                    }

                    // Find end of bold
                    size_t bold_end = line.find( "**", bold_start + 2 );
                    if ( bold_end != std::string::npos ) {
                        std::string bold_text = line.substr( bold_start + 2, bold_end - bold_start - 2 );
                        if ( !first_element )
                            ImGui::SameLine( 0, 0 );
                        ImGui::PushStyleColor( ImGuiCol_Text, bold_color );
                        ImGui::TextUnformatted( bold_text.c_str( ), bold_text.c_str( ) + bold_text.size( ) );
                        ImGui::PopStyleColor( );
                        first_element = false;
                        pos           = bold_end + 2;
                        continue;
                    }
                }

                // Check for *italic*
                size_t italic_start = line.find( "*", pos );
                if ( italic_start != std::string::npos && italic_start != bold_start ) {
                    // Render text before italic
                    if ( italic_start > pos ) {
                        std::string before = line.substr( pos, italic_start - pos );
                        if ( !first_element )
                            ImGui::SameLine( 0, 0 );
                        ImGui::TextUnformatted( before.c_str( ), before.c_str( ) + before.size( ) );
                        first_element = false;
                    }

                    // Find end of italic
                    size_t italic_end = line.find( "*", italic_start + 1 );
                    if ( italic_end != std::string::npos ) {
                        std::string italic_text = line.substr( italic_start + 1, italic_end - italic_start - 1 );
                        if ( !first_element )
                            ImGui::SameLine( 0, 0 );
                        ImGui::PushStyleColor( ImGuiCol_Text, italic_color );
                        ImGui::TextUnformatted( italic_text.c_str( ), italic_text.c_str( ) + italic_text.size( ) );
                        ImGui::PopStyleColor( );
                        first_element = false;
                        pos           = italic_end + 1;
                        continue;
                    }
                }

                // Check for `inline code`
                size_t code_start = line.find( "`", pos );
                if ( code_start != std::string::npos ) {
                    // Render text before code
                    if ( code_start > pos ) {
                        std::string before = line.substr( pos, code_start - pos );
                        if ( !first_element )
                            ImGui::SameLine( 0, 0 );
                        ImGui::TextUnformatted( before.c_str( ), before.c_str( ) + before.size( ) );
                        first_element = false;
                    }

                    // Find end of code
                    size_t code_end = line.find( "`", code_start + 1 );
                    if ( code_end != std::string::npos ) {
                        std::string code_text = line.substr( code_start + 1, code_end - code_start - 1 );
                        if ( !first_element )
                            ImGui::SameLine( 0, 0 );
                        ImGui::PushStyleColor( ImGuiCol_Text, m_colors.number );
                        ImGui::TextUnformatted( code_text.c_str( ), code_text.c_str( ) + code_text.size( ) );
                        ImGui::PopStyleColor( );
                        first_element = false;
                        pos           = code_end + 1;
                        continue;
                    }
                }

                // No more special formatting, render rest of line
                if ( pos < line.length( ) ) {
                    std::string rest = line.substr( pos );
                    if ( !first_element )
                        ImGui::SameLine( 0, 0 );
                    ImGui::TextUnformatted( rest.c_str( ), rest.c_str( ) + rest.size( ) );
                }
                break;
            }
        }

        // Handle unclosed code block
        if ( in_code_block && !code_block_content.empty( ) ) {
            ImGui::PushStyleColor( ImGuiCol_ChildBg, code_bg );
            ImGui::BeginChild( "##code_block", ImVec2( 0, 0 ), true, ImGuiWindowFlags_AlwaysAutoResize );
            render_text( code_block_content, false );
            ImGui::EndChild( );
            ImGui::PopStyleColor( );
        }
    }
} // namespace ida_re::utils
