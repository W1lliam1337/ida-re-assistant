#pragma once

#include "../api/llm_api.hpp"
#include "../api/mcp_client.hpp"
#include "../core/config.hpp"
#include "../core/installer.hpp"
#include "../utils/analysis_history.hpp"
#include "../utils/syntax_highlighter.hpp"

#include <imgui.h>

namespace ida_re::ui {
    struct chat_message_t {
        bool                                  m_is_user { };
        std::string                           m_content { };
        std::chrono::system_clock::time_point m_timestamp { };
    };

    struct function_data_t {
        std::string                m_address { };
        std::string                m_name { };
        std::string                m_pseudocode { };
        std::string                m_assembly { };
        std::vector< std::string > m_xrefs_to { };
        std::vector< std::string > m_xrefs_from { };
        bool                       m_loaded { false };
    };

    struct bookmark_t {
        std::string                           m_address { };
        std::string                           m_name { };
        std::string                           m_note { };
        std::string                           m_file_md5 { };
        std::string                           m_file_name { };
        std::chrono::system_clock::time_point m_timestamp { };
    };

    struct custom_prompt_t {
        std::string m_name { };
        std::string m_prompt { };
        bool        m_enabled { true };
    };

    struct pinned_function_t {
        std::string                           m_address { };
        std::string                           m_name { };
        std::string                           m_file_md5 { };
        std::string                           m_file_name { };
        std::string                           m_note { };
        std::chrono::system_clock::time_point m_timestamp { };
    };

    class c_ui {
      public:
        c_ui( );
        ~c_ui( );

        void init( );
        void render( );
        void shutdown( );

        void set_mcp_client( api::c_mcp_client *client ) noexcept {
            m_mcp = client;
        }

        void set_llm_manager( api::c_llm_manager *manager ) noexcept {
            m_llm = manager;
        }

        void set_config( core::app_config_t *config ) noexcept {
            m_config = config;
        }

        [[nodiscard]] utils::c_analysis_history &get_history( ) noexcept {
            return m_history;
        }

      private:
        void render_menu_bar( );
        void render_connection_panel( );
        void render_function_panel( );
        void render_analysis_panel( );
        void render_chat_panel( );
        void render_settings_window( );
        void render_history_window( );
        void render_bookmarks_window( );
        void render_custom_prompts_window( );

        void        apply_style( );
        void        apply_dark_theme( );
        void        apply_light_theme( );
        void        load_function( std::string_view address );
        void        send_chat_message( std::string_view message );
        void        send_analysis_chat_message( std::string_view message );
        void        analyze_current_function( );
        void        perform_analysis( std::string_view type, bool force_new = false );
        void        perform_custom_analysis( std::string_view prompt_name );
        void        apply_config_to_llm( );
        void        save_cache( );
        void        load_cache( );
        void        clear_cache( );
        void        save_bookmarks( );
        void        load_bookmarks( );
        void        save_custom_prompts( );
        void        load_custom_prompts( );
        void        save_pinned_functions( );
        void        load_pinned_functions( );
        void        render_pinned_window( );
        void        render_local_variables_panel( );
        void        render_diff_viewer_window( );
        void        render_ai_log_window( );
        void        render_plugin_installer_window( );
        void        export_history_markdown( std::string_view path );
        void        export_history_html( std::string_view path );
        void        rename_function_in_ida( std::string_view new_name );
        void        add_comment_in_ida( std::string_view comment );
        void        load_local_variables( );
        void        apply_refactoring_changes( );
        void        ai_improve_pseudocode( );
        std::string parse_and_apply_ai_suggestions( std::string_view ai_response, std::string_view func_address );

        api::c_mcp_client       *m_mcp { nullptr };
        api::c_llm_manager      *m_llm { nullptr };
        core::app_config_t      *m_config { nullptr };
        core::c_plugin_installer m_installer { };

        utils::c_syntax_highlighter m_highlighter { };
        utils::c_analysis_history   m_history { };

        // state
        char                                                 m_address_input[ 64 ] { "0x" };
        char                                                 m_function_filter[ 256 ] { };
        function_data_t                                      m_current_func { };
        std::vector< std::pair< std::string, std::string > > m_function_list { };

        // chat
        std::deque< chat_message_t > m_chat_history { };
        char                         m_chat_input[ 4096 ] { };
        std::string                  m_streaming_buffer { };
        std::atomic< bool >          m_chat_loading { false };
        std::mutex                   m_chat_mutex { };
        std::thread                  m_chat_thread { };

        // analysis
        std::string                  m_analysis_result { };
        std::atomic< bool >          m_analysis_loading { false };
        std::thread                  m_analysis_thread { };
        std::deque< chat_message_t > m_analysis_chat_history { };
        char                         m_analysis_chat_input[ 4096 ] { };
        std::string                  m_analysis_context { }; // stores the initial analysis context

        // windows
        bool m_show_settings { false };
        bool m_show_history { false };
        int  m_current_tab { 0 };

        // settings state
        int  m_selected_provider { 2 }; // 0=Claude, 1=OpenAI, 2=Gemini, 3=OpenRouter
        char m_claude_key_buf[ 256 ] { };
        char m_openai_key_buf[ 256 ] { };
        char m_gemini_key_buf[ 256 ] { };
        char m_openrouter_key_buf[ 256 ] { };
        char m_openrouter_model_filter[ 128 ] { };
        int  m_openrouter_selected_model { 0 };
        bool m_openrouter_free_only { true };
        bool m_openrouter_models_fetched { false };
        char m_openai_base_url_buf[ 256 ] { };
        char m_anthropic_base_url_buf[ 256 ] { };
        char m_mcp_host_buf[ 64 ] { "127.0.0.1" };
        int  m_mcp_port_buf { 13120 };
        bool m_settings_initialized { false };

        // history state
        int m_selected_history_entry { -1 };

        // xref preview cache
        std::unordered_map< std::string, std::string > m_xref_preview_cache { };

        // analysis results cache: file_md5 -> {address -> {type -> result}}
        std::unordered_map< std::string, std::unordered_map< std::string, std::unordered_map< std::string, std::string > > >
                    m_analysis_cache { };
        std::string m_current_file_md5 { };
        std::string m_current_file_name { };

        // cache popup state
        bool        m_show_cache_popup { false };
        std::string m_pending_analysis_type { };

        // bookmarks
        std::vector< bookmark_t > m_bookmarks { };
        bool                      m_show_bookmarks { false };
        char                      m_bookmark_note[ 256 ] { };

        // custom prompts
        std::vector< custom_prompt_t > m_custom_prompts { };
        bool                           m_show_custom_prompts { false };
        char                           m_new_prompt_name[ 64 ] { };
        char                           m_new_prompt_text[ 2048 ] { };

        // pinned functions
        std::vector< pinned_function_t > m_pinned_functions { };
        bool                             m_show_pinned { false };

        // history search
        char m_history_filter[ 256 ] { };

        // theme
        bool m_dark_theme { true };

        // suggested name for apply
        std::string m_suggested_name { };
        bool        m_show_rename_popup { false };
        char        m_rename_input[ 256 ] { };

        // last analysis type for context
        std::string m_last_analysis_type { };

        // local variables for refactoring
        struct local_var_t {
            std::string m_name { };
            std::string m_type { };
            bool        m_is_arg { false };
            bool        m_selected { false };
            char        m_new_name[ 128 ] { };
            char        m_new_type[ 128 ] { };
        };

        std::vector< local_var_t > m_local_variables { };
        bool                       m_show_local_vars { false };
        std::atomic< bool >        m_loading_vars { false };

        // diff viewer
        std::string         m_diff_before { };
        std::string         m_diff_after { };
        bool                m_show_diff_viewer { false };
        std::atomic< bool > m_loading_diff_after { false };

        // AI improvement
        std::atomic< bool > m_ai_improving { false };
        std::string         m_ai_improvement_result { };
        std::string         m_ai_improvement_log { };
        bool                m_show_ai_log { false };

        // Plugin installer
        bool                                    m_show_plugin_installer { false };
        std::vector< core::ida_installation_t > m_ida_installations { };
        int                                     m_selected_ida { -1 };
        std::string                             m_install_status { };
        bool                                    m_install_success { false };
    };

} // namespace ida_re::ui
