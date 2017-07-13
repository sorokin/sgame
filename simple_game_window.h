#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <cairo.h>
#include <SDL2/SDL_keycode.h>

namespace sg
{
    struct context;
    struct model;
    struct win_params;

    void run(win_params const&);

    struct context
    {
        void quit();
        void toggle_fullscreen();
        uint32_t width() const;
        uint32_t height() const;

    private:
        context(void* window, uint32_t tex_width, uint32_t tex_height);

        bool should_quit;
        void* window;
        uint32_t tex_width;
        uint32_t tex_height;

        friend void run(win_params const&);
    };

    struct model
    {
        model(context& ctx);

        context& ctx();

        struct draw_params
        {
            uint32_t frame_time; // milliseconds
            cairo_surface_t* surface;
        };

        struct key_down_params
        {
            SDL_Keycode key;
            Uint16 mod;
        };

        struct key_up_params
        {
            SDL_Keycode key;
            Uint16 mod;
        };

        struct resize_params
        {};

        virtual void draw(draw_params const&);
        virtual void key_down(key_down_params const&);
        virtual void key_up(key_up_params const&);
        virtual void resize(resize_params const&);

    private:
        sg::context* ctx_;
    };

    struct win_params
    {
        enum class resizing_policy_t
        {
            no_resize,
            centered,
            preserve_aspect_ratio,
            scaled,
        };

        win_params();

        win_params& width(uint32_t value);
        win_params& height(uint32_t value);
        win_params& resizing_policy(enum resizing_policy_t value);
        win_params& title(std::string title);

        win_params& min_frame_interval(uint32_t value);

        template <typename M, typename... Args>
        win_params& model(Args&&... args)
        {
            using namespace std::placeholders;
            model_creation_func_ = std::bind(&create_model<M, Args...>, _1, std::forward<Args>(args)...);
            return *this;
        }

    private:
        template <typename M, typename... Args>
        static std::unique_ptr<sg::model> create_model(sg::context& ctx, Args&&... args)
        {
            return std::make_unique<M>(ctx, std::forward<Args>(args)...);
        }

    private:
        uint32_t width_;
        uint32_t height_;
        resizing_policy_t resizing_policy_;
        std::string title_;

        uint32_t min_frame_interval_;

        std::function<std::unique_ptr<sg::model> (sg::context&)> model_creation_func_;

        friend void run(win_params const&);
    };
}
