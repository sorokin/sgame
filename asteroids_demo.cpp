#include "simple_game_window.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <random>

struct point
{
    constexpr point()
        : x(0)
        , y(0)
    {}

    constexpr point(double x, double y)
        : x(x)
        , y(y)
    {}

    double x;
    double y;
};

point operator+(point a, point b)
{
    return point(a.x + b.x, a.y + b.y);
}

point operator-(point a, point b)
{
    return point(a.x - b.x, a.y - b.y);
}

point operator*(point a, double b)
{
    return point(a.x * b, a.y * b);
}

point operator*(double a, point b)
{
    return point(a * b.x, a * b.y);
}

double dot(point a, point b)
{
    return a.x * b.x + a.y * b.y;
}

double norm2(point a)
{
    return a.x * a.x + a.y * a.y;
}

double norm(point a)
{
    return hypot(a.x, a.y);
}

double distance2(point a, point b)
{
    return norm2(a - b);
}

double distance(point a, point b)
{
    return norm(a - b);
}

bool intersect(point p, point q, point cc, double r)
{
    assert(r > 0);

    p = p - cc;
    q = q - cc;

    double a = norm2(p) - 2 * dot(p, q) + norm2(q);
    double b = dot(p, q) - norm2(q);
    double c = norm2(q) - r * r;

    double d = b * b - a * c;

    if (d < 0)
        return false;

    double t1 = (-b - sqrt(d)) / a;
    
    if (t1 >= 0 && t1 <= 1)
        return true;

    double t2 = (-b - sqrt(d)) / a;
    if (t2 >= 0 && t2 <= 1)
        return true;

    return false;
}

constexpr double asteroid_sizes[3] = {0.016, 0.029, 0.053};
constexpr double bullet_radius = 0.007;
constexpr double line_width = 0.0025;

constexpr point ship_p1(0.1/3.5, 0.);
constexpr point ship_p2(-0.1/3.5, 0.07/3.5);
constexpr point ship_p3(-0.1/3.5, -0.07/3.5);
constexpr double collision_tolerance = 0.003;

struct asteroids_model : sg::model
{
    enum class ship_rotation
    {
        none,
        left,
        right,
    };

    struct asteroid
    {
        point pos;
        point velocity;
        int size;
        int health;
    };
    
    struct bullet
    {
        point pos;
        point velocity;
        double ttl;
    };

    asteroids_model(sg::context& ctx)
        : model(ctx)
        , dead(false)
        , engine_enabled(false)
        , shooting_enabled(false)
    {
        reset();
    }

    void reset()
    {
        dead = false;
        ship = point(0.5, 0.5);
        ship_velocity = point();
        ship_yaw = 2. * 3.1415 * rand() / RAND_MAX;
        time_till_next_shot = 0.;
        asteroids.clear();
        bullets.clear();
    }

    bool collide(asteroid const& e)
    {
        /*
        return !dead && e.health != 0
            && distance(e.pos, ship) < (asteroid_sizes[e.size] + line_width + 0.01);
        */
        
        if (dead)
            return false;
        if (e.health == 0)
            return false;

        point mx(cos(ship_yaw), sin(ship_yaw));
        point my(sin(ship_yaw), -cos(ship_yaw));

        point s1 = ship + point(dot(mx, ship_p1), dot(my, ship_p1));
        point s2 = ship + point(dot(mx, ship_p2), dot(my, ship_p2));
        point s3 = ship + point(dot(mx, ship_p3), dot(my, ship_p3));

        return intersect(s1, s2, e.pos, asteroid_sizes[e.size] - collision_tolerance)
            || intersect(s2, s3, e.pos, asteroid_sizes[e.size] - collision_tolerance)
            || intersect(s3, s1, e.pos, asteroid_sizes[e.size] - collision_tolerance);
    }

    virtual void draw(draw_params const& p)
    {
        if (!dead)
        {
            switch (ship_rot)
            {
            case ship_rotation::left:
                ship_yaw -= p.frame_time * 0.005;
                break;
            case ship_rotation::right:
                ship_yaw += p.frame_time * 0.005;
                break;
            default:
                break;
            }
    
            if (engine_enabled)
            {
                ship_velocity.x += p.frame_time * 0.00007 * 180 * cos(ship_yaw);
                ship_velocity.y += p.frame_time * 0.00007 * 180 * sin(ship_yaw);
            }
    
            ship.x = trim_01(ship.x + ship_velocity.x * p.frame_time * 0.0001);
            ship.y = trim_01(ship.y + ship_velocity.y * p.frame_time * 0.0001);
    
            if (shooting_enabled)
            {
                if (time_till_next_shot >= 0)
                    time_till_next_shot -= p.frame_time * 0.001;
                else
                {
                    bullet b;
                    b.pos.x = ship.x + 0.1/3.5 * cos(ship_yaw);
                    b.pos.y = ship.y + 0.1/3.5 * sin(ship_yaw);
                    b.velocity.x = ship_velocity.x + 10. * cos(ship_yaw);
                    b.velocity.y = ship_velocity.y + 10. * sin(ship_yaw);
                    b.ttl = 0.8;
                    bullets.push_back(b);
                    
                    time_till_next_shot = 0.2;
                }
            }
        }

        for (size_t i = 0; i != asteroids.size();)
        {
            asteroid& e = asteroids[i];

            e.pos.x = trim_01(e.pos.x + e.velocity.x * p.frame_time * 0.0001);
            e.pos.y = trim_01(e.pos.y + e.velocity.y * p.frame_time * 0.0001);

            for (size_t j = 0; j != bullets.size(); ++j)
            {
                bullet& b = bullets[j];

                if (distance(e.pos, b.pos) < (asteroid_sizes[e.size] + line_width + bullet_radius))
                {
                    std::swap(b, bullets.back());
                    bullets.pop_back();
                    --e.health;
                    break;
                }
            }

            if (collide(e))
            {
                dead = true;
                e.health = 0;
                break;
            }
            
            if (e.health == 0)
            {
                point pos = e.pos;
                int size = e.size;
                std::swap(e, asteroids.back());
                asteroids.pop_back();
                destroy_asteroid(pos, size);
            }
            else
                ++i;
        }

        for (size_t i = 0; i != bullets.size();)
        {
            bullet& e = bullets[i];

            e.pos.x = trim_01(e.pos.x + e.velocity.x * p.frame_time * 0.0001);
            e.pos.y = trim_01(e.pos.y + e.velocity.y * p.frame_time * 0.0001);
            e.ttl -= p.frame_time * 0.001;
            if (e.ttl < 0.)
            {
                std::swap(e, bullets.back());
                bullets.pop_back();
            }
            else
                ++i;
        }

        if (asteroids.empty())
        {
            gen_asteroid();
            gen_asteroid();
            gen_asteroid();
        }

        cairo_t* cr = cairo_create(p.surface);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_scale(cr, ctx().width(), ctx().height());
        cairo_set_line_width (cr, line_width);

        cairo_move_to(cr, 0., 0.);
        cairo_line_to(cr, 1., 0.);
        cairo_line_to(cr, 1., 1.);
        cairo_line_to(cr, 0., 1.);
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, 0., 0., 0.);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 1., 1., 1.);
        cairo_stroke(cr);

        if (!dead)
        {
            paint(ship, 0.02, [&](point pos)
            {
                cairo_save(cr);
                cairo_translate(cr, pos.x, pos.y);
                cairo_rotate(cr, ship_yaw);
        
                if (engine_enabled)
                {
                    cairo_move_to(cr, -0.1/3.5, -0.06/3.5);
                    cairo_line_to(cr, -0.1/3.5, 0.06/3.5);
                    cairo_line_to(cr, -0.2/3.5, 0.05/3.5);
                    cairo_line_to(cr, -0.1/3.5, 0.);
                    cairo_line_to(cr, -0.2/3.5, -0.05/3.5);
                    cairo_close_path(cr);
                    cairo_set_source_rgb(cr, 200./255., 50./255., 40./255.);
                    cairo_fill(cr);
                }

                cairo_move_to(cr, ship_p1.x, ship_p1.y);
                cairo_line_to(cr, ship_p2.x, ship_p2.y);
                cairo_line_to(cr, ship_p3.x, ship_p3.y);
                cairo_close_path(cr);
                cairo_set_source_rgb(cr, 50./255., 130./255., 40./255.);
                cairo_fill_preserve(cr);
                cairo_set_source_rgb(cr, 1., 1., 1.);
                cairo_stroke(cr);
                cairo_restore(cr);
            });
        }

        for (asteroid const& e : asteroids)
        {
            paint(e.pos, asteroid_sizes[e.size] + line_width, [&] (point pos)
            {
                cairo_arc(cr, pos.x, pos.y, asteroid_sizes[e.size], 0., 2 * 3.1415);
                cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
                cairo_fill_preserve(cr);
                cairo_set_source_rgb(cr, 1., 1., 1.);
                cairo_stroke(cr);
            });
        }

        for (bullet const& e : bullets)
        {
            paint(e.pos, bullet_radius, [&](point pos) {
                cairo_arc(cr, pos.x, pos.y, bullet_radius, 0., 2 * 3.1415);
                cairo_set_source_rgb(cr, 200./255., 221./255., 40./255.);
                cairo_fill(cr);
                cairo_set_source_rgb(cr, 1., 1., 1.);
            });
        }
        
        if (dead)
        {
            cairo_select_font_face(cr, "Purisa",
                  CAIRO_FONT_SLANT_NORMAL,
                  CAIRO_FONT_WEIGHT_BOLD);

            cairo_set_source_rgb(cr, 1., 1., 1.);
            cairo_set_font_size(cr, 0.1);
            draw_text(cr, "Died!", 0.5, 0.5);
            cairo_set_font_size(cr, 0.05);
            draw_text(cr, "Press ESC to restart", 0.5, 0.56);
            
        }
        cairo_surface_flush(p.surface);
        cairo_destroy(cr);        
    }

    template <typename F>
    void paint(point pos, double size, F const& func)
    {
        //assert(pos.x >= 0 && pos.x <= 1);
        //assert(pos.y >= 0 && pos.y <= 1);

        if (pos.x > (1. - size))
            pos.x -= 1.;

        if (pos.y > (1. - size))
            pos.y -= 1.;
        
        func(pos);
        
        if (pos.x < size)
        {
            func(point(pos.x + 1., pos.y));
            if (pos.y < size)
                func(point(pos.x + 1., pos.y + 1.));
        }

        if (pos.y < size)
            func(point(pos.x, pos.y + 1.));
    }

    void draw_text(cairo_t* cr, char const* text, double x, double y)
    {
        cairo_text_extents_t extents;
        cairo_text_extents(cr, text, &extents);
        cairo_move_to(cr, x - extents.width / 2, y - extents.height / 2);
        cairo_show_text(cr, text);
    }
    
    void key_down(key_down_params const& p)
    {
        switch (p.key)
        {
        case SDLK_LEFT:
        case SDLK_a:
            ship_rot = ship_rotation::left;
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            ship_rot = ship_rotation::right;
            break;
        case SDLK_UP:
        case SDLK_w:
            engine_enabled = true;
            break;
        case SDLK_SPACE:
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            shooting_enabled = true;
            break;
        case SDLK_ESCAPE:
            if (dead)
                reset();
            break;
        case SDLK_f:
            ctx().toggle_fullscreen();
            break;
        case SDLK_q:
            if (dead)
                ctx().quit();
            break;
        default:
            sg::model::key_down(p);
            break;
        }
    }

    void key_up(key_up_params const& p)
    {
        switch (p.key)
        {
        case SDLK_LEFT:
        case SDLK_a:
            if (ship_rot == ship_rotation::left)
                ship_rot = ship_rotation::none;
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            if (ship_rot == ship_rotation::right)
                ship_rot = ship_rotation::none;
            break;
        case SDLK_UP:
        case SDLK_w:
            engine_enabled = false;
            break;
        case SDLK_SPACE:
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            shooting_enabled = false;
            break;
        default:
            sg::model::key_up(p);
            break;
        }
    }
    
    void gen_asteroid()
    {
        asteroid c;
        for (size_t i;; ++i)
        {
            c.pos.x = (double)rand() / RAND_MAX;
            c.pos.y = (double)rand() / RAND_MAX;
            
            if ((distance(c.pos, ship) > 0.2) || i == 20)
                break;

            ++i;
        }

        double norm = 1.22 * (0.4 + 0.6 * (double)rand() / RAND_MAX);
        double arg = (double)rand() / RAND_MAX * 2 * 3.141592;
        c.velocity.x = norm * cos(arg);
        c.velocity.y = norm * sin(arg);
        
        c.size = 2;
        c.health = 3;
        asteroids.push_back(c);
    }
    
    void destroy_asteroid(point pos, int size)
    {
        size_t n;
        double velocity;
        switch (size)
        {
        case 0:
            return;
        case 1:
            n = 3;
            velocity = 3.1;
            break;
        case 2:
            n = 2;
            velocity = 2.;
            break;
        default:
            assert(false);
            return;
        }

        for (size_t i = 0; i != n; ++i)
        {
            asteroid c;
            c.pos = pos;

            double norm = velocity * (0.6 + 0.4 * (double)rand() / RAND_MAX);
            double arg = (double)rand() / RAND_MAX * 2 * 3.141592;
            c.velocity.x = norm * cos(arg);
            c.velocity.y = norm * sin(arg);
            
            c.size = size - 1;
            c.health = size;
            asteroids.push_back(c);
        }
    }

    static double trim_01(double v)
    {
        if (v > 0)
            return v - trunc(v);
        else
            return 1 - (v - trunc(v));
    }

private:
    bool dead;
    point ship;
    point ship_velocity;
    double ship_yaw;
    ship_rotation ship_rot;
    bool engine_enabled;
    bool shooting_enabled;
    double time_till_next_shot;
    std::vector<asteroid> asteroids;
    std::vector<bullet> bullets;
};

int main(int argc, char** argv)
{
    run(sg::win_params()
        .width(720)
        .height(720)
        .title("Asteroids")
        .min_frame_interval(15)
        .model<asteroids_model>());

    return 0;
}
