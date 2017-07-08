#include "simple_game_window.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>

struct snake_model : sg::model
{
    static constexpr uint32_t turn_interval = 110;
    static constexpr uint32_t field_size_x = 4 * 5;
    static constexpr uint32_t field_size_y = 3 * 5;
    static constexpr double aspect = (double)field_size_x/field_size_y;
    static constexpr size_t action_queue_max_size = 4;

    struct point
    {
        int32_t x;
        int32_t y;
    };

    enum class direction
    {
        up,
        left,
        down,
        right,
    };

    snake_model(sg::context& ctx)
        : sg::model(ctx)
    {
        reset_snake();
    }

    void reset_snake()
    {
        need_redraw = true;
        started = false;
        time_till_next_turn = 0;
        snake.clear();
        snake.push_back({0, 0});
        snake.push_back({1, 0});
        snake.push_back({2, 0});
        snake_is_dead = false;
        queued_actions.clear();
        queued_actions.push_back(direction::right);
        apple = find_empty_place();
    }

    void draw(draw_params const& p)
    {
        if (started && !snake_is_dead)
        {
            time_till_next_turn -= p.frame_time;

            if (time_till_next_turn < 0)
            {
                time_till_next_turn += turn_interval;

                point next = snake.back();
                direction snake_direction;
                if (queued_actions.size() > 1)
                {
                    snake_direction = queued_actions[1];
                    queued_actions.pop_front();
                }
                else
                    snake_direction = queued_actions.front();

                switch (snake_direction)
                {
                case direction::up:
                    --next.y;
                    break;
                case direction::left:
                    --next.x;
                    break;
                case direction::down:
                    ++next.y;
                    break;
                case direction::right:
                    ++next.x;
                    break;
                default:
                    assert(false);
                    break;
                }
                if (next.x < 0 || next.y < 0
                 || next.x >= field_size_x || next.y >= field_size_y
                 || snake_contains(next))
                    snake_is_dead = true;
                else
                {
                    snake.push_back(next);
                    if (next.x == apple.x && next.y == apple.y)
                        apple = find_empty_place();
                    else
                        snake.pop_front();
                }

                need_redraw = true;
            }
        }

        if (need_redraw)
        {
            draw_scene(p);
            need_redraw = false;
        }
    }

    void draw_rect(cairo_t* cr, point p,
                   double fr, double fg, double fb,
                   double lr, double lg, double lb)
    {
        double left   = aspect * (double)p.x / field_size_x;
        double top    = (double)p.y / field_size_y;
        double right  = aspect * (p.x + 1.0) / field_size_x;
        double bottom = (p.y + 1.0) / field_size_y;
        cairo_move_to(cr, left, top);
        cairo_line_to(cr, right, top);
        cairo_line_to(cr, right, bottom);
        cairo_line_to(cr, left, bottom);
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, fr, fg, fb);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, lr, lg, lb);
        cairo_stroke(cr);
    }

    void key_down(key_down_params const& p)
    {
        switch (p.key)
        {
        case SDLK_ESCAPE:
        case SDLK_RETURN:
            sg::model::key_down(p);
            break;
        case SDLK_SPACE:
            reset_snake();
            break;
        case SDLK_UP:
        case SDLK_w:
            started = true;
            enqueue_action(direction::up);
            break;
        case SDLK_LEFT:
        case SDLK_a:
            started = true;
            enqueue_action(direction::left);
            break;
        case SDLK_DOWN:
        case SDLK_s:
            started = true;
            enqueue_action(direction::down);
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            started = true;
            enqueue_action(direction::right);
            break;
        default:
            break;
        }
    }

    void resized(resized_params const&)
    {
        need_redraw = true;
    }

    void enqueue_action(direction dir)
    {
        direction last;
        if (queued_actions.size() < action_queue_max_size)
            last = queued_actions.back();
        else
            last = queued_actions[queued_actions.size() - 2];

        if ((dir == direction::up || dir == direction::down)
         && (last == direction::up || last == direction::down))
            return;

        if ((dir == direction::left || dir == direction::right)
         && (last == direction::left || last == direction::right))
            return;

        if (queued_actions.size() < action_queue_max_size)
            queued_actions.push_back(dir);
        else
            queued_actions.back() = dir;
    }

    void draw_scene(draw_params const& p)
    {
        cairo_t* cr = cairo_create(p.surface);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_scale(cr, ctx().height(), ctx().height());
        cairo_set_line_width (cr, 0.072 / field_size_y);

        double r;
        double g;
        double b;
        double lrgb;
        double ar;
        double ag;
        double ab;
        if (snake_is_dead)
        {
            r = g = b = 132./255.;
            lrgb = 32./255.;
            ar = 189./255.;
            ag = 123./255.;
            ab = 101./255.;
        }
        else
        {
            r = 21./255.;
            g = 102./255.;
            b = 25./255.;
            lrgb = 0.;
            ar = 189./255.;
            ag = 23./255.;
            ab = 1./255.;
        }

        for (size_t i = 0; i != snake.size(); ++i)
        {
            draw_rect(cr, snake[i], r, g, b, lrgb, lrgb, lrgb);
        }

        draw_rect(cr, apple, ar, ag, ab, lrgb, lrgb, lrgb);

        cairo_select_font_face(cr, "Purisa",
              CAIRO_FONT_SLANT_NORMAL,
              CAIRO_FONT_WEIGHT_BOLD);

        if (!started)
        {
            cairo_set_source_rgb(cr, 0., 0., 0.);
            cairo_set_font_size(cr, 0.05);
            draw_text(cr, "Press any key to start", aspect * 0.5, 0.56);
        }
        else if (snake_is_dead)
        {
            cairo_set_source_rgb(cr, 0., 0., 0.);
            cairo_set_font_size(cr, 0.1);
            draw_text(cr, "Died!", aspect * 0.5, 0.5);
            cairo_set_font_size(cr, 0.05);
            draw_text(cr, "Press SPACE to restart", aspect * 0.5, 0.56);
        }

        cairo_surface_flush(p.surface);
        cairo_destroy(cr);
    }

    void draw_text(cairo_t* cr, char const* text, double x, double y)
    {
        cairo_text_extents_t extents;
        cairo_text_extents(cr, text, &extents);
        cairo_move_to(cr, x - extents.width / 2, y - extents.height / 2);
        cairo_show_text(cr, text);
    }

    point find_empty_place()
    {
        constexpr size_t max_number_of_tries = 20;

        for (size_t i = 0;; ++i)
        {
            point p;
            p.x = rand() % field_size_x;
            p.y = rand() % field_size_y;

            if (!snake_contains(p) || i == max_number_of_tries)
                return p;
        }
    }

    bool snake_contains(point p)
    {
        for (size_t i = 0; i != snake.size(); ++i)
        {
            point c = snake[i];
            if (p.x == c.x && p.y == c.y)
                return true;
        }

        return false;
    }

private:
    bool need_redraw;
    bool started;
    int32_t time_till_next_turn;
    std::deque<point> snake;
    bool snake_is_dead;
    std::deque<direction> queued_actions;
    point apple;
};

int main(int argc, char** argv)
{
    constexpr uint32_t default_cell_size = 42;
    run(sg::win_params()
        .width(snake_model::field_size_x * default_cell_size)
        .height(snake_model::field_size_y * default_cell_size)
        .title("Snake")
        .min_frame_interval(15)
        .model<snake_model>());

    return 0;
}
