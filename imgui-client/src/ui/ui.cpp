#include "vendor.hpp"

#include "ui.hpp"

namespace ida_re::ui {
    c_ui::c_ui( ) { }

    c_ui::~c_ui( ) {
        shutdown( );
    }

    void c_ui::init( ) {
        apply_style( );
        m_history.load_from_file( utils::c_analysis_history::get_default_history_path( ) );
        m_highlighter.set_color_scheme( utils::c_syntax_highlighter::ios_dark_theme( ) );
        load_cache( );
        load_bookmarks( );
        load_custom_prompts( );
        load_pinned_functions( );
    }

    void c_ui::shutdown( ) {
        m_chat_loading     = false;
        m_analysis_loading = false;
        if ( m_chat_thread.joinable( ) )
            m_chat_thread.join( );
        if ( m_analysis_thread.joinable( ) )
            m_analysis_thread.join( );
        m_history.save_to_file( utils::c_analysis_history::get_default_history_path( ) );
        save_cache( );
    }

    void c_ui::apply_style( ) {
        ImGuiStyle &style  = ImGui::GetStyle( );
        ImVec4     *colors = style.Colors;

        // Clean dark theme
        colors[ ImGuiCol_WindowBg ]             = ImVec4( 0.10f, 0.10f, 0.12f, 1.00f );
        colors[ ImGuiCol_ChildBg ]              = ImVec4( 0.12f, 0.12f, 0.14f, 1.00f );
        colors[ ImGuiCol_PopupBg ]              = ImVec4( 0.12f, 0.12f, 0.14f, 0.98f );
        colors[ ImGuiCol_Border ]               = ImVec4( 0.25f, 0.25f, 0.28f, 0.60f );
        colors[ ImGuiCol_FrameBg ]              = ImVec4( 0.14f, 0.14f, 0.16f, 1.00f );
        colors[ ImGuiCol_FrameBgHovered ]       = ImVec4( 0.18f, 0.18f, 0.20f, 1.00f );
        colors[ ImGuiCol_FrameBgActive ]        = ImVec4( 0.22f, 0.22f, 0.25f, 1.00f );
        colors[ ImGuiCol_TitleBg ]              = ImVec4( 0.08f, 0.08f, 0.10f, 1.00f );
        colors[ ImGuiCol_TitleBgActive ]        = ImVec4( 0.12f, 0.12f, 0.14f, 1.00f );
        colors[ ImGuiCol_MenuBarBg ]            = ImVec4( 0.08f, 0.08f, 0.10f, 1.00f );
        colors[ ImGuiCol_ScrollbarBg ]          = ImVec4( 0.06f, 0.06f, 0.08f, 0.60f );
        colors[ ImGuiCol_ScrollbarGrab ]        = ImVec4( 0.30f, 0.30f, 0.35f, 0.80f );
        colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.35f, 0.35f, 0.40f, 0.80f );
        colors[ ImGuiCol_ScrollbarGrabActive ]  = ImVec4( 0.40f, 0.40f, 0.45f, 1.00f );
        colors[ ImGuiCol_CheckMark ]            = ImVec4( 0.50f, 0.70f, 1.00f, 1.00f );
        colors[ ImGuiCol_SliderGrab ]           = ImVec4( 0.45f, 0.65f, 0.95f, 1.00f );
        colors[ ImGuiCol_SliderGrabActive ]     = ImVec4( 0.55f, 0.75f, 1.00f, 1.00f );
        colors[ ImGuiCol_Button ]               = ImVec4( 0.20f, 0.25f, 0.35f, 1.00f );
        colors[ ImGuiCol_ButtonHovered ]        = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
        colors[ ImGuiCol_ButtonActive ]         = ImVec4( 0.30f, 0.45f, 0.65f, 1.00f );
        colors[ ImGuiCol_Header ]               = ImVec4( 0.20f, 0.25f, 0.35f, 0.80f );
        colors[ ImGuiCol_HeaderHovered ]        = ImVec4( 0.25f, 0.35f, 0.50f, 0.80f );
        colors[ ImGuiCol_HeaderActive ]         = ImVec4( 0.30f, 0.45f, 0.65f, 1.00f );
        colors[ ImGuiCol_Separator ]            = ImVec4( 0.25f, 0.25f, 0.28f, 0.60f );
        colors[ ImGuiCol_SeparatorHovered ]     = ImVec4( 0.35f, 0.45f, 0.60f, 0.78f );
        colors[ ImGuiCol_SeparatorActive ]      = ImVec4( 0.45f, 0.55f, 0.70f, 1.00f );
        colors[ ImGuiCol_ResizeGrip ]           = ImVec4( 0.20f, 0.25f, 0.35f, 0.25f );
        colors[ ImGuiCol_ResizeGripHovered ]    = ImVec4( 0.25f, 0.35f, 0.50f, 0.67f );
        colors[ ImGuiCol_ResizeGripActive ]     = ImVec4( 0.30f, 0.45f, 0.65f, 0.95f );
        colors[ ImGuiCol_Tab ]                  = ImVec4( 0.15f, 0.15f, 0.18f, 1.00f );
        colors[ ImGuiCol_TabHovered ]           = ImVec4( 0.25f, 0.35f, 0.50f, 0.80f );
        colors[ ImGuiCol_TabActive ]            = ImVec4( 0.20f, 0.30f, 0.45f, 1.00f );
        colors[ ImGuiCol_TabUnfocused ]         = ImVec4( 0.12f, 0.12f, 0.15f, 1.00f );
        colors[ ImGuiCol_TabUnfocusedActive ]   = ImVec4( 0.18f, 0.22f, 0.32f, 1.00f );
        colors[ ImGuiCol_TextSelectedBg ]       = ImVec4( 0.30f, 0.45f, 0.65f, 0.35f );

        style.WindowRounding    = 6.0f;
        style.ChildRounding     = 4.0f;
        style.FrameRounding     = 4.0f;
        style.PopupRounding     = 4.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 4.0f;
        style.WindowPadding     = ImVec2( 10, 10 );
        style.FramePadding      = ImVec2( 8, 4 );
        style.ItemSpacing       = ImVec2( 8, 6 );
        style.ItemInnerSpacing  = ImVec2( 6, 4 );
        style.ScrollbarSize     = 14.0f;
        style.GrabMinSize       = 12.0f;
    }

    void c_ui::render( ) {
        render_menu_bar( );

        ImGuiViewport *viewport = ImGui::GetMainViewport( );
        ImGui::SetNextWindowPos( viewport->Pos );
        ImGui::SetNextWindowSize( viewport->Size );

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                               | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;

        ImGui::Begin( "##main", nullptr, flags );

        float panel_width = ImGui::GetContentRegionAvail( ).x;
        float left_width  = panel_width * 0.35f;
        float right_width = panel_width - left_width - 10;

        // Left panel
        ImGui::BeginChild( "##left", ImVec2( left_width, 0 ), true );
        render_connection_panel( );
        ImGui::Separator( );
        render_function_panel( );
        ImGui::EndChild( );

        ImGui::SameLine( );

        // Right panel
        ImGui::BeginChild( "##right", ImVec2( right_width, 0 ), true );
        float height          = ImGui::GetContentRegionAvail( ).y;
        float analysis_height = height * 0.55f;
        float chat_height     = height - analysis_height - 10;

        ImGui::BeginChild( "##analysis_area", ImVec2( 0, analysis_height ), true );
        render_analysis_panel( );
        ImGui::EndChild( );

        ImGui::BeginChild( "##chat_area", ImVec2( 0, chat_height ), true );
        render_chat_panel( );
        ImGui::EndChild( );

        ImGui::EndChild( );
        ImGui::End( );

        if ( m_show_settings )
            render_settings_window( );
        if ( m_show_history )
            render_history_window( );
        if ( m_show_bookmarks )
            render_bookmarks_window( );
        if ( m_show_custom_prompts )
            render_custom_prompts_window( );
        if ( m_show_pinned )
            render_pinned_window( );
        if ( m_show_local_vars )
            render_local_variables_panel( );
        if ( m_show_diff_viewer )
            render_diff_viewer_window( );
        if ( m_show_ai_log )
            render_ai_log_window( );
        if ( m_show_plugin_installer )
            render_plugin_installer_window( );
    }

    void c_ui::render_menu_bar( ) {
        if ( ImGui::BeginMainMenuBar( ) ) {
            if ( ImGui::BeginMenu( "File" ) ) {
                if ( ImGui::MenuItem( "Settings", "Ctrl+," ) )
                    m_show_settings = true;
                ImGui::Separator( );
                if ( ImGui::MenuItem( "Export History (Markdown)" ) ) {
                    export_history_markdown( "analysis_report.md" );
                }
                if ( ImGui::MenuItem( "Export History (HTML)" ) ) {
                    export_history_html( "analysis_report.html" );
                }
                ImGui::Separator( );
                if ( ImGui::MenuItem( "Exit", "Alt+F4" ) ) { }
                ImGui::EndMenu( );
            }
            if ( ImGui::BeginMenu( "View" ) ) {
                if ( ImGui::MenuItem( "Analysis History", "Ctrl+H" ) ) {
                    m_show_history = !m_show_history;
                }
                if ( ImGui::MenuItem( "Bookmarks", "Ctrl+B" ) ) {
                    m_show_bookmarks = !m_show_bookmarks;
                }
                if ( ImGui::MenuItem( "Pinned Functions", "Ctrl+P" ) ) {
                    m_show_pinned = !m_show_pinned;
                }
                if ( ImGui::MenuItem( "Custom Prompts" ) ) {
                    m_show_custom_prompts = !m_show_custom_prompts;
                }
                ImGui::Separator( );
                if ( ImGui::MenuItem( "Local Variables", "Ctrl+L" ) ) {
                    m_show_local_vars = !m_show_local_vars;
                    if ( m_show_local_vars && !m_current_func.m_address.empty( ) ) {
                        load_local_variables( );
                    }
                }
                if ( ImGui::MenuItem( "Diff Viewer", "Ctrl+D" ) ) {
                    m_show_diff_viewer = !m_show_diff_viewer;
                }
                ImGui::Separator( );
                if ( ImGui::MenuItem( "Dark Theme", nullptr, m_dark_theme ) ) {
                    m_dark_theme = true;
                    apply_dark_theme( );
                }
                if ( ImGui::MenuItem( "Light Theme", nullptr, !m_dark_theme ) ) {
                    m_dark_theme = false;
                    apply_light_theme( );
                }
                ImGui::Separator( );
                if ( ImGui::MenuItem( "Clear Chat" ) ) {
                    m_chat_history.clear( );
                }
                ImGui::EndMenu( );
            }
            if ( ImGui::BeginMenu( "Plugin" ) ) {
                if ( ImGui::MenuItem( "Install IDA Plugin..." ) ) {
                    m_show_plugin_installer = true;
                    m_ida_installations     = m_installer.detect_ida_installations( );
                }
                ImGui::Separator( );

                auto installations = m_installer.detect_ida_installations( );
                if ( !installations.empty( ) ) {
                    ImGui::TextDisabled( "IDA installations found: %zu", installations.size( ) );
                    for ( const auto &ida : installations ) {
                        ImGui::Text( "  %s (%s)", ida.m_install_dir.string( ).c_str( ), ida.m_version.c_str( ) );
                    }
                } else {
                    ImGui::TextDisabled( "No IDA installations detected" );
                }
                ImGui::EndMenu( );
            }
            ImGui::EndMainMenuBar( );
        }

        // Keyboard shortcuts
        const auto &io = ImGui::GetIO( );
        if ( io.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_H ) )
            m_show_history = !m_show_history;
        if ( io.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_B ) )
            m_show_bookmarks = !m_show_bookmarks;
        if ( io.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_P ) )
            m_show_pinned = !m_show_pinned;
        if ( io.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_L ) ) {
            m_show_local_vars = !m_show_local_vars;
            if ( m_show_local_vars && !m_current_func.m_address.empty( ) ) {
                load_local_variables( );
            }
        }
        if ( io.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_D ) )
            m_show_diff_viewer = !m_show_diff_viewer;
        if ( io.KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_Comma ) )
            m_show_settings = true;
    }

    void c_ui::render_connection_panel( ) {
        ImGui::Text( "MCP Connection" );
        ImGui::Separator( );

        bool connected = m_mcp && m_mcp->is_connected( );
        if ( connected ) {
            ImGui::TextColored( ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ), "Connected" );
        } else {
            ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ), "Disconnected" );
        }

        if ( m_config ) {
            ImGui::TextDisabled( "%s:%d", m_config->m_mcp_host.c_str( ), m_config->m_mcp_port );
        }

        ImGui::BeginDisabled( connected );
        if ( ImGui::Button( "Connect", ImVec2( 100, 0 ) ) ) {
            if ( m_mcp && m_config ) {
                m_mcp->set_host( m_config->m_mcp_host );
                m_mcp->set_port( m_config->m_mcp_port );
                if ( m_mcp->connect( ) ) {
                    // Get database info for cache key
                    auto db_info = m_mcp->get_database_info( );
                    if ( db_info.m_success ) {
                        m_current_file_md5  = db_info.m_data.value( "md5", "Unknown" );
                        m_current_file_name = db_info.m_data.value( "input_file_name", "Unknown" );
                    } else {
                        m_current_file_md5  = "Unknown";
                        m_current_file_name = "Unknown";
                    }

                    auto result = m_mcp->list_functions( 500 );
                    if ( result.m_success && result.m_data.contains( "functions" ) ) {
                        m_function_list.clear( );
                        for ( const auto &f : result.m_data[ "functions" ] ) {
                            m_function_list.push_back( { f.value( "address", "" ), f.value( "name", "" ) } );
                        }
                    }
                }
            }
        }
        ImGui::EndDisabled( );

        ImGui::SameLine( );

        ImGui::BeginDisabled( !connected );
        if ( ImGui::Button( "Disconnect", ImVec2( 100, 0 ) ) ) {
            if ( m_mcp )
                m_mcp->disconnect( );
            m_function_list.clear( );
            m_current_func = function_data_t( );
        }
        ImGui::EndDisabled( );
    }

    void c_ui::render_function_panel( ) {
        ImGui::Text( "Function Navigation" );
        ImGui::Separator( );

        ImGui::SetNextItemWidth( -1 );
        ImGui::InputText( "##addr", m_address_input, sizeof( m_address_input ) );

        if ( ImGui::Button( "Load", ImVec2( 80, 0 ) ) ) {
            load_function( m_address_input );
        }
        ImGui::SameLine( );
        if ( ImGui::Button( "Current", ImVec2( 80, 0 ) ) ) {
            if ( m_mcp && m_mcp->is_connected( ) ) {
                auto result = m_mcp->get_current_function( );
                if ( result.m_success ) {
                    std::string addr = result.m_data.value( "address", "" );
                    if ( !addr.empty( ) ) {
                        strncpy( m_address_input, addr.c_str( ), sizeof( m_address_input ) - 1 );
                        load_function( addr );
                    }
                }
            }
        }

        ImGui::Separator( );

        // Search filter
        ImGui::Text( "Search:" );
        ImGui::SameLine( );
        ImGui::SetNextItemWidth( -1 );
        ImGui::InputText( "##filter", m_function_filter, sizeof( m_function_filter ) );

        // Function list
        ImGui::BeginChild( "##funclist", ImVec2( 0, 0 ), false );

        std::string filter_lower = m_function_filter;
        std::transform( filter_lower.begin( ), filter_lower.end( ), filter_lower.begin( ), ::tolower );

        int visible_count = 0;
        for ( const auto &[ addr, name ] : m_function_list ) {
            if ( filter_lower.length( ) > 0 ) {
                std::string addr_lower = addr;
                std::string name_lower = name;
                std::transform( addr_lower.begin( ), addr_lower.end( ), addr_lower.begin( ), ::tolower );
                std::transform( name_lower.begin( ), name_lower.end( ), name_lower.begin( ), ::tolower );
                if ( addr_lower.find( filter_lower ) == std::string::npos && name_lower.find( filter_lower ) == std::string::npos ) {
                    continue;
                }
            }
            visible_count++;

            char label[ 512 ];
            snprintf( label, sizeof( label ), "%s: %s", addr.c_str( ), name.c_str( ) );

            if ( ImGui::Selectable( label, m_current_func.m_address == addr ) ) {
                strncpy( m_address_input, addr.c_str( ), sizeof( m_address_input ) - 1 );
                load_function( addr );
            }
        }

        ImGui::EndChild( );

        ImGui::Text( "Functions: %d / %zu", visible_count, m_function_list.size( ) );
    }

    void c_ui::render_analysis_panel( ) {
        if ( ImGui::BeginTabBar( "##tabs" ) ) {
            if ( ImGui::BeginTabItem( "Pseudocode" ) ) {
                m_current_tab = 0;
                if ( m_current_func.m_loaded ) {
                    ImGui::TextWrapped( "%s @ %s", m_current_func.m_name.c_str( ), m_current_func.m_address.c_str( ) );

                    // AI Improve button
                    if ( m_ai_improving.load( std::memory_order_acquire ) ) {
                        ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.3f, 1.0f ), "AI is improving pseudocode..." );
                    } else {
                        if ( ImGui::Button( "AI Improve Pseudocode" ) ) {
                            ai_improve_pseudocode( );
                        }
                        ImGui::SameLine( );
                        ImGui::TextDisabled( "Let AI rename vars & add comments" );
                    }

                    ImGui::Separator( );
                    ImGui::BeginChild( "##pseudo_scroll" );
                    m_highlighter.render_text( m_current_func.m_pseudocode, true );
                    ImGui::EndChild( );
                } else {
                    ImGui::TextDisabled( "No function loaded" );
                }
                ImGui::EndTabItem( );
            }

            if ( ImGui::BeginTabItem( "Assembly" ) ) {
                m_current_tab = 1;
                if ( m_current_func.m_loaded ) {
                    ImGui::BeginChild( "##asm_scroll" );
                    m_highlighter.render_assembly( m_current_func.m_assembly, true );
                    ImGui::EndChild( );
                } else {
                    ImGui::TextDisabled( "No function loaded" );
                }
                ImGui::EndTabItem( );
            }

            if ( ImGui::BeginTabItem( "XRefs" ) ) {
                m_current_tab = 2;
                if ( m_current_func.m_loaded ) {
                    ImGui::TextColored( ImVec4( 0.7f, 0.9f, 0.7f, 1.0f ), "Calls TO this function:" );
                    ImGui::TextDisabled( "Functions that call this one" );
                    ImGui::Spacing( );

                    for ( const auto &x : m_current_func.m_xrefs_to ) {
                        ImGui::PushID( x.c_str( ) );

                        bool clicked = ImGui::Selectable( x.c_str( ), false, ImGuiSelectableFlags_AllowDoubleClick );

                        // Prefetch preview when hovering starts (before tooltip)
                        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayNone ) ) {
                            if ( m_xref_preview_cache.find( x ) == m_xref_preview_cache.end( ) ) {
                                if ( m_mcp && m_mcp->is_connected( ) ) {
                                    auto result = m_mcp->get_function_pseudocode( x );
                                    if ( result.m_success && result.m_data.contains( "pseudocode" ) ) {
                                        std::string preview = result.m_data[ "pseudocode" ].get< std::string >( );
                                        // Limit preview to first 5 lines
                                        size_t count = 0;
                                        size_t pos   = 0;
                                        while ( count < 5 && ( pos = preview.find( '\n', pos ) ) != std::string::npos ) {
                                            pos++;
                                            count++;
                                        }
                                        if ( pos != std::string::npos ) {
                                            preview = preview.substr( 0, pos ) + "\n...";
                                        }
                                        m_xref_preview_cache[ x ] = preview;
                                    } else {
                                        m_xref_preview_cache[ x ] = "Preview not available";
                                    }
                                } else {
                                    m_xref_preview_cache[ x ] = "Not connected to IDA";
                                }
                            }
                        }

                        // Handle double-click navigation
                        if ( ImGui::IsItemHovered( ) && ImGui::IsMouseDoubleClicked( 0 ) ) {
                            strncpy( m_address_input, x.c_str( ), sizeof( m_address_input ) - 1 );
                            m_address_input[ sizeof( m_address_input ) - 1 ] = '\0';
                            load_function( x );
                            m_xref_preview_cache.clear( ); // Clear cache when navigating
                        }

                        // Show tooltip with cached preview
                        if ( ImGui::IsItemHovered( ImGuiHoveredFlags_DelayShort ) && ImGui::BeginTooltip( ) ) {
                            ImGui::TextColored( ImVec4( 0.8f, 0.9f, 0.6f, 1.0f ), "%s", x.c_str( ) );
                            ImGui::Separator( );

                            if ( m_xref_preview_cache.find( x ) != m_xref_preview_cache.end( ) ) {
                                ImGui::TextWrapped( "%s", m_xref_preview_cache[ x ].c_str( ) );
                            } else {
                                ImGui::TextDisabled( "Loading preview..." );
                            }

                            ImGui::Spacing( );
                            ImGui::TextDisabled( "Double-click to navigate" );
                            ImGui::EndTooltip( );
                        }

                        ImGui::PopID( );
                    }

                    ImGui::Spacing( );
                    ImGui::Separator( );
                    ImGui::Spacing( );

                    ImGui::TextColored( ImVec4( 0.9f, 0.7f, 0.7f, 1.0f ), "Calls FROM this function:" );
                    ImGui::TextDisabled( "Functions/APIs that this one calls" );
                    ImGui::Spacing( );

                    for ( const auto &x : m_current_func.m_xrefs_from ) {
                        ImGui::PushID( x.c_str( ) );

                        ImGui::Bullet( );
                        ImGui::SameLine( );
                        ImGui::TextColored( ImVec4( 0.8f, 0.8f, 1.0f, 1.0f ), "%s", x.c_str( ) );

                        // Hover tooltip
                        if ( ImGui::IsItemHovered( ) ) {
                            ImGui::BeginTooltip( );
                            ImGui::TextColored( ImVec4( 0.8f, 0.9f, 0.6f, 1.0f ), "%s", x.c_str( ) );
                            ImGui::TextDisabled( "Called by current function" );
                            ImGui::EndTooltip( );
                        }

                        ImGui::PopID( );
                    }
                } else {
                    ImGui::TextDisabled( "No function loaded" );
                }
                ImGui::EndTabItem( );
            }

            if ( ImGui::BeginTabItem( "AI Analysis" ) ) {
                static constexpr std::array provider_names = { "Claude", "OpenAI", "Gemini" };
                const auto                  provider_idx   = m_llm ? static_cast< int >( m_llm->get_provider( ) ) : 2;
                ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.9f, 1.0f ), "Using: %s", provider_names[ provider_idx ] );

                ImGui::SameLine( );
                if ( ImGui::Button( "View History" ) ) {
                    m_show_history = true;
                }

                ImGui::Separator( );

                // Display chat history
                ImGui::BeginChild( "##analysis_chat_scroll", ImVec2( 0, -60 ), true );

                if ( m_analysis_chat_history.empty( ) ) {
                    ImGui::TextDisabled( "Click 'Analyze', 'Find Vulns', or 'Suggest Name' to start" );
                } else {
                    for ( const auto &msg : m_analysis_chat_history ) {
                        // Format timestamp
                        auto      time_t_val = std::chrono::system_clock::to_time_t( msg.m_timestamp );
                        struct tm tm_val;
                        localtime_s( &tm_val, &time_t_val );
                        char time_str[ 32 ];
                        strftime( time_str, sizeof( time_str ), "%H:%M:%S", &tm_val );

                        if ( msg.m_is_user ) {
                            ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.6f, 0.8f, 1.0f, 1.0f ) );
                            ImGui::Text( "You" );
                            ImGui::PopStyleColor( );
                            ImGui::SameLine( );
                            ImGui::TextDisabled( "[%s]", time_str );
                            ImGui::TextWrapped( "%s", msg.m_content.c_str( ) );
                        } else {
                            ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.8f, 1.0f, 0.8f, 1.0f ) );
                            ImGui::Text( "AI" );
                            ImGui::PopStyleColor( );
                            ImGui::SameLine( );
                            ImGui::TextDisabled( "[%s]", time_str );
                            m_highlighter.render_markdown( msg.m_content );

                            // Action buttons for AI response
                            ImGui::Spacing( );
                            if ( m_last_analysis_type == "naming" && m_mcp && m_mcp->is_connected( ) ) {
                                if ( ImGui::SmallButton( "Apply Name" ) ) {
                                    // Extract suggested name from response - look for first word after common patterns
                                    std::string content = msg.m_content;
                                    size_t      pos     = content.find( "`" );
                                    if ( pos != std::string::npos ) {
                                        size_t end = content.find( "`", pos + 1 );
                                        if ( end != std::string::npos ) {
                                            m_suggested_name = content.substr( pos + 1, end - pos - 1 );
                                            strncpy( m_rename_input, m_suggested_name.c_str( ), sizeof( m_rename_input ) - 1 );
                                            m_show_rename_popup = true;
                                        }
                                    }
                                }
                                ImGui::SameLine( );
                            }
                            if ( ImGui::SmallButton( "Add as Comment" ) ) {
                                add_comment_in_ida( msg.m_content );
                            }
                            ImGui::SameLine( );
                            if ( ImGui::SmallButton( "Copy" ) ) {
                                ImGui::SetClipboardText( msg.m_content.c_str( ) );
                            }
                        }
                        ImGui::Spacing( );
                        ImGui::Separator( );
                    }
                }

                if ( m_analysis_loading ) {
                    ImGui::TextDisabled( "Thinking..." );
                }

                ImGui::EndChild( );

                // Input area
                if ( !m_analysis_chat_history.empty( ) ) {
                    ImGui::PushItemWidth( -70 );
                    bool enter_pressed = ImGui::InputText( "##analysis_input", m_analysis_chat_input, sizeof( m_analysis_chat_input ),
                                                           ImGuiInputTextFlags_EnterReturnsTrue );
                    ImGui::PopItemWidth( );

                    ImGui::SameLine( );
                    bool send_clicked = ImGui::Button( "Send", ImVec2( 60, 0 ) );

                    if ( ( enter_pressed || send_clicked ) && strlen( m_analysis_chat_input ) > 0 && !m_analysis_loading ) {
                        send_analysis_chat_message( m_analysis_chat_input );
                        m_analysis_chat_input[ 0 ] = '\0';
                    }
                }

                if ( m_current_func.m_loaded && !m_analysis_loading ) {
                    // Check if cached results exist
                    bool has_general_cache = false;
                    bool has_vuln_cache    = false;
                    bool has_naming_cache  = false;

                    if ( !m_current_file_md5.empty( ) && m_analysis_cache.find( m_current_file_md5 ) != m_analysis_cache.end( )
                         && m_analysis_cache[ m_current_file_md5 ].find( m_current_func.m_address )
                                != m_analysis_cache[ m_current_file_md5 ].end( ) ) {
                        auto &func_cache  = m_analysis_cache[ m_current_file_md5 ][ m_current_func.m_address ];
                        has_general_cache = func_cache.find( "general" ) != func_cache.end( );
                        has_vuln_cache    = func_cache.find( "vulnerability" ) != func_cache.end( );
                        has_naming_cache  = func_cache.find( "naming" ) != func_cache.end( );
                    }

                    // Analyze button
                    if ( has_general_cache ) {
                        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.6f, 0.3f, 0.4f ) );
                    }
                    if ( ImGui::Button( "Analyze", ImVec2( 120, 0 ) ) ) {
                        if ( has_general_cache ) {
                            m_pending_analysis_type = "general";
                            m_show_cache_popup      = true;
                        } else {
                            perform_analysis( "general", true );
                        }
                    }
                    if ( has_general_cache ) {
                        ImGui::PopStyleColor( );
                        if ( ImGui::IsItemHovered( ) ) {
                            ImGui::SetTooltip( "Cached result available" );
                        }
                    }
                    ImGui::SameLine( );

                    // Find Vulns button
                    if ( has_vuln_cache ) {
                        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.6f, 0.3f, 0.4f ) );
                    }
                    if ( ImGui::Button( "Find Vulns", ImVec2( 120, 0 ) ) ) {
                        if ( has_vuln_cache ) {
                            m_pending_analysis_type = "vulnerability";
                            m_show_cache_popup      = true;
                        } else {
                            perform_analysis( "vulnerability", true );
                        }
                    }
                    if ( has_vuln_cache ) {
                        ImGui::PopStyleColor( );
                        if ( ImGui::IsItemHovered( ) ) {
                            ImGui::SetTooltip( "Cached result available" );
                        }
                    }
                    ImGui::SameLine( );

                    // Suggest Name button
                    if ( has_naming_cache ) {
                        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.6f, 0.3f, 0.4f ) );
                    }
                    if ( ImGui::Button( "Suggest Name", ImVec2( 120, 0 ) ) ) {
                        if ( has_naming_cache ) {
                            m_pending_analysis_type = "naming";
                            m_show_cache_popup      = true;
                        } else {
                            perform_analysis( "naming", true );
                        }
                    }
                    if ( has_naming_cache ) {
                        ImGui::PopStyleColor( );
                        if ( ImGui::IsItemHovered( ) ) {
                            ImGui::SetTooltip( "Cached result available" );
                        }
                    }

                    // Show "Clear Cache" button if any cache exists
                    if ( has_general_cache || has_vuln_cache || has_naming_cache ) {
                        ImGui::SameLine( );
                        ImGui::TextDisabled( "|" );
                        ImGui::SameLine( );
                        if ( ImGui::Button( "Clear Cache" ) ) {
                            if ( !m_current_file_md5.empty( ) ) {
                                m_analysis_cache[ m_current_file_md5 ][ m_current_func.m_address ].clear( );
                            }
                        }
                        if ( ImGui::IsItemHovered( ) ) {
                            ImGui::SetTooltip( "Clear all cached results for this function" );
                        }
                    }

                    // Custom prompts dropdown
                    std::vector< std::string > enabled_prompts;
                    for ( const auto &cp : m_custom_prompts ) {
                        if ( cp.m_enabled ) {
                            enabled_prompts.push_back( cp.m_name );
                        }
                    }

                    if ( !enabled_prompts.empty( ) ) {
                        ImGui::Spacing( );
                        ImGui::SetNextItemWidth( 200 );

                        static int                  selected_prompt_idx = 0;
                        std::vector< const char * > items;
                        items.reserve( enabled_prompts.size( ) );
                        for ( const auto &name : enabled_prompts ) {
                            items.push_back( name.c_str( ) );
                        }

                        if ( ImGui::Combo( "##custom_prompt_select", &selected_prompt_idx, items.data( ),
                                           static_cast< int >( items.size( ) ) ) ) {
                            // Selection changed
                        }

                        ImGui::SameLine( );
                        if ( ImGui::Button( "Run Custom", ImVec2( 100, 0 ) ) ) {
                            if ( selected_prompt_idx >= 0 && selected_prompt_idx < static_cast< int >( enabled_prompts.size( ) ) ) {
                                perform_custom_analysis( enabled_prompts[ selected_prompt_idx ] );
                            }
                        }
                        if ( ImGui::IsItemHovered( ) ) {
                            ImGui::SetTooltip( "Run selected custom analysis prompt" );
                        }
                    }
                }

                // Cache popup modal
                if ( m_show_cache_popup ) {
                    ImGui::OpenPopup( "Use Cached Result?" );
                }

                if ( ImGui::BeginPopupModal( "Use Cached Result?", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
                    ImGui::Text( "A cached result is available for this analysis." );
                    ImGui::Spacing( );
                    ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.8f, 1.0f ), "Using cache will not spend API tokens." );
                    ImGui::Spacing( );
                    ImGui::Separator( );
                    ImGui::Spacing( );

                    if ( ImGui::Button( "Use Cache", ImVec2( 150, 0 ) ) ) {
                        perform_analysis( m_pending_analysis_type, false );
                        m_show_cache_popup = false;
                        ImGui::CloseCurrentPopup( );
                    }
                    ImGui::SameLine( );
                    if ( ImGui::Button( "Get New Result", ImVec2( 150, 0 ) ) ) {
                        perform_analysis( m_pending_analysis_type, true );
                        m_show_cache_popup = false;
                        ImGui::CloseCurrentPopup( );
                    }
                    ImGui::SameLine( );
                    if ( ImGui::Button( "Cancel", ImVec2( 80, 0 ) ) ) {
                        m_show_cache_popup = false;
                        ImGui::CloseCurrentPopup( );
                    }

                    ImGui::EndPopup( );
                }

                // Rename popup modal
                if ( m_show_rename_popup ) {
                    ImGui::OpenPopup( "Rename Function" );
                }

                if ( ImGui::BeginPopupModal( "Rename Function", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) {
                    ImGui::Text( "Rename function to:" );
                    ImGui::SetNextItemWidth( 300 );
                    ImGui::InputText( "##rename_input", m_rename_input, sizeof( m_rename_input ) );

                    ImGui::Spacing( );
                    if ( ImGui::Button( "Apply", ImVec2( 100, 0 ) ) ) {
                        rename_function_in_ida( m_rename_input );
                        m_show_rename_popup = false;
                        ImGui::CloseCurrentPopup( );
                    }
                    ImGui::SameLine( );
                    if ( ImGui::Button( "Cancel", ImVec2( 100, 0 ) ) ) {
                        m_show_rename_popup = false;
                        ImGui::CloseCurrentPopup( );
                    }
                    ImGui::EndPopup( );
                }

                ImGui::EndTabItem( );
            }

            ImGui::EndTabBar( );
        }

        // bookmark_t and Pin buttons at bottom
        if ( m_current_func.m_loaded ) {
            ImGui::Separator( );
            if ( ImGui::Button( "Add Bookmark", ImVec2( 120, 0 ) ) ) {
                bookmark_t bm;
                bm.m_address   = m_current_func.m_address;
                bm.m_name      = m_current_func.m_name;
                bm.m_note      = "";
                bm.m_file_md5  = m_current_file_md5;
                bm.m_file_name = m_current_file_name;
                bm.m_timestamp = std::chrono::system_clock::now( );
                m_bookmarks.push_back( bm );
                save_bookmarks( );
            }
            ImGui::SameLine( );

            // Check if already pinned
            const auto is_pinned = std::find_if( m_pinned_functions.begin( ), m_pinned_functions.end( ),
                                                 [ this ]( const auto &p ) {
                                                     return p.m_address == m_current_func.m_address && p.m_file_md5 == m_current_file_md5;
                                                 } )
                                != m_pinned_functions.end( );

            if ( is_pinned ) {
                ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.8f, 0.6f, 0.2f, 1.0f ) );
                if ( ImGui::Button( "Unpin", ImVec2( 80, 0 ) ) ) {
                    m_pinned_functions.erase( std::remove_if( m_pinned_functions.begin( ), m_pinned_functions.end( ),
                                                              [ this ]( const auto &p ) {
                                                                  return p.m_address == m_current_func.m_address
                                                                      && p.m_file_md5 == m_current_file_md5;
                                                              } ),
                                              m_pinned_functions.end( ) );
                    save_pinned_functions( );
                }
                ImGui::PopStyleColor( );
            } else {
                if ( ImGui::Button( "Pin", ImVec2( 80, 0 ) ) ) {
                    pinned_function_t pf;
                    pf.m_address   = m_current_func.m_address;
                    pf.m_name      = m_current_func.m_name;
                    pf.m_file_md5  = m_current_file_md5;
                    pf.m_file_name = m_current_file_name;
                    pf.m_note      = "";
                    pf.m_timestamp = std::chrono::system_clock::now( );
                    m_pinned_functions.push_back( pf );
                    save_pinned_functions( );
                }
            }
            ImGui::SameLine( );
            ImGui::TextDisabled( "(Bookmarks: %zu | Pinned: %zu)", m_bookmarks.size( ), m_pinned_functions.size( ) );
        }
    }

    void c_ui::render_chat_panel( ) {
        ImGui::Text( "AI Chat" );
        ImGui::Separator( );

        ImGui::BeginChild( "##chat_messages", ImVec2( 0, -30 ), false );
        for ( const auto &msg : m_chat_history ) {
            if ( msg.m_is_user ) {
                ImGui::TextColored( ImVec4( 0.6f, 0.8f, 1.0f, 1.0f ), "You:" );
            } else {
                static constexpr std::array provider_names = { "Claude", "OpenAI", "Gemini" };
                int                         provider_idx   = m_llm ? static_cast< int >( m_llm->get_provider( ) ) : 2;
                ImGui::TextColored( ImVec4( 0.8f, 0.6f, 1.0f, 1.0f ), "%s:", provider_names[ provider_idx ] );
            }
            ImGui::TextWrapped( "%s", msg.m_content.c_str( ) );
            ImGui::Spacing( );
        }

        if ( m_chat_loading ) {
            if ( !m_streaming_buffer.empty( ) ) {
                static constexpr std::array provider_names = { "Claude", "OpenAI", "Gemini" };
                int                         provider_idx   = m_llm ? static_cast< int >( m_llm->get_provider( ) ) : 2;
                ImGui::TextColored( ImVec4( 0.8f, 0.6f, 1.0f, 1.0f ), "%s:", provider_names[ provider_idx ] );
                ImGui::TextWrapped( "%s", m_streaming_buffer.c_str( ) );
            } else {
                ImGui::TextDisabled( "Thinking..." );
            }
        }

        if ( ImGui::GetScrollY( ) >= ImGui::GetScrollMaxY( ) ) {
            ImGui::SetScrollHereY( 1.0f );
        }
        ImGui::EndChild( );

        ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail( ).x - 70 );
        bool enter = ImGui::InputText( "##chat_input", m_chat_input, sizeof( m_chat_input ), ImGuiInputTextFlags_EnterReturnsTrue );

        ImGui::SameLine( );
        bool send_clicked = ImGui::Button( "Send", ImVec2( 60, 0 ) );

        if ( ( enter || send_clicked ) && strlen( m_chat_input ) > 0 && !m_chat_loading ) {
            send_chat_message( m_chat_input );
            m_chat_input[ 0 ] = '\0';
        }
    }

    void c_ui::render_settings_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 550, 500 ), ImGuiCond_FirstUseEver );
        if ( ImGui::Begin( "Settings", &m_show_settings ) ) {
            if ( !m_settings_initialized && m_config ) {
                if ( m_config->m_provider == "claude" )
                    m_selected_provider = 0;
                else if ( m_config->m_provider == "openai" )
                    m_selected_provider = 1;
                else if ( m_config->m_provider == "gemini" )
                    m_selected_provider = 2;
                else if ( m_config->m_provider == "openrouter" )
                    m_selected_provider = 3;

                strncpy( m_claude_key_buf, m_config->m_claude_api_key.c_str( ), sizeof( m_claude_key_buf ) - 1 );
                strncpy( m_openai_key_buf, m_config->m_openai_api_key.c_str( ), sizeof( m_openai_key_buf ) - 1 );
                strncpy( m_gemini_key_buf, m_config->m_gemini_api_key.c_str( ), sizeof( m_gemini_key_buf ) - 1 );
                strncpy( m_openrouter_key_buf, m_config->m_openrouter_api_key.c_str( ), sizeof( m_openrouter_key_buf ) - 1 );
                strncpy( m_openai_base_url_buf, m_config->m_openai_base_url.c_str( ), sizeof( m_openai_base_url_buf ) - 1 );
                strncpy( m_anthropic_base_url_buf, m_config->m_anthropic_base_url.c_str( ), sizeof( m_anthropic_base_url_buf ) - 1 );
                strncpy( m_mcp_host_buf, m_config->m_mcp_host.c_str( ), sizeof( m_mcp_host_buf ) - 1 );
                m_mcp_port_buf          = m_config->m_mcp_port;
                m_openrouter_free_only  = m_config->m_openrouter_free_only;
                m_settings_initialized  = true;
            }

            ImGui::Text( "LLM Provider" );
            ImGui::Separator( );

            static constexpr std::array providers = { "Claude (Anthropic)", "OpenAI (ChatGPT)", "Gemini (Google)", "OpenRouter (Multi-LLM)" };
            ImGui::Combo( "Provider", &m_selected_provider, providers.data( ), static_cast< int >( providers.size( ) ) );

            // Model selection based on provider
            ImGui::Spacing( );
            ImGui::Text( "Model:" );
            if ( m_selected_provider == 0 ) { // Claude
                static constexpr std::array claude_models  = { "claude-sonnet-4-5-20250929", "claude-opus-4-5-20251101",
                                                               "claude-3-5-sonnet-20241022", "claude-3-5-haiku-20241022" };
                static int                  selected_model = 0;
                if ( m_config && !m_config->m_model.empty( ) ) {
                    for ( int i = 0; i < ( int ) claude_models.size( ); i++ ) {
                        if ( m_config->m_model == claude_models[ i ] ) {
                            selected_model = i;
                            break;
                        }
                    }
                }
                if ( ImGui::Combo( "##model", &selected_model, claude_models.data( ), static_cast< int >( claude_models.size( ) ) ) ) {
                    if ( m_config )
                        m_config->m_model = claude_models[ selected_model ];
                }
            } else if ( m_selected_provider == 1 ) { // OpenAI
                static constexpr std::array openai_models  = { "gpt-4o", "gpt-4o-mini", "gpt-4-turbo" };
                static int                  selected_model = 0;
                if ( m_config && !m_config->m_model.empty( ) ) {
                    for ( int i = 0; i < ( int ) openai_models.size( ); i++ ) {
                        if ( m_config->m_model == openai_models[ i ] ) {
                            selected_model = i;
                            break;
                        }
                    }
                }
                if ( ImGui::Combo( "##model", &selected_model, openai_models.data( ), static_cast< int >( openai_models.size( ) ) ) ) {
                    if ( m_config )
                        m_config->m_model = openai_models[ selected_model ];
                }
            } else if ( m_selected_provider == 2 ) { // Gemini
                static constexpr std::array gemini_models
                    = { "gemini-2.0-flash-exp", "gemini-exp-1206", "gemini-1.5-pro", "gemini-1.5-flash" };
                static int selected_model = 0;
                if ( m_config && !m_config->m_model.empty( ) ) {
                    for ( int i = 0; i < ( int ) gemini_models.size( ); i++ ) {
                        if ( m_config->m_model == gemini_models[ i ] ) {
                            selected_model = i;
                            break;
                        }
                    }
                }
                if ( ImGui::Combo( "##model", &selected_model, gemini_models.data( ), static_cast< int >( gemini_models.size( ) ) ) ) {
                    if ( m_config )
                        m_config->m_model = gemini_models[ selected_model ];
                }
            } else { // OpenRouter
                // Fetch models on first visit or refresh
                if ( !m_openrouter_models_fetched && m_llm ) {
                    m_llm->openrouter( ).set_show_free_only( m_openrouter_free_only );
                    m_llm->openrouter( ).fetch_models( );
                    m_openrouter_models_fetched = true;
                }

                const auto &models = m_llm ? m_llm->openrouter( ).cached_models( ) : std::vector< api::model_t >{ };

                // Free models filter checkbox
                if ( ImGui::Checkbox( "Free models only", &m_openrouter_free_only ) ) {
                    if ( m_llm ) {
                        m_llm->openrouter( ).set_show_free_only( m_openrouter_free_only );
                        m_llm->openrouter( ).fetch_models( true ); // Force refresh
                    }
                    m_openrouter_selected_model = 0;
                    if ( m_config )
                        m_config->m_openrouter_free_only = m_openrouter_free_only;
                }

                ImGui::SameLine( );
                if ( ImGui::Button( "Refresh" ) && m_llm ) {
                    m_llm->openrouter( ).fetch_models( true );
                    m_openrouter_selected_model = 0;
                }

                // Model filter
                ImGui::SetNextItemWidth( -1 );
                ImGui::InputTextWithHint( "##model_filter", "Search models...", m_openrouter_model_filter, sizeof( m_openrouter_model_filter ) );

                // Model list with filter
                if ( !models.empty( ) ) {
                    std::string filter_lower = m_openrouter_model_filter;
                    std::transform( filter_lower.begin( ), filter_lower.end( ), filter_lower.begin( ), ::tolower );

                    std::vector< const api::model_t * > filtered;
                    for ( const auto &model : models ) {
                        std::string name_lower = model.m_name;
                        std::string id_lower   = model.m_id;
                        std::transform( name_lower.begin( ), name_lower.end( ), name_lower.begin( ), ::tolower );
                        std::transform( id_lower.begin( ), id_lower.end( ), id_lower.begin( ), ::tolower );
                        if ( filter_lower.empty( ) || name_lower.find( filter_lower ) != std::string::npos
                             || id_lower.find( filter_lower ) != std::string::npos ) {
                            filtered.push_back( &model );
                        }
                    }

                    // Combo with filtered models
                    if ( ImGui::BeginCombo( "##openrouter_model",
                             m_openrouter_selected_model < ( int ) filtered.size( ) ? filtered[ m_openrouter_selected_model ]->m_name.c_str( )
                                                                                    : "Select model..." ) ) {
                        for ( int i = 0; i < ( int ) filtered.size( ); i++ ) {
                            bool        is_selected = ( m_openrouter_selected_model == i );
                            std::string label       = filtered[ i ]->m_name;
                            if ( filtered[ i ]->m_id.ends_with( ":free" ) )
                                label += " [FREE]";

                            if ( ImGui::Selectable( label.c_str( ), is_selected ) ) {
                                m_openrouter_selected_model = i;
                                if ( m_config )
                                    m_config->m_model = filtered[ i ]->m_id;
                            }
                            if ( is_selected )
                                ImGui::SetItemDefaultFocus( );

                            // Tooltip with model details
                            if ( ImGui::IsItemHovered( ) ) {
                                ImGui::BeginTooltip( );
                                ImGui::Text( "ID: %s", filtered[ i ]->m_id.c_str( ) );
                                ImGui::Text( "Context: %d tokens", filtered[ i ]->m_context_window );
                                ImGui::EndTooltip( );
                            }
                        }
                        ImGui::EndCombo( );
                    }

                    ImGui::TextDisabled( "%zu models available", filtered.size( ) );
                } else {
                    ImGui::TextDisabled( "No models loaded. Click Refresh." );
                }
            }

            ImGui::Spacing( );
            ImGui::Text( "API Keys" );
            ImGui::Separator( );

            ImGui::Text( "Claude API Key:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##claude_key", m_claude_key_buf, sizeof( m_claude_key_buf ), ImGuiInputTextFlags_Password );

            ImGui::Text( "OpenAI API Key:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##openai_key", m_openai_key_buf, sizeof( m_openai_key_buf ), ImGuiInputTextFlags_Password );

            ImGui::Text( "Gemini API Key:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##gemini_key", m_gemini_key_buf, sizeof( m_gemini_key_buf ), ImGuiInputTextFlags_Password );

            ImGui::Text( "OpenRouter API Key:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##openrouter_key", m_openrouter_key_buf, sizeof( m_openrouter_key_buf ), ImGuiInputTextFlags_Password );
            ImGui::SameLine( );
            ImGui::TextDisabled( "(?)" );
            if ( ImGui::IsItemHovered( ) ) {
                ImGui::BeginTooltip( );
                ImGui::Text( "Get your key at: https://openrouter.ai/keys" );
                ImGui::Text( "Free models available without payment!" );
                ImGui::EndTooltip( );
            }

            ImGui::Spacing( );
            ImGui::Text( "Custom API Endpoints" );
            ImGui::Separator( );
            ImGui::TextDisabled( "Leave empty to use default endpoints" );

            ImGui::Text( "OpenAI Base URL:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputTextWithHint( "##openai_base_url", "e.g., api.openai.com or your-proxy.com", 
                                      m_openai_base_url_buf, sizeof( m_openai_base_url_buf ) );
            ImGui::SameLine( );
            ImGui::TextDisabled( "(?)" );
            if ( ImGui::IsItemHovered( ) ) {
                ImGui::BeginTooltip( );
                ImGui::Text( "Custom endpoint for OpenAI-compatible APIs" );
                ImGui::Text( "Examples: api.openai.com, your-proxy.com" );
                ImGui::Text( "Do not include 'https://' or path" );
                ImGui::EndTooltip( );
            }

            ImGui::Text( "Anthropic Base URL:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputTextWithHint( "##anthropic_base_url", "e.g., api.anthropic.com or your-proxy.com", 
                                      m_anthropic_base_url_buf, sizeof( m_anthropic_base_url_buf ) );
            ImGui::SameLine( );
            ImGui::TextDisabled( "(?)" );
            if ( ImGui::IsItemHovered( ) ) {
                ImGui::BeginTooltip( );
                ImGui::Text( "Custom endpoint for Anthropic-compatible APIs" );
                ImGui::Text( "Examples: api.anthropic.com, your-proxy.com" );
                ImGui::Text( "Do not include 'https://' or path" );
                ImGui::EndTooltip( );
            }

            ImGui::Spacing( );
            ImGui::Text( "MCP Connection" );
            ImGui::Separator( );

            ImGui::Text( "Host:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputText( "##mcp_host", m_mcp_host_buf, sizeof( m_mcp_host_buf ) );

            ImGui::Text( "Port:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputInt( "##mcp_port", &m_mcp_port_buf );

            ImGui::Spacing( );
            ImGui::Text( "Cache Settings" );
            ImGui::Separator( );

            if ( m_config ) {
                ImGui::Checkbox( "Enable Analysis Cache", &m_config->m_enable_cache );
                ImGui::TextDisabled( "Cache saves API tokens by storing analysis results" );
            }

            ImGui::Spacing( );

            if ( ImGui::Button( "Clear Cache", ImVec2( 120, 0 ) ) ) {
                clear_cache( );
            }
            ImGui::SameLine( );
            // Count total cached results across all files
            size_t total_cached = 0;
            for ( const auto &[ md5, file_cache ] : m_analysis_cache ) {
                for ( const auto &[ addr, func_cache ] : file_cache ) {
                    total_cached += func_cache.size( );
                }
            }
            ImGui::TextDisabled( "(%zu cached results)", total_cached );

            ImGui::Spacing( );

            if ( ImGui::Button( "Open Config Folder", ImVec2( 160, 0 ) ) ) {
                auto config_dir = core::app_config_t::get_config_dir( );
#ifdef IDA_RE_PLATFORM_WINDOWS
                std::string cmd = "explorer \"" + config_dir.string( ) + "\"";
                std::system( cmd.c_str( ) );
#elif defined( IDA_RE_PLATFORM_MACOS )
                std::string cmd = "open \"" + config_dir.string( ) + "\"";
                std::system( cmd.c_str( ) );
#else
                std::string cmd = "xdg-open \"" + config_dir.string( ) + "\"";
                std::system( cmd.c_str( ) );
#endif
            }

            ImGui::Spacing( );
            ImGui::Separator( );

            if ( ImGui::Button( "Save Settings", ImVec2( 120, 0 ) ) ) {
                if ( m_config ) {
                    m_config->m_claude_api_key     = m_claude_key_buf;
                    m_config->m_openai_api_key     = m_openai_key_buf;
                    m_config->m_gemini_api_key     = m_gemini_key_buf;
                    m_config->m_openrouter_api_key = m_openrouter_key_buf;
                    m_config->m_openai_base_url    = m_openai_base_url_buf;
                    m_config->m_anthropic_base_url = m_anthropic_base_url_buf;
                    m_config->m_mcp_host           = m_mcp_host_buf;
                    m_config->m_mcp_port           = m_mcp_port_buf;

                    if ( m_selected_provider == 0 )
                        m_config->m_provider = "claude";
                    else if ( m_selected_provider == 1 )
                        m_config->m_provider = "openai";
                    else if ( m_selected_provider == 2 )
                        m_config->m_provider = "gemini";
                    else if ( m_selected_provider == 3 )
                        m_config->m_provider = "openrouter";

                    m_config->save( );
                    apply_config_to_llm( );
                }
            }

            ImGui::SameLine( );

            if ( ImGui::Button( "Reset to Defaults", ImVec2( 140, 0 ) ) ) {
                if ( m_config ) {
                    m_config->m_provider           = "gemini";
                    m_config->m_model              = "gemini-2.0-flash-exp";
                    m_config->m_claude_api_key     = "";
                    m_config->m_openai_api_key     = "";
                    m_config->m_gemini_api_key     = "";
                    m_config->m_openrouter_api_key = "";
                    m_config->m_openai_base_url    = "";
                    m_config->m_anthropic_base_url = "";
                    m_selected_provider            = 2;
                    m_settings_initialized         = false;
                }
            }
        }
        ImGui::End( );
    }

    void c_ui::render_history_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 900, 650 ), ImGuiCond_FirstUseEver );
        if ( ImGui::Begin( "Analysis History", &m_show_history ) ) {
            const auto &entries = m_history.get_entries( );

            // Search bar
            ImGui::SetNextItemWidth( 300 );
            ImGui::InputTextWithHint( "##history_search", "Search functions...", m_history_filter, sizeof( m_history_filter ) );
            ImGui::SameLine( );

            ImGui::Text( "Total: %zu", entries.size( ) );
            ImGui::SameLine( );
            if ( ImGui::Button( "Export MD" ) ) {
                export_history_markdown( "analysis_report.md" );
            }
            ImGui::SameLine( );
            if ( ImGui::Button( "Export HTML" ) ) {
                export_history_html( "analysis_report.html" );
            }
            ImGui::SameLine( );
            if ( ImGui::Button( "Clear All" ) ) {
                m_history.clear( );
                m_selected_history_entry = -1;
            }

            ImGui::Separator( );

            float list_width = 400;
            ImGui::BeginChild( "##history_list", ImVec2( list_width, 0 ), true );

            std::string filter_lower = m_history_filter;
            std::transform( filter_lower.begin( ), filter_lower.end( ), filter_lower.begin( ), ::tolower );

            for ( int i = 0; i < ( int ) entries.size( ); i++ ) {
                const auto &entry = entries[ i ];

                // Filter check
                if ( !filter_lower.empty( ) ) {
                    std::string name_lower = entry.m_function_name;
                    std::transform( name_lower.begin( ), name_lower.end( ), name_lower.begin( ), ::tolower );
                    std::string addr_lower = entry.m_function_address;
                    std::transform( addr_lower.begin( ), addr_lower.end( ), addr_lower.begin( ), ::tolower );

                    if ( name_lower.find( filter_lower ) == std::string::npos && addr_lower.find( filter_lower ) == std::string::npos ) {
                        continue;
                    }
                }

                char      label[ 256 ];
                auto      time_t_val = std::chrono::system_clock::to_time_t( entry.m_timestamp );
                struct tm tm_val;
                localtime_s( &tm_val, &time_t_val );
                char time_str[ 64 ];
                strftime( time_str, sizeof( time_str ), "%Y-%m-%d %H:%M:%S", &tm_val );

                snprintf( label, sizeof( label ), "%s - %s###hist_%d", time_str, entry.m_function_name.c_str( ), i );

                if ( ImGui::Selectable( label, m_selected_history_entry == i ) ) {
                    m_selected_history_entry = i;
                }
            }

            ImGui::EndChild( );

            ImGui::SameLine( );

            ImGui::BeginChild( "##history_detail", ImVec2( 0, 0 ), true );
            if ( m_selected_history_entry >= 0 && m_selected_history_entry < ( int ) entries.size( ) ) {
                const auto &entry = entries[ m_selected_history_entry ];

                // Function info with colored labels
                ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.8f, 1.0f ), "Function:" );
                ImGui::SameLine( );
                ImGui::Text( "%s", entry.m_function_name.c_str( ) );

                ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.8f, 1.0f ), "Address:" );
                ImGui::SameLine( );
                ImGui::TextColored( ImVec4( 0.8f, 0.9f, 0.6f, 1.0f ), "%s", entry.m_function_address.c_str( ) );

                ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.8f, 1.0f ), "Provider:" );
                ImGui::SameLine( );
                ImGui::TextColored( ImVec4( 0.6f, 0.8f, 1.0f, 1.0f ), "%s", entry.m_provider.c_str( ) );

                ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.8f, 1.0f ), "Type:" );
                ImGui::SameLine( );
                ImGui::TextColored( ImVec4( 0.9f, 0.7f, 0.9f, 1.0f ), "%s", entry.m_analysis_type.c_str( ) );

                ImGui::Separator( );

                if ( ImGui::Button( "Load Function" ) ) {
                    strncpy( m_address_input, entry.m_function_address.c_str( ), sizeof( m_address_input ) - 1 );
                    load_function( entry.m_function_address );
                }
                ImGui::SameLine( );
                if ( ImGui::Button( "Copy Result" ) ) {
                    ImGui::SetClipboardText( entry.m_result.c_str( ) );
                }
                ImGui::SameLine( );
                if ( ImGui::Button( "Delete" ) ) {
                    auto &mutable_entries = const_cast< std::vector< utils::analysis_entry_t > & >( m_history.get_entries( ) );
                    mutable_entries.erase( mutable_entries.begin( ) + m_selected_history_entry );
                    m_selected_history_entry = -1;
                }

                ImGui::Separator( );

                ImGui::BeginChild( "##history_result" );
                m_highlighter.render_markdown( entry.m_result );
                ImGui::EndChild( );
            } else {
                ImGui::TextDisabled( "Select an entry to view details" );
            }
            ImGui::EndChild( );
        }
        ImGui::End( );
    }

    void c_ui::apply_config_to_llm( ) {
        if ( !m_llm || !m_config )
            return;

        api::e_provider provider = api::e_provider::gemini; // default

        if ( m_config->m_provider == "claude" ) {
            provider = api::e_provider::claude;
            m_llm->claude( ).set_api_key( m_config->m_claude_api_key );
            m_llm->claude( ).set_model( m_config->m_model );
            if ( !m_config->m_anthropic_base_url.empty( ) ) {
                m_llm->claude( ).set_base_url( m_config->m_anthropic_base_url );
            }
        } else if ( m_config->m_provider == "openai" ) {
            provider = api::e_provider::openai;
            m_llm->openai( ).set_api_key( m_config->m_openai_api_key );
            m_llm->openai( ).set_model( m_config->m_model );
            if ( !m_config->m_openai_base_url.empty( ) ) {
                m_llm->openai( ).set_base_url( m_config->m_openai_base_url );
            }
        } else if ( m_config->m_provider == "gemini" ) {
            provider = api::e_provider::gemini;
            m_llm->gemini( ).set_api_key( m_config->m_gemini_api_key );
            m_llm->gemini( ).set_model( m_config->m_model );
        } else if ( m_config->m_provider == "openrouter" ) {
            provider = api::e_provider::openrouter;
            m_llm->openrouter( ).set_api_key( m_config->m_openrouter_api_key );
            m_llm->openrouter( ).set_model( m_config->m_model );
            m_llm->openrouter( ).set_show_free_only( m_config->m_openrouter_free_only );
        }

        m_llm->set_provider( provider );
    }

    void c_ui::save_cache( ) {
        if ( !m_config || !m_config->m_enable_cache )
            return;

        try {
            auto cache_path = core::app_config_t::get_cache_path( );
            std::filesystem::create_directories( cache_path.parent_path( ) );

            json_t j;
            // Structure: { "file_md5": { "address": { "type": "result" } } }
            for ( const auto &[ file_md5, file_cache ] : m_analysis_cache ) {
                for ( const auto &[ addr, func_cache ] : file_cache ) {
                    for ( const auto &[ type, result ] : func_cache ) {
                        j[ file_md5 ][ addr ][ type ] = result;
                    }
                }
            }

            std::ofstream f( cache_path );
            f << j.dump( 2 );
        } catch ( ... ) {
            // Silently fail - cache is not critical
        }
    }

    void c_ui::load_cache( ) {
        if ( !m_config || !m_config->m_enable_cache )
            return;

        try {
            auto cache_path = core::app_config_t::get_cache_path( );
            if ( !std::filesystem::exists( cache_path ) )
                return;

            std::ifstream f( cache_path );
            json_t        j = json_t::parse( f );

            m_analysis_cache.clear( );
            // Structure: { "file_md5": { "address": { "type": "result" } } }
            for ( auto it_file = j.begin( ); it_file != j.end( ); ++it_file ) {
                std::string file_md5 = it_file.key( );
                for ( auto it_addr = it_file.value( ).begin( ); it_addr != it_file.value( ).end( ); ++it_addr ) {
                    std::string addr = it_addr.key( );
                    for ( auto it_type = it_addr.value( ).begin( ); it_type != it_addr.value( ).end( ); ++it_type ) {
                        m_analysis_cache[ file_md5 ][ addr ][ it_type.key( ) ] = it_type.value( ).get< std::string >( );
                    }
                }
            }
        } catch ( ... ) {
            // Silently fail - cache is not critical
        }
    }

    void c_ui::clear_cache( ) {
        m_analysis_cache.clear( );

        try {
            auto cache_path = core::app_config_t::get_cache_path( );
            if ( std::filesystem::exists( cache_path ) ) {
                std::filesystem::remove( cache_path );
            }
        } catch ( ... ) {
            // Silently fail
        }
    }

    void c_ui::load_function( std::string_view address ) {
        if ( !m_mcp || !m_mcp->is_connected( ) )
            return;

        // Get pseudocode
        auto pseudo_result = m_mcp->get_function_pseudocode( address );
        if ( !pseudo_result.m_success )
            return;

        // Get assembly
        auto asm_result = m_mcp->get_function_assembly( address );

        // Get xrefs
        auto xrefs_result = m_mcp->get_function_xrefs( address );

        m_current_func.m_loaded     = true;
        m_current_func.m_address    = address;
        m_current_func.m_name       = pseudo_result.m_data.value( "name", "Unknown" );
        m_current_func.m_pseudocode = pseudo_result.m_data.value( "pseudocode", "" );

        if ( asm_result.m_success ) {
            m_current_func.m_assembly = asm_result.m_data.value( "assembly", "" );
        }

        m_current_func.m_xrefs_to.clear( );
        m_current_func.m_xrefs_from.clear( );

        if ( xrefs_result.m_success && xrefs_result.m_data.contains( "calls_to" ) ) {
            for ( const auto &x : xrefs_result.m_data[ "calls_to" ] ) {
                if ( x.is_string( ) ) {
                    m_current_func.m_xrefs_to.push_back( x.get< std::string >( ) );
                }
            }
        }

        if ( xrefs_result.m_success && xrefs_result.m_data.contains( "calls_from" ) ) {
            for ( const auto &x : xrefs_result.m_data[ "calls_from" ] ) {
                if ( x.is_string( ) ) {
                    m_current_func.m_xrefs_from.push_back( x.get< std::string >( ) );
                }
            }
        }

        m_analysis_result.clear( );

        // Switch to Pseudocode tab to show the loaded function
        m_current_tab = 0;
    }

    void c_ui::send_chat_message( std::string_view message ) {
        if ( !m_llm )
            return;

        m_chat_history.push_back( { true, std::string( message ), std::chrono::system_clock::now( ) } );
        m_chat_loading = true;
        m_streaming_buffer.clear( );

        if ( m_chat_thread.joinable( ) )
            m_chat_thread.join( );

        m_chat_thread = std::thread( [ this, message ]( ) {
            auto resp = m_llm->send( message );

            {
                std::lock_guard< std::mutex > lock( m_chat_mutex );
                if ( resp.m_success ) {
                    m_chat_history.push_back( { false, resp.m_content, std::chrono::system_clock::now( ) } );
                } else {
                    m_chat_history.push_back( { false, "Error: " + resp.m_error, std::chrono::system_clock::now( ) } );
                }
                m_streaming_buffer.clear( );
            }

            m_chat_loading = false;
        } );
    }

    void c_ui::send_analysis_chat_message( std::string_view message ) {
        if ( !m_llm )
            return;

        // Add user message to chat history
        m_analysis_chat_history.push_back( { true, std::string( message ), std::chrono::system_clock::now( ) } );
        m_analysis_loading = true;

        if ( m_analysis_thread.joinable( ) )
            m_analysis_thread.join( );

        m_analysis_thread = std::thread( [ this, message ]( ) {
            // Build context: include the analysis context + full chat history
            std::string full_message = m_analysis_context;

            // Add chat history
            for ( const auto &msg : m_analysis_chat_history ) {
                if ( msg.m_is_user ) {
                    full_message += "\n\nUser: " + msg.m_content;
                } else {
                    full_message += "\n\nAssistant: " + msg.m_content;
                }
            }

            auto resp = m_llm->send( full_message );

            if ( resp.m_success ) {
                m_analysis_chat_history.push_back( { false, resp.m_content, std::chrono::system_clock::now( ) } );
                m_analysis_result = resp.m_content;

                // Save custom query to history
                if ( !m_current_func.m_address.empty( ) ) {
                    utils::analysis_entry_t entry;
                    entry.m_function_address = m_current_func.m_address;
                    entry.m_function_name    = m_current_func.m_name;
                    entry.m_analysis_type    = "custom";
                    entry.m_result           = resp.m_content;
                    entry.m_timestamp        = std::chrono::system_clock::now( );

                    static constexpr std::array provider_names = { "Claude", "OpenAI", "Gemini" };
                    int                         provider_idx   = static_cast< int >( m_llm->get_provider( ) );
                    entry.m_provider                           = provider_names[ provider_idx ];

                    m_history.add_entry( entry );
                }
            } else {
                m_analysis_chat_history.push_back( { false, "Error: " + resp.m_error, std::chrono::system_clock::now( ) } );
                m_analysis_result = "Error: " + resp.m_error;
            }

            m_analysis_loading = false;
        } );
    }

    void c_ui::perform_analysis( std::string_view type, bool force_new ) {
        if ( !m_llm || m_current_func.m_pseudocode.empty( ) )
            return;

        const std::string type_str = std::string( type );
        m_last_analysis_type       = type_str;

        std::string addr     = m_current_func.m_address;
        std::string name     = m_current_func.m_name;
        std::string code     = m_current_func.m_pseudocode;
        std::string file_md5 = m_current_file_md5;

        // Check cache if not forcing new analysis
        if ( !force_new && !file_md5.empty( ) && m_analysis_cache.find( file_md5 ) != m_analysis_cache.end( )
             && m_analysis_cache[ file_md5 ].find( addr ) != m_analysis_cache[ file_md5 ].end( ) ) {
            auto &func_cache = m_analysis_cache[ file_md5 ][ addr ];
            if ( func_cache.find( type_str ) != func_cache.end( ) ) {
                // Use cached result
                m_analysis_chat_history.clear( );

                std::string context_prompt;
                if ( type_str == "general" ) {
                    context_prompt
                        = "Analyzing function " + name + " at " + addr + ":\n\n" + code + "\n\nPlease explain what this function does.";
                } else if ( type_str == "vulnerability" ) {
                    context_prompt = "Finding vulnerabilities in function " + name + " at " + addr + ":\n\n" + code
                                   + "\n\nPlease identify potential security vulnerabilities.";
                } else if ( type_str == "naming" ) {
                    context_prompt = "Suggesting name for function " + name + " at " + addr + ":\n\n" + code
                                   + "\n\nPlease suggest a better name for this function.";
                }

                m_analysis_context = context_prompt;
                m_analysis_chat_history.push_back( { false, func_cache[ type_str ], std::chrono::system_clock::now( ) } );
                m_analysis_result = func_cache[ type_str ];
                return;
            }
        }

        m_analysis_loading = true;
        if ( m_analysis_thread.joinable( ) )
            m_analysis_thread.join( );

        static constexpr std::array provider_names = { "Claude", "OpenAI", "Gemini" };
        int                         provider_idx   = m_llm ? static_cast< int >( m_llm->get_provider( ) ) : 2;
        std::string                 provider       = provider_names[ provider_idx ];

        m_analysis_thread = std::thread( [ this, addr, name, code, provider, type_str, file_md5 ]( ) {
            api::response_t resp;
            std::string     context_prompt;

            if ( type_str == "general" ) {
                resp = m_llm->explain_function( code );
                context_prompt
                    = "Analyzing function " + name + " at " + addr + ":\n\n" + code + "\n\nPlease explain what this function does.";
            } else if ( type_str == "vulnerability" ) {
                resp           = m_llm->find_vulnerabilities( code );
                context_prompt = "Finding vulnerabilities in function " + name + " at " + addr + ":\n\n" + code
                               + "\n\nPlease identify potential security vulnerabilities.";
            } else if ( type_str == "naming" ) {
                resp           = m_llm->suggest_name( code );
                context_prompt = "Suggesting name for function " + name + " at " + addr + ":\n\n" + code
                               + "\n\nPlease suggest a better name for this function.";
            }

            m_analysis_result = resp.m_success ? resp.m_content : ( "Error: " + resp.m_error );

            if ( resp.m_success ) {
                // Cache the result
                if ( !file_md5.empty( ) ) {
                    m_analysis_cache[ file_md5 ][ addr ][ type_str ] = resp.m_content;
                    save_cache( );
                }

                // Clear previous chat and set context
                m_analysis_chat_history.clear( );
                m_analysis_context = context_prompt;
                m_analysis_chat_history.push_back( { false, resp.m_content, std::chrono::system_clock::now( ) } );

                utils::analysis_entry_t entry;
                entry.m_function_address = addr;
                entry.m_function_name    = name;
                entry.m_analysis_type    = type_str;
                entry.m_result           = resp.m_content;
                entry.m_timestamp        = std::chrono::system_clock::now( );
                entry.m_provider         = provider;
                m_history.add_entry( entry );
            }

            m_analysis_loading = false;
        } );
    }

    void c_ui::analyze_current_function( ) {
        perform_analysis( "general", false );
    }

    void c_ui::rename_function_in_ida( std::string_view new_name ) {
        if ( !m_mcp || !m_mcp->is_connected( ) || m_current_func.m_address.empty( ) )
            return;

        auto result = m_mcp->rename_function( m_current_func.m_address, new_name );
        if ( result.m_success ) {
            m_current_func.m_name = new_name;
            // Refresh function list
            if ( auto it = std::ranges::find_if( m_function_list,
                                                 [ this ]( const auto &f ) {
                                                     return f.first == m_current_func.m_address;
                                                 } );
                 it != m_function_list.end( ) ) {
                it->second = new_name;
            }
        }
    }

    void c_ui::add_comment_in_ida( std::string_view comment ) {
        if ( !m_mcp || !m_mcp->is_connected( ) || m_current_func.m_address.empty( ) )
            return;

        // Truncate comment if too long and add note
        std::string final_comment { comment };
        if ( final_comment.length( ) > 1024 ) {
            final_comment = final_comment.substr( 0, 1000 ) + "\n\n[Comment truncated - see full analysis in tool]";
        }

        m_mcp->add_comment( m_current_func.m_address, final_comment, true );
    }

    void c_ui::render_bookmarks_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 550, 450 ), ImGuiCond_FirstUseEver );
        if ( ImGui::Begin( "Bookmarks", &m_show_bookmarks ) ) {
            if ( m_bookmarks.empty( ) ) {
                ImGui::TextDisabled( "No bookmarks yet. Add bookmarks from the Analysis panel." );
            } else {
                // Group bookmarks by file
                std::unordered_map< std::string, std::vector< int > > by_file;
                for ( int i = 0; i < ( int ) m_bookmarks.size( ); i++ ) {
                    std::string key = m_bookmarks[ i ].m_file_name.empty( ) ? "Unknown" : m_bookmarks[ i ].m_file_name;
                    by_file[ key ].push_back( i );
                }

                for ( const auto &[ file_name, indices ] : by_file ) {
                    // Check if this file matches current
                    bool is_current = !m_current_file_name.empty( ) && file_name == m_current_file_name;

                    if ( is_current ) {
                        ImGui::PushStyleColor( ImGuiCol_Header, ImVec4( 0.2f, 0.4f, 0.3f, 0.8f ) );
                    }

                    if ( ImGui::CollapsingHeader( file_name.c_str( ), ImGuiTreeNodeFlags_DefaultOpen ) ) {
                        for ( int i : indices ) {
                            const auto &bm = m_bookmarks[ i ];
                            ImGui::PushID( i );

                            auto      time_t_val = std::chrono::system_clock::to_time_t( bm.m_timestamp );
                            struct tm tm_val;
                            localtime_s( &tm_val, &time_t_val );
                            char time_str[ 64 ];
                            strftime( time_str, sizeof( time_str ), "%Y-%m-%d %H:%M", &tm_val );

                            ImGui::Indent( 10 );

                            // Only allow navigation if same file is loaded
                            bool can_navigate = is_current || m_current_file_md5.empty( );

                            if ( !can_navigate ) {
                                ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
                            }

                            if ( ImGui::Selectable( ( "##bm" + std::to_string( i ) ).c_str( ), false, 0, ImVec2( 0, 40 ) ) ) {
                                if ( can_navigate ) {
                                    strncpy( m_address_input, bm.m_address.c_str( ), sizeof( m_address_input ) - 1 );
                                    load_function( bm.m_address );
                                }
                            }

                            if ( !can_navigate ) {
                                ImGui::PopStyleVar( );
                                if ( ImGui::IsItemHovered( ) ) {
                                    ImGui::SetTooltip( "Load this file in IDA to navigate" );
                                }
                            }

                            ImGui::SameLine( 0, 0 );
                            ImGui::BeginGroup( );
                            ImGui::TextColored( ImVec4( 0.8f, 0.9f, 0.6f, 1.0f ), "%s", bm.m_name.c_str( ) );
                            ImGui::TextDisabled( "%s - %s", bm.m_address.c_str( ), time_str );
                            ImGui::EndGroup( );

                            ImGui::SameLine( ImGui::GetContentRegionAvail( ).x - 30 );
                            if ( ImGui::SmallButton( "X" ) ) {
                                m_bookmarks.erase( m_bookmarks.begin( ) + i );
                                save_bookmarks( );
                                ImGui::PopID( );
                                ImGui::Unindent( 10 );
                                break;
                            }

                            ImGui::Unindent( 10 );
                            ImGui::PopID( );
                        }
                    }

                    if ( is_current ) {
                        ImGui::PopStyleColor( );
                    }
                }
            }
        }
        ImGui::End( );
    }

    void c_ui::render_custom_prompts_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 600, 500 ), ImGuiCond_FirstUseEver );
        if ( ImGui::Begin( "Custom Prompts", &m_show_custom_prompts ) ) {
            ImGui::Text( "Add custom analysis prompts:" );
            ImGui::Separator( );

            ImGui::Text( "Name:" );
            ImGui::SetNextItemWidth( 200 );
            ImGui::InputText( "##prompt_name", m_new_prompt_name, sizeof( m_new_prompt_name ) );

            ImGui::Text( "Prompt (use {code} for pseudocode, {name} for function name):" );
            ImGui::InputTextMultiline( "##prompt_text", m_new_prompt_text, sizeof( m_new_prompt_text ), ImVec2( -1, 100 ) );

            if ( ImGui::Button( "Add Prompt" ) ) {
                if ( strlen( m_new_prompt_name ) > 0 && strlen( m_new_prompt_text ) > 0 ) {
                    custom_prompt_t cp;
                    cp.m_name    = m_new_prompt_name;
                    cp.m_prompt  = m_new_prompt_text;
                    cp.m_enabled = true;
                    m_custom_prompts.push_back( cp );
                    save_custom_prompts( );
                    m_new_prompt_name[ 0 ] = '\0';
                    m_new_prompt_text[ 0 ] = '\0';
                }
            }

            ImGui::Separator( );
            ImGui::Text( "Your prompts:" );

            for ( int i = 0; i < ( int ) m_custom_prompts.size( ); i++ ) {
                auto &cp = m_custom_prompts[ i ];
                ImGui::PushID( i );

                ImGui::Checkbox( "##enabled", &cp.m_enabled );
                ImGui::SameLine( );
                ImGui::Text( "%s", cp.m_name.c_str( ) );
                ImGui::SameLine( ImGui::GetWindowWidth( ) - 120 );

                if ( ImGui::SmallButton( "Run" ) ) {
                    perform_custom_analysis( cp.m_name );
                }
                ImGui::SameLine( );
                if ( ImGui::SmallButton( "Delete" ) ) {
                    m_custom_prompts.erase( m_custom_prompts.begin( ) + i );
                    save_custom_prompts( );
                    ImGui::PopID( );
                    break;
                }

                ImGui::PopID( );
            }
        }
        ImGui::End( );
    }

    void c_ui::perform_custom_analysis( std::string_view prompt_name ) {
        if ( !m_llm || m_current_func.m_pseudocode.empty( ) )
            return;

        // Find the prompt
        std::string prompt_template;
        for ( const auto &cp : m_custom_prompts ) {
            if ( cp.m_name == prompt_name && cp.m_enabled ) {
                prompt_template = cp.m_prompt;
                break;
            }
        }
        if ( prompt_template.empty( ) )
            return;

        // Replace placeholders
        std::string prompt = prompt_template;
        size_t      pos;
        while ( ( pos = prompt.find( "{code}" ) ) != std::string::npos ) {
            prompt.replace( pos, 6, m_current_func.m_pseudocode );
        }
        while ( ( pos = prompt.find( "{name}" ) ) != std::string::npos ) {
            prompt.replace( pos, 6, m_current_func.m_name );
        }
        while ( ( pos = prompt.find( "{address}" ) ) != std::string::npos ) {
            prompt.replace( pos, 9, m_current_func.m_address );
        }

        m_last_analysis_type = "custom";
        m_analysis_loading   = true;
        if ( m_analysis_thread.joinable( ) )
            m_analysis_thread.join( );

        std::string addr = m_current_func.m_address;
        std::string name = m_current_func.m_name;

        m_analysis_thread = std::thread( [ this, prompt, addr, name, prompt_name ]( ) {
            auto resp = m_llm->send( prompt );

            if ( resp.m_success ) {
                m_analysis_chat_history.clear( );
                m_analysis_context = prompt;
                chat_message_t msg;
                msg.m_is_user   = false;
                msg.m_content   = resp.m_content;
                msg.m_timestamp = std::chrono::system_clock::now( );
                m_analysis_chat_history.push_back( msg );

                utils::analysis_entry_t entry;
                entry.m_function_address = addr;
                entry.m_function_name    = name;
                entry.m_analysis_type    = "custom:" + std::string( prompt_name );
                entry.m_result           = resp.m_content;
                entry.m_timestamp        = std::chrono::system_clock::now( );
                entry.m_provider         = "custom";
                m_history.add_entry( entry );
            }

            m_analysis_loading = false;
        } );
    }

    void c_ui::save_bookmarks( ) {
        try {
            auto path = core::app_config_t::get_config_dir( ) / "bookmarks.json";
            std::filesystem::create_directories( path.parent_path( ) );

            json_t j = json_t::array( );
            for ( const auto &bm : m_bookmarks ) {
                j.push_back( {
                    {   "address",                                           bm.m_address },
                    {      "name",                                              bm.m_name },
                    {      "note",                                              bm.m_note },
                    {  "file_md5",                                          bm.m_file_md5 },
                    { "file_name",                                         bm.m_file_name },
                    { "timestamp", std::chrono::system_clock::to_time_t( bm.m_timestamp ) }
                } );
            }

            std::ofstream f( path );
            f << j.dump( 2 );
        } catch ( ... ) { }
    }

    void c_ui::load_bookmarks( ) {
        try {
            auto path = core::app_config_t::get_config_dir( ) / "bookmarks.json";
            if ( !std::filesystem::exists( path ) )
                return;

            std::ifstream f( path );
            json_t        j = json_t::parse( f );

            m_bookmarks.clear( );
            for ( const auto &item : j ) {
                bookmark_t bm;
                bm.m_address   = item.value( "address", "" );
                bm.m_name      = item.value( "name", "" );
                bm.m_note      = item.value( "note", "" );
                bm.m_file_md5  = item.value( "file_md5", "" );
                bm.m_file_name = item.value( "file_name", "" );
                bm.m_timestamp = std::chrono::system_clock::from_time_t( item.value( "timestamp", 0 ) );
                m_bookmarks.push_back( bm );
            }
        } catch ( ... ) { }
    }

    void c_ui::save_custom_prompts( ) {
        try {
            auto path = core::app_config_t::get_config_dir( ) / "custom_prompts.json";
            std::filesystem::create_directories( path.parent_path( ) );

            json_t j = json_t::array( );
            for ( const auto &cp : m_custom_prompts ) {
                j.push_back( {
                    {    "name",    cp.m_name },
                    {  "prompt",  cp.m_prompt },
                    { "enabled", cp.m_enabled }
                } );
            }

            std::ofstream f( path );
            f << j.dump( 2 );
        } catch ( ... ) { }
    }

    void c_ui::load_custom_prompts( ) {
        try {
            auto path = core::app_config_t::get_config_dir( ) / "custom_prompts.json";
            if ( !std::filesystem::exists( path ) ) {
                // Add default example prompts
                m_custom_prompts = {
                    {       "Find Crypto",
                     "Analyze this function for cryptographic operations:\n\n{code}\n\nLook for: encryption algorithms, hash functions, "
                     "key generation, XOR operations, magic constants (like 0x5A827999), S-boxes, bit rotations.", true },
                    { "Detect Anti-Debug",
                     "Check this function for anti-debugging techniques:\n\n{code}\n\nLook for: IsDebuggerPresent, "
                     "NtQueryInformationProcess, timing checks (rdtsc, QueryPerformanceCounter), exception handling tricks, TEB/PEB "
                     "access, hardware breakpoint detection.", true                                                    },
                    {  "Network Analysis",
                     "Analyze network operations in this function:\n\n{code}\n\nIdentify: socket calls, protocols used, IP "
                     "addresses/ports, data structures for network communication, C2 patterns, DNS queries.", true     },
                    {    "String Decoder",
                     "This function might decode/decrypt strings. Analyze it:\n\n{code}\n\nIdentify: decoding algorithm, key/seed values, "
                     "and provide Python code to decode strings using the same algorithm.", true                       },
                    {  "Malware Behavior",
                     "Analyze this function for malicious behavior:\n\n{code}\n\nLook for: file operations, registry access, process "
                     "injection, persistence mechanisms, data exfiltration, privilege escalation.", true               },
                    { "Enhance Variables",
                     "Suggest better variable names for this function:\n\n{code}\n\nFor each variable (v1, v2, a1, a2, etc), suggest a "
                     "descriptive name based on usage. Format as:\nv1 -> descriptive_name\nv2 -> another_name", true   },
                    {    "Create Structs",
                     "Identify structures in this function and provide IDA Python code to create them:\n\n{code}\n\nProvide complete IDA "
                     "Python code using idc.add_struc(), idc.add_struc_member() to define the structures.", true       }
                };
                save_custom_prompts( );
                return;
            }

            std::ifstream f( path );
            json_t        j = json_t::parse( f );

            m_custom_prompts.clear( );
            for ( const auto &item : j ) {
                custom_prompt_t cp;
                cp.m_name    = item.value( "name", "" );
                cp.m_prompt  = item.value( "prompt", "" );
                cp.m_enabled = item.value( "enabled", true );
                m_custom_prompts.push_back( cp );
            }
        } catch ( ... ) { }
    }

    void c_ui::export_history_markdown( std::string_view filename ) {
        try {
            // Create exports directory
            auto exports_dir = core::app_config_t::get_config_dir( ) / "exports";
            std::filesystem::create_directories( exports_dir );

            // Generate timestamped filename
            auto      now        = std::chrono::system_clock::now( );
            auto      time_t_val = std::chrono::system_clock::to_time_t( now );
            struct tm tm_val;
            localtime_s( &tm_val, &time_t_val );
            char timestamp[ 32 ];
            strftime( timestamp, sizeof( timestamp ), "%Y%m%d_%H%M%S", &tm_val );

            std::string   final_filename = "report_" + std::string( timestamp ) + ".md";
            auto          path           = exports_dir / final_filename;
            std::ofstream f( path );

            char gen_time[ 64 ];
            strftime( gen_time, sizeof( gen_time ), "%Y-%m-%d %H:%M:%S", &tm_val );

            f << "# IDA RE Assistant - Analysis Report\n\n";
            f << "Generated: " << gen_time << "\n\n";

            if ( !m_current_file_name.empty( ) ) {
                f << "**File:** " << m_current_file_name << "\n";
                f << "**MD5:** " << m_current_file_md5 << "\n\n";
            }

            const auto &entries = m_history.get_entries( );
            for ( const auto &entry : entries ) {
                auto      entry_time = std::chrono::system_clock::to_time_t( entry.m_timestamp );
                struct tm entry_tm;
                localtime_s( &entry_tm, &entry_time );
                char time_str[ 64 ];
                strftime( time_str, sizeof( time_str ), "%Y-%m-%d %H:%M:%S", &entry_tm );

                f << "## " << entry.m_function_name << " (" << entry.m_function_address << ")\n\n";
                f << "- **Type:** " << entry.m_analysis_type << "\n";
                f << "- **Provider:** " << entry.m_provider << "\n";
                f << "- **Time:** " << time_str << "\n\n";
                f << "### Result\n\n";
                f << entry.m_result << "\n\n";
                f << "---\n\n";
            }

            // Open exports folder
#ifdef IDA_RE_PLATFORM_WINDOWS
            std::string cmd = "explorer \"" + exports_dir.string( ) + "\"";
            std::system( cmd.c_str( ) );
#elif defined( IDA_RE_PLATFORM_MACOS )
            std::string cmd = "open \"" + exports_dir.string( ) + "\"";
            std::system( cmd.c_str( ) );
#else
            std::string cmd = "xdg-open \"" + exports_dir.string( ) + "\"";
            std::system( cmd.c_str( ) );
#endif
        } catch ( ... ) { }
    }

    void c_ui::export_history_html( std::string_view filename ) {
        try {
            // Create exports directory
            auto exports_dir = core::app_config_t::get_config_dir( ) / "exports";
            std::filesystem::create_directories( exports_dir );

            // Generate timestamped filename
            auto      now        = std::chrono::system_clock::now( );
            auto      time_t_val = std::chrono::system_clock::to_time_t( now );
            struct tm tm_val;
            localtime_s( &tm_val, &time_t_val );
            char timestamp[ 32 ];
            strftime( timestamp, sizeof( timestamp ), "%Y%m%d_%H%M%S", &tm_val );

            std::string   final_filename = "report_" + std::string( timestamp ) + ".html";
            auto          path           = exports_dir / final_filename;
            std::ofstream f( path );

            char gen_time[ 64 ];
            strftime( gen_time, sizeof( gen_time ), "%Y-%m-%d %H:%M:%S", &tm_val );

            f << "<!DOCTYPE html>\n<html>\n<head>\n";
            f << "<title>IDA RE Assistant Report</title>\n";
            f << "<style>\n";
            f << "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 900px; margin: 0 auto; "
                 "padding: 20px; background: #1e1e1e; color: #d4d4d4; }\n";
            f << "h1 { color: #569cd6; }\n";
            f << "h2 { color: #4ec9b0; border-bottom: 1px solid #333; padding-bottom: 5px; }\n";
            f << ".meta { color: #808080; font-size: 0.9em; }\n";
            f << ".file-info { background: #252526; padding: 10px 15px; border-radius: 5px; margin-bottom: 20px; }\n";
            f << ".result { background: #252526; padding: 15px; border-radius: 5px; white-space: pre-wrap; }\n";
            f << "code { background: #333; padding: 2px 6px; border-radius: 3px; color: #ce9178; }\n";
            f << "</style>\n</head>\n<body>\n";

            f << "<h1>IDA RE Assistant - Analysis Report</h1>\n";
            f << "<p class='meta'>Generated: " << gen_time << "</p>\n";

            if ( !m_current_file_name.empty( ) ) {
                f << "<div class='file-info'>\n";
                f << "<strong>File:</strong> " << m_current_file_name << "<br>\n";
                f << "<strong>MD5:</strong> <code>" << m_current_file_md5 << "</code>\n";
                f << "</div>\n";
            }

            const auto &entries = m_history.get_entries( );
            for ( const auto &entry : entries ) {
                auto      entry_time = std::chrono::system_clock::to_time_t( entry.m_timestamp );
                struct tm entry_tm;
                localtime_s( &entry_tm, &entry_time );
                char time_str[ 64 ];
                strftime( time_str, sizeof( time_str ), "%Y-%m-%d %H:%M:%S", &entry_tm );

                f << "<h2>" << entry.m_function_name << " <code>" << entry.m_function_address << "</code></h2>\n";
                f << "<p class='meta'>Type: " << entry.m_analysis_type << " | Provider: " << entry.m_provider << " | " << time_str
                  << "</p>\n";
                f << "<div class='result'>" << entry.m_result << "</div>\n";
                f << "<hr>\n";
            }

            f << "</body>\n</html>\n";

            // Open exports folder
#ifdef IDA_RE_PLATFORM_WINDOWS
            std::string cmd = "explorer \"" + exports_dir.string( ) + "\"";
            std::system( cmd.c_str( ) );
#elif defined( IDA_RE_PLATFORM_MACOS )
            std::string cmd = "open \"" + exports_dir.string( ) + "\"";
            std::system( cmd.c_str( ) );
#else
            std::string cmd = "xdg-open \"" + exports_dir.string( ) + "\"";
            std::system( cmd.c_str( ) );
#endif
        } catch ( ... ) { }
    }

    void c_ui::save_pinned_functions( ) {
        try {
            const auto path = core::app_config_t::get_config_dir( ) / "pinned_functions.json";
            std::filesystem::create_directories( path.parent_path( ) );

            json_t j = json_t::array( );
            for ( const auto &pf : m_pinned_functions ) {
                j.push_back( {
                    {   "address",                                           pf.m_address },
                    {      "name",                                              pf.m_name },
                    {  "file_md5",                                          pf.m_file_md5 },
                    { "file_name",                                         pf.m_file_name },
                    {      "note",                                              pf.m_note },
                    { "timestamp", std::chrono::system_clock::to_time_t( pf.m_timestamp ) }
                } );
            }

            std::ofstream f( path );
            f << j.dump( 2 );
        } catch ( ... ) { }
    }

    void c_ui::load_pinned_functions( ) {
        try {
            const auto path = core::app_config_t::get_config_dir( ) / "pinned_functions.json";
            if ( !std::filesystem::exists( path ) )
                return;

            std::ifstream f( path );
            const auto    j = json_t::parse( f );

            m_pinned_functions.clear( );
            for ( const auto &item : j ) {
                pinned_function_t pf;
                pf.m_address   = item.value( "address", "" );
                pf.m_name      = item.value( "name", "" );
                pf.m_file_md5  = item.value( "file_md5", "" );
                pf.m_file_name = item.value( "file_name", "" );
                pf.m_note      = item.value( "note", "" );
                pf.m_timestamp = std::chrono::system_clock::from_time_t( item.value( "timestamp", 0 ) );
                m_pinned_functions.push_back( pf );
            }
        } catch ( ... ) { }
    }

    void c_ui::render_pinned_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 600, 500 ), ImGuiCond_FirstUseEver );
        if ( ImGui::Begin( "Pinned Functions", &m_show_pinned ) ) {
            if ( m_pinned_functions.empty( ) ) {
                ImGui::TextDisabled( "No pinned functions. Pin functions from the Analysis panel." );
            } else {
                ImGui::Text( "Quick access to important functions" );
                ImGui::Separator( );

                for ( size_t i = 0; i < m_pinned_functions.size( ); ++i ) {
                    const auto &pf = m_pinned_functions[ i ];
                    ImGui::PushID( static_cast< int >( i ) );

                    const auto is_current = !m_current_file_name.empty( ) && pf.m_file_name == m_current_file_name;

                    if ( is_current ) {
                        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.4f, 0.3f, 0.8f ) );
                    }

                    if ( ImGui::Button( pf.m_name.c_str( ), ImVec2( 300, 0 ) ) ) {
                        if ( is_current || m_current_file_md5.empty( ) ) {
                            strncpy( m_address_input, pf.m_address.c_str( ), sizeof( m_address_input ) - 1 );
                            load_function( pf.m_address );
                        }
                    }

                    if ( is_current ) {
                        ImGui::PopStyleColor( );
                    }

                    if ( !is_current && !m_current_file_md5.empty( ) && ImGui::IsItemHovered( ) ) {
                        ImGui::SetTooltip( "Load %s in IDA to navigate", pf.m_file_name.c_str( ) );
                    }

                    ImGui::SameLine( );
                    ImGui::TextDisabled( "%s | %s", pf.m_address.c_str( ), pf.m_file_name.c_str( ) );
                    ImGui::SameLine( ImGui::GetWindowWidth( ) - 70 );
                    if ( ImGui::SmallButton( "Unpin" ) ) {
                        m_pinned_functions.erase( m_pinned_functions.begin( ) + i );
                        save_pinned_functions( );
                        ImGui::PopID( );
                        break;
                    }

                    ImGui::PopID( );
                }
            }
        }
        ImGui::End( );
    }

    void c_ui::apply_dark_theme( ) {
        ImGuiStyle &style  = ImGui::GetStyle( );
        ImVec4     *colors = style.Colors;

        colors[ ImGuiCol_WindowBg ]             = ImVec4( 0.10f, 0.10f, 0.12f, 1.0f );
        colors[ ImGuiCol_ChildBg ]              = ImVec4( 0.12f, 0.12f, 0.14f, 1.0f );
        colors[ ImGuiCol_PopupBg ]              = ImVec4( 0.12f, 0.12f, 0.14f, 0.98f );
        colors[ ImGuiCol_Border ]               = ImVec4( 0.25f, 0.25f, 0.28f, 0.60f );
        colors[ ImGuiCol_FrameBg ]              = ImVec4( 0.14f, 0.14f, 0.16f, 1.0f );
        colors[ ImGuiCol_FrameBgHovered ]       = ImVec4( 0.18f, 0.18f, 0.20f, 1.0f );
        colors[ ImGuiCol_FrameBgActive ]        = ImVec4( 0.22f, 0.22f, 0.25f, 1.0f );
        colors[ ImGuiCol_TitleBg ]              = ImVec4( 0.08f, 0.08f, 0.10f, 1.0f );
        colors[ ImGuiCol_TitleBgActive ]        = ImVec4( 0.12f, 0.12f, 0.14f, 1.0f );
        colors[ ImGuiCol_MenuBarBg ]            = ImVec4( 0.08f, 0.08f, 0.10f, 1.0f );
        colors[ ImGuiCol_ScrollbarBg ]          = ImVec4( 0.06f, 0.06f, 0.08f, 0.60f );
        colors[ ImGuiCol_ScrollbarGrab ]        = ImVec4( 0.30f, 0.30f, 0.35f, 0.80f );
        colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.35f, 0.35f, 0.40f, 0.80f );
        colors[ ImGuiCol_ScrollbarGrabActive ]  = ImVec4( 0.40f, 0.40f, 0.45f, 1.0f );
        colors[ ImGuiCol_CheckMark ]            = ImVec4( 0.50f, 0.70f, 1.00f, 1.0f );
        colors[ ImGuiCol_SliderGrab ]           = ImVec4( 0.45f, 0.65f, 0.95f, 1.0f );
        colors[ ImGuiCol_SliderGrabActive ]     = ImVec4( 0.55f, 0.75f, 1.00f, 1.0f );
        colors[ ImGuiCol_Button ]               = ImVec4( 0.20f, 0.25f, 0.35f, 1.0f );
        colors[ ImGuiCol_ButtonHovered ]        = ImVec4( 0.25f, 0.35f, 0.50f, 1.0f );
        colors[ ImGuiCol_ButtonActive ]         = ImVec4( 0.30f, 0.45f, 0.65f, 1.0f );
        colors[ ImGuiCol_Header ]               = ImVec4( 0.20f, 0.25f, 0.35f, 0.80f );
        colors[ ImGuiCol_HeaderHovered ]        = ImVec4( 0.25f, 0.35f, 0.50f, 0.80f );
        colors[ ImGuiCol_HeaderActive ]         = ImVec4( 0.30f, 0.45f, 0.65f, 1.0f );
        colors[ ImGuiCol_Separator ]            = ImVec4( 0.25f, 0.25f, 0.28f, 0.60f );
        colors[ ImGuiCol_SeparatorHovered ]     = ImVec4( 0.35f, 0.45f, 0.60f, 0.78f );
        colors[ ImGuiCol_SeparatorActive ]      = ImVec4( 0.45f, 0.55f, 0.70f, 1.0f );
        colors[ ImGuiCol_Tab ]                  = ImVec4( 0.15f, 0.15f, 0.18f, 1.0f );
        colors[ ImGuiCol_TabHovered ]           = ImVec4( 0.25f, 0.35f, 0.50f, 0.80f );
        colors[ ImGuiCol_TabActive ]            = ImVec4( 0.20f, 0.30f, 0.45f, 1.0f );
        colors[ ImGuiCol_TabSelected ]          = ImVec4( 0.20f, 0.30f, 0.45f, 1.0f );
        colors[ ImGuiCol_Text ]                 = ImVec4( 0.95f, 0.95f, 0.97f, 1.0f );
        colors[ ImGuiCol_TextDisabled ]         = ImVec4( 0.50f, 0.50f, 0.55f, 1.0f );
        colors[ ImGuiCol_TextSelectedBg ]       = ImVec4( 0.30f, 0.45f, 0.65f, 0.35f );

        m_highlighter.set_dark_mode( true );
    }

    void c_ui::apply_light_theme( ) {
        ImGuiStyle &style  = ImGui::GetStyle( );
        ImVec4     *colors = style.Colors;

        colors[ ImGuiCol_WindowBg ]             = ImVec4( 0.95f, 0.95f, 0.96f, 1.0f );
        colors[ ImGuiCol_ChildBg ]              = ImVec4( 0.98f, 0.98f, 0.98f, 1.0f );
        colors[ ImGuiCol_PopupBg ]              = ImVec4( 1.0f, 1.0f, 1.0f, 0.98f );
        colors[ ImGuiCol_Border ]               = ImVec4( 0.70f, 0.70f, 0.72f, 0.60f );
        colors[ ImGuiCol_FrameBg ]              = ImVec4( 0.90f, 0.90f, 0.92f, 1.0f );
        colors[ ImGuiCol_FrameBgHovered ]       = ImVec4( 0.85f, 0.85f, 0.88f, 1.0f );
        colors[ ImGuiCol_FrameBgActive ]        = ImVec4( 0.80f, 0.80f, 0.85f, 1.0f );
        colors[ ImGuiCol_TitleBg ]              = ImVec4( 0.88f, 0.88f, 0.90f, 1.0f );
        colors[ ImGuiCol_TitleBgActive ]        = ImVec4( 0.82f, 0.82f, 0.85f, 1.0f );
        colors[ ImGuiCol_MenuBarBg ]            = ImVec4( 0.92f, 0.92f, 0.94f, 1.0f );
        colors[ ImGuiCol_ScrollbarBg ]          = ImVec4( 0.94f, 0.94f, 0.96f, 0.60f );
        colors[ ImGuiCol_ScrollbarGrab ]        = ImVec4( 0.70f, 0.70f, 0.75f, 0.80f );
        colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.60f, 0.60f, 0.65f, 0.80f );
        colors[ ImGuiCol_ScrollbarGrabActive ]  = ImVec4( 0.50f, 0.50f, 0.55f, 1.0f );
        colors[ ImGuiCol_CheckMark ]            = ImVec4( 0.20f, 0.45f, 0.80f, 1.0f );
        colors[ ImGuiCol_SliderGrab ]           = ImVec4( 0.30f, 0.55f, 0.85f, 1.0f );
        colors[ ImGuiCol_SliderGrabActive ]     = ImVec4( 0.20f, 0.45f, 0.75f, 1.0f );
        colors[ ImGuiCol_Button ]               = ImVec4( 0.75f, 0.80f, 0.88f, 1.0f );
        colors[ ImGuiCol_ButtonHovered ]        = ImVec4( 0.65f, 0.72f, 0.85f, 1.0f );
        colors[ ImGuiCol_ButtonActive ]         = ImVec4( 0.55f, 0.65f, 0.80f, 1.0f );
        colors[ ImGuiCol_Header ]               = ImVec4( 0.75f, 0.80f, 0.88f, 0.80f );
        colors[ ImGuiCol_HeaderHovered ]        = ImVec4( 0.65f, 0.72f, 0.85f, 0.80f );
        colors[ ImGuiCol_HeaderActive ]         = ImVec4( 0.55f, 0.65f, 0.80f, 1.0f );
        colors[ ImGuiCol_Separator ]            = ImVec4( 0.70f, 0.70f, 0.72f, 0.60f );
        colors[ ImGuiCol_SeparatorHovered ]     = ImVec4( 0.50f, 0.60f, 0.75f, 0.78f );
        colors[ ImGuiCol_SeparatorActive ]      = ImVec4( 0.40f, 0.50f, 0.70f, 1.0f );
        colors[ ImGuiCol_Tab ]                  = ImVec4( 0.85f, 0.85f, 0.88f, 1.0f );
        colors[ ImGuiCol_TabHovered ]           = ImVec4( 0.70f, 0.75f, 0.85f, 0.80f );
        colors[ ImGuiCol_TabActive ]            = ImVec4( 0.75f, 0.80f, 0.90f, 1.0f );
        colors[ ImGuiCol_TabSelected ]          = ImVec4( 0.75f, 0.80f, 0.90f, 1.0f );
        colors[ ImGuiCol_Text ]                 = ImVec4( 0.10f, 0.10f, 0.12f, 1.0f );
        colors[ ImGuiCol_TextDisabled ]         = ImVec4( 0.50f, 0.50f, 0.55f, 1.0f );
        colors[ ImGuiCol_TextSelectedBg ]       = ImVec4( 0.55f, 0.70f, 0.90f, 0.35f );

        m_highlighter.set_dark_mode( false );
    }

    void c_ui::load_local_variables( ) {
        if ( !m_mcp || m_current_func.m_address.empty( ) ) {
            return;
        }

        m_loading_vars.store( true, std::memory_order_release );

        std::thread( [ this ]( ) {
            auto result = m_mcp->get_function_local_variables( m_current_func.m_address );

            if ( result.m_success && result.m_data.contains( "variables" ) ) {
                std::lock_guard< std::mutex > lock( m_chat_mutex );
                m_local_variables.clear( );

                for ( const auto &v : result.m_data[ "variables" ] ) {
                    local_var_t var;
                    var.m_name   = v.value( "name", "" );
                    var.m_type   = v.value( "type", "" );
                    var.m_is_arg = v.value( "is_arg", false );
                    std::strncpy( var.m_new_name, var.m_name.c_str( ), sizeof( var.m_new_name ) - 1 );
                    std::strncpy( var.m_new_type, var.m_type.c_str( ), sizeof( var.m_new_type ) - 1 );
                    m_local_variables.push_back( var );
                }
            }

            m_loading_vars.store( false, std::memory_order_release );
        } ).detach( );
    }

    void c_ui::render_local_variables_panel( ) {
        ImGui::SetNextWindowSize( ImVec2( 700, 500 ), ImGuiCond_FirstUseEver );
        if ( !ImGui::Begin( "Local Variables", &m_show_local_vars ) ) {
            ImGui::End( );
            return;
        }

        if ( m_current_func.m_address.empty( ) ) {
            ImGui::TextDisabled( "No function loaded" );
            ImGui::End( );
            return;
        }

        ImGui::Text( "Function: %s (%s)", m_current_func.m_name.c_str( ), m_current_func.m_address.c_str( ) );
        ImGui::Separator( );

        if ( ImGui::Button( "Reload Variables" ) ) {
            load_local_variables( );
        }

        ImGui::SameLine( );
        if ( ImGui::Button( "Apply Changes & Show Diff" ) ) {
            apply_refactoring_changes( );
        }

        ImGui::Separator( );

        if ( m_loading_vars.load( std::memory_order_acquire ) ) {
            ImGui::TextDisabled( "Loading variables..." );
        } else if ( m_local_variables.empty( ) ) {
            ImGui::TextDisabled( "No local variables found" );
        } else {
            if ( ImGui::BeginTable( "##vars_table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY ) ) {
                ImGui::TableSetupColumn( "Select", ImGuiTableColumnFlags_WidthFixed, 50.0f );
                ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthFixed, 150.0f );
                ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed, 150.0f );
                ImGui::TableSetupColumn( "New Name", ImGuiTableColumnFlags_WidthFixed, 150.0f );
                ImGui::TableSetupColumn( "New Type", ImGuiTableColumnFlags_WidthFixed, 150.0f );
                ImGui::TableHeadersRow( );

                for ( size_t i = 0; i < m_local_variables.size( ); ++i ) {
                    auto &var = m_local_variables[ i ];

                    ImGui::TableNextRow( );

                    ImGui::TableSetColumnIndex( 0 );
                    ImGui::Checkbox( ( "##sel" + std::to_string( i ) ).c_str( ), &var.m_selected );

                    ImGui::TableSetColumnIndex( 1 );
                    ImGui::Text( "%s%s", var.m_name.c_str( ), var.m_is_arg ? " (arg)" : "" );

                    ImGui::TableSetColumnIndex( 2 );
                    ImGui::Text( "%s", var.m_type.c_str( ) );

                    ImGui::TableSetColumnIndex( 3 );
                    ImGui::SetNextItemWidth( -1 );
                    ImGui::InputText( ( "##newname" + std::to_string( i ) ).c_str( ), var.m_new_name, sizeof( var.m_new_name ) );

                    ImGui::TableSetColumnIndex( 4 );
                    ImGui::SetNextItemWidth( -1 );
                    ImGui::InputText( ( "##newtype" + std::to_string( i ) ).c_str( ), var.m_new_type, sizeof( var.m_new_type ) );
                }

                ImGui::EndTable( );
            }
        }

        ImGui::End( );
    }

    void c_ui::apply_refactoring_changes( ) {
        if ( !m_mcp || m_current_func.m_address.empty( ) ) {
            return;
        }

        // Store original pseudocode for diff
        m_diff_before = m_current_func.m_pseudocode;

        // Apply variable renames and type changes
        for ( const auto &var : m_local_variables ) {
            if ( !var.m_selected )
                continue;

            // Rename if name changed
            if ( std::string( var.m_new_name ) != var.m_name ) {
                auto result = m_mcp->rename_local_variable( m_current_func.m_address, var.m_name, var.m_new_name );
                if ( !result.m_success ) {
                    // Log error but continue
                }
            }

            // Change type if changed
            if ( std::string( var.m_new_type ) != var.m_type ) {
                auto result = m_mcp->set_variable_type( m_current_func.m_address, std::string( var.m_new_name ), var.m_new_type );
                if ( !result.m_success ) {
                    // Log error but continue
                }
            }
        }

        // Fetch updated pseudocode for diff
        m_loading_diff_after.store( true, std::memory_order_release );

        std::thread( [ this ]( ) {
            auto result = m_mcp->get_function_pseudocode( m_current_func.m_address );

            if ( result.m_success && result.m_data.contains( "pseudocode" ) ) {
                std::lock_guard< std::mutex > lock( m_chat_mutex );
                m_diff_after = result.m_data[ "pseudocode" ].get< std::string >( );

                // Update current function pseudocode
                m_current_func.m_pseudocode = m_diff_after;

                // Show diff viewer
                m_show_diff_viewer = true;
            }

            m_loading_diff_after.store( false, std::memory_order_release );
        } ).detach( );
    }

    void c_ui::render_diff_viewer_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 1200, 700 ), ImGuiCond_FirstUseEver );
        if ( !ImGui::Begin( "Diff Viewer - Before/After Refactoring", &m_show_diff_viewer ) ) {
            ImGui::End( );
            return;
        }

        if ( m_diff_before.empty( ) && m_diff_after.empty( ) ) {
            ImGui::TextDisabled( "No diff to display. Apply refactoring changes first." );
            ImGui::End( );
            return;
        }

        if ( m_loading_diff_after.load( std::memory_order_acquire ) ) {
            ImGui::TextDisabled( "Loading updated pseudocode..." );
            ImGui::End( );
            return;
        }

        if ( ImGui::Button( "Close" ) ) {
            m_show_diff_viewer = false;
        }

        ImGui::SameLine( );
        if ( ImGui::Button( "Clear" ) ) {
            m_diff_before.clear( );
            m_diff_after.clear( );
        }

        ImGui::Separator( );

        // Side-by-side view
        float width = ImGui::GetContentRegionAvail( ).x;
        float half  = width * 0.5f - 5.0f;

        ImGui::BeginChild( "##before", ImVec2( half, 0 ), true );
        ImGui::Text( "Before" );
        ImGui::Separator( );
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.9f, 0.3f, 0.3f, 1.0f ) ); // Red for before
        m_highlighter.render_text( m_diff_before );
        ImGui::PopStyleColor( );
        ImGui::EndChild( );

        ImGui::SameLine( );

        ImGui::BeginChild( "##after", ImVec2( half, 0 ), true );
        ImGui::Text( "After" );
        ImGui::Separator( );
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.3f, 0.9f, 0.3f, 1.0f ) ); // Green for after
        m_highlighter.render_text( m_diff_after );
        ImGui::PopStyleColor( );
        ImGui::EndChild( );

        ImGui::End( );
    }

    void c_ui::ai_improve_pseudocode( ) {
        if ( !m_llm || !m_mcp || m_current_func.m_address.empty( ) ) {
            return;
        }

        // Store original for diff
        m_diff_before = m_current_func.m_pseudocode;

        // Capture current function data for thread
        const std::string func_address    = m_current_func.m_address;
        const std::string func_name       = m_current_func.m_name;
        const std::string func_pseudocode = m_current_func.m_pseudocode;

        // Clear previous log
        m_ai_improvement_log.clear( );
        m_show_ai_log = true;

        m_ai_improving.store( true, std::memory_order_release );

        std::thread( [ this, func_address, func_name, func_pseudocode ]( ) {
            std::string log;

            log += "[INFO] Starting AI improvement for " + func_name + " @ " + func_address + "\n\n";

            // Get local variables first
            auto vars_result = m_mcp->get_function_local_variables( func_address );

            if ( !vars_result.m_success ) {
                log += "[ERROR] Failed to get local variables: " + vars_result.m_error + "\n";
            } else {
                log += "[OK] Got " + std::to_string( vars_result.m_data[ "variables" ].size( ) ) + " local variables\n";
            }

            std::string vars_list;
            if ( vars_result.m_success && vars_result.m_data.contains( "variables" ) ) {
                vars_list = "Current local variables:\n";
                for ( const auto &v : vars_result.m_data[ "variables" ] ) {
                    vars_list += "- " + v.value( "name", "" ) + " : " + v.value( "type", "" );
                    if ( v.value( "is_arg", false ) ) {
                        vars_list += " (argument)";
                    }
                    vars_list += "\n";
                }
            }

            // Create AI prompt
            std::string prompt = R"(Analyze this decompiled function and suggest improvements to make it more readable.

Function: )" + func_name + R"(
Address: )" + func_address + R"(

)" + vars_list + R"(

Pseudocode:
)" + func_pseudocode + R"(

Please provide specific improvements in this EXACT JSON format:
{
  "variable_renames": [
    {"old_name": "v1", "new_name": "buffer_size"},
    {"old_name": "a2", "new_name": "input_data"}
  ],
  "comments": [
    {"line": 0, "text": "Parse configuration from input buffer"},
    {"line": 5, "text": "Validate checksum"}
  ],
  "explanation": "Brief explanation of changes"
}

Focus on:
1. Meaningful variable names based on usage patterns
2. Comments for complex logic
3. ONLY suggest changes you're confident about

Return ONLY the JSON, nothing else.)";

            // Call LLM
            log += "\n[INFO] Calling LLM...\n";

            std::vector< api::message_t > messages;
            messages.push_back( api::message_t::user( prompt ) );

            auto response = m_llm->send( messages );

            if ( response.ok( ) ) {
                log += "[OK] LLM responded with " + std::to_string( response.m_content.length( ) ) + " characters\n";
                log += "[INFO] LLM Response:\n" + response.m_content + "\n\n";

                {
                    std::lock_guard< std::mutex > lock( m_chat_mutex );
                    m_ai_improvement_result = response.m_content;
                    m_ai_improvement_log    = log;
                }

                // Parse and apply suggestions
                std::string apply_log = parse_and_apply_ai_suggestions( response.m_content, func_address );

                {
                    std::lock_guard< std::mutex > lock( m_chat_mutex );
                    m_ai_improvement_log += apply_log;
                }
            } else {
                log += "[ERROR] LLM call failed: " + response.m_error + "\n";
                std::lock_guard< std::mutex > lock( m_chat_mutex );
                m_ai_improvement_log = log;
            }

            m_ai_improving.store( false, std::memory_order_release );
        } ).detach( );
    }

    std::string c_ui::parse_and_apply_ai_suggestions( std::string_view ai_response, std::string_view func_address ) {
        std::string log;

        if ( !m_mcp || func_address.empty( ) ) {
            return "[ERROR] MCP not connected or no function address\n";
        }

        try {
            log += "[INFO] Parsing AI suggestions...\n";

            // Extract JSON from response
            std::string json_str( ai_response );

            size_t json_start = json_str.find( '{' );
            size_t json_end   = json_str.rfind( '}' );

            if ( json_start == std::string::npos || json_end == std::string::npos ) {
                log += "[ERROR] Could not find JSON in response\n";
                return log;
            }

            json_str  = json_str.substr( json_start, json_end - json_start + 1 );
            log      += "[INFO] Extracted JSON (" + std::to_string( json_str.length( ) ) + " chars)\n";

            auto suggestions  = json_t::parse( json_str );
            log              += "[OK] JSON parsed successfully\n\n";

            // Apply variable renames
            if ( suggestions.contains( "variable_renames" ) && suggestions[ "variable_renames" ].is_array( ) ) {
                log += "[INFO] Applying " + std::to_string( suggestions[ "variable_renames" ].size( ) ) + " variable renames...\n";
                for ( const auto &rename : suggestions[ "variable_renames" ] ) {
                    if ( rename.contains( "old_name" ) && rename.contains( "new_name" ) ) {
                        std::string old_name = rename[ "old_name" ].get< std::string >( );
                        std::string new_name = rename[ "new_name" ].get< std::string >( );

                        log += "  - Renaming '" + old_name + "' -> '" + new_name + "'... ";

                        auto result = m_mcp->rename_local_variable( func_address, old_name, new_name );

                        if ( result.m_success ) {
                            log += "OK\n";
                        } else {
                            log += "FAILED: " + result.m_error + "\n";
                        }

                        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
                    }
                }
            } else {
                log += "[INFO] No variable renames suggested\n";
            }

            // Apply comments
            if ( suggestions.contains( "comments" ) && suggestions[ "comments" ].is_array( ) ) {
                log += "[INFO] Applying " + std::to_string( suggestions[ "comments" ].size( ) ) + " comments...\n";
                for ( const auto &comment : suggestions[ "comments" ] ) {
                    if ( comment.contains( "text" ) ) {
                        std::string          text = comment[ "text" ].get< std::string >( );
                        std::optional< int > line_num;

                        if ( comment.contains( "line" ) && comment[ "line" ].is_number( ) ) {
                            line_num = comment[ "line" ].get< int >( );
                        }

                        log += "  - Adding comment: " + text.substr( 0, 50 ) + "...\n";
                        m_mcp->add_function_comment( func_address, text, line_num );

                        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
                    }
                }
            }

            // Wait for IDA to process
            std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

            // Fetch updated pseudocode for diff
            m_loading_diff_after.store( true, std::memory_order_release );

            auto result = m_mcp->get_function_pseudocode( func_address );

            if ( result.m_success && result.m_data.contains( "pseudocode" ) ) {
                std::lock_guard< std::mutex > lock( m_chat_mutex );
                m_diff_after = result.m_data[ "pseudocode" ].get< std::string >( );

                // Update current function pseudocode
                m_current_func.m_pseudocode = m_diff_after;

                // Show diff viewer
                m_show_diff_viewer  = true;
                log                += "\n[OK] All changes applied successfully\n";
            } else {
                log += "\n[ERROR] Failed to fetch updated pseudocode\n";
            }

            m_loading_diff_after.store( false, std::memory_order_release );

            return log;

        } catch ( const json_t::exception &e ) {
            log += "[ERROR] JSON parse error: " + std::string( e.what( ) ) + "\n";
            return log;
        } catch ( ... ) {
            log += "[ERROR] Unknown error occurred\n";
            return log;
        }
    }

    void c_ui::render_ai_log_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 800, 600 ), ImGuiCond_FirstUseEver );
        if ( !ImGui::Begin( "AI Improvement Log", &m_show_ai_log ) ) {
            ImGui::End( );
            return;
        }

        if ( ImGui::Button( "Close" ) ) {
            m_show_ai_log = false;
        }

        ImGui::SameLine( );
        if ( ImGui::Button( "Clear Log" ) ) {
            m_ai_improvement_log.clear( );
        }

        ImGui::Separator( );

        ImGui::BeginChild( "##log_scroll", ImVec2( 0, 0 ), true, ImGuiWindowFlags_HorizontalScrollbar );
        ImGui::TextUnformatted( m_ai_improvement_log.c_str( ) );
        ImGui::EndChild( );

        ImGui::End( );
    }

    void c_ui::render_plugin_installer_window( ) {
        ImGui::SetNextWindowSize( ImVec2( 700, 500 ), ImGuiCond_FirstUseEver );
        if ( !ImGui::Begin( "IDA Plugin Installer", &m_show_plugin_installer ) ) {
            ImGui::End( );
            return;
        }

        ImGui::TextWrapped( "This will install ida_mcp_plugin.py to your IDA Pro plugins directory." );
        ImGui::Spacing( );

        ImGui::SeparatorText( "Detected IDA Installations" );

        if ( ImGui::Button( "Refresh" ) ) {
            m_ida_installations = m_installer.detect_ida_installations( );
            m_selected_ida      = -1;
            m_install_status.clear( );
        }

        ImGui::Spacing( );

        if ( m_ida_installations.empty( ) ) {
            ImGui::TextColored( ImVec4( 1.0f, 0.7f, 0.3f, 1.0f ), "No IDA installations detected automatically." );
            ImGui::Spacing( );
            ImGui::TextWrapped( "You can manually copy the plugin file to your IDA plugins directory:" );
            ImGui::BulletText( "Windows: %%APPDATA%%\\Hex-Rays\\IDA Pro\\plugins\\" );
            ImGui::BulletText( "macOS: ~/.idapro/plugins/" );
            ImGui::BulletText( "Linux: ~/.idapro/plugins/" );
            ImGui::Spacing( );

            // Show plugin source path for manual copy
            auto exe_path      = std::filesystem::current_path( );
            auto plugin_source = exe_path.parent_path( ).parent_path( ).parent_path( ).parent_path( ) / "ida-plugin" / "ida_mcp_plugin.py";

            ImGui::Text( "Plugin source location:" );
            ImGui::InputText( "##plugin_source", const_cast< char * >( plugin_source.string( ).c_str( ) ), plugin_source.string( ).size( ),
                              ImGuiInputTextFlags_ReadOnly );
            ImGui::SameLine( );
            if ( ImGui::Button( "Copy Path" ) ) {
                ImGui::SetClipboardText( plugin_source.string( ).c_str( ) );
            }
        } else {
            // List detected installations
            ImGui::BeginChild( "##ida_list", ImVec2( 0, 200 ), true );

            for ( size_t i = 0; i < m_ida_installations.size( ); ++i ) {
                const auto &ida      = m_ida_installations[ i ];
                bool        selected = static_cast< int >( i ) == m_selected_ida;

                char label[ 512 ];
                snprintf( label, sizeof( label ), "IDA %s", ida.m_version.c_str( ) );

                if ( ImGui::Selectable( label, selected, ImGuiSelectableFlags_AllowDoubleClick ) ) {
                    m_selected_ida = static_cast< int >( i );

                    if ( ImGui::IsMouseDoubleClicked( 0 ) ) {
                        // Double-click to install
                        auto exe_path = std::filesystem::current_path( );
                        auto plugin_source
                            = exe_path.parent_path( ).parent_path( ).parent_path( ).parent_path( ) / "ida-plugin" / "ida_mcp_plugin.py";

                        if ( m_installer.install_plugin( ida, plugin_source ) ) {
                            m_install_status  = "Plugin installed successfully to:\n" + ida.m_plugin_dir.string( );
                            m_install_success = true;
                        } else {
                            m_install_status  = "Installation failed:\n" + std::string( m_installer.get_last_error( ) );
                            m_install_success = false;
                        }
                    }
                }

                if ( selected ) {
                    ImGui::Indent( );
                    ImGui::TextDisabled( "Path: %s", ida.m_install_dir.string( ).c_str( ) );
                    ImGui::TextDisabled( "Plugin Dir: %s", ida.m_plugin_dir.string( ).c_str( ) );
                    ImGui::TextDisabled( "Python: %s", ida.m_python_path.string( ).c_str( ) );
                    ImGui::Unindent( );
                }
            }

            ImGui::EndChild( );

            ImGui::Spacing( );

            // Install button
            ImGui::BeginDisabled( m_selected_ida < 0 );
            if ( ImGui::Button( "Install Plugin", ImVec2( 150, 0 ) ) ) {
                if ( m_selected_ida >= 0 && m_selected_ida < static_cast< int >( m_ida_installations.size( ) ) ) {
                    auto exe_path = std::filesystem::current_path( );
                    auto plugin_source
                        = exe_path.parent_path( ).parent_path( ).parent_path( ).parent_path( ) / "ida-plugin" / "ida_mcp_plugin.py";

                    const auto &ida = m_ida_installations[ m_selected_ida ];

                    if ( m_installer.install_plugin( ida, plugin_source ) ) {
                        m_install_status = "Plugin installed successfully!\n\nLocation: " + ida.m_plugin_dir.string( )
                                         + "\n\nRestart IDA Pro and press Ctrl+Shift+M to start the HTTP server.";
                        m_install_success = true;
                    } else {
                        m_install_status  = "Installation failed:\n" + std::string( m_installer.get_last_error( ) );
                        m_install_success = false;
                    }
                }
            }
            ImGui::EndDisabled( );

            ImGui::SameLine( );
            ImGui::TextDisabled( "Select an IDA installation and click Install" );
        }

        // Show installation status
        if ( !m_install_status.empty( ) ) {
            ImGui::Spacing( );
            ImGui::Separator( );
            ImGui::Spacing( );

            if ( m_install_success ) {
                ImGui::TextColored( ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ), "Success!" );
            } else {
                ImGui::TextColored( ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ), "Error" );
            }

            ImGui::Spacing( );
            ImGui::TextWrapped( "%s", m_install_status.c_str( ) );
        }

        ImGui::Spacing( );
        ImGui::Separator( );
        ImGui::Spacing( );

        ImGui::TextWrapped( "Note: Supports IDA Pro 8.3+ (IDA 9.0 recommended). Currently Windows only. macOS and Linux support planned." );

        ImGui::End( );
    }
} // namespace ida_re::ui
