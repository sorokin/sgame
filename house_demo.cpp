#include "simple_game_window.h"
#include <cmath>

struct house_model : sg::model
{
    house_model(sg::context& ctx)
        : sg::model(ctx)
        , s(1.0)
        , right_pressed(false)
    {}

    void draw(draw_params const& p)
    {
        cairo_t* cr = cairo_create(p.surface);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_scale(cr, ctx().width(), ctx().height());

        cairo_move_to(cr, 0., 0.5);
        cairo_line_to(cr, 1., 0.5);
        cairo_line_to(cr, 1., 1.);
        cairo_line_to(cr, 0., 1.);
        cairo_set_source_rgb(cr, 17./255., 126./255., 17./255.);
        cairo_fill(cr);

        double t = 1. - std::abs(s - 0.5) * 1.2;

        cairo_move_to(cr, 0., 0.);
        cairo_line_to(cr, 1., 0.);
        cairo_line_to(cr, 1., 0.5);
        cairo_line_to(cr, 0., 0.5);
        cairo_set_source_rgb(cr, 97./255. * t, 188./255. * t, 251./255. * t);
        cairo_fill(cr);

        cairo_set_line_width (cr, 0.006);

        cairo_move_to(cr, 0.0, 0.5);
        cairo_line_to(cr, 1.0, 0.5);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_stroke (cr);

        cairo_move_to(cr, 0.33, 0.55);
        cairo_line_to(cr, 0.67, 0.55);
        cairo_line_to(cr, 0.67, 0.82);
        cairo_line_to(cr, 0.33, 0.82);
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, 238./255., 217./255., 39./255.);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_stroke (cr);

        cairo_move_to(cr, 0.43, 0.61);
        cairo_line_to(cr, 0.57, 0.61);
        cairo_line_to(cr, 0.57, 0.75);
        cairo_line_to(cr, 0.43, 0.75);
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, 12./255., 145./255., 205./255.);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_stroke (cr);

        cairo_move_to(cr, 0.33, 0.55);
        cairo_line_to(cr, 0.5,  0.39);
        cairo_line_to(cr, 0.67, 0.55);
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, 205./255., 12./255., 12./255.);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_stroke (cr);

        cairo_arc(cr, s, 0.14, 0.07, 0.0, 2 * 3.1415);
        cairo_set_source_rgb(cr, 1., 1., 0.);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_stroke (cr);

        cairo_surface_flush(p.surface);
        cairo_destroy(cr);

        if (!right_pressed)
        {
            s -= 0.00018 * p.frame_time;
            if (s < -0.2)
                s = 1.2;
        }
        else
        {
            s += 0.00018 * p.frame_time;
            if (s > 1.2)
                s = -0.2;
        }
    }

    void key_down(key_down_params const& p)
    {
        if (p.key == SDLK_RIGHT)
            right_pressed = true;
        else if (p.key == SDLK_q)
            ctx().quit();
        else if (p.key == SDLK_f)
            ctx().toggle_fullscreen();
        else
            sg::model::key_down(p);
    }

    void key_up(key_up_params const& p)
    {
        if (p.key == SDLK_RIGHT)
            right_pressed = false;
    }

private:
    double s;
    bool right_pressed;
};

int main(int argc, char** argv)
{
    run(sg::win_params()
        .width(512)
        .height(512)
        .min_frame_interval(15)
        .model<house_model>());

    return 0;
}
