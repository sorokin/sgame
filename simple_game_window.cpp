#include "simple_game_window.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <iostream>
#include <sstream>
#include <tuple>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <cairo-gl.h>
#include <GL/glu.h>

namespace
{
    struct sdl_initializer
    {
        sdl_initializer(uint32_t flags)
        {
            int result = SDL_Init(flags);
            if (result != 0)
            {
                std::stringstream ss;
                ss << "failed to initialize SDL: " << SDL_GetError();
                throw std::runtime_error(ss.str());
            }
        }

        sdl_initializer(sdl_initializer const&) = delete;
        sdl_initializer& operator=(sdl_initializer const&) = delete;

        ~sdl_initializer()
        {
            SDL_Quit();
        }
    };

    struct sdl_window
    {
        typedef std::pair<int, int> size_type;

        sdl_window(char const* title, int width, int height, uint32_t flags)
        {
            handle = SDL_CreateWindow(title,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      width,
                                      height,
                                      flags);
            if (!handle)
            {
                std::stringstream ss;
                ss << "failed to create SDL window: " << SDL_GetError();
                throw std::runtime_error(ss.str());
            }
        }

        sdl_window(sdl_window const&) = delete;
        sdl_window& operator=(sdl_window const&) = delete;

        SDL_SysWMinfo get_wm_info() const
        {
            SDL_SysWMinfo result;
            SDL_VERSION(&result.version);

            if(!SDL_GetWindowWMInfo(handle, &result))
            {
                std::stringstream ss;
                ss << "failed to get SysWMInfo: " << SDL_GetError();
                throw std::runtime_error(ss.str());
            }

            return result;
        }

        size_type size() const
        {
            int w = 0;
            int h = 0;
            SDL_GetWindowSize(handle, &w, &h);
            return size_type(w, h);
        }

        SDL_Window* get() const
        {
            return handle;
        }

        void toggle_fullscreen()
        {
            bool is_fullscreen = SDL_GetWindowFlags(handle) & SDL_WINDOW_FULLSCREEN_DESKTOP;
            SDL_SetWindowFullscreen(handle, is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
        }

        ~sdl_window()
        {
            SDL_DestroyWindow(handle);
        }

    private:
        SDL_Window* handle;
    };

    struct sdl_glcontext
    {
        sdl_glcontext(SDL_Window* window)
        {
            handle = SDL_GL_CreateContext(window);

            if (!handle)
            {
                std::stringstream ss;
                ss << "failed to create GL context: " << SDL_GetError();
                throw std::runtime_error(ss.str());
            }
        }

        sdl_glcontext(sdl_glcontext const&) = delete;
        sdl_glcontext& operator=(sdl_glcontext const&) = delete;

        ~sdl_glcontext()
        {
            SDL_GL_DeleteContext(handle);
        }

        SDL_GLContext get()
        {
            return handle;
        }

    private:
        SDL_GLContext handle;
    };

    void make_current(sdl_window& window, sdl_glcontext& context)
    {
        if (SDL_GL_MakeCurrent(window.get(), context.get()))
            std::abort();
    }

    struct sdl_makecurrent_null
    {
        sdl_makecurrent_null(SDL_Window* window)
            : window(window)
        {}

        sdl_makecurrent_null(sdl_makecurrent_null const&) = delete;
        sdl_makecurrent_null& operator=(sdl_makecurrent_null const&) = delete;

        ~sdl_makecurrent_null()
        {
            if (SDL_GL_MakeCurrent(window, nullptr))
                std::abort();
        }

    private:
        SDL_Window* window;
    };

    struct cairo_device
    {
        cairo_device(Display* dpy, GLXContext context)
        {
            handle = cairo_glx_device_create(dpy, context);

            if (!handle)
            {
                std::stringstream ss;
                ss << "failed to create cairo device";
                throw std::runtime_error(ss.str());
            }
        }

        cairo_device(cairo_device const&) = delete;
        cairo_device& operator=(cairo_device const&) = delete;

        ~cairo_device()
        {
            cairo_device_destroy(handle);
        }

        cairo_device_t* get() const
        {
            return handle;
        }

    private:
        cairo_device_t* handle;
    };

    struct cairo_surface
    {
        cairo_surface(sdl_window& win,
                      sdl_glcontext& context,
                      cairo_device_t* device,
                      cairo_content_t content,
                      unsigned int tex,
                      int width, int height)
            : win(win)
            , context(context)
            , handle(nullptr)
        {
            create(device, content, tex, width, height);
        }

        cairo_surface(cairo_surface const&) = delete;
        cairo_surface& operator=(cairo_surface const&) = delete;

        void create(cairo_device_t *device,
                    cairo_content_t content,
                    unsigned int tex,
                    int width, int height)
        {
            assert(handle == nullptr);

            make_current(win, context);
            handle = cairo_gl_surface_create_for_texture(
                device,
                content,
                tex,
                width, height
            );

            if (!handle)
            {
                std::stringstream ss;
                ss << "failed to create cairo surface";
                throw std::runtime_error(ss.str());
            }
        }

        void destroy()
        {
            if (handle)
            {
                make_current(win, context);
                cairo_surface_destroy(handle);
                handle = nullptr;
            }
        }

        ~cairo_surface()
        {
            destroy();
        }

        cairo_surface_t* get() const
        {
            return handle;
        }

        void swap_buffers()
        {
            cairo_gl_surface_swapbuffers(handle);
        }

    private:
        sdl_window& win;
        sdl_glcontext& context;
        cairo_surface_t* handle;
    };

    void resize_surface(int texture,
                        int width, int height,
                        sdl_window& sdl_win,
                        sdl_glcontext& context,
                        sdl_glcontext& cairo_context,
                        cairo_surface& surface,
                        cairo_device const& device)
    {
        surface.destroy();

        make_current(sdl_win, context);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_BGRA_EXT,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        try
        {
            surface.create(device.get(),
                           CAIRO_CONTENT_COLOR_ALPHA,
                           texture,
                           width,
                           height);
        }
        catch (...)
        {
            std::abort();
        }
    }
}

using namespace sg;

context::context(void* window, uint32_t tex_width, uint32_t tex_height)
    : should_quit(false)
    , window(window)
    , tex_width(tex_width)
    , tex_height(tex_height)
{}

void context::quit()
{
    should_quit = true;
}

void context::toggle_fullscreen()
{
    static_cast<sdl_window*>(window)->toggle_fullscreen();
}

uint32_t context::width() const
{
    return tex_width;
}

uint32_t context::height() const
{
    return tex_height;
}

model::model(context& ctx)
    : ctx_(&ctx)
{}

context& model::ctx()
{
    return *ctx_;
}

void model::draw(draw_params const&)
{}

void model::key_down(key_down_params const& p)
{
    if (p.key == SDLK_ESCAPE)
        ctx().quit();
    if (p.key == SDLK_RETURN && (p.mod & (KMOD_RALT | KMOD_LALT)))
        ctx().toggle_fullscreen();
}

void model::key_up(key_up_params const&)
{}

void model::resize(resize_params const&)
{}

win_params::win_params()
    : width_(640)
    , height_(480)
    , resizing_policy_(resizing_policy::preserve_aspect_ratio)
    , title_("Simple Game Window")
    , model_creation_func_([] (sg::context& ctx) {
        return std::make_unique<sg::model>(ctx);
    })
{}

win_params& win_params::width(uint32_t value)
{
    width_ = value;
    return *this;
}

win_params& win_params::height(uint32_t value)
{
    height_ = value;
    return *this;
}

win_params& win_params::resizing_policy(enum resizing_policy value)
{
    resizing_policy_ = value;
    return *this;
}

win_params& win_params::title(std::string title)
{
    title_ = std::move(title);
    return *this;
}

win_params& win_params::min_frame_interval(uint32_t value)
{
    min_frame_interval_ = value;
    return *this;
}

void sg::run(win_params const& p)
{
    sdl_initializer sdl_init(SDL_INIT_VIDEO);
    sdl_window sdl_win(p.title_.c_str(), p.width_, p.height_,
        SDL_WINDOW_OPENGL
      | (p.resizing_policy_ != win_params::resizing_policy::no_resize ? SDL_WINDOW_RESIZABLE : 0));

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    sdl_glcontext context(sdl_win.get());
    sdl_glcontext cairo_context(sdl_win.get());
    sdl_makecurrent_null makecurrent_null(sdl_win.get());

    SDL_SysWMinfo wm_info = sdl_win.get_wm_info();

    cairo_device device(wm_info.info.x11.display,
                        reinterpret_cast<GLXContext>(cairo_context.get()));

    make_current(sdl_win, context);
    glEnable(GL_TEXTURE_2D);
    glViewport(0.0, 0.0, p.width_, p.height_);
    glClearColor(0., 0., 0., 1.0);

    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        p.width_,
        p.height_,
        0,
        GL_BGRA_EXT,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    cairo_surface surface(sdl_win, cairo_context,
                          device.get(),
                          CAIRO_CONTENT_COLOR_ALPHA,
                          texture,
                          p.width_,
                          p.height_);

    uint32_t start = SDL_GetTicks();
    uint32_t last_frame_start = start;

    SDL_Event event;

    sg::context ctx(&sdl_win, p.width_, p.height_);
    make_current(sdl_win, cairo_context);
    std::unique_ptr<sg::model> model = p.model_creation_func_(ctx);
    while (!ctx.should_quit)
    {
        uint32_t this_frame_start = SDL_GetTicks();
        make_current(sdl_win, cairo_context);
        {
            sg::model::draw_params dp = {
                this_frame_start - last_frame_start,
                surface.get()
            };
            model->draw(dp);
        }

        surface.swap_buffers();

        make_current(sdl_win, context);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, 1.0, 0.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBegin(GL_QUADS);

        glTexCoord2i(0., 1.);
        glVertex2i(0., 0.);

        glTexCoord2i(0., 0.);
        glVertex2i(0., 1.);

        glTexCoord2i(1., 0.);
        glVertex2i(1., 1.);

        glTexCoord2i(1., 1.);
        glVertex2i(1., 0.);

        glEnd();

        SDL_GL_SwapWindow(sdl_win.get());

        last_frame_start = this_frame_start;

        while (!ctx.should_quit)
        {
            uint32_t current_time = SDL_GetTicks();
            uint32_t timeout = p.min_frame_interval_ - std::min(current_time - last_frame_start, p.min_frame_interval_);

            if (!SDL_WaitEventTimeout(&event, std::max(timeout, 1u)))
                break;

            switch (event.type)
            {
            case SDL_QUIT:
                ctx.quit();
                break;
            case SDL_KEYDOWN:
                {
                    model::key_down_params kdp;
                    kdp.key = event.key.keysym.sym;
                    kdp.mod = event.key.keysym.mod;
                    model->key_down(kdp);
                    break;
                }
            case SDL_KEYUP:
                {
                    model::key_up_params kup;
                    kup.key = event.key.keysym.sym;
                    kup.mod = event.key.keysym.mod;
                    model->key_up(kup);
                    break;
                }
            case SDL_WINDOWEVENT:
                {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        switch (p.resizing_policy_)
                        {
                        case win_params::resizing_policy::no_resize:
                            assert(false);
                            break;
                        case win_params::resizing_policy::centered:
                            make_current(sdl_win, context);
                            glViewport(event.window.data1 / 2 - p.width_ / 2,
                                       event.window.data2 / 2 - p.height_ / 2,
                                       p.width_,
                                       p.height_);
                            break;
                        case win_params::resizing_policy::preserve_aspect_ratio:
                            {
                                if ((uint64_t)event.window.data1 * p.height_ < (uint64_t)event.window.data2 * p.width_)
                                {
                                    ctx.tex_width = event.window.data1;
                                    ctx.tex_height = (uint64_t)event.window.data1 * p.height_ / p.width_;
                                }
                                else
                                {
                                    ctx.tex_width = (uint64_t)event.window.data2 * p.width_ / p.height_;
                                    ctx.tex_height = event.window.data2;
                                }
                                make_current(sdl_win, context);
                                glViewport(event.window.data1 / 2 - ctx.tex_width / 2,
                                           event.window.data2 / 2 - ctx.tex_height / 2,
                                           ctx.tex_width,
                                           ctx.tex_height);
                                resize_surface(texture, ctx.tex_width, ctx.tex_height, sdl_win, context, cairo_context, surface, device);
                                break;
                            }
                        case win_params::resizing_policy::scaled:
                            ctx.tex_width = event.window.data1;
                            ctx.tex_height = event.window.data2;

                            make_current(sdl_win, context);
                            glViewport(0, 0, ctx.tex_width, ctx.tex_height);

                            resize_surface(texture, ctx.tex_width, ctx.tex_height, sdl_win, context, cairo_context,surface, device);
                            break;

                        default:
                            assert(false);
                            break;
                        }

                        model->resize(sg::model::resize_params());
                    }
                    break;
                }
            default:
                break;
            }

            if (timeout == 0)
                break;
        }
    }
}
