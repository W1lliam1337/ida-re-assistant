#pragma once

namespace ida_re::api {
    enum class e_provider {
        claude,
        openai,
        gemini,
        openrouter
    };

    constexpr std::string_view to_string( e_provider p ) noexcept {
        switch ( p ) {
            case e_provider::claude :
                return "claude";
            case e_provider::openai :
                return "openai";
            case e_provider::gemini :
                return "gemini";
            case e_provider::openrouter :
                return "openrouter";
            default :
                return "unknown";
        }
    }

    struct message_t {
        std::string m_role { };
        std::string m_content { };

        [[nodiscard]] static constexpr message_t user( std::string_view c ) {
            return { "user", std::string( c ) };
        }

        [[nodiscard]] static constexpr message_t assistant( std::string_view c ) {
            return { "assistant", std::string( c ) };
        }

        [[nodiscard]] static constexpr message_t system( std::string_view c ) {
            return { "system", std::string( c ) };
        }
    };

    struct token_usage_t {
        int m_input { 0 };
        int m_output { 0 };

        [[nodiscard]] constexpr int total( ) const noexcept {
            return m_input + m_output;
        }
    };

    struct response_t {
        bool          m_success { false };
        std::string   m_content { };
        std::string   m_error { };
        std::string   m_model { };
        std::string   m_finish_reason { };
        token_usage_t m_usage { };
        e_provider    m_provider { e_provider::claude };

        [[nodiscard]] constexpr bool ok( ) const noexcept {
            return m_success && m_error.empty( );
        }

        [[nodiscard]] explicit constexpr operator bool ( ) const noexcept {
            return ok( );
        }
    };

    using stream_callback_t = std::function< void( std::string_view chunk ) >;

    struct model_t {
        std::string m_id { };
        std::string m_name { };
        e_provider  m_provider { e_provider::claude };
        int         m_context_window { 0 };
        bool        m_supports_streaming { true };
    };

    struct client_config_t {
        std::string m_api_key { };
        std::string m_model { };
        std::string m_system_prompt { };
        std::string m_base_url { };
        int         m_max_tokens { 4096 };
        float       m_temperature { 0.7f };
    };

    // Base client template
    template < typename Derived >
    class c_client_base {
      public:
        void set_api_key( std::string_view key ) {
            const std::lock_guard< std::mutex > lk( m_mutex );
            m_config.m_api_key = key;
        }

        void set_model( std::string_view model ) {
            const std::lock_guard< std::mutex > lk( m_mutex );
            m_config.m_model = model;
        }

        void set_system_prompt( std::string_view prompt ) {
            const std::lock_guard< std::mutex > lk( m_mutex );
            m_config.m_system_prompt = prompt;
        }

        void set_max_tokens( int tokens ) {
            const std::lock_guard< std::mutex > lk( m_mutex );
            m_config.m_max_tokens = tokens;
        }

        [[nodiscard]] bool has_api_key( ) const {
            const std::lock_guard< std::mutex > lk( m_mutex );
            return !m_config.m_api_key.empty( );
        }

        [[nodiscard]] std::string get_model( ) const {
            const std::lock_guard< std::mutex > lk( m_mutex );
            return m_config.m_model;
        }

        [[nodiscard]] bool is_busy( ) const noexcept {
            return m_busy.load( );
        }

        void cancel( ) noexcept {
            m_cancel.store( true );
        }

      protected:
        client_config_t     m_config { };
        mutable std::mutex  m_mutex { };
        std::atomic< bool > m_busy { false };
        std::atomic< bool > m_cancel { false };

        [[nodiscard]] client_config_t snapshot( ) const {
            const std::lock_guard< std::mutex > lk( m_mutex );
            return m_config;
        }

        struct guard_t {
            std::atomic< bool > &busy;
            std::atomic< bool > &cancel;

            guard_t( std::atomic< bool > &b, std::atomic< bool > &c ) : busy( b ), cancel( c ) {
                busy.store( true, std::memory_order_release );
                cancel.store( false, std::memory_order_release );
            }

            ~guard_t( ) {
                busy.store( false, std::memory_order_release );
            }

            [[nodiscard]] bool cancelled( ) const noexcept {
                return cancel.load( std::memory_order_acquire );
            }
        };
    };

    // Claude
    class c_claude : public c_client_base< c_claude > {
      public:
        c_claude( ) {
            m_config.m_model = "claude-sonnet-4-5-20250929";
        }

        void set_base_url( std::string_view url ) {
            const std::lock_guard< std::mutex > lk( m_mutex );
            m_config.m_base_url = url;
        }

        response_t send( std::string_view message );
        response_t send( const std::vector< message_t > &messages );
        void       stream( std::string_view message, stream_callback_t cb );
        void       stream( const std::vector< message_t > &messages, stream_callback_t cb );

        [[nodiscard]] static consteval e_provider provider( ) noexcept {
            return e_provider::claude;
        }

        [[nodiscard]] static std::vector< model_t > models( );

      private:
        response_t  request( const json_t &body );
        void        stream_request( const json_t &body, stream_callback_t cb );
        json_t      make_body( const std::vector< message_t > &msgs, bool stream ) const;
        std::string host( ) const;
    };

    // OpenAI
    class c_openai : public c_client_base< c_openai > {
      public:
        c_openai( ) {
            m_config.m_model = "gpt-4o";
        }

        void set_base_url( std::string_view url ) {
            const std::lock_guard< std::mutex > lk( m_mutex );
            m_config.m_base_url = url;
        }

        response_t send( std::string_view message );
        response_t send( const std::vector< message_t > &messages );
        void       stream( std::string_view message, stream_callback_t cb );
        void       stream( const std::vector< message_t > &messages, stream_callback_t cb );

        [[nodiscard]] static consteval e_provider provider( ) noexcept {
            return e_provider::openai;
        }

        [[nodiscard]] static std::vector< model_t > models( );

      private:
        response_t  request( const json_t &body );
        void        stream_request( const json_t &body, stream_callback_t cb );
        json_t      make_body( const std::vector< message_t > &msgs, bool stream ) const;
        std::string host( ) const;
    };

    // Gemini (Google AI)
    class c_gemini : public c_client_base< c_gemini > {
      public:
        c_gemini( ) {
            m_config.m_model = "gemini-2.0-flash";
        }

        response_t send( std::string_view message );
        response_t send( const std::vector< message_t > &messages );
        void       stream( std::string_view message, stream_callback_t cb );
        void       stream( const std::vector< message_t > &messages, stream_callback_t cb );

        [[nodiscard]] static consteval e_provider provider( ) noexcept {
            return e_provider::gemini;
        }

        [[nodiscard]] static std::vector< model_t > models( );

      private:
        response_t  request( const std::vector< message_t > &messages );
        void        stream_request( const std::vector< message_t > &messages, stream_callback_t cb );
        json_t      make_body( const std::vector< message_t > &msgs ) const;
        std::string get_url( bool stream ) const;
    };

    // OpenRouter (multi-model aggregator)
    class c_openrouter : public c_client_base< c_openrouter > {
      public:
        c_openrouter( ) {
            m_config.m_model = "mistralai/devstral-small:free";
        }

        response_t send( std::string_view message );
        response_t send( const std::vector< message_t > &messages );
        void       stream( std::string_view message, stream_callback_t cb );
        void       stream( const std::vector< message_t > &messages, stream_callback_t cb );

        [[nodiscard]] static consteval e_provider provider( ) noexcept {
            return e_provider::openrouter;
        }

        // Dynamic model fetching
        [[nodiscard]] std::vector< model_t > fetch_models( bool force_refresh = false );
        [[nodiscard]] const std::vector< model_t > &cached_models( ) const noexcept {
            return m_cached_models;
        }

        void set_show_free_only( bool free_only ) noexcept {
            m_show_free_only = free_only;
        }

        [[nodiscard]] bool show_free_only( ) const noexcept {
            return m_show_free_only;
        }

      private:
        response_t                                        request( const json_t &body );
        void                                              stream_request( const json_t &body, stream_callback_t cb );
        json_t                                            make_body( const std::vector< message_t > &msgs, bool stream ) const;
        std::vector< model_t >                            parse_models_response( const json_t &data );

        std::vector< model_t >                            m_cached_models { };
        std::chrono::steady_clock::time_point             m_models_fetched_at { };
        bool                                              m_show_free_only { true };
        static constexpr std::chrono::minutes             k_cache_duration { 30 };
    };

    // Manager
    class c_llm_manager {
      public:
        c_llm_manager( ) = default;

        void set_provider( e_provider p ) noexcept {
            m_provider = p;
        }

        [[nodiscard]] e_provider get_provider( ) const noexcept {
            return m_provider;
        }

        [[nodiscard]] c_claude &claude( ) noexcept {
            return m_claude;
        }

        [[nodiscard]] c_openai &openai( ) noexcept {
            return m_openai;
        }

        [[nodiscard]] c_gemini &gemini( ) noexcept {
            return m_gemini;
        }

        [[nodiscard]] c_openrouter &openrouter( ) noexcept {
            return m_openrouter;
        }

        response_t send( std::string_view message );
        response_t send( const std::vector< message_t > &messages );
        void       stream( std::string_view message, stream_callback_t cb );

        std::vector< model_t > all_models( ) const;

        // RE helpers
        response_t analyze_code( std::string_view code, std::string_view prompt = "" );
        response_t explain_function( std::string_view pseudocode );
        response_t find_vulnerabilities( std::string_view code );
        response_t suggest_name( std::string_view pseudocode );

      private:
        e_provider   m_provider { e_provider::claude };
        c_claude     m_claude { };
        c_openai     m_openai { };
        c_gemini     m_gemini { };
        c_openrouter m_openrouter { };
    };

} // namespace ida_re::api
