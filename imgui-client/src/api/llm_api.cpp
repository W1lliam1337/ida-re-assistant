#include "vendor.hpp"

#include "llm_api.hpp"
#include <httplib.h>

namespace ida_re::api {
    // ==================== Claude ====================
    std::vector< model_t > c_claude::models( ) {
        return {
            { "claude-sonnet-4-5-20250929", "Claude Sonnet 4.5", e_provider::claude, 200000, true },
            {   "claude-opus-4-5-20251101",   "Claude Opus 4.5", e_provider::claude, 200000, true },
            { "claude-3-5-sonnet-20241022", "Claude 3.5 Sonnet", e_provider::claude, 200000, true },
            {  "claude-3-5-haiku-20241022",  "Claude 3.5 Haiku", e_provider::claude, 200000, true },
        };
    }

    std::string c_claude::host( ) const {
        std::lock_guard< std::mutex > lk( m_mutex );
        return m_config.m_base_url.empty( ) ? "api.anthropic.com" : m_config.m_base_url;
    }

    json_t c_claude::make_body( const std::vector< message_t > &msgs, bool stream ) const {
        json_t messages        = json_t::array( );
        auto   non_system_msgs = msgs | std::views::filter( []( const auto &m ) {
                                   return m.m_role != "system";
                               } );
        for ( const auto &m : non_system_msgs ) {
            messages.push_back( {
                {    "role",    m.m_role },
                { "content", m.m_content }
            } );
        }

        json_t body = {
            {      "model",      m_config.m_model },
            { "max_tokens", m_config.m_max_tokens },
            {   "messages",              messages }
        };

        if ( !m_config.m_system_prompt.empty( ) ) {
            body[ "system" ] = m_config.m_system_prompt;
        }

        if ( stream ) {
            body[ "stream" ] = true;
        }

        return body;
    }

    response_t c_claude::send( std::string_view message ) {
        return send( std::vector< message_t > { message_t::user( message ) } );
    }

    response_t c_claude::send( const std::vector< message_t > &messages ) {
        auto cfg = snapshot( );
        return request( make_body( messages, false ) );
    }

    void c_claude::stream( std::string_view message, stream_callback_t cb ) {
        stream( std::vector< message_t > { message_t::user( message ) }, cb );
    }

    void c_claude::stream( const std::vector< message_t > &messages, stream_callback_t cb ) {
        auto cfg = snapshot( );
        stream_request( make_body( messages, true ), cb );
    }

    response_t c_claude::request( const json_t &body ) {
        guard_t    guard( m_busy, m_cancel );
        response_t resp;
        resp.m_provider = e_provider::claude;

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) ) {
            resp.m_error = "API key not set";
            return resp;
        }

        httplib::SSLClient client( host( ), 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            {         "x-api-key",        cfg.m_api_key },
            { "anthropic-version",         "2023-06-01" },
            {      "content-type", "application/json_t" }
        };

        auto result = client.Post( "/v1/messages", headers, body.dump( ), "application/json_t" );

        if ( !result ) {
            resp.m_error = "Request failed: " + httplib::to_string( result.error( ) );
            return resp;
        }

        if ( result->status != 200 ) {
            try {
                auto err = json_t::parse( result->body );
                if ( err.contains( "error" ) && err[ "error" ].contains( "message" ) ) {
                    resp.m_error = err[ "error" ][ "message" ].get< std::string >( );
                } else {
                    resp.m_error = result->body;
                }
            } catch ( ... ) {
                resp.m_error = "HTTP " + std::to_string( result->status );
            }
            return resp;
        }

        try {
            auto j = json_t::parse( result->body );

            if ( j.contains( "content" ) && j[ "content" ].is_array( ) ) {
                for ( const auto &block : j[ "content" ] ) {
                    if ( block.value( "type", "" ) == "text" ) {
                        resp.m_content += block.value( "text", "" );
                    }
                }
            }

            if ( j.contains( "usage" ) ) {
                resp.m_usage.m_input  = j[ "usage" ].value( "input_tokens", 0 );
                resp.m_usage.m_output = j[ "usage" ].value( "output_tokens", 0 );
            }

            resp.m_model         = j.value( "model", cfg.m_model );
            resp.m_finish_reason = j.value( "stop_reason", "" );
            resp.m_success       = true;

        } catch ( const std::exception &e ) {
            resp.m_error = std::string( "Parse error: " ) + e.what( );
        }

        return resp;
    }

    void c_claude::stream_request( const json_t &body, stream_callback_t cb ) {
        guard_t guard( m_busy, m_cancel );

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) || !cb )
            return;

        httplib::SSLClient client( host( ), 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            {         "x-api-key",        cfg.m_api_key },
            { "anthropic-version",         "2023-06-01" },
            {      "content-type", "application/json_t" }
        };

        std::string buffer;

        client.Post( "/v1/messages", headers, body.dump( ), "application/json_t", [ & ]( const char *data, size_t len ) -> bool {
            if ( guard.cancelled( ) )
                return false;

            buffer.append( data, len );

            size_t pos;
            while ( ( pos = buffer.find( '\n' ) ) != std::string::npos ) {
                std::string line = buffer.substr( 0, pos );
                buffer.erase( 0, pos + 1 );

                if ( line.empty( ) || line[ 0 ] == ':' )
                    continue;

                if ( line.rfind( "data: ", 0 ) == 0 ) {
                    std::string json_t_str = line.substr( 6 );
                    if ( json_t_str == "[DONE]" )
                        continue;

                    try {
                        auto j = json_t::parse( json_t_str );
                        if ( j.value( "type", "" ) == "content_block_delta" ) {
                            if ( j.contains( "delta" ) && j[ "delta" ].value( "type", "" ) == "text_delta" ) {
                                std::string text = j[ "delta" ].value( "text", "" );
                                if ( !text.empty( ) )
                                    cb( text );
                            }
                        }
                    } catch ( ... ) { }
                }
            }
            return true;
        } );
    }

    // ==================== OpenAI ====================

    std::vector< model_t > c_openai::models( ) {
        return {
            {      "gpt-4o",      "GPT-4o", e_provider::openai, 128000, true },
            { "gpt-4o-mini", "GPT-4o Mini", e_provider::openai, 128000, true },
            { "gpt-4-turbo", "GPT-4 Turbo", e_provider::openai, 128000, true },
        };
    }

    std::string c_openai::host( ) const {
        std::lock_guard< std::mutex > lk( m_mutex );
        return m_config.m_base_url.empty( ) ? "api.openai.com" : m_config.m_base_url;
    }

    json_t c_openai::make_body( const std::vector< message_t > &msgs, bool stream ) const {
        json_t messages = json_t::array( );

        if ( !m_config.m_system_prompt.empty( ) ) {
            messages.push_back( {
                {    "role",                 "system" },
                { "content", m_config.m_system_prompt }
            } );
        }

        for ( const auto &m : msgs ) {
            messages.push_back( {
                {    "role",    m.m_role },
                { "content", m.m_content }
            } );
        }

        json_t body = {
            {      "model",      m_config.m_model },
            { "max_tokens", m_config.m_max_tokens },
            {   "messages",              messages }
        };

        if ( stream ) {
            body[ "stream" ] = true;
        }

        return body;
    }

    response_t c_openai::send( std::string_view message ) {
        return send( std::vector< message_t > { message_t::user( message ) } );
    }

    response_t c_openai::send( const std::vector< message_t > &messages ) {
        auto cfg = snapshot( );
        return request( make_body( messages, false ) );
    }

    void c_openai::stream( std::string_view message, stream_callback_t cb ) {
        stream( std::vector< message_t > { message_t::user( message ) }, cb );
    }

    void c_openai::stream( const std::vector< message_t > &messages, stream_callback_t cb ) {
        auto cfg = snapshot( );
        stream_request( make_body( messages, true ), cb );
    }

    response_t c_openai::request( const json_t &body ) {
        guard_t    guard( m_busy, m_cancel );
        response_t resp;
        resp.m_provider = e_provider::openai;

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) ) {
            resp.m_error = "API key not set";
            return resp;
        }

        httplib::SSLClient client( host( ), 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            { "Authorization", "Bearer " + cfg.m_api_key },
            {  "content-type",      "application/json_t" }
        };

        auto result = client.Post( "/v1/chat/completions", headers, body.dump( ), "application/json_t" );

        if ( !result ) {
            resp.m_error = "Request failed: " + httplib::to_string( result.error( ) );
            return resp;
        }

        if ( result->status != 200 ) {
            try {
                auto err = json_t::parse( result->body );
                if ( err.contains( "error" ) && err[ "error" ].contains( "message" ) ) {
                    resp.m_error = err[ "error" ][ "message" ].get< std::string >( );
                } else {
                    resp.m_error = result->body;
                }
            } catch ( ... ) {
                resp.m_error = "HTTP " + std::to_string( result->status );
            }
            return resp;
        }

        try {
            auto j = json_t::parse( result->body );

            if ( j.contains( "choices" ) && !j[ "choices" ].empty( ) ) {
                auto &choice = j[ "choices" ][ 0 ];
                if ( choice.contains( "message" ) ) {
                    resp.m_content = choice[ "message" ].value( "content", "" );
                }
                resp.m_finish_reason = choice.value( "finish_reason", "" );
            }

            if ( j.contains( "usage" ) ) {
                resp.m_usage.m_input  = j[ "usage" ].value( "prompt_tokens", 0 );
                resp.m_usage.m_output = j[ "usage" ].value( "completion_tokens", 0 );
            }

            resp.m_model   = j.value( "model", cfg.m_model );
            resp.m_success = true;

        } catch ( const std::exception &e ) {
            resp.m_error = std::string( "Parse error: " ) + e.what( );
        }

        return resp;
    }

    void c_openai::stream_request( const json_t &body, stream_callback_t cb ) {
        guard_t guard( m_busy, m_cancel );

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) || !cb )
            return;

        httplib::SSLClient client( host( ), 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            { "Authorization", "Bearer " + cfg.m_api_key },
            {  "content-type",      "application/json_t" }
        };

        std::string buffer;

        client.Post( "/v1/chat/completions", headers, body.dump( ), "application/json_t", [ & ]( const char *data, size_t len ) -> bool {
            if ( guard.cancelled( ) )
                return false;

            buffer.append( data, len );

            size_t pos;
            while ( ( pos = buffer.find( '\n' ) ) != std::string::npos ) {
                std::string line = buffer.substr( 0, pos );
                buffer.erase( 0, pos + 1 );

                if ( line.empty( ) )
                    continue;

                if ( line.rfind( "data: ", 0 ) == 0 ) {
                    std::string json_t_str = line.substr( 6 );
                    if ( json_t_str == "[DONE]" )
                        continue;

                    try {
                        auto j = json_t::parse( json_t_str );
                        if ( j.contains( "choices" ) && !j[ "choices" ].empty( ) ) {
                            auto &delta = j[ "choices" ][ 0 ][ "delta" ];
                            if ( delta.contains( "content" ) ) {
                                std::string text = delta[ "content" ].get< std::string >( );
                                if ( !text.empty( ) )
                                    cb( text );
                            }
                        }
                    } catch ( ... ) { }
                }
            }
            return true;
        } );
    }

    // ==================== Gemini ====================

    std::vector< model_t > c_gemini::models( ) {
        return {
            {               "gemini-2.0-flash-exp", "Gemini 2.0 Flash Experimental (Free)", e_provider::gemini, 1000000, true },
            { "gemini-2.0-flash-thinking-exp-1219",        "Gemini 2.0 Flash Thinking Exp", e_provider::gemini, 1000000, true },
            {                   "gemini-1.5-flash",                     "Gemini 1.5 Flash", e_provider::gemini, 1000000, true },
            {                "gemini-1.5-flash-8b",                  "Gemini 1.5 Flash 8B", e_provider::gemini, 1000000, true },
            {                     "gemini-1.5-pro",                       "Gemini 1.5 Pro", e_provider::gemini, 2000000, true },
        };
    }

    std::string c_gemini::get_url( bool stream ) const {
        std::string action = stream ? "streamGenerateContent" : "generateContent";
        return "/v1beta/models/" + m_config.m_model + ":" + action + "?key=" + m_config.m_api_key;
    }

    json_t c_gemini::make_body( const std::vector< message_t > &msgs ) const {
        json_t contents = json_t::array( );

        // System instruction goes separately in Gemini API
        json_t system_instruction;
        if ( !m_config.m_system_prompt.empty( ) ) {
            system_instruction = {
                { "parts", { { { "text", m_config.m_system_prompt } } } }
            };
        }

        auto non_system_msgs = msgs | std::views::filter( []( const auto &m ) {
                                   return m.m_role != "system";
                               } );
        for ( const auto &m : non_system_msgs ) {
            const std::string role = ( m.m_role == "assistant" ) ? "model" : "user";
            contents.push_back( {
                {  "role",                            role },
                { "parts", { { { "text", m.m_content } } } }
            } );
        }

        json_t body = {
            {         "contents",                                                                                    contents },
            { "generationConfig", { { "maxOutputTokens", m_config.m_max_tokens }, { "temperature", m_config.m_temperature } } }
        };

        if ( !system_instruction.is_null( ) ) {
            body[ "systemInstruction" ] = system_instruction;
        }

        return body;
    }

    response_t c_gemini::send( std::string_view message ) {
        return send( std::vector< message_t > { message_t::user( message ) } );
    }

    response_t c_gemini::send( const std::vector< message_t > &messages ) {
        return request( messages );
    }

    void c_gemini::stream( std::string_view message, stream_callback_t cb ) {
        stream( std::vector< message_t > { message_t::user( message ) }, cb );
    }

    void c_gemini::stream( const std::vector< message_t > &messages, stream_callback_t cb ) {
        stream_request( messages, cb );
    }

    response_t c_gemini::request( const std::vector< message_t > &messages ) {
        guard_t    guard( m_busy, m_cancel );
        response_t resp;
        resp.m_provider = e_provider::gemini;

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) ) {
            resp.m_error = "API key not set";
            return resp;
        }

        httplib::SSLClient client( "generativelanguage.googleapis.com", 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            { "content-type", "application/json_t" }
        };

        json_t      body = make_body( messages );
        std::string url  = get_url( false );

        auto result = client.Post( url, headers, body.dump( ), "application/json_t" );

        if ( !result ) {
            resp.m_error = "Request failed: " + httplib::to_string( result.error( ) );
            return resp;
        }

        if ( result->status != 200 ) {
            try {
                auto err = json_t::parse( result->body );
                if ( err.contains( "error" ) && err[ "error" ].contains( "message" ) ) {
                    resp.m_error = err[ "error" ][ "message" ].get< std::string >( );
                } else {
                    resp.m_error = result->body;
                }
            } catch ( ... ) {
                resp.m_error = "HTTP " + std::to_string( result->status );
            }
            return resp;
        }

        try {
            auto j = json_t::parse( result->body );

            if ( j.contains( "candidates" ) && !j[ "candidates" ].empty( ) ) {
                auto &candidate = j[ "candidates" ][ 0 ];
                if ( candidate.contains( "content" ) && candidate[ "content" ].contains( "parts" ) ) {
                    for ( const auto &part : candidate[ "content" ][ "parts" ] ) {
                        if ( part.contains( "text" ) ) {
                            resp.m_content += part[ "text" ].get< std::string >( );
                        }
                    }
                }
                resp.m_finish_reason = candidate.value( "finishReason", "" );
            }

            if ( j.contains( "usageMetadata" ) ) {
                resp.m_usage.m_input  = j[ "usageMetadata" ].value( "promptTokenCount", 0 );
                resp.m_usage.m_output = j[ "usageMetadata" ].value( "candidatesTokenCount", 0 );
            }

            resp.m_model   = cfg.m_model;
            resp.m_success = true;

        } catch ( const std::exception &e ) {
            resp.m_error = std::string( "Parse error: " ) + e.what( );
        }

        return resp;
    }

    void c_gemini::stream_request( const std::vector< message_t > &messages, stream_callback_t cb ) {
        guard_t guard( m_busy, m_cancel );

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) || !cb )
            return;

        httplib::SSLClient client( "generativelanguage.googleapis.com", 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            { "content-type", "application/json_t" }
        };

        json_t      body = make_body( messages );
        std::string url  = get_url( true ) + "&alt=sse";

        std::string buffer;

        client.Post( url, headers, body.dump( ), "application/json_t", [ & ]( const char *data, size_t len ) -> bool {
            if ( guard.cancelled( ) )
                return false;

            buffer.append( data, len );

            size_t pos;
            while ( ( pos = buffer.find( '\n' ) ) != std::string::npos ) {
                std::string line = buffer.substr( 0, pos );
                buffer.erase( 0, pos + 1 );

                if ( line.empty( ) )
                    continue;

                if ( line.rfind( "data: ", 0 ) == 0 ) {
                    std::string json_t_str = line.substr( 6 );

                    try {
                        auto j = json_t::parse( json_t_str );
                        if ( j.contains( "candidates" ) && !j[ "candidates" ].empty( ) ) {
                            auto &candidate = j[ "candidates" ][ 0 ];
                            if ( candidate.contains( "content" ) && candidate[ "content" ].contains( "parts" ) ) {
                                for ( const auto &part : candidate[ "content" ][ "parts" ] ) {
                                    if ( part.contains( "text" ) ) {
                                        std::string text = part[ "text" ].get< std::string >( );
                                        if ( !text.empty( ) )
                                            cb( text );
                                    }
                                }
                            }
                        }
                    } catch ( ... ) { }
                }
            }
            return true;
        } );
    }

    // ==================== OpenRouter ====================

    json_t c_openrouter::make_body( const std::vector< message_t > &msgs, bool stream ) const {
        json_t messages = json_t::array( );

        if ( !m_config.m_system_prompt.empty( ) ) {
            messages.push_back( {
                {    "role",                 "system" },
                { "content", m_config.m_system_prompt }
            } );
        }

        for ( const auto &m : msgs ) {
            messages.push_back( {
                {    "role",    m.m_role },
                { "content", m.m_content }
            } );
        }

        json_t body = {
            {      "model",      m_config.m_model },
            { "max_tokens", m_config.m_max_tokens },
            {   "messages",              messages }
        };

        if ( stream ) {
            body[ "stream" ] = true;
        }

        return body;
    }

    response_t c_openrouter::send( std::string_view message ) {
        return send( std::vector< message_t > { message_t::user( message ) } );
    }

    response_t c_openrouter::send( const std::vector< message_t > &messages ) {
        return request( make_body( messages, false ) );
    }

    void c_openrouter::stream( std::string_view message, stream_callback_t cb ) {
        stream( std::vector< message_t > { message_t::user( message ) }, cb );
    }

    void c_openrouter::stream( const std::vector< message_t > &messages, stream_callback_t cb ) {
        stream_request( make_body( messages, true ), cb );
    }

    response_t c_openrouter::request( const json_t &body ) {
        guard_t    guard( m_busy, m_cancel );
        response_t resp;
        resp.m_provider = e_provider::openrouter;

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) ) {
            resp.m_error = "API key not set";
            return resp;
        }

        httplib::SSLClient client( "openrouter.ai", 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            { "Authorization", "Bearer " + cfg.m_api_key },
            {  "content-type",          "application/json" },
            { "HTTP-Referer",   "https://github.com/W1lliam1337/ida-re-assistant" },
            {    "X-Title",                "IDA RE Assistant" }
        };

        auto result = client.Post( "/api/v1/chat/completions", headers, body.dump( ), "application/json" );

        if ( !result ) {
            resp.m_error = "Request failed: " + httplib::to_string( result.error( ) );
            return resp;
        }

        if ( result->status != 200 ) {
            try {
                auto err = json_t::parse( result->body );
                if ( err.contains( "error" ) && err[ "error" ].contains( "message" ) ) {
                    resp.m_error = err[ "error" ][ "message" ].get< std::string >( );
                } else {
                    resp.m_error = result->body;
                }
            } catch ( ... ) {
                resp.m_error = "HTTP " + std::to_string( result->status );
            }
            return resp;
        }

        try {
            auto j = json_t::parse( result->body );

            if ( j.contains( "choices" ) && !j[ "choices" ].empty( ) ) {
                auto &choice = j[ "choices" ][ 0 ];
                if ( choice.contains( "message" ) ) {
                    resp.m_content = choice[ "message" ].value( "content", "" );
                }
                resp.m_finish_reason = choice.value( "finish_reason", "" );
            }

            if ( j.contains( "usage" ) ) {
                resp.m_usage.m_input  = j[ "usage" ].value( "prompt_tokens", 0 );
                resp.m_usage.m_output = j[ "usage" ].value( "completion_tokens", 0 );
            }

            resp.m_model   = j.value( "model", cfg.m_model );
            resp.m_success = true;

        } catch ( const std::exception &e ) {
            resp.m_error = std::string( "Parse error: " ) + e.what( );
        }

        return resp;
    }

    void c_openrouter::stream_request( const json_t &body, stream_callback_t cb ) {
        guard_t guard( m_busy, m_cancel );

        auto cfg = snapshot( );
        if ( cfg.m_api_key.empty( ) || !cb )
            return;

        httplib::SSLClient client( "openrouter.ai", 443 );
        client.set_connection_timeout( 30 );
        client.set_read_timeout( 120 );

        httplib::Headers headers = {
            { "Authorization", "Bearer " + cfg.m_api_key },
            {  "content-type",          "application/json" },
            { "HTTP-Referer",   "https://github.com/W1lliam1337/ida-re-assistant" },
            {    "X-Title",                "IDA RE Assistant" }
        };

        std::string buffer;

        client.Post( "/api/v1/chat/completions", headers, body.dump( ), "application/json", [ & ]( const char *data, size_t len ) -> bool {
            if ( guard.cancelled( ) )
                return false;

            buffer.append( data, len );

            size_t pos;
            while ( ( pos = buffer.find( '\n' ) ) != std::string::npos ) {
                std::string line = buffer.substr( 0, pos );
                buffer.erase( 0, pos + 1 );

                if ( line.empty( ) )
                    continue;

                if ( line.rfind( "data: ", 0 ) == 0 ) {
                    std::string json_str = line.substr( 6 );
                    if ( json_str == "[DONE]" )
                        continue;

                    try {
                        auto j = json_t::parse( json_str );
                        if ( j.contains( "choices" ) && !j[ "choices" ].empty( ) ) {
                            auto &delta = j[ "choices" ][ 0 ][ "delta" ];
                            if ( delta.contains( "content" ) ) {
                                std::string text = delta[ "content" ].get< std::string >( );
                                if ( !text.empty( ) )
                                    cb( text );
                            }
                        }
                    } catch ( ... ) { }
                }
            }
            return true;
        } );
    }

    std::vector< model_t > c_openrouter::parse_models_response( const json_t &data ) {
        std::vector< model_t > models;

        if ( !data.contains( "data" ) || !data[ "data" ].is_array( ) )
            return models;

        for ( const auto &model : data[ "data" ] ) {
            std::string id   = model.value( "id", "" );
            std::string name = model.value( "name", id );
            int         ctx  = model.value( "context_length", 0 );

            if ( id.empty( ) )
                continue;

            // Check if free model (ends with :free)
            bool is_free = id.ends_with( ":free" );

            // Filter by free_only setting
            if ( m_show_free_only && !is_free )
                continue;

            models.push_back( {
                id,
                name,
                e_provider::openrouter,
                ctx,
                true // streaming supported
            } );
        }

        // Sort: free models first, then by name
        std::ranges::sort( models, []( const model_t &a, const model_t &b ) {
            bool a_free = a.m_id.ends_with( ":free" );
            bool b_free = b.m_id.ends_with( ":free" );
            if ( a_free != b_free )
                return a_free > b_free;
            return a.m_name < b.m_name;
        } );

        return models;
    }

    std::vector< model_t > c_openrouter::fetch_models( bool force_refresh ) {
        auto now = std::chrono::steady_clock::now( );

        // Return cached if valid
        if ( !force_refresh && !m_cached_models.empty( ) && ( now - m_models_fetched_at ) < k_cache_duration ) {
            return m_cached_models;
        }

        httplib::SSLClient client( "openrouter.ai", 443 );
        client.set_connection_timeout( 15 );
        client.set_read_timeout( 30 );

        auto result = client.Get( "/api/v1/models" );

        if ( !result || result->status != 200 ) {
            // Return cached on error
            return m_cached_models;
        }

        try {
            auto data        = json_t::parse( result->body );
            m_cached_models  = parse_models_response( data );
            m_models_fetched_at = now;
        } catch ( ... ) {
            // Keep cached on parse error
        }

        return m_cached_models;
    }

    // ==================== Manager ====================

    response_t c_llm_manager::send( std::string_view message ) {
        switch ( m_provider ) {
            case e_provider::claude :
                return m_claude.send( message );
            case e_provider::openai :
                return m_openai.send( message );
            case e_provider::gemini :
                return m_gemini.send( message );
            case e_provider::openrouter :
                return m_openrouter.send( message );
        }
        return response_t { .m_error = "Unknown provider" };
    }

    response_t c_llm_manager::send( const std::vector< message_t > &messages ) {
        switch ( m_provider ) {
            case e_provider::claude :
                return m_claude.send( messages );
            case e_provider::openai :
                return m_openai.send( messages );
            case e_provider::gemini :
                return m_gemini.send( messages );
            case e_provider::openrouter :
                return m_openrouter.send( messages );
        }
        return response_t { .m_error = "Unknown provider" };
    }

    void c_llm_manager::stream( std::string_view message, stream_callback_t cb ) {
        switch ( m_provider ) {
            case e_provider::claude :
                m_claude.stream( message, cb );
                break;
            case e_provider::openai :
                m_openai.stream( message, cb );
                break;
            case e_provider::gemini :
                m_gemini.stream( message, cb );
                break;
            case e_provider::openrouter :
                m_openrouter.stream( message, cb );
                break;
        }
    }

    std::vector< model_t > c_llm_manager::all_models( ) const {
        auto claude_models = c_claude::models( );
        auto openai_models = c_openai::models( );
        auto gemini_models = c_gemini::models( );
        // Note: OpenRouter models are fetched dynamically, not included in all_models()
        claude_models.insert( claude_models.end( ), openai_models.begin( ), openai_models.end( ) );
        claude_models.insert( claude_models.end( ), gemini_models.begin( ), gemini_models.end( ) );
        return claude_models;
    }

    response_t c_llm_manager::analyze_code( std::string_view code, std::string_view custom_prompt ) {
        std::string prompt;
        if ( custom_prompt.empty( ) ) {
            prompt  = "Analyze this decompiled code:\n"
                      "1. What does the function do?\n"
                      "2. Key variables and their purpose\n"
                      "3. Suspicious or interesting patterns\n"
                      "4. Suggested names for variables/functions\n\n"
                      "```c\n";
            prompt += code;
            prompt += "\n```";
        } else {
            prompt = std::string( custom_prompt ) + "\n\n```c\n" + std::string( code ) + "\n```";
        }
        return send( prompt );
    }

    response_t c_llm_manager::explain_function( std::string_view pseudocode ) {
        std::string prompt  = "Explain what this function does. Be concise.\n\n```c\n";
        prompt             += pseudocode;
        prompt             += "\n```";
        return send( prompt );
    }

    response_t c_llm_manager::find_vulnerabilities( std::string_view code ) {
        std::string prompt  = "Analyze for security vulnerabilities:\n"
                              "- Buffer overflows\n"
                              "- Integer overflows\n"
                              "- Format string bugs\n"
                              "- Use-after-free\n"
                              "- Memory leaks\n"
                              "- Command/SQL injection\n"
                              "- Path traversal\n\n"
                              "```c\n";
        prompt             += code;
        prompt             += "\n```";
        return send( prompt );
    }

    response_t c_llm_manager::suggest_name( std::string_view pseudocode ) {
        std::string prompt  = "Suggest a descriptive function name based on this code. "
                              "Reply with just the name.\n\n```c\n";
        prompt             += pseudocode;
        prompt             += "\n```";
        return send( prompt );
    }
} // namespace ida_re::api
