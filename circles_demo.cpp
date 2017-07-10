#include "simple_game_window.h"
#include <cmath>
#include <vector>

struct circles_model : sg::model
{
    struct circle
    {
        float x;
        float y;
        float vx;
        float vy;
        float r;
        float g;
        float b;
    };

    circles_model(sg::context& ctx)
        : sg::model(ctx)
    {
        gen();
        gen();
        gen();
        time_till_next_spawn = 1.5;
    }

    void draw(draw_params const& p)
    {
        cairo_t* cr = cairo_create(p.surface);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_scale(cr, ctx().width(), ctx().height());

        double ft = p.frame_time * 0.001;
        time_till_next_spawn -= ft;
        if (time_till_next_spawn < 0)
        {
            gen();
            time_till_next_spawn = 1.5;
        }

        for (circle& c : circles)
        {
            c.x += c.vx * ft;
            c.y += c.vy * ft;
            if (c.x <= circle_radius)
            {
                c.vx = std::abs(c.vx);
            }
            else if (c.x >= (1 - circle_radius))
            {
                c.vx = -std::abs(c.vx);
            }
            if (c.y <= circle_radius)
            {
                c.vy = std::abs(c.vy);
            }
            else if (c.y >= (1 - circle_radius))
            {
                c.vy = -std::abs(c.vy);
            }

            cairo_set_line_width (cr, 0.006);

            cairo_arc(cr, c.x, c.y, circle_radius, 0.0, 2 * 3.1415);
            cairo_set_source_rgb(cr, c.r, c.g, c.b);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0., 0., 0.);
            cairo_stroke (cr);
        }

        cairo_surface_flush(p.surface);
        cairo_destroy(cr);
    }

private:
    void gen()
    {
        circle c;
        c.x = circle_radius + (double)rand() / RAND_MAX * (1 - 2. * circle_radius);
        c.y = circle_radius + (double)rand() / RAND_MAX * (1 - 2. * circle_radius);

        float norm = (double)rand() / RAND_MAX;
        float arg = (double)rand() / RAND_MAX * 2 * 3.141592;
        c.vx = norm * cos(arg);
        c.vy = norm * sin(arg);
        c.r = (double)rand() / RAND_MAX;
        c.g = (double)rand() / RAND_MAX;
        c.b = (double)rand() / RAND_MAX;
        circles.push_back(c);
    }

    void key_down(key_down_params const& p)
    {
        if (p.key == SDLK_q)
            ctx().quit();
        else if (p.key == SDLK_f)
            ctx().toggle_fullscreen();
        else
            sg::model::key_down(p);
    }

private:
    static constexpr float circle_radius = 0.04f;
    float time_till_next_spawn;
    std::vector<circle> circles;
};

int main(int argc, char** argv)
{
    run(sg::win_params()
        .width(512)
        .height(512)
        .min_frame_interval(15)
        .model<circles_model>());

    return 0;
}
