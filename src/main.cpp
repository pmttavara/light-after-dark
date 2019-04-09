#pragma warning(disable : 4081)

#ifdef NDEBUG
#define DevBool(name) enum { name = false }
#else
#define DevBool(name) bool name = false
#endif
DevBool(dev);
DevBool(dev_drawdebugrects);
DevBool(dev_notarget);
DevBool(dev_noclip);
DevBool(dev_nomusic);
DevBool(dev_nolights);
DevBool(dev_fast);
DevBool(dev_slow);
DevBool(dev_god);
static float dev_god_torch_amt = 0;
DevBool(dev_noreverb);

#include "general.hpp"
using namespace ng::int_types;

inline uint32_t equidistribute(uint32_t n, uint32_t range) {
    double scaled = n * 1.6180339887498948;
    scaled -= (uint64_t)scaled; // modulo 1; negative numbers ignored here
    return scaled * range;
}

// #define KEY(X) SDL_GetKeyboardState(null)[SDL_GetScancodeFromKey(SDLK_##X)]
#define KEY(X) SDL_GetKeyboardState(null)[SDL_SCANCODE_##X]

#define Use (void)

float rand_range(ng::mt127_64 *rng, float min, float max) {
    auto range = max - min;
    return rng->next_float() * range + min;
}

template <class T> T damp(T current, T target, float rate, float dt) {
    return ng::lerp(current, target, 1 - SDL_pow(rate, dt));
}

SDL_Color lerp(SDL_Color a, SDL_Color b, float t) {
    SDL_Color result;
    result.r = ng::lerp(a.r, b.r, t);
    result.g = ng::lerp(a.g, b.g, t);
    result.b = ng::lerp(a.b, b.b, t);
    result.a = ng::lerp(a.a, b.a, t);
    return result;
}

struct Vector2 {
    float x;
    float y;
    Vector2 &operator+=(Vector2 v) { return x += v.x, y += v.y, *this; }
    Vector2 &operator-=(Vector2 v) { return x -= v.x, y -= v.y, *this; }
    Vector2 &operator*=(float v) { return x *= v, y *= v, *this; }
    Vector2 &operator/=(float v) { return x /= v, y /= v, *this; }
    float magsq() const { return x * x + y * y; }
    float mag() const { return SDL_sqrtf(magsq()); }
    Vector2 hat() const {
        Vector2 result;
        auto m = magsq();
        if (m != 0.0f) m = 1 / SDL_sqrtf(m);
        result.x = x * m;
        result.y = y * m;
        return result;
    }
    Vector2 perp() const { return {-y, x}; }
    Vector2 operator-() const { return {-x, -y}; }
};

constexpr Vector2 v2(float x, float y) { return Vector2{x, y}; }
constexpr Vector2 v2() { return Vector2{}; }

Vector2 operator+(Vector2 a, Vector2 b) { return {a.x + b.x, a.y + b.y}; }
Vector2 operator-(Vector2 a, Vector2 b) { return {a.x - b.x, a.y - b.y}; }
Vector2 operator*(Vector2 v, float f) { return {v.x * f, v.y * f}; }
Vector2 operator/(Vector2 v, float f) { return {v.x / f, v.y / f}; }
Vector2 operator*(float f, Vector2 v) { return {v.x * f, v.y * f}; }
Vector2 operator/(float f, Vector2 v) { return {v.x / f, v.y / f}; }
bool operator==(Vector2 a, Vector2 b) { return a.x == b.x && a.y == b.y; }
bool operator!=(Vector2 a, Vector2 b) { return a.x != b.x || a.y != b.y; }
float dot(Vector2 l, Vector2 r) { return l.x * r.x + l.y * r.y; }
// proj_u(v) = (v . u / u . u) * u
Vector2 proj(Vector2 v, Vector2 u) { return dot(u, v) / u.magsq() * u; }

namespace ng {
s64 print_get_item_size(Vector2 v) {
    return print_get_item_size(v.x) + print_get_item_size(v.y) + 4;
}
inline void print_item(print_buffer *buf, Vector2 v) {
    buf->putchar('(');
    print_item(buf, v.x);
    buf->putchar(','), buf->putchar(' ');
    print_item(buf, v.y);
    buf->putchar(')');
}
} // namespace ng

union Rectangle {
    struct {
        float x;
        float y;
        float w;
        float h;
    };
    struct {
        Vector2 pos;
        Vector2 size;
    };
    constexpr Rectangle() : x{}, y{}, w{}, h{} {}
    constexpr Rectangle(float x, float y, float w, float h) : x{x}, y{y}, w{w}, h{h} {}
    constexpr Rectangle(Vector2 pos, Vector2 size) : pos{pos}, size{size} {}

    constexpr float l() const { return x; }
    constexpr float r() const { return x + w; }
    constexpr float b() const { return y; }
    constexpr float t() const { return y + h; }
    constexpr float cx() const { return x + w / 2; }
    constexpr float cy() const { return y + h / 2; }
};
constexpr Rectangle rect() { return Rectangle{}; }
constexpr Rectangle rect(f32 x, f32 y, f32 w, f32 h) { return Rectangle{x, y, w, h}; }
constexpr Rectangle rect(Vector2 pos, Vector2 size) { return Rectangle{pos, size}; }

// Vector2 rect_center(Rectangle r) { return r.pos + r.size / 2; }

bool rect_intersect(Rectangle a, Rectangle b) {
    bool horizontal = (a.x < b.x + b.w && a.x + a.w > b.x);
    bool vertical = (a.y < b.y + b.h && a.y + a.h > b.y);
    return horizontal && vertical;
}

Rectangle offset_rect(Rectangle r, Vector2 v) {
    r.pos += v;
    return r;
}

SDL_Rect rect_to_sdl(Rectangle r) {
    SDL_Rect result;
    result.x = r.x;
    result.y = r.y;
    result.w = r.w;
    result.h = r.h;
    return result;
}

#include "constants.h"

struct Render_Rect {
    Rectangle rect = {};
    SDL_Color color = {};
    SDL_BlendMode blend_mode = SDL_BLENDMODE_NONE;
};

struct Input {
    Vector2 mouse_pos = {};
    bool lmb_new = false;
    bool rmb_new = false;
    bool space = false;
    bool space_new = false;
    bool shift_new = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

struct Texture {
    SDL_Texture *tex = null;
    Vector2 offset = {};
    int w = 0;
    int h = 0;
    int frame_w = 0;
    float fps = 0;
    bool flip = false;
};

Texture dummy_texture() {
    Texture result = {};
    result.w = 1;
    result.frame_w = 1;
    result.h = 1;
    return result;
}

struct Entity {
    bool scheduled_for_deletion = false;
    float z = 0;
    Vector2 pos = {};
    Vector2 vel = {};
    Texture texture = {};
    float animation_timer = 0;

    bool about_to_finish(float dt) { return anim_timer_advanced(dt) >= 1; }
    // Hack to non-loop an animation.
    bool switch_if_about_to_finish(float dt, const Texture &new_texture) {
        if (about_to_finish(dt)) {
            switch_to_animation(new_texture);
            return true;
        }
        return false;
    }

    float anim_timer_advanced(float dt) {
        auto num_frames = (texture.w / texture.frame_w);
        return animation_timer + dt * texture.fps / num_frames;
    }

    void switch_to_animation(const Texture &new_texture) {
        if (texture.tex != new_texture.tex) {
            auto flip = texture.flip;
            texture = new_texture;
            animation_timer = 0;
            texture.flip = flip;
        }
    }
};

struct Particle : Entity {
    Vector2 acc = {};
    float acc_z = 0;

    float vel_z = 0;

    f64 birth_time = 0;
    float lifespan = 0;
    float size = 0;

    SDL_Color color = {};
};

struct Render_Texture {
    Vector2 pos = {};
    Vector2 offset = {};
    float z = 0;
    float w = 0;
    float h = 0;
    int atlas_index = 0;
    bool flip = false;
    SDL_Color color = {};
    SDL_Texture *tex = null;
};

#define For(...)                                                                                   \
    ng_for(__VA_ARGS__) if (it.scheduled_for_deletion) {}                                          \
    else
#define Foreach(name, ...)                                                                         \
    ng_foreach(&&name : (__VA_ARGS__)) if (name.scheduled_for_deletion) {}                         \
    else

float health_to_light_radius(float health) { return health * GUY_MAX_LIGHT_RADIUS; }

struct Guy : Entity {
    float torch = 0;
    bool is_dead = false;

    Vector2 torch_offset() {
        auto torch_offset = GUY_TORCH_OFFSET_SIDE;
        if (facing_up) {
            torch_offset = GUY_TORCH_OFFSET_UP;
        }
        if (facing_left) torch_offset.x *= -1;
        return torch_offset;
    }

    float torch_charge_particle_timer = 0;

    bool facing_left = false;
    bool facing_up = false;
    bool facing_down = false;

    int attack_direction = 0; // [0, 4) counterclockwise

    float attack_windup_timer = 0;
    float attack_cooldown_timer = 0;

    float stun_timer = 0;
    float stun_torch_shrink_timer = 0;

    float dash_particle_timer = 0;

    float dash_timer = 0;
    float dash_cooldown_timer = 0;
    Vector2 dash_velocity = {};

    float sfx_walk_timer = 0;

    bool is_dashing = false;
};

#define ENEM_CHOOSE(value_name)                                                                    \
    auto value_name() { return is_swarm ? SWARM_##value_name : GOBLIN_##value_name; }

struct Enemy : Entity {
    float health = 0;

    bool is_swarm = false;

    bool aggroed = false;

    float attack_windup_timer = 0;
    float attack_cooldown_timer = 0;

    float stun_timer = 0;

    float death_timer = 0;

    float fire_timer = 0;
    float fire_hurt_timer = 0;
    float fire_particle_timer = 0;

    float sfx_walk_timer = 0;

    int vocal_channel(struct Game *game);

    ENEM_CHOOSE(HITBOX);
    ENEM_CHOOSE(SPEED);
    ENEM_CHOOSE(ATTACK_WINDUP_TIME);
    ENEM_CHOOSE(ATTACK_COOLDOWN_TIME);
    ENEM_CHOOSE(SWING_DISTANCE);
    ENEM_CHOOSE(STUN_TIME);
    ENEM_CHOOSE(HEALTH);
    ENEM_CHOOSE(DEATH_TIME);
    ENEM_CHOOSE(AGGRO_RADIUS);
    ENEM_CHOOSE(DAMAGE);
    ENEM_CHOOSE(STRIKE_RADIUS);
    ENEM_CHOOSE(STRIKE_REWARD);
    ENEM_CHOOSE(DEATH_REWARD);
};

struct Torch : Entity {
    // float radius = 0;

    float radius() { return light_level * TORCH_MAX_LIGHT_RADIUS; }

    float light_level = 0;
    float oscillation_offset = 0;
};

Vector2 rand_v2(ng::mt127_64 *rng, Vector2 min, Vector2 max) {
    Vector2 result;
    result.x = rand_range(rng, min.x, max.x);
    result.y = rand_range(rng, min.y, max.y);
    return result;
}

Vector2 rand_v2(ng::mt127_64 *rng) {
    Vector2 result;
    result.x = (rng->next_float() * 2 - 1);
    result.y = (rng->next_float() * 2 - 1);
    auto mag = result.magsq();
    if (mag > 1) {
        mag = 1 / SDL_sqrtf(mag);
        result *= mag;
    }
    return result;
}

struct Camera {
    Vector2 pos = {}; // effective pos; no time to refactor

    Vector2 real_pos = {};
    float scale = 0;

    struct Shake {
        float amount = 0;
        float timer = 0;
    };
    ng::auto_array<Shake> shakes = {};

#define SHAKE_TIME 0.75f
    void shake(float amt) {
        auto shake = shakes.push();
        shake->amount = amt * 0.5f; // lol
        shake->timer = SHAKE_TIME;
    }
};

struct Collision_Info {
    int x = 0;
    int y = 0;
    int tile = 0;
};

struct Level {
    int w = 0;
    int h = 0;
    int *tiles = null;

    ng::array<Collision_Info> get_colliding_tiles(Rectangle aabb) {
        ng::array<Collision_Info> result = {};

        const auto aabb_l = aabb.x;
        const auto aabb_r = aabb.x + aabb.w;
        const auto aabb_b = aabb.y;
        const auto aabb_t = aabb.y + aabb.h;

        auto pxmin = aabb_l;
        auto pxmax = aabb_r;

        auto pymin = aabb_b;
        auto pymax = aabb_t;

        pxmin += 8;
        pxmax += 8;

        if (cast(int)(pymax / 16) * 16 == pymax) { // Exactly flush.
            pymax -= 0.001;                        // Hacky.
        }
        if (cast(int)(pxmax / 16) * 16 == pxmax) { // Exactly flush.
            pxmax -= 0.001;                        // Hacky.
        }

        auto txmin = cast(int)((pxmin) / 16);
        auto txmax = cast(int)((pxmax) / 16);

        // Max and min switch here, because the Y coord is negated.
        auto tymin = -cast(int)((pymax) / 16) + h;
        auto tymax = -cast(int)((pymin) / 16) + h;

        txmin = ng::clamp(txmin, 0, w - 1);
        tymin = ng::clamp(tymin, 0, h - 1);
        txmax = ng::clamp(txmax, 0, w - 1);
        tymax = ng::clamp(tymax, 0, h - 1);

        for (int j = tymin; j <= tymax; j += 1) {
            for (int i = txmin; i <= txmax; i += 1) {
                auto index = cast(int)(i + (j * w));

                auto type = tiles[index];

                auto tile = result.push();
                tile->x = i;
                tile->y = -j + h;
                tile->tile = type;
            }
        }
        return result;
    }
};

#define ENEM_CHOOSE_GAME(value_name)                                                               \
    auto value_name(Enemy *e) { return e->is_swarm ? swarm_##value_name : goblin_##value_name; }

struct Game {
    Vector2 screen_size = {};
    bool quit = false;

    Level *level = null;

    Guy guy = {};

    bool guy_dead = false;

    ng::array<Torch> torches = {};
    ng::array<Enemy> enemies = {};
    ng::array<Particle> particles = {};
                                   
    ng::array<SDL_Texture *> all_textures = {};
    Texture tex_guy_idle = {};
    Texture tex_guy_run = {};
    Texture tex_guy_run_up = {};
    Texture tex_guy_attack = {};
    Texture tex_guy_attack_up = {};
    Texture tex_guy_attack_down = {};
    Texture tex_guy_stun = {};
    Texture tex_guy_dash = {};
    Texture tex_torch_active = {};
    Texture tex_torch_snuffed = {};

    Texture goblin_tex_idle = {};
    Texture goblin_tex_run = {};
    Texture goblin_tex_stun = {};
    Texture goblin_tex_death = {};
    Texture goblin_tex_attack = {};
    Texture goblin_tex_attack_up = {};
    Texture goblin_tex_attack_down = {};

    Texture swarm_tex_idle = {};
    Texture swarm_tex_run = {};
    Texture swarm_tex_stun = {};
    Texture swarm_tex_death = {};
    Texture swarm_tex_attack = {};
    Texture swarm_tex_attack_up = {};
    Texture swarm_tex_attack_down = {};

    ENEM_CHOOSE_GAME(tex_idle);
    ENEM_CHOOSE_GAME(tex_run);
    ENEM_CHOOSE_GAME(tex_stun);
    ENEM_CHOOSE_GAME(tex_death);
    ENEM_CHOOSE_GAME(tex_attack);
    ENEM_CHOOSE_GAME(tex_attack_up);
    ENEM_CHOOSE_GAME(tex_attack_down);

    Texture tex_particle = {};
    Texture tex_particle_fire = {};
    Texture tex_shadow = {};

    Texture tex_cursor = {};

    Texture tex_icon_wasd = {};
    Texture tex_icon_lmb = {};
    Texture tex_icon_rmb = {};

    Mix_Music *mus_ambient = null;
    Mix_Music *mus_credits = null;

    Mix_Chunk *sfx_guy_run[9] = {};
    u16 sfx_guy_run_picker = 0; // This 16-bit integer will cleanly overflow before any float precision problems arise.

    Mix_Chunk *sfx_guy_dash = null;

    Mix_Chunk *sfx_whoosh = null;

    Mix_Chunk *sfx_slam = null;

    Mix_Chunk *sfx_fire = null;

    f64 last_time_sfx_fire_played = -SFX_FIRE_LENGTH; // Sorry mister zero-is-initialization, you got screwed.

    Mix_Chunk *sfx_sword_clang = null;
    Mix_Chunk *sfx_sword_slice = null;

    Mix_Chunk *goblin_sfx_run[7] = {};
    u16 goblin_sfx_run_picker = 0;    
    Mix_Chunk *goblin_sfx_aggro = null;
    Mix_Chunk *goblin_sfx_attack = null;
    Mix_Chunk *goblin_sfx_death = null;
    Mix_Chunk *goblin_sfx_fire_tick = null;
    Mix_Chunk *goblin_sfx_hurt = null;
    Mix_Chunk *goblin_sfx_stunned = null;

    Mix_Chunk *swarm_sfx_run[7] = {};
    u16 swarm_sfx_run_picker = 0;      
    Mix_Chunk *swarm_sfx_aggro = null;
    Mix_Chunk *swarm_sfx_attack = null;
    Mix_Chunk *swarm_sfx_death = null;
    Mix_Chunk *swarm_sfx_fire_tick = null;
    Mix_Chunk *swarm_sfx_hurt = null;
    Mix_Chunk *swarm_sfx_stunned = null;

    ENEM_CHOOSE_GAME(sfx_run);
    ENEM_CHOOSE_GAME(sfx_aggro);
    ENEM_CHOOSE_GAME(sfx_attack);
    ENEM_CHOOSE_GAME(sfx_death);
    ENEM_CHOOSE_GAME(sfx_fire_tick);
    ENEM_CHOOSE_GAME(sfx_hurt);
    ENEM_CHOOSE_GAME(sfx_stunned);

    float sfx_volume(Enemy *e) { return e->is_swarm ? 1.5f : 1.0f; }

    Camera camera = {};

    ng::array<Render_Rect> render_rects_world = {};
    ng::array<Render_Rect> render_rects_screen = {};

    ng::array<Render_Texture> render_textures = {};

    ng::mt127_64 rng = {};
    ng::mt127_64 rng_fx = {};
};

int Enemy::vocal_channel(Game *game) {
    return (this - game->enemies.data) % NUM_ENEMY_VOCAL_CHANNELS;
}

float amplify_pan(float distance) { return SDL_pow(distance, 1.0f); }

void play_sound(Game *game, Mix_Chunk *which, float volume, Vector2 whence, int override_channel = -1) {
    if (!which) {
        return;
    }
    if (volume <= 0) return;
    if (volume > 1) volume = 1;

    int channel = Mix_PlayChannel(override_channel, which, 0);

    if (channel == -1) {
        // auto num_chans = Mix_AllocateChannels(-1);
        // for (int i = 0; i < num_chans; i += 1) {
        //     auto chunk = Mix_GetChunk(i);
        //     if (chunk == null || chunk == which) { // stop this sound at play it again
        //         Mix_HaltChannel(i);
        //         channel = Mix_PlayChannel(i, which, 0);
        //         break;
        //     }
        // }
        return;
    }

    if (channel == -1) {
        ng::string error = Mix_GetError();
        using ng::operator""_s;
        if (error == "No free channels available"_s) {
            auto old_num_chans = Mix_AllocateChannels(-1);
            auto new_num_chans = Mix_AllocateChannels(old_num_chans * 3 / 2);
            play_sound(game, which, volume, whence);
        }
    } else {
        auto disp = (whence - game->camera.real_pos);
        auto fade = 1 - (disp.mag() / (game->screen_size.mag() / 2));
        fade = ng::clamp(fade, 0.0f, 1.0f);
        Mix_Volume(channel, (volume * fade) * MIX_MAX_VOLUME);

        auto pan = ng::clamp(amplify_pan(disp.x / game->screen_size.x), -1.0f, +1.0f);
        pan += 1;
        pan /= 2;

        auto right = cast(u8)(pan * 255);
        Mix_SetPanning(channel, 255 - right, right);
    }
}

bool coin_flip(ng::mt127_64 *rng) { return rng->next_u64() >= (1ull << 63); }
void game_destroy(Game *game) {
    game->quit = true;
    game->all_textures.release();
    game->torches.release();
    game->enemies.release();
    game->render_rects_world.release();
    game->render_rects_screen.release();
    game->render_textures.release();
}
void game_clear(Game *game) {
    game->quit = false;
    game->guy.is_dead = false;
    game->torches.clear();
    game->enemies.clear();
    game->particles.clear();
    // game->goblin_spawn_timer = 0;
    // game->camera = {};
    game->render_rects_world.clear();
    game->render_rects_screen.clear();
    game->render_textures.clear();
    game->rng = {};
    game->rng_fx = {};
}

void push_rect_world(Game *game, Rectangle rect, SDL_Color color,
                     SDL_BlendMode blend_mode = SDL_BLENDMODE_BLEND) {
    if (dev_drawdebugrects) {
        auto render_rect = game->render_rects_world.push();
        render_rect->rect = rect;
        render_rect->color = color;
        render_rect->blend_mode = blend_mode;
    }
}
void push_rect_screen(Game *game, Rectangle rect, SDL_Color color,
                      SDL_BlendMode blend_mode = SDL_BLENDMODE_BLEND) {
    if (dev_drawdebugrects) {
        auto render_rect = game->render_rects_screen.push();
        render_rect->rect = rect;
        render_rect->color = color;
        render_rect->blend_mode = blend_mode;
    }
}

Rectangle rect_world_to_screen(Rectangle r, Game *game) {
    { // shift based on camera
        r.pos -= game->camera.pos;
        { // scale based on camera
            r.size *= game->camera.scale;
            r.pos *= game->camera.scale;
        }
        r.pos += game->screen_size / 2;
    }
    // cartesian to screen
    r.y = -r.y;
    r.y += game->screen_size.y;
    r.y -= r.h;
    return r;
}

Vector2 v2_world_to_screen(Vector2 v, Game *game) {
    { // shift based on camera
        v -= game->camera.pos;
        { // scale based on camera
            v *= game->camera.scale;
        }
        v += game->screen_size / 2;
    }
    // cartesian to screen
    v.y = -v.y;
    v.y += game->screen_size.y;
    return v;
}
Vector2 v2_screen_to_world(Vector2 v, Game *game) {
    // cartesian to screen
    v.y -= game->screen_size.y;
    v.y = -v.y;
    { // shift based on camera
        v -= game->screen_size / 2;
        { // scale based on camera
            v /= game->camera.scale;
        }
        v += game->camera.pos;
    }
    return v;
}

Rectangle camera_bounds_in_world_space(Game *game) {
    Rectangle result = {};
    result.pos = game->camera.pos;
    result.size = game->screen_size / game->camera.scale;
    result.pos -= result.size / 2;
    return result;
}

Particle *make_fire_particle(Game *game, f64 birth_time, float lifespan, Vector2 pos, float z,
                             float rand_pos, float rand_vel, float rand_z) {
    auto p = game->particles.push();
    p->texture = game->tex_particle_fire;
    p->size = p->texture.h;
    p->birth_time = birth_time;
    p->lifespan = lifespan;
    p->z = rand_range(&game->rng_fx, 0, 1) * rand_z + z;
    p->pos = pos;
    if (rand_pos != 0) {
        p->pos += rand_v2(&game->rng_fx) * rand_pos;
    }
    if (rand_vel != 0) {
        p->vel = rand_v2(&game->rng_fx) * rand_vel;
    }
    p->color = GENERIC_FIRE_PARTICLE_COLOR;
    return p;
}

Particle *make_impact_particle(Game *game, Rectangle hitbox, f64 birth_time, Vector2 pos, float z, Vector2 base_vel, float lifespan, SDL_Color color) {

    pos.x += rand_range(&game->rng_fx, -0.5, 0.5) * hitbox.w;
    pos.y += rand_range(&game->rng_fx, -0.5, 0.5) * hitbox.h;

    auto p = game->particles.push();
    p->pos = pos;
    p->texture = game->tex_particle;
    // p->vel = (p->pos - it.pos).hat() * 100;
    // p->acc = p->vel * 0.5;
    // p->acc.y += 100;
    p->vel = base_vel;
    p->vel += p->vel.hat().perp() * rand_range(&game->rng_fx, -0.5, 0.5) * 64;
    // p->acc = (p->pos - it.pos).hat() * 64;
    p->acc = base_vel * -2;
    p->acc.y -= 48;
    p->birth_time = birth_time;
    p->lifespan = lifespan;
    p->color = color;
    p->size = 4;

    // p->z = 32;
    p->z = z;
    p->pos.y -= p->z / 2;
    return p;
}

float is_lit(Vector2 pos, ng::array<Torch> *lights) {
    For(*lights) {
        if (it.light_level <= 0) continue;

        auto disp = (pos - (it.pos + v2(0, it.z)));
        if (disp.mag() < it.radius()) {
            // YO THATS LIT FAM
            return it.radius() - disp.mag();
        }
    }
    return 0;
}

Torch *find_dash_target(Vector2 pos, ng::array<Torch> *lights) {
    For(*lights) {
        if (it.light_level <= 0) continue;

        auto disp = (pos - (it.pos + v2(0, it.z)));

        if (disp.mag() < (it.radius() / 4)) {
            return &it;
        }
    }
    return null;
}

#define CAMERA_ROUNDING 2
void camera_update(Camera *camera, f64 dt, Game *game) {
    camera->real_pos =
        damp(camera->real_pos, game->guy.pos + v2(0, 16) + game->guy.vel * 0.25f, 0.25, dt);

    camera->pos = camera->real_pos;

    if (dt > 0) { // apply shake
        if (camera->shakes.count) {

            float shake_amt = 0;
            ng_for(camera->shakes) {
                shake_amt += it.amount * (it.timer / SHAKE_TIME);
                it.timer -= dt;
            }

            auto theta = rand_range(&game->rng_fx, 0.0f, ng::TAU32);
            auto r = shake_amt;
            // r *= rand_range(&game->rng_fx, 0.2, 1);
            if (r > 0) {
                auto offset = v2(r * SDL_cosf(theta), r * SDL_sinf(theta));

                camera->pos += offset;
            }
        }
    }

    for (int i = 0; i < camera->shakes.count;) {
        if (camera->shakes[i].timer <= 0) {
            camera->shakes.remove(i);
        } else {
            i += 1;
        }
    }

    auto round = [&camera](float f) {
        return SDL_floor(f * camera->scale / CAMERA_ROUNDING) / camera->scale * CAMERA_ROUNDING;
    };
    camera->pos.x = round(camera->pos.x);
    camera->pos.y = round(camera->pos.y);
}

bool tile_is_floor(int value) {
    if (value == 0) return true;

    if (value >= 22 && value < 28) return true;

    return false;
}

bool tile_is_below_floor(int value) {
    if (value >= 28 && value < 32) return true;

    return false;
}

bool tile_is_nothing(int value) { return value >= 47; }

bool tile_is_wall(int value) {
    if (tile_is_floor(value)) return false;
    if (tile_is_below_floor(value)) return false;
    if (tile_is_nothing(value)) return false;

    return true;
}

bool tile_is_collisible(int value) {
    if (tile_is_nothing(value)) return true;

    if (tile_is_floor(value)) return false;
    if (tile_is_below_floor(value)) return true;

    if (tile_is_wall(value)) return true;

    return false;
}

bool tile_is_not_dashable(int value) {
    if (tile_is_nothing(value)) return false;

    if (tile_is_floor(value)) return false;
    if (tile_is_below_floor(value)) return false;

    if (tile_is_wall(value)) return true;

    return false;
}

#include "collide.hpp"

bool collide_like_total_magic(Level *level, Vector2 &pos, Vector2 &vel, f64 dt, Rectangle hitbox,
                              bool (&collision_checker)(int) = tile_is_collisible) {
    Collide_State cstate = {};
    cstate.pos = pos;
    cstate.delta = vel * dt;
    cstate.collider_bottom = rect(hitbox.x + 1, hitbox.y, hitbox.w - 2, hitbox.h / 2);
    cstate.collider_top = rect(hitbox.x + 1, hitbox.cy(), hitbox.w - 2, hitbox.h / 2);
    cstate.collider_right = rect(hitbox.cx(), hitbox.y + 1, hitbox.w / 2, hitbox.h - 2);
    cstate.collider_left = rect(hitbox.x, hitbox.y + 1, hitbox.w / 2, hitbox.h - 2);
    auto result = collide(cstate, level, collision_checker);
    pos = result.new_state.pos;

    if (result.top.did_collide) {
        vel.y = ng::min(vel.y, 0.0f);
    }
    if (result.bottom.did_collide) {
        vel.y = ng::max(vel.y, 0.0f);
    }
    if (result.right.did_collide) {
        vel.x = ng::min(vel.x, 0.0f);
    }
    if (result.left.did_collide) {
        vel.x = ng::max(vel.x, 0.0f);
    }

    return result.top.did_collide || result.bottom.did_collide || result.right.did_collide ||
           result.left.did_collide;
}

void guy_update(Guy *guy, f64 dt, f64 t, Input *input, Game *game) {

    if (dt == 0) return;
    
    // // mostly for debug
    // if (game->guy.dash_velocity.magsq() <= 0.5f*0.5f) game->guy.dash_velocity = v2(DASH_SPEED, 0);
    if (game->guy.is_dashing && game->guy.dash_velocity.magsq() == 0) {
        game->guy.dash_velocity = v2(1, 1);
        game->guy.vel = game->guy.dash_velocity;
    }


    auto input_vec = v2(input->right - input->left, input->up - input->down).hat();
    auto acc = v2();

    auto do_the_dash = [&](Vector2 desired_dash_vector) {

        auto dash_dir = desired_dash_vector;

        bool can_dash = true;

        if (guy->is_dashing || guy->dash_cooldown_timer > 0) can_dash = false;

        if (guy->torch < CAN_DASH_THRESHOLD) can_dash = false;

        if (can_dash) {

            guy->torch -= DASH_HURT_GUY_AMT;

            guy->is_dashing = true;

            guy->dash_timer = DASH_TIME;
            guy->dash_cooldown_timer = DASH_COOLDOWN_TIME;
            guy->dash_particle_timer = 0;

            auto distance_to_dash = DASH_DISTANCE;

            guy->dash_velocity = dash_dir.hat() * DASH_SPEED;

            play_sound(game, game->sfx_guy_dash, 1, guy->pos);
        }
    };
                                      
    float max_speed = GUY_SPEED;

    // returns just leave this func
    [&] {
        if (guy->stun_torch_shrink_timer > 0) {
            guy->stun_torch_shrink_timer -= dt;
        } else {
            guy->stun_torch_shrink_timer = 0;
        }

        if (guy->torch <= 0) guy->is_dead = true;

        if (guy->is_dead) {
            max_speed = 0;
            return;
        }

        if (guy->stun_timer > 0) {
            guy->stun_timer -= dt;

            guy->dash_timer = 0;
            guy->attack_windup_timer = 0;
            // guy->attack_cooldown_timer = 0;

            max_speed = 0;
            return;
        } else {
            guy->stun_timer = 0;
        }

        if (guy->attack_windup_timer > 0) {
            guy->attack_windup_timer -= dt;

            if (guy->attack_windup_timer <= 0) {
                auto strike_hitbox = rect();
                {
                    auto &hb = strike_hitbox;
                    auto &ad = guy->attack_direction;

                    Rectangle hbs[] = {
                        rect(0, -16, GUY_STRIKE_DISTANCE, 32),
                        rect(-24, 0, 48, GUY_STRIKE_DISTANCE * 0.75),
                        rect(-GUY_STRIKE_DISTANCE, -16, GUY_STRIKE_DISTANCE, 32),
                        rect(-24, -GUY_STRIKE_DISTANCE * 0.75, 48, GUY_STRIKE_DISTANCE * 0.75),
                    };

                    hb = hbs[ad];
                }

                auto strike_aabb = offset_rect(strike_hitbox, guy->pos);

                ng::auto_array<Vector2> walls_hit = {};

                auto tiles = game->level->get_colliding_tiles(strike_aabb);
                defer { tiles.release(); };
                foreach (&&tile : tiles) {
                    if (tile_is_not_dashable(tile.tile)) {
                        walls_hit.push(v2(tile.x - 0.5f, tile.y) * 16);
                    }
                }

                bool got_one = false;

                For(game->enemies) {
                    if (it.death_timer > 0) continue;

                    if (rect_intersect(strike_aabb, offset_rect(it.HITBOX(), it.pos))) {

                        got_one = true;
                        // if (it.is_swarm) {
                        //
                        // } else {

                        it.stun_timer += GUY_STRIKE_STUN_TIME;
                        it.aggroed = true;

                        it.health -= 1.0f;

                        game->camera.shake(3);
                        
                        for (int i = 0; i < 10; i += 1) {
                            auto p = make_impact_particle(game, it.HITBOX(), t, it.pos, it.is_swarm ? 64 : 32, (it.pos - guy->pos).hat() * 128, 0.2, ATTACK_IMPACT_PARTICLE_COLOR);
                        }

                        if (it.health <= 0) {
                            guy->torch += it.DEATH_REWARD();

                            it.death_timer = it.DEATH_TIME();
                            play_sound(game, game->sfx_death(&it), game->sfx_volume(&it) * 0.5, it.pos, it.vocal_channel(game));
                        } else {
                            guy->torch += it.STRIKE_REWARD();

                            play_sound(game, game->sfx_hurt(&it), game->sfx_volume(&it) * 0.3, it.pos, it.vocal_channel(game));
                        }

                        it.vel += (it.pos - guy->pos).hat() * 100;
                    }
                }

                guy->attack_cooldown_timer += guy->attack_windup_timer;
                guy->attack_cooldown_timer += GUY_ATTACK_COOLDOWN_TIME;

                // FX

                if (got_one) {
                    play_sound(game, game->sfx_slam, 0.6, guy->pos);
                } else if (walls_hit.count) {
                    play_sound(game, game->sfx_slam, 0.3, guy->pos);
                }
                if (walls_hit.count) {
                    play_sound(game, game->sfx_sword_clang, 0.3, guy->pos);
                    game->camera.shake(1);
                }
                if (got_one) {
                    play_sound(game, game->sfx_sword_slice, 0.5, guy->pos);
                }

                auto attack_dir_to_v2 = [](int dir) {
                    if (dir == 0) return v2(1, 0);
                    if (dir == 1) return v2(0, 1);
                    if (dir == 2) return v2(-1, 0);
                    if (dir == 3) return v2(0, -1);
                    return v2();
                };

                foreach (&&wall : walls_hit) {
                    auto rng = &game->rng_fx;

                    // create sparks
                    for (int i = rand_range(&game->rng_fx, 3, 1) + 0.5f; i > 0; i -= 1) {
                        auto p = game->particles.push();
                        p->texture = game->tex_particle;
                        p->birth_time = t;
                        p->lifespan = 0.5;

                        p->pos = wall + rand_v2(rng) * 4;
                        // auto dir = v2_to_cardinal(guy->pos - p->pos);
                        auto dir = -attack_dir_to_v2(guy->attack_direction);
                        p->vel = dir * 256 + rand_v2(rng) * 32;

                        p->vel += dir.perp() * rand_range(rng, -32, 32);

                        p->acc = -p->vel / 2 / p->lifespan;
                        p->z = 20 + rand_range(rng, -8, 8);
                        p->vel_z = rand_range(rng, 8, 16) / p->lifespan;
                        p->acc_z = -p->vel_z / 2 / p->lifespan;

                        p->size = 3;
                        p->color = WALL_HIT_PARTICLE_COLOR;
                    }
                }
            }

            max_speed *= 0.35;
            return;
        } else if (guy->attack_cooldown_timer > 0) {
            guy->attack_cooldown_timer -= dt;

            max_speed *= 0.35;
            return;
        }

        if (guy->is_dashing) {
            guy->dash_timer -= dt;

            guy->dash_particle_timer -= dt;
            if (guy->dash_particle_timer < 0) {
                guy->dash_particle_timer += DASH_PARTICLE_TIME;

                for (int i = 0; i < 5; i += 1) {
                    auto p = make_fire_particle(game, t, 0.75f, guy->pos, 0, 12, 12, 16);
                    p->size = p->texture.h;
                    p->color = DASH_PARTICLE_COLOR;
                }
            }

            guy->vel = guy->dash_velocity;

            bool got_one = false;
            bool got_non_swarm = false;
            For(game->enemies) {
                if (it.death_timer > 0) continue;

                if (it.stun_timer > 0) continue;

                if (rect_intersect(offset_rect(GUY_HITBOX, guy->pos),
                                   offset_rect(it.HITBOX(), it.pos))) {

                    got_one = true;

                    auto dir = (it.pos - guy->pos);

                    auto dash_intensity = (guy->dash_velocity.mag() / DASH_SPEED); // slower -> weaker

                    it.stun_timer += it.STUN_TIME() * dash_intensity;
                    it.aggroed = true;
                    
                    play_sound(game, game->sfx_stunned(&it), game->sfx_volume(&it) * 0.6 * dash_intensity, it.pos, it.vocal_channel(game));

                    it.health -= DASH_IMPACT_DAMAGE * dash_intensity;

                    if (1 || !it.is_swarm) {
                        got_non_swarm = true;
                        guy->dash_timer *= 0.5f;
                        guy->dash_velocity *= 0.5f;
                    }

                    game->camera.shake(4);

                    for (int i = 0; i < 20; i += 1) {
                        make_impact_particle(game, it.HITBOX(), t, it.pos, it.is_swarm ? 64 : 32, guy->dash_velocity.hat() * 32, 0.4, DASH_IMPACT_PARTICLE_COLOR);
                    }

                    it.fire_timer = DASH_FIRE_TIME;
                    it.fire_hurt_timer = DASH_FIRE_HURT_INTERVAL;
                    it.fire_particle_timer = 0;

                    // break;
                }
            }
            if (got_one) {
                // guy->torch += DASH_HURT_GUY_AMT;
                // guy->torch += 0.05f;

                // if (got_non_swarm) {
                    play_sound(game, game->sfx_slam, 0.7, guy->pos);
                // }
                if (t - game->last_time_sfx_fire_played >= SFX_FIRE_LENGTH * 0.5f) {
                    game->last_time_sfx_fire_played = t;
                    play_sound(game, game->sfx_fire, 1, guy->pos);
                }
            }

            return;
        }

        auto mouse_pos = v2_screen_to_world(input->mouse_pos, game);
        auto mouse_pos_that_makes_things_feel_actually_good = mouse_pos - v2(0, 16);

        auto desired_attack_direction = -1;
        if (input->lmb_new) {

            auto disp = (mouse_pos_that_makes_things_feel_actually_good - guy->pos);

            if (SDL_fabs(disp.y) > SDL_fabs(disp.x)) {
                if (disp.y >= 0) {
                    desired_attack_direction = 1;
                } else {
                    desired_attack_direction = 3;
                }
            } else {
                if (disp.x >= 0) {
                    desired_attack_direction = 0;
                } else {
                    desired_attack_direction = 2;
                }
            }
        } else if (input->space_new) {
            desired_attack_direction = 0;

            if (guy->facing_left) desired_attack_direction = 2;

            if (guy->facing_up || input_vec.y > 0) desired_attack_direction = 1;

            if (guy->facing_down || input_vec.y < 0) desired_attack_direction = 3;
        }

        if (desired_attack_direction >= 0) {
            guy->attack_windup_timer = GUY_ATTACK_WINDUP_TIME;
            guy->attack_direction = desired_attack_direction;

            play_sound(game, game->sfx_whoosh, 0.3, guy->pos);
            return;
        }

        auto desired_dash_vector = v2();
        if (input->rmb_new) {
            desired_dash_vector = (mouse_pos_that_makes_things_feel_actually_good - guy->pos);
        } else if (input->shift_new) {
            desired_dash_vector = input_vec;
        }

        if (desired_dash_vector != v2()) {
            do_the_dash(desired_dash_vector);
            return;
        }
    }();

    if (!guy->is_dashing) {
        acc = input_vec * GUY_ACCEL;

        { // Technically you need to subtract the overflow
            guy->dash_cooldown_timer += guy->dash_timer;
            guy->dash_timer = 0; // no more overflow
            guy->is_dashing = false;
        }

        guy->dash_cooldown_timer -= dt;

        {
            auto max_post_dash_speed = guy->dash_velocity.mag() * (guy->dash_cooldown_timer / DASH_COOLDOWN_TIME);
            if (max_post_dash_speed > GUY_SPEED) max_speed = max_post_dash_speed;
        }
        // remove component of acceleration in direction of velocity
        auto new_vel = guy->vel + acc * dt;
        auto mag = new_vel.mag();
        if (mag > max_speed) {
            new_vel *= max_speed / mag;
            if (dot(acc, new_vel) > 0) {
                acc -= proj(acc, new_vel);
            }
        }
        guy->vel = new_vel;
    }

    if (guy->torch >= 1) { // greater OR EQUAL!
        guy->torch = 1;
    } else {
        Torch *charging_torch = null;
        Torch *dead_charging_torch = null;
        For(game->torches) {
            if (guy->is_dashing) break;

            // auto torch_offset_x = GUY_TORCH_OFFSET.x;
            // if (guy->facing_left) torch_offset_x *= -1;
            //
            // auto gpos = guy->pos;
            // gpos.x += torch_offset_x;
            //
            // auto gz = GUY_TORCH_OFFSET.y;
            //
            // if ((it.pos - guy->pos).mag() < 16 && it.z - gz < 8) {
            //     guy->torch += it.light_level * dt;
            // }
            if ((it.pos - guy->pos).mag() > TORCH_CHARGE_RADIUS) {
                continue;
            }

            if (it.light_level <= TORCH_MINIMUM_LIGHT_TO_CHARGE) {
                dead_charging_torch = &it;
                continue;
            }

            auto amount = TORCH_CHARGE_SPEED * dt;
            if (it.light_level - amount < TORCH_MINIMUM_LIGHT_TO_CHARGE) {
                amount = it.light_level - TORCH_MINIMUM_LIGHT_TO_CHARGE;
                it.light_level = TORCH_MINIMUM_LIGHT_TO_CHARGE;
            } else {
                it.light_level -= amount;
            }
            if (guy->torch + amount > 1) {
                amount = 1 - guy->torch;
                guy->torch = 1;
            } else {
                guy->torch += amount;
            }
            charging_torch = &it;
            break;
        }

        Torch *which_torch_to_use = charging_torch;
        if (charging_torch) {
            guy->torch_charge_particle_timer -= dt;
        } else if (dead_charging_torch) {
            if (guy->torch < CAN_DASH_THRESHOLD) {
                // Hack to make sure the player doesn't get trapped on one side of a pit.
                guy->torch += TORCH_DEAD_CHARGE_SPEED * dt;
                if (guy->torch > CAN_DASH_THRESHOLD) {
                    guy->torch = CAN_DASH_THRESHOLD;
                }
                guy->torch_charge_particle_timer -= dt * (TORCH_DEAD_CHARGE_SPEED / TORCH_CHARGE_SPEED);

                which_torch_to_use = dead_charging_torch;
            }
        } else {
            guy->torch_charge_particle_timer = 0;
        }
        while (guy->torch_charge_particle_timer < 0) {

            auto x = rand_range(&game->rng_fx, -0.5, 0.5) * 4;
            auto y = rand_range(&game->rng_fx, 0, 1) * 4;

            x += which_torch_to_use->pos.x;
            y += which_torch_to_use->pos.y;

            auto p = game->particles.push();
            p->pos = v2(x, y);
            p->z = which_torch_to_use->z + rand_range(&game->rng_fx, 0, 1) * 4;

            p->texture = game->tex_particle;
            p->birth_time = t;

            auto torch_offset = guy->torch_offset();

            auto guy_torch_pos = guy->pos;
            guy_torch_pos.x += torch_offset.x;
            guy_torch_pos.x += rand_range(&game->rng_fx, -0.5, 0.5) * 4;
            guy_torch_pos.y += rand_range(&game->rng_fx, -0.5, 0.5) * 4;
            auto guy_torch_pos_z = guy->z + torch_offset.y;
#if 0
            p->lifespan = 0.5f;
            auto flytime = p->lifespan * 0.5;
            p->vel = ((guy_torch_pos + guy->vel * flytime) - p->pos) / flytime;
            p->vel_z = (guy_torch_pos_z - p->z) / flytime;
#elif 1
            auto flyspeed = 75.0f;
            auto disp = guy_torch_pos - p->pos;
            auto disp_z = guy_torch_pos_z - p->z;

            auto dist3d = disp.magsq() + disp_z * disp_z;

            if (dist3d > 0) {
                dist3d = SDL_sqrtf(dist3d);

                auto dir = disp / dist3d;
                auto dir_z = disp_z / dist3d;

                p->vel = dir * flyspeed;
                p->lifespan = dist3d / flyspeed * 1.0f;
                p->vel_z = dir_z * flyspeed;
            }
#else
            auto disp = (guy_torch_pos - p->pos);
            auto flyspeed = 50;
            p->vel = disp.hat() * flyspeed;
            p->vel_z = (guy_torch_pos_z > p->z ? 1 : -1) * flyspeed;

            p->lifespan = disp.mag() / flyspeed;
#endif
            p->color = TORCH_CHARGE_PARTICLE_COLOR;
            p->size = 4;

            p->pos += p->vel * guy->torch_charge_particle_timer; // back-walk particle to exact birth time
            p->z += p->vel_z * guy->torch_charge_particle_timer; // back-walk particle to exact birth time

            guy->torch_charge_particle_timer += TORCH_CHARGE_PARTICLE_TIME;
        }
    }

    bool collided = false;

    bool above_pit = false;

    // if (guy->is_dashing)
    { // check whether the player is above a pit.
      // If he is, keep dashing.
        auto hitbox = GUY_HITBOX;
        hitbox.pos += v2(1, 1) * 1; // Be generous. Actually a hack.
        hitbox.size -= v2(2, 2) * 1;

        auto aabb = offset_rect(hitbox, guy->pos);

        auto tiles = game->level->get_colliding_tiles(aabb);
        defer { tiles.release(); };
        ng_for(tiles) {
            if (tile_is_collisible(it.tile) && !tile_is_not_dashable(it.tile)) {
                above_pit = true;
                break;
            }
        }
    }

    if (dev_noclip) above_pit = false;

    if (above_pit) {
        guy->is_dashing = true;
    } else if (guy->dash_timer <= 0) {
        guy->is_dashing = false;
    }

    if (!above_pit && guy->dash_timer <= 0) {
        guy->is_dashing = false;
    }

    // try to move the player based on vel, but also collide with world

    ng_assert(game->level);

    bool dashing_too_slow_to_continue_over_pit = false;
    if (guy->vel.mag() < DASH_SPEED * 0.95f) {
        dashing_too_slow_to_continue_over_pit = true;
    }

    if (dt > 0.0) { // collision
        if (dev_noclip) {
            guy->pos += guy->vel * dt;
        } else {
            auto collision_checker = tile_is_collisible;
            if (above_pit || (guy->is_dashing && !dashing_too_slow_to_continue_over_pit)) {
                collision_checker = tile_is_not_dashable;
            }
            collided = collide_like_total_magic(game->level, guy->pos, guy->vel, dt, GUY_HITBOX,
                                                *collision_checker);
        }
    }

    if (above_pit) {
        guy->dash_velocity = guy->vel.hat() * DASH_SPEED;
    }

    { // calculate friction
        auto sin_theta = dot(acc.hat(), guy->vel.hat());
        auto theta = SDL_asin(sin_theta);
        auto friction_multiplier = theta;
        friction_multiplier /= (-ng::TAU32 / 4);
        friction_multiplier += 1;
        friction_multiplier = ng_clamp(friction_multiplier, 0, 1);
        auto factor = SDL_pow(1 - GUY_FRICTION * friction_multiplier, dt);
        guy->vel *= factor;
    }

    if (collided && !dashing_too_slow_to_continue_over_pit) {
        game->camera.shake(2);

        // slam into wall sound
        play_sound(game, game->sfx_slam, 0.5, guy->pos);

        guy->dash_timer = 0;
        guy->is_dashing = false;
    }

    if (guy->vel.mag() < 1.0f / 60) guy->vel = v2();

    guy->facing_up = false;
    guy->facing_down = false;

    auto moving_mostly_vertically = (SDL_fabs(guy->vel.x) < SDL_fabs(guy->vel.y * 2));
    if (moving_mostly_vertically) {
        if (guy->vel.y > GUY_SPEED * 0.25f) {
            guy->facing_up = true;
        } else if (-guy->vel.y > GUY_SPEED * 0.25f) {
            guy->facing_down = true;
        }
    }

    if (guy->facing_up || guy->facing_down) {
        ng_assert(guy->facing_up != guy->facing_down);
    }

    // returns just leave this func
    [&] {

        if (guy->stun_timer > 0) {
            guy->switch_to_animation(game->tex_guy_stun);
            return;
        }

        if (guy->attack_windup_timer > 0) {
            Texture *anims[] = {
                &game->tex_guy_attack, &game->tex_guy_attack_up, &game->tex_guy_attack,
                &game->tex_guy_attack_down,
            };
            guy->switch_to_animation(*anims[guy->attack_direction]);

            if (guy->attack_direction == 0) {
                guy->facing_left = false;
            } else if (guy->attack_direction == 2) {
                guy->facing_left = true;
            } else {
                // It will remain as it was.
            }
            return;
        }

        if (guy->attack_cooldown_timer > 0) {
            return;
        }

        if (guy->is_dashing) {
            guy->switch_to_animation(game->tex_guy_dash);
            if (guy->vel.x > 0) {
                guy->facing_left = false;
            } else if (guy->vel.x < 0) {
                guy->facing_left = true;
            }
            return;
        }

        auto is_walking = guy->vel.mag() > GUY_SPEED * 0.25f;
        if (is_walking) {
            if (guy->facing_up) {
                guy->switch_to_animation(game->tex_guy_run_up);
            } else {
                guy->switch_to_animation(game->tex_guy_run);
            }

// #define GUY_SFX_WALK_BEGIN_TIME 0.2
// #define GUY_SFX_WALK_TIME 0.35
#define GUY_SFX_WALK_TIME (4 / game->tex_guy_run.fps) // MEGA-@Hack !!!
#if 0
#define GUY_SFX_WALK_BEGIN_TIME (3 / game->tex_guy_run.fps) // MEGA-@Hack !!!

            guy->sfx_walk_timer -= dt;
            if (guy->sfx_walk_timer <= 0) {
                play_footstep();
                guy->sfx_walk_timer += GUY_SFX_WALK_TIME;
            }
#else
            // @Hack to play the sound exactly when the step frame starts.

#define GUY_SFX_WALK_BEGIN_TIME 0.0f
            
            guy->sfx_walk_timer -= dt;
            if (guy->sfx_walk_timer <= 0) {

                auto num_frames = guy->texture.w / guy->texture.frame_w;
                auto which_frame = guy->animation_timer * num_frames;

                if ((which_frame >= 2+0.5 && which_frame <= 3+0.5) ||
                    (which_frame >= 6+0.5 && which_frame <= 7+0.5)) {
                    play_sound(game, game->sfx_guy_run[equidistribute(game->sfx_guy_run_picker, ng::countof(game->sfx_guy_run))], 0.25, guy->pos);
                    game->sfx_guy_run_picker += 1;
                                                                      
                    push_rect_world(game, offset_rect(GUY_HITBOX, guy->pos), SDL_Color{255,0,0,64});

                    guy->sfx_walk_timer = 1 / guy->texture.fps;
                }
            }
#endif
        } else {
            guy->switch_to_animation(game->tex_guy_idle);

            guy->sfx_walk_timer = GUY_SFX_WALK_BEGIN_TIME;
        }

        // if (guy->vel.x != 0) {
        //     guy->facing_left = (guy->vel.x < 0);
        // } else if (input_vec.x * guy->vel.x != 0) {
        //     guy->facing_left = (input_vec.x < 0);
        // }
        if (input_vec.x != 0) {
            guy->facing_left = (input_vec.x < 0);
        }
    }();

    guy->texture.flip = guy->facing_left;

    if (guy->torch <= 0) {
        guy->is_dead = true;
    }
}

bool should_aggro(Game *game, Enemy *enemy) {
    auto disp = (game->guy.pos - enemy->pos);
    auto m = disp.magsq();
    auto aggro_radius = enemy->AGGRO_RADIUS();
    if (enemy->is_swarm) {
        aggro_radius *= health_to_light_radius(game->guy.torch);
        return m < aggro_radius * aggro_radius;
    } else {
        if (m > aggro_radius * aggro_radius) return false;
        m = SDL_sqrtf(m);
        auto dir = disp / m;

        auto basis = enemy->texture.flip ? v2(-1, -1) : v2(1, -1);

        auto threshold = ng::lerp(-1.0f, 1.0f, 1 - ng::clamp(m / aggro_radius, 0.0f, 1.0f));

        auto drawdbgrect = [&] (float size, bool red) {
            auto dbgrect = rect();
            dbgrect.pos = enemy->pos;
            dbgrect.size = basis * size * aggro_radius;
            if (dbgrect.w > 0) {
                dbgrect.x -= dbgrect.w;
            } else {
                dbgrect.w = -dbgrect.w;
            }
            if (dbgrect.h < 0) {
                dbgrect.h = -dbgrect.h;
                // dbgrect.y += dbgrect.h;
            }
            u8 r = 255;
            if (red) r = 0;
            push_rect_world(game, dbgrect, SDL_Color{r, 0, 0, 128});
        };
        drawdbgrect(threshold, dot(dir, basis) / 2 < -threshold);
        drawdbgrect(dot(dir, basis) / 2, dot(dir, basis) / 2 < -threshold);

        if (dot(dir, basis) / 2 < -threshold) return false;
        return true;
    }
}

void enemy_update(Enemy *enemy, f64 dt, f64 t, Game *game) {

    if (enemy->aggroed) {
        For(game->enemies) {
            if (&it == enemy) break; // the rest will do it in their loop (Hack)

            if (!it.aggroed) continue;

            if (it.is_swarm != enemy->is_swarm) continue; // goblins push goblins, swarms push swarms

            auto displacement = it.pos - enemy->pos;

            auto repulsion_denominator = displacement.mag();
            if (repulsion_denominator < 1.0f) { // would cause massive forces, maybe even NaNs
                repulsion_denominator = 1.0f;
            }

            auto repulsion = displacement.hat() * 1000 / repulsion_denominator;
            it.vel += repulsion * dt;
            enemy->vel -= repulsion * dt;
        }
    }

    bool locomoted = false;
                              
    bool stun_should_override_death = false; // if false, enemies with health <= 0 will switch to death animation instantly. if true, stunned enemies will stay in the stun animation even if health <= 0, and only switch to death animation once the stun is over.

    auto enemy_max_speed = enemy->SPEED();
    // returns just leave this func
    [&] {
        auto disp = (game->guy.pos - enemy->pos);

        if (enemy->fire_timer > 0) {
            enemy->fire_timer -= dt;

            enemy->fire_hurt_timer -= dt;
            if (enemy->fire_hurt_timer < 0) {
                enemy->fire_hurt_timer += DASH_FIRE_HURT_INTERVAL;

                enemy->health -= (enemy->HEALTH() / DASH_FIRE_TIME * 0.25f) * enemy->fire_hurt_timer;
                enemy->stun_timer += DASH_FIRE_HURT_STUN_TIME;
                if (enemy->health <= 0) {
                    enemy->death_timer = enemy->DEATH_TIME();
                    play_sound(game, game->sfx_death(enemy), game->sfx_volume(enemy) * 0.5, enemy->pos, enemy->vocal_channel(game));
                } else {
                    play_sound(game, game->sfx_fire_tick(enemy), game->sfx_volume(enemy) * 0.3, enemy->pos, enemy->vocal_channel(game));
                }
            }

            enemy->fire_particle_timer -= dt;
            while (enemy->fire_particle_timer < 0) {
                enemy->fire_particle_timer += FIRE_PARTICLE_TIME;

                for (int i = 0; i < 5; i += 1) {
                    auto p = make_fire_particle(game, t, 0.8f, enemy->pos, enemy->z, 8, 8, 16);
                    p->size *= 1.5f;
                    p->acc.y += 10;
                }
            }
        } else {
            enemy->fire_timer = 0;
            enemy->fire_hurt_timer = 0;
            enemy->fire_particle_timer = 0;
        }

        if (enemy->fire_timer > 0 && enemy->stun_timer > 0 && enemy->stun_timer <= DASH_FIRE_HURT_STUN_TIME) { // stun comes from being hurt by fire (probably, although we could just be unlucky and he was on fire and also stunned from something else unrelated, but nobody cares)
            stun_should_override_death = true;
        }
        
        if (enemy->death_timer > 0 && !stun_should_override_death) {
            enemy->death_timer -= dt;
            if (enemy->death_timer <= 0) {
                enemy->scheduled_for_deletion = true;

                for (int i = 0; i < 20; i += 1) {

                    auto x = rand_range(&game->rng_fx, -0.5, 0.5) * enemy->HITBOX().w;
                    auto y = rand_range(&game->rng_fx, -0.5, 0.5) * enemy->HITBOX().h;

                    x += enemy->pos.x;
                    y += enemy->pos.y;

                    auto p = game->particles.push();
                    p->pos = v2(x, y);
                    p->texture = game->tex_particle;
                    // p->vel = (p->pos - enemy->pos).hat() * 100;
                    // p->acc = p->vel * 0.5;
                    // p->acc.y += 100;
                    p->vel = (p->pos - enemy->pos).hat() * rand_range(&game->rng_fx, 50, 150);
                    p->vel += v2(rand_range(&game->rng_fx, -0.5, 0.5) * 25,
                                 rand_range(&game->rng_fx, -0.5, 0.5) * 25);
                    p->acc.y += 100;
                    p->birth_time = t;
                    p->lifespan = 0.5;
                    p->color = ENEMY_DEATH_PARTICLE_COLOR;
                    p->size = 5;

                    p->z = 8;
                    p->pos.y -= p->z / 2;
                }
            }

            enemy->fire_timer = 0;
            enemy->stun_timer = 0;
            return;
        }
        
        if (enemy->stun_timer > 0) {
            enemy->stun_timer -= dt;

            enemy->attack_windup_timer = 0;
            // enemy->attack_cooldown_timer = 0;

            return;
        } else {
            enemy->stun_timer = 0;
        }

        if (game->guy.is_dead) return;

        if (enemy->attack_cooldown_timer > 0) {
            enemy->attack_cooldown_timer -= dt;
            // We want to continue chasing the guy.
            // return;
        }

        if (enemy->attack_windup_timer > 0) {
            enemy->attack_windup_timer -= dt;
            if (enemy->attack_windup_timer <= 0) {

                if (disp.mag() < enemy->STRIKE_RADIUS()) {

                    game->guy.torch -= enemy->DAMAGE();

                    game->guy.vel += disp.hat() * 64;

                    game->guy.stun_timer += GUY_STUN_TIME;
                    game->guy.stun_torch_shrink_timer = GUY_STUN_TORCH_SHRINK_TIME;

                    for (int i = 0; i < 8; i += 1) {
                        auto p = make_fire_particle(game, t, 1.5, game->guy.pos, 0, 0, 64, 0);
                        auto off = game->guy.torch_offset();

                        p->pos.x += off.x;
                        p->z += off.y;
                        p->vel_z = 12;
                        p->acc = -(p->vel / 2) / p->lifespan;

                        p->acc_z =
                            -((p->vel_z / p->lifespan) + (p->z / (p->lifespan * p->lifespan)));
                    }
                    for (int i = 0; i < 20; i += 1) {
                        auto hitbox = GUY_HITBOX;
                        auto k = 0.5f;
                        hitbox.pos -= hitbox.size * k / 2;
                        hitbox.size += hitbox.size * k;
                        auto p = make_impact_particle(game, hitbox, t, game->guy.pos, 32, game->guy.vel, 0.05f, ATTACK_IMPACT_PARTICLE_COLOR);
                        p->color.a = 255;
                    }

                    game->camera.shake(5);

                    play_sound(game, game->sfx_slam, 0.5, enemy->pos);
                }

                enemy->attack_cooldown_timer += enemy->attack_windup_timer;
                enemy->attack_cooldown_timer = enemy->ATTACK_COOLDOWN_TIME();
            }
            
            // enemy_max_speed *= 0.75f; // return;
        }

#if 1
        if (should_aggro(game, enemy) && !enemy->aggroed && !dev_notarget) {
            play_sound(game, game->sfx_aggro(enemy), game->sfx_volume(enemy) * 0.2, enemy->pos, enemy->vocal_channel(game));
            enemy->attack_cooldown_timer = enemy->ATTACK_WINDUP_TIME() + enemy->ATTACK_COOLDOWN_TIME();
            enemy->aggroed = true;
        }
#else
        if (enemy->is_swarm) {
            // if (is_lit(enemy->pos, &game->torches) > 1 - enemy->AGGRO_RADIUS()) {
            //     enemy->aggroed = true;
            // }

            if (disp.mag() < enemy->AGGRO_RADIUS() * health_to_light_radius(game->guy.torch)) {
                enemy->aggroed = true;
            }
        } else {
            if (disp.mag() < enemy->AGGRO_RADIUS()) enemy->aggroed = true;
        }
#endif

        if (enemy->aggroed) {
            if (enemy->is_swarm && should_aggro(game, enemy)) {
                // Wake up nearby swarm guys, but only if we're close enough to the player
                // that we ourselves would be aggroed. We don't want to be some guy that
                // just got woken up and is 2 miles from the player and still be able to
                // wake up even more people another 2 miles away.
                For(game->enemies) {
                    if (!it.is_swarm) continue;

                    if (&it == enemy) continue;

                    auto distance = (it.pos - enemy->pos).mag();
                    if (distance < SWARM_WAKE_UP_FRIENDS_RADIUS) {
                        it.aggroed = true;
                    }
                }
            }

            if (disp.mag() < enemy->SWING_DISTANCE()) {
                if (enemy->attack_windup_timer <= 0 && enemy->attack_cooldown_timer <= 0) {
                    // Attack guy
                    enemy->attack_windup_timer = enemy->ATTACK_WINDUP_TIME();

                    play_sound(game, game->sfx_whoosh, 0.3, enemy->pos);
                    play_sound(game, game->sfx_attack(enemy), game->sfx_volume(enemy) * 0.2, enemy->pos, enemy->vocal_channel(game));
                }
            } else {
                auto dir = disp.hat();
                enemy->vel += dir * 1000 * dt;
                locomoted = true;
            }
        }

    }();

    auto mag = enemy->vel.mag();
    if (mag > enemy_max_speed) {
        enemy->vel *= enemy_max_speed / mag;
    }   

    if (!locomoted) {
        enemy->vel *= SDL_pow(0.001, dt);
    }

    ng_assert(game->level);
    auto check_collider = tile_is_collisible;
    if (enemy->is_swarm) {
        check_collider = tile_is_not_dashable;
    }
    collide_like_total_magic(game->level, enemy->pos, enemy->vel, dt, enemy->HITBOX(),
                             *check_collider);

    if (dev_notarget) {
        enemy->aggroed = false;
    }

    // returns just leave this func
    [&] {

        if (stun_should_override_death) { // @Hack
            enemy->switch_to_animation(game->tex_stun(enemy));
        } else if (enemy->death_timer > 0) {
            if (enemy->texture.tex != dummy_texture().tex) {
                enemy->switch_to_animation(game->tex_death(enemy));
            }
            enemy->switch_if_about_to_finish(dt, dummy_texture());
            return;
        }

        if (enemy->stun_timer > 0) {
            enemy->switch_to_animation(game->tex_stun(enemy));
            return;
        }

        if (enemy->aggroed) enemy->texture.flip = (game->guy.pos.x < enemy->pos.x);

        if (enemy->attack_windup_timer > 0) {
            enemy->switch_to_animation(game->tex_attack(enemy));
            return;
        }
        if (enemy->attack_cooldown_timer > 0) {
            if (!enemy->is_swarm) {
                if (enemy->texture.tex == game->tex_attack(enemy).tex &&
                    !enemy->switch_if_about_to_finish(dt, game->tex_idle(enemy))) {
                    return;
                }
            } else {
                if (enemy->texture.tex == game->tex_attack(enemy).tex &&
                    !enemy->switch_if_about_to_finish(dt, game->tex_run(enemy))) {
                    return;
                }
            }
        }

        if (game->guy.is_dead) {
            enemy->switch_to_animation(game->tex_idle(enemy));
            return;
        }

        if (enemy->vel.mag() > enemy->SPEED() * 0.25f) {
            enemy->switch_to_animation(game->tex_run(enemy));

            enemy->sfx_walk_timer -= dt;
            if (enemy->sfx_walk_timer <= 0) {
                enum { N = 7 };

                Mix_Chunk *(*which_array)[N] = enemy->is_swarm ? &game->swarm_sfx_run : &game->goblin_sfx_run;
                u16 *which_picker = enemy->is_swarm ? &game->swarm_sfx_run_picker : &game->goblin_sfx_run_picker;
                play_sound(game, (*which_array)[equidistribute(*which_picker, N)], game->sfx_volume(enemy) * 0.25, enemy->pos);
                *which_picker += 1;

                enemy->sfx_walk_timer += GUY_SFX_WALK_TIME; // @Lazy just use guy's values.
            }
        } else {
            
            enemy->sfx_walk_timer = GUY_SFX_WALK_BEGIN_TIME; // @Lazy just use guy's values.

            if (!enemy->is_swarm) {
                enemy->switch_to_animation(game->tex_idle(enemy));
            } else {
                if (enemy->aggroed) {
                    enemy->switch_to_animation(game->tex_run(enemy));
                } else {
                    enemy->switch_to_animation(game->tex_idle(enemy));
                }
            }
        }
    }();
}

void game_update(Game *game, Input *input, f64 dt, f64 t) {
    Use game, Use input, Use dt, Use t;

    // game->goblin_spawn_timer -= dt;
    // if (game->goblin_spawn_timer <= 0) {
    //     game->goblin_spawn_timer += GOBLIN_SPAWN_TIME;
    //
    // }

    camera_update(&game->camera, dt, game);
    // if (!game->guy.is_dead) //
    { guy_update(&game->guy, dt, t, input, game); }
    For(game->enemies) enemy_update(&it, dt, t, game);

    For(game->torches) {
        if (it.light_level <= 0) {
            it.switch_to_animation(game->tex_torch_snuffed);
        }
    }

    if (dev_god) {
        game->guy.torch = dev_god_torch_amt;
        game->guy.is_dead = 0;
    }
}

#define DEFAULT_SCREEN_SIZE v2(1280, 720)

float get_good_window_scale(float w, float h) {
    int ratio_w = w / (DEFAULT_SCREEN_SIZE.x + 50);
    int ratio_h = h / (DEFAULT_SCREEN_SIZE.y + 50);
    int result = ng::min(ratio_w, ratio_h);
    result = ng::max(result, 1);
    return cast(float) result;
}


/* #define ASSETS(X,IGNORE) \
X(ambient,ogg) \
X(credits,ogg) \
X(player_walk1,ogg) \
X(player_walk2,ogg) \
X(player_walk3,ogg) \
X(player_walk4,ogg) \
X(player_walk5,ogg) \
X(player_walk6,ogg) \
X(player_walk7,ogg) \
X(player_walk8,ogg) \
X(player_walk9,ogg) \
X(dash,ogg) \
X(whoosh,ogg) \
X(hit,ogg) \
X(fire,ogg) \
X(sword_clang,ogg) \
X(sword_slice,ogg) \
X(goblin_walk1,ogg) \
X(goblin_walk2,ogg) \
X(goblin_walk3,ogg) \
X(goblin_walk4,ogg) \
X(goblin_walk5,ogg) \
X(goblin_walk6,ogg) \
X(goblin_walk7,ogg) \
X(swarm_walk1,ogg) \
X(swarm_walk2,ogg) \
X(swarm_walk3,ogg) \
X(swarm_walk4,ogg) \
X(swarm_walk5,ogg) \
X(swarm_walk6,ogg) \
X(swarm_walk7,ogg) \
X(goblin_aggro,ogg) \
X(goblin_attack,ogg) \
X(goblin_death,ogg) \
X(goblin_fire_tick,ogg) \
X(goblin_hurt,ogg) \
X(goblin_stunned,ogg) \
X(swarm_aggro,ogg) \
X(swarm_attack,ogg) \
X(swarm_death,ogg) \
X(swarm_fire_tick,ogg) \
X(swarm_hurt,ogg) \
X(swarm_stunned,ogg) \
X(light1,png) \
X(walltorch,png) \
INGORE(torch_snuffed,png) \
X(goblin_idle,png) \
X(goblin_run,png) \
X(goblin_stun,png) \
X(goblin_death,png) \
X(goblin_attack,png) \
X(swarm_idle,png) \
X(swarm_fly,png) \
X(swarm_stun,png) \
X(swarm_death,png) \
X(swarm_attack,png) \
X(walls,png) \
X(player_idle,png) \
X(player_run,png) \
X(player_hurt,png) \
X(player_run_up,png) \
X(player_attack,png) \
X(player_attack_up,png) \
X(player_attack_down,png) \
X(player_dash,png) \
X(particle,png) \
X(particle_fire,png) \
X(cursor,png) \
X(icon_wasd,png) \
X(icon_lmb,png) \
X(icon_rmb,png) \
X(player_shadow,png) \
X(title_text,png) \
X(credits,png) \
*/

static SDL_RWops *metagenerated_file;
static SDL_RWops *export_asset(SDL_RWops *file, const char *name) {
    // if (!file) {
    //     return file;
    // }
    auto filesize = SDL_RWsize(file);
    ng_assert(filesize >= 0 && filesize <= INT_MAX);

    char name_buffer[1024] = {0};
    SDL_snprintf(name_buffer, sizeof(name_buffer), "const uint8_t %s[] = {", name);
    SDL_RWwrite(metagenerated_file, name_buffer, SDL_strlen(name_buffer), 1);
    Uint8 byte = 0;
    char byte_out_buffer[5] = {0};
    for (int i = 0; i < filesize; i += 1) {
        SDL_RWread(file, &byte, 1, 1);
        SDL_snprintf(byte_out_buffer, sizeof(byte_out_buffer), "%3d,", byte);
        SDL_RWwrite(metagenerated_file, byte_out_buffer, 4, 1);
    }
    SDL_RWwrite(metagenerated_file, "};\n", 3, 1);
    SDL_RWseek(file, 0, RW_SEEK_SET);

    return file;
}

#ifdef METAGENERATED
#include "metagenerated.h"
#define ASSET(name, ext) (void *)name##_##ext, sizeof(name##_##ext)
#else
#define ASSET_(dir, name, ext) SDL_RWFromFile(dir #name "." #ext, "r")
#ifdef METAGEN
#define ASSET(name, ext) export_asset(ASSET_("bin/data/", name, ext), #name "_" #ext)
#else
#define ASSET(name, ext) ASSET_("data/", name, ext)
#endif
#endif

SDL_Texture *load_sdl_texture(SDL_Renderer *renderer,
#ifdef METAGENERATED
                              void *file, int filesize
#else
                              SDL_RWops *rw
#endif
) {
#ifndef METAGENERATED
    ng_assert(rw);
    defer { SDL_RWclose(rw); };

    auto filesize = SDL_RWsize(rw);
    ng_assert(filesize >= 0 && filesize <= INT_MAX);

    auto file = SDL_malloc(filesize);
    defer { SDL_free(file); };

    auto read_result = SDL_RWread(rw, file, 1, filesize);
    ng_assert(read_result <= INT_MAX);
    ng_assert(read_result == cast(u64) filesize);
#endif

    int w = 0;
    int h = 0;
    int n = 0;
    auto image = stbi_load_from_memory(cast(stbi_uc *) file, filesize, &w, &h, &n, 4);
    ng_assert(image);
    defer { stbi_image_free(image); };

    auto surf = SDL_CreateRGBSurfaceFrom(image, w, h, 32, w * 4,
                                         0xff,
                                         0xff00,
                                         0xff0000,
                                         0xff000000);
    ng_assert(surf);
    defer { SDL_FreeSurface(surf); };

    auto tex = SDL_CreateTextureFromSurface(renderer, surf);
    ng_assert(tex);
    return tex;
};

Vector2 default_offset(const Texture &texture) { return v2(-texture.frame_w / 2, texture.h); };

Texture load_texture(Game *game, SDL_Renderer *renderer,
#ifdef METAGENERATED
                     void *file, int filesize,
#else
                     SDL_RWops *rw,
#endif
                                 int frame_w = -1,
                     int fps = 0) {
    Texture result = {};
    result.tex = load_sdl_texture(renderer,
#ifdef METAGENERATED
                                  file, filesize
#else
                                  rw
#endif
    );
    auto query_result = SDL_QueryTexture(result.tex, null, null, &result.w, &result.h);
    ng_assert(!query_result);
    if (frame_w < 0) {
        result.frame_w = result.w;
    } else {
        result.frame_w = frame_w;
    }
    result.fps = fps;
    game->all_textures.push(result.tex);
    SDL_SetTextureBlendMode(result.tex, SDL_BLENDMODE_BLEND);
    result.offset = default_offset(result);
    return result;
}

Mix_Music *load_music(
#ifdef METAGENERATED
    void *file, int filesize
#else
    SDL_RWops *rw
#endif
) {
    auto result = Mix_LoadMUS_RW(
#ifdef METAGENERATED
        SDL_RWFromConstMem(file, filesize),
#else
        rw,
#endif
        1);
    // ng_assert(result);
    return result;
}

Mix_Chunk *load_sound(
#ifdef METAGENERATED
    void *file, int filesize
#else
    SDL_RWops *rw
#endif
) {
    auto result = Mix_LoadWAV_RW(
#ifdef METAGENERATED
        SDL_RWFromConstMem(file, filesize),
#else
        rw,
#endif
        1);
    // ng_assert(result);
    return result;
}

template <class T, class Func> void quick_sort(ng::array<T> *arr, Func comparator) {
    quick_sort(arr, 0, arr->count, comparator);
}

template <class T, class Func>
void quick_sort(ng::array<T> *arr, int begin, int end, Func comparator) {
    if (begin < end) {
        int r = partition(arr, begin, end, comparator);
        quick_sort(arr, begin, r, comparator);
        quick_sort(arr, r + 1, end, comparator);
    }
}

template <class T, class Func>
int partition(ng::array<T> *arr, int begin, int end, Func comparator) {
    auto &&x = (*arr)[begin];
    int i = begin;

    for (int j = begin + 1; j < end; j++) {
        if (comparator((*arr)[j], x)) {
            i = i + 1;
            ng::swap((*arr)[i], (*arr)[j]);
        }
    }

    ng::swap((*arr)[i], (*arr)[begin]);
    return i;
}

const int level_1_tiles[] = {
#include "level_1.csv"
};

const int level_1_entities[] = {
#include "level_1_ents.csv"
};

Level tiles_to_level(const int level_tiles[], int w, int h, Texture *walls_atlas) {
    Level result = {};
    result.tiles = (int *)SDL_malloc(w * h * sizeof(level_tiles));
    result.w = 50;
    result.h = 50;
    int num_columns = walls_atlas->w / walls_atlas->frame_w;

    for (int i = 0; i < result.w * result.h; i += 1) {

        result.tiles[i] = level_tiles[i] % num_columns;
        if (result.tiles[i] < 0) result.tiles[i] += num_columns;
    }
    return result;
}
void destroy_level(Level *level) { SDL_free(level->tiles); }

using ng::operator""_s;

enum { SAMPLE_RATE = 44100 };
#if 0
// #define SAMPLE_RATE 44100
#if 0
enum { DELAY_MS = 500 };
enum { DELAY_SAMPLES = (int)((float)DELAY_MS * ((float)SAMPLE_RATE / 1000)) };
#else
enum { DELAY_SAMPLES = 32768 };
#endif

enum { NUM_SAMPLES = DELAY_SAMPLES };
Sint16 samples_l[NUM_SAMPLES];
Sint16 samples_r[NUM_SAMPLES];
SDL_Point sample_points[NUM_SAMPLES * 2]; // For visualizing
u64 sample_cursor = 0;
#endif

SDL_Point viz_samples[SAMPLE_RATE];

void mix_func(void *udata, Uint8 *stream, int len) {
    ng_assert(len > 0);
    ng_assert(len % 4 == 0); // 2 bytes per sample, 2 channels (L and R)

    Sint16 *buffer = (Sint16 *)stream;
    len /= 2;

#define CombFilter(NUM_SAMPLES, gain, stereo)                                                                                                \
    {                                                                                                                                        \
        enum { nsamples = (NUM_SAMPLES) };                                                                                                   \
        static u64 cursor;                                                                                                                   \
        static Sint16 delay_line_l[nsamples];                                                                                                \
        static Sint16 delay_line_r[nsamples];                                                                                                \
        float gain_ = (gain);                                                                                                                \
        enum { stereo_ = (stereo) };                                                                                                         \
        enum { idx_offset_l_base = ng_abs(ng_min(stereo_, 0)) - nsamples };                                                                  \
        enum { idx_offset_r_base = ng_abs(ng_max(stereo_, 0)) - nsamples };                                                                  \
        uint64_t idx_offset_l = cursor + idx_offset_l_base;                                                                                  \
        uint64_t idx_offset_r = cursor + idx_offset_r_base;                                                                                  \
        for (int i = 0; i < len / 2; i += 1) {                                                                                               \
            buffer[i * 2 + 0] += (delay_line_l[(i + idx_offset_l) % nsamples] + 0.5f * delay_line_r[(i + idx_offset_l) % nsamples]) * gain_; \
            buffer[i * 2 + 1] += (delay_line_r[(i + idx_offset_r) % nsamples] + 0.5f * delay_line_l[(i + idx_offset_r) % nsamples]) * gain_; \
        }                                                                                                                                    \
        for (int i = 0; i < len / 2; i += 1) {                                                                                               \
            delay_line_l[(cursor + i) % nsamples] = buffer[i * 2];                                                                           \
            delay_line_r[(cursor + i) % nsamples] = buffer[i * 2 + 1];                                                                       \
        }                                                                                                                                    \
        cursor += len / 2;                                                                                                                   \
    }
#if 1//def NDEBUG
#define PRINTER(type, name, v) static const type name = (v);
#else
#define PRINTER(type, name, v)                             \
    static type name = (v);                                \
    {                                                      \
        static float tweak_looper;                         \
        static bool initted;                               \
        static double invfreq;                             \
        static uint64_t last;                              \
        if (!initted) {                                    \
            initted = true;                                \
            invfreq = 1.0 / SDL_GetPerformanceFrequency(); \
            last = SDL_GetPerformanceCounter();            \
        }                                                  \
        uint64_t next = SDL_GetPerformanceCounter();       \
        tweak_looper += (next - last) * invfreq;           \
        last = next;                                       \
        if (tweak_looper >= 1) {                           \
            ng::print(#name " = %\n", (name));             \
            tweak_looper -= (uint64_t)tweak_looper;        \
        }                                                  \
    }
#endif
    const float BASE = 0.4f;
    if (!dev_noreverb) {
        enum { N = SAMPLE_RATE / 16 };
        CombFilter(2 * N, BASE / (1 << 2), 1 << 6);
        CombFilter(3 * N, BASE / (1 << 3), -1 << 6);
        CombFilter(5 * N, BASE / (1 << 4), 1 << 6);
        CombFilter(7 * N, BASE / (1 << 5), -1 << 6);
        CombFilter(11 * N, BASE / (1 << 6), 1 << 6);
    }
}
int main(int argc, char *argv[]) {
#ifndef NDEBUG
    if (argc > 1) {
        if (ng::string{argv[1]} == "dev"_s) {
            dev = true;
        }
    }
#endif

    ng::print_init();

    Game game_ = {};
    // initialize everything
    auto game = &game_;
    defer { game_destroy(game); };

    auto real_screen_size = game->screen_size; // @Hack.

    auto update_screen_size = [&](Game *game, Vector2 new_size) {
        Use game;
        game->screen_size = DEFAULT_SCREEN_SIZE;
        real_screen_size = new_size;
    };
    update_screen_size(game, DEFAULT_SCREEN_SIZE);

    auto update_cam_scale = [](Game *game) {
        game->camera.scale = game->screen_size.mag() / (DEFAULT_SCREEN_SIZE.mag());
    };
    update_cam_scale(game);

    auto result = SDL_Init(SDL_INIT_EVERYTHING);
    if (result) return 1;
    defer { SDL_Quit(); };

#ifdef METAGEN
    metagenerated_file = SDL_RWFromFile("src/metagenerated.h", "w");
    ng_assert(metagenerated_file);
    defer { SDL_RWclose(metagenerated_file); };
#endif
    // ng_assert(IMG_Init(IMG_INIT_PNG));

    bool no_audio = false;

    Mix_Init(MIX_INIT_OGG);
    if (Mix_OpenAudio(SAMPLE_RATE, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) != 0) {
        // audio failed.
        // Just ignore it; user gets no sound.
        no_audio = true;
    }

    if (!no_audio) {
        int num_chans = 128;
        if (Mix_AllocateChannels(num_chans) != num_chans) {
            no_audio = true; // Couldn't handle 128 channels? Your stuff sucks bro.
        }
        Mix_ReserveChannels(NUM_ENEMY_VOCAL_CHANNELS);
        Mix_SetPostMix(&mix_func, null);
    }

    auto window =
        SDL_CreateWindow("Light After Dark v2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         DEFAULT_SCREEN_SIZE.x, DEFAULT_SCREEN_SIZE.y, SDL_WINDOW_HIDDEN);
    if (!window) return 1;

    auto renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) return 1;

    bool fullscreen = false;
    auto set_fullscreen = [&](bool fullscreen) {
        // Use fullscreen;
        auto idx = SDL_GetWindowDisplayIndex(window);

        SDL_Rect bounds = {};
        SDL_GetDisplayBounds(idx, &bounds);

        SDL_SetWindowResizable(window, cast(SDL_bool) !fullscreen);
        SDL_SetWindowBordered(window, cast(SDL_bool) !fullscreen);
        SDL_DisplayMode display = {};
        if (fullscreen) {
            if (!SDL_GetCurrentDisplayMode(idx, &display) && display.w && display.h) {
                SDL_SetWindowSize(window, display.w, display.h);
            }                                                                      
            // update_screen_size(game, v2(display.w, display.h));
            real_screen_size = v2(display.w, display.h);
        } else {
            display.w = DEFAULT_SCREEN_SIZE.x;
            display.h = DEFAULT_SCREEN_SIZE.y;
            SDL_SetWindowSize(window, display.w, display.h);
                                       
            // update_screen_size(game, DEFAULT_SCREEN_SIZE);
            real_screen_size = DEFAULT_SCREEN_SIZE;
        }
        SDL_RenderSetLogicalSize(renderer, DEFAULT_SCREEN_SIZE.x, DEFAULT_SCREEN_SIZE.y);
        SDL_SetWindowPosition(window, bounds.x + bounds.w / 2 - display.w / 2, bounds.y + bounds.h / 2 - display.h / 2);
    };
    set_fullscreen(fullscreen);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

#ifndef METAGEN
    SDL_ShowWindow(window);
#endif

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);          
    SDL_RenderPresent(renderer);


    // @Todo: Re-create this texture whenever resolution changes!!!
    SDL_Texture *dynamic_lighting_render_target = null;
    {
        int w;
        int h;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        ng_assert(w > 0 && h > 0);
        dynamic_lighting_render_target =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    }
    ng_assert(dynamic_lighting_render_target);
    defer { SDL_DestroyTexture(dynamic_lighting_render_target); };

    auto light_spot = load_sdl_texture(renderer, ASSET(light1,png));
    defer { SDL_DestroyTexture(light_spot); };
    SDL_SetTextureBlendMode(light_spot, SDL_BLENDMODE_ADD);

    defer {
        ng_for(game->all_textures) {
            SDL_DestroyTexture(it);
        }
    };

    game->tex_torch_active = load_texture(game, renderer, ASSET(walltorch,png), 16, 12);
    game->tex_torch_snuffed = load_texture(game, renderer, ASSET(torch_snuffed,png));
    game->goblin_tex_idle = load_texture(game, renderer, ASSET(goblin_idle,png), 32, 8);
    game->goblin_tex_run = load_texture(game, renderer, ASSET(goblin_run,png), 32, 10);
    game->goblin_tex_stun = load_texture(game, renderer, ASSET(goblin_stun,png), 32, 18);
    game->goblin_tex_death = load_texture(game, renderer, ASSET(goblin_death,png), 32, 8 / GOBLIN_DEATH_TIME);
    game->goblin_tex_attack = load_texture(game, renderer, ASSET(goblin_attack,png), 40, 18);
    game->goblin_tex_attack.offset.x += 4;
    game->goblin_tex_attack.offset.y -= 4;

    game->swarm_tex_idle = load_texture(game, renderer, ASSET(swarm_idle,png), 32, 12);
    game->swarm_tex_run = load_texture(game, renderer, ASSET(swarm_fly,png), 32, 14);
    game->swarm_tex_stun = load_texture(game, renderer, ASSET(swarm_stun,png), 32, 18);
    game->swarm_tex_death = load_texture(game, renderer, ASSET(swarm_death,png), 32, 8 / SWARM_DEATH_TIME);
    game->swarm_tex_attack = load_texture(game, renderer, ASSET(swarm_attack,png), 32, 24);
    // game->swarm_tex_attack.offset.x += 4;
    // game->swarm_tex_attack.offset.y -= 4;

    auto walls_atlas = load_texture(game, renderer, ASSET(walls,png), 16);

    game->tex_guy_idle = load_texture(game, renderer, ASSET(player_idle,png), 32, 10);

    game->tex_guy_run = load_texture(game, renderer, ASSET(player_run,png), 32, 12);

    game->tex_guy_stun = load_texture(game, renderer, ASSET(player_hurt,png), 32, 18);

    game->tex_guy_run_up = load_texture(game, renderer, ASSET(player_run_up,png), 32, 12);

    game->tex_guy_attack = load_texture(game, renderer, ASSET(player_attack,png), 48, 13);
    game->tex_guy_attack.offset.x += 8;
    game->tex_guy_attack.offset.y -= 8;

    game->tex_guy_attack_up = load_texture(game, renderer, ASSET(player_attack_up,png), 56, 13);

    game->tex_guy_attack_down = load_texture(game, renderer, ASSET(player_attack_down,png), 56, 13);
    game->tex_guy_attack_down.offset.y -= 20;

    game->tex_guy_dash = load_texture(game, renderer, ASSET(player_dash,png));

    game->tex_particle = load_texture(game, renderer, ASSET(particle,png));
    game->tex_particle_fire = load_texture(game, renderer, ASSET(particle_fire,png), 8, 8);

    game->tex_cursor = load_texture(game, renderer, ASSET(cursor,png));

    game->tex_icon_wasd = load_texture(game, renderer, ASSET(icon_wasd,png));
    game->tex_icon_lmb = load_texture(game, renderer, ASSET(icon_lmb,png));
    game->tex_icon_rmb = load_texture(game, renderer, ASSET(icon_rmb,png));

    game->tex_shadow = load_texture(game, renderer, ASSET(player_shadow,png));

    //
    //
    //
    //
    //
    //
    //
    //

    if (!no_audio) { // Kinda hacky.
        game->mus_ambient = load_music(ASSET(ambient,ogg));
        game->mus_credits = load_music(ASSET(credits,ogg));

#define load_sfx_guy_run(n) game->sfx_guy_run[n - 1] = load_sound(ASSET(player_walk##n,ogg));
        load_sfx_guy_run(1);
        load_sfx_guy_run(2);
        load_sfx_guy_run(3);
        load_sfx_guy_run(4);
        load_sfx_guy_run(5);
        load_sfx_guy_run(6);
        load_sfx_guy_run(7);
        load_sfx_guy_run(8);
        load_sfx_guy_run(9);
        game->sfx_guy_dash = load_sound(ASSET(dash,ogg));
        game->sfx_whoosh = load_sound(ASSET(whoosh,ogg));
        game->sfx_slam = load_sound(ASSET(hit,ogg));
        game->sfx_fire = load_sound(ASSET(fire,ogg));
        game->sfx_sword_clang = load_sound(ASSET(sword_clang,ogg));
        game->sfx_sword_slice = load_sound(ASSET(sword_slice,ogg));

#define load_sfx_enemy_run(n) { game->goblin_sfx_run[n - 1] = load_sound(ASSET(goblin_walk##n,ogg)); \
                                game->swarm_sfx_run[n - 1] = load_sound(ASSET(swarm_walk##n,ogg)); }
        load_sfx_enemy_run(1);
        load_sfx_enemy_run(2);
        load_sfx_enemy_run(3);
        load_sfx_enemy_run(4);
        load_sfx_enemy_run(5);
        load_sfx_enemy_run(6);
        load_sfx_enemy_run(7);
        game->goblin_sfx_aggro = load_sound(ASSET(goblin_aggro,ogg));
        game->goblin_sfx_attack = load_sound(ASSET(goblin_attack,ogg));
        game->goblin_sfx_death = load_sound(ASSET(goblin_death,ogg));
        game->goblin_sfx_fire_tick = load_sound(ASSET(goblin_fire_tick,ogg));
        game->goblin_sfx_hurt = load_sound(ASSET(goblin_hurt,ogg));
        game->goblin_sfx_stunned = load_sound(ASSET(goblin_stunned,ogg));
        
        game->swarm_sfx_aggro = load_sound(ASSET(swarm_aggro,ogg));
        game->swarm_sfx_attack = load_sound(ASSET(swarm_attack,ogg));
        game->swarm_sfx_death = load_sound(ASSET(swarm_death,ogg));
        game->swarm_sfx_fire_tick = load_sound(ASSET(swarm_fire_tick,ogg));
        game->swarm_sfx_hurt = load_sound(ASSET(swarm_hurt,ogg));
        game->swarm_sfx_stunned = load_sound(ASSET(swarm_stunned,ogg));
                                               
        Mix_FadeInMusic(game->mus_ambient, -1, AMBIENT_FADE_MS);
        Mix_VolumeMusic(MIX_MAX_VOLUME);
        Mix_ResumeMusic();
    }

    auto title_text = load_texture(game, renderer, ASSET(title_text,png));
    auto credits_text = load_texture(game, renderer, ASSET(credits,png));

#define FADE_OUT_TIME 2
#define FADE_IN_TIME 4

    float screen_fader = 0;
    // if (dev) screen_fader = FADE_IN_TIME;
    auto screen_fader_alpha = [&] {
        auto x = screen_fader;
        // x *= 0.7;
        // x -= 0.8;
        // x = (x / (1 + SDL_fabs(x)));
        // x *= 2.6;
        // x += 1;
        return x;
    };
    bool fade_out = false;

    float credits_timer = 0;
    auto credits_scroll_pos = [&] {
        return (1 - SDL_pow(0.9, credits_timer)) * credits_timer;
    };

    auto level_1 = tiles_to_level(level_1_tiles, 50, 50, &walls_atlas);
    defer { destroy_level(&level_1); };

    game->level = &level_1;

    // int x = 0;
    // for (int i = 0; i < level_1.w * level_1.h; i += 1) {
    //     if (x++ % 8 != 0) continue;
    //     auto &it = level_1.tiles[i];
    //     if (it != 0) continue;
    //     if (i > level_1.w && level_1.tiles[i - level_1.w] == 0) continue;
    //
    //     auto x = (i % level_1.w);
    //     auto y = -(i / level_1.w - 1) + level_1.w;
    //
    //     add_floor_torch(x, y);
    // }

    auto add_goblin = [&](int x, int y) {
        auto goblin = game->enemies.push();
        goblin->pos = v2(x, y) * 16;
        goblin->health = goblin->HEALTH();
        goblin->texture = game->tex_idle(goblin);
        goblin->z = 0;
        // for (int i = 0; i < 2; i += 1) {
        //     game->enemies.push(*goblin)->pos.x += i;
        // }
        return goblin;
    };
    auto add_swarm = [&](int x, int y) {
        auto swarm = game->enemies.push();
        swarm->is_swarm = true;
        swarm->pos = v2(x, y) * 16;
        swarm->health = swarm->HEALTH();
        swarm->texture = game->tex_idle(swarm);
        swarm->z = 16;
        return swarm;
    };
    auto add_floor_torch = [&](int x, int y) {

        auto torch = game->torches.push();
        torch->switch_to_animation(game->tex_torch_active);
        auto rand_offset = rand_range(&game->rng_fx, 0.1, 0.9);
        torch->animation_timer += rand_offset;
        torch->oscillation_offset = rand_offset;
        // torch->radius = health_to_light_radius;

        torch->pos = v2(x, y) * 16;
        torch->pos.y += 7;
        // torch->light_level = rand_range(&game->rng, -0.1, 1.0);
        torch->light_level = 1.0f;
        torch->z = 8;

        return torch;
    };
    auto add_wall_torch = [&](int x, int y) {
        auto torch = add_floor_torch(x, y);
        torch->z = 8;
        return torch;
    };
    // auto add_sidewall_torch = [&](int x, int y) {
    //     auto torch = add_wall_torch(x, y);
    //     torch->switch_to_animation(game->tex_torch_side_active);
    //     torch->animation_timer += rand_range(&game->rng_fx, 0.1, 0.9);
    //     return torch;
    // };
    auto level_init = [&] {
        const auto w = level_1.w;
        const auto h = level_1.h;
        for (int j = 0; j < h; j += 1) {

            for (int i = 0; i < w; i += 1) {

                auto idx = i + (j * w);

                auto val = level_1_entities[idx];

                auto x = i;
                auto y = -(j - 1) + h;

                if (val == -1) {
                } else if (val == 1) {
                    auto gob = add_goblin(x, y);
                } else if (val == 2) {
                    auto torch = add_floor_torch(x, y);
                } else if (val == 3) {
                    auto swarm = add_swarm(x, y);
                } else if (val == 4) {
                    auto gob = add_goblin(x, y);
                    gob->texture.flip = true;
                } 
            }
        }
                        
        // add_goblin(25, 6);
    };
    level_init();

    auto guy_init = [&] {
        game->guy = {};
        // game->guy.torch = 1;
        game->guy.torch = GUY_STARTING_LIGHT;
        game->guy.pos = v2(3 * 16 + 8, 6 * 16);
    };
    guy_init();

    game->camera.scale = 4;

    camera_update(&game->camera, 1000, game);

    game->render_textures.reserve(1024);

    bool paused = false;

    auto restart = [&] {

        // Mix_HaltChannel(-1);
        Mix_SetPostMix(&mix_func, game);

        game_clear(game);
        level_init();
        guy_init();
        camera_update(&game->camera, 1000, game);

        fade_out = false;

        screen_fader = 0;
        credits_timer = 0;
    };

    f64 t = 0;

#ifdef METAGEN
    return 0;
#else            

#define PORTFOLIO 
#ifdef PORTFOLIO        
    {
        fade_out = false;
        screen_fader = 1000;
        Input input = {};

        game->guy.dash_timer = 5;
        game->guy.is_dashing = true;
        game->guy.pos = v2(30, 30) * 16;
        game->guy.dash_velocity = v2(1, 0.25).hat() * DASH_SPEED;
        game->guy.animation_timer = 0.7;

        auto &en = game->enemies[0];
        en.is_swarm = 0;
        en.pos = v2(41, 32) * 16 + v2(-13, -8);
        en.aggroed = 1;
        en.texture.flip = 1;

        {
            auto p = game->particles.push();
            p->texture = game->tex_particle;
            p->lifespan = 100;
            p->z = -100;
            p->pos = game->guy.pos + v2(10 - 4, 50 + 64 + 8);
            p->size = 12;
            p->color = SDL_Color{255, 255, 0, 96};
        }

        camera_update(&game->camera, 1000, game);
        const float DT = 0.001;
        for (float f = 0.5; f >= 0; f -= DT) {
            game_update(game, &input, DT, t += DT);
            en.attack_windup_timer = GOBLIN_ATTACK_WINDUP_TIME / 2;
            en.fire_timer = 1;
            en.animation_timer = 0.7;
        }
    }
    paused = true;
#endif // PORTFOLIO
    restart_frame:;   
    //
    // timing stuff
    //

    //


    //
    // handle input
    //

    Input input = {};

    auto add = [](bool &a, bool b) { a = a || b; };

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const auto t = event.type;
        if (t == SDL_QUIT) {
            game->quit = true;
            // SDL_Quit();
            // ng::exit(0);
        } else if (t == SDL_WINDOWEVENT) {
            const auto t = event.window.event;
            if (t == SDL_WINDOWEVENT_SIZE_CHANGED) {
                update_screen_size(game, v2(event.window.data1, event.window.data2));
                // update_cam_scale(game);
                goto restart_frame;
            }
        } else if (t == SDL_MOUSEBUTTONDOWN) {
            auto but = event.button.button;
            if (but == SDL_BUTTON_LEFT) {
                input.lmb_new = true;
            } else if (but == SDL_BUTTON_RIGHT) {
                input.rmb_new = true;
            }
        } else if (t == SDL_MOUSEWHEEL) {
            if (0) {
                if (event.wheel.y > 0) {
                    game->camera.scale *= 1 + 0.05 * event.wheel.y;
                } else {
                    game->camera.scale /= 1 + 0.05 * -event.wheel.y;
                }
            }
        } else if (t == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_RETURN && event.key.keysym.mod & KMOD_ALT) {
                fullscreen = !fullscreen;
                set_fullscreen(fullscreen);
                goto restart_frame;
            }
            if (!event.key.repeat) {
                add(input.space_new, event.key.keysym.sym == SDLK_SPACE);
                add(input.shift_new,
                    event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT);
            }
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                // if (game->guy.is_dead) {
                // } else {
                paused = !paused;
                // }
            }

#ifndef NDEBUG
            if (dev) {
                if (event.key.keysym.sym == SDLK_F1) {
                    dev_drawdebugrects ^= 1;
                }
                if (event.key.keysym.sym == SDLK_F2) {
                    if (dev_slow) {
                        dev_slow = false;
                        dev_fast = true;
                    } else {
                        dev_fast ^= 1;
                    }
                }
                if (event.key.keysym.sym == SDLK_F3) {
                    if (dev_fast) {
                        dev_fast = false;
                        dev_slow = true;
                    } else {
                        dev_slow ^= 1;
                    }
                }
                if (event.key.keysym.sym == SDLK_F4) {
                    dev_noreverb ^= 1;
                }
                if (event.key.keysym.sym == SDLK_F7) {
                    dev_god ^= 1;
                    if (dev_god) {
                        dev_god_torch_amt = game->guy.torch;
                    }
                }
                if (event.key.keysym.sym == SDLK_F8) {
                    dev_notarget ^= 1;
                }
                if (event.key.keysym.sym == SDLK_F9) {
                    dev_noclip ^= 1;
                }
                if (event.key.keysym.sym == SDLK_F10) {
                    dev_nomusic ^= 1;
                }
                if (event.key.keysym.sym == SDLK_F11) {
                    dev_nolights ^= 1;
                }
            }
#endif
        }
    }

    if (paused) {
        Mix_Pause(-1);
        Mix_PauseMusic();
    } else {
        Mix_Resume(-1);
        if (dev_nomusic) {
            Mix_PauseMusic();
        } else {           
            Mix_ResumeMusic();
        }
    }

    SDL_ShowCursor(0);

    {
        int x;
        int y;
        SDL_GetMouseState(&x, &y);
        input.mouse_pos = v2(x * DEFAULT_SCREEN_SIZE.x / real_screen_size.x, y * DEFAULT_SCREEN_SIZE.y / real_screen_size.y);
        // input.mouse_pos = v2(x, y);
    }

    auto keystate = SDL_GetKeyboardState(null);

    add(input.space, keystate[SDL_GetScancodeFromKey(SDLK_SPACE)]);
    add(input.up, keystate[SDL_GetScancodeFromKey(SDLK_UP)]);
    add(input.down, keystate[SDL_GetScancodeFromKey(SDLK_DOWN)]);
    add(input.left, keystate[SDL_GetScancodeFromKey(SDLK_LEFT)]);
    add(input.right, keystate[SDL_GetScancodeFromKey(SDLK_RIGHT)]);

    add(input.up, keystate[SDL_SCANCODE_W]);
    add(input.down, keystate[SDL_SCANCODE_S]);
    add(input.left, keystate[SDL_SCANCODE_A]);
    add(input.right, keystate[SDL_SCANCODE_D]);

    //
    // update
    //

    SDL_SetRenderTarget(renderer, null);
    // SDL_SetRenderDrawColor(renderer, 0x88, 0x88, 0x88, 255);
    SDL_SetRenderDrawColor(renderer, 6, 8, 14, 255);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderClear(renderer);

    SDL_SetRenderTarget(renderer, dynamic_lighting_render_target);
    // SDL_SetRenderDrawColor(renderer, 6, 8, 14, 255);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    if (dev_nolights) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_RenderClear(renderer);

    // SDL_SetRenderTarget(renderer, null);
    // SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0xff, 255);
    // auto sdl_rect = rect_to_sdl(rect(0, 0, game->screen_size.x, game->screen_size.y));
    // SDL_RenderFillRect(renderer, &sdl_rect);

    f64 dt = 1.0 / 60;
    /**/ if (paused) dt = 0;
    else if (dev_fast) dt = 1.0 / 10;
    else if (dev_slow) dt = 1.0 / 480;
    t += dt;

    if (game->guy.is_dead) {
        fade_out = true;
    }
    
    if (!game->enemies.count) {  
        fade_out = true;
        if (screen_fader <= 0 && !game->guy.is_dead) {
            if (credits_timer <= 0) { // @Hack: use some real state for this, the whole credits/screenfade thing is becoming a massive pile of spaghetti code
                Mix_VolumeMusic(MIX_MAX_VOLUME);
                Mix_FadeInMusic(game->mus_credits, 0, 100);
                Mix_SetPostMix(null, null);
            }
            credits_timer += dt;
        } else {
            Mix_FadeOutChannel(-1, screen_fader * 1000); // @Inexplicable this actually works properly lol
            Mix_VolumeMusic(screen_fader * screen_fader * MIX_MAX_VOLUME);
        }
    }

    if (fade_out) {
        screen_fader -= dt / FADE_OUT_TIME;
        if (game->guy.is_dead) { // Faded out due to death.
            if (screen_fader <= 0) {
                restart();
            }
        } else { // Beat the game, time for credits.
            if (screen_fader > 0) {
                
                // Mix_Volume(-1, screen_fader * MIX_MAX_VOLUME);
                // Mix_VolumeMusic(screen_fader * MIX_MAX_VOLUME);
            } else {
                // auto vol = ng_clamp(credits_timer / 0.25f, 0, 1);
                // Mix_VolumeMusic(vol * MIX_MAX_VOLUME);
            }
        }
    } else {
        screen_fader += dt / FADE_IN_TIME;
        if (screen_fader > 1) {
            screen_fader = 1;
        }
    }

    Input dummy_input = {};
    auto input_ptr = &input;
    if (fade_out) input_ptr = &dummy_input;
    game_update(game, input_ptr, dt, t);

    auto delete_scheduled = [](auto &arr) {
        for (int i = 0; i < arr.count;) {
            if (arr.data[i].scheduled_for_deletion) {
                arr.remove(i);
            } else {
                i += 1;
            }
        }
    };

    delete_scheduled(game->torches);
    delete_scheduled(game->enemies);
    delete_scheduled(game->particles);

    ng::array<Entity *> all_entities = {};
    // if (!game->guy.is_dead) //
    { all_entities.push(&game->guy); }
    For(game->torches) all_entities.push(&it);
    For(game->enemies) all_entities.push(&it);

    ng_for(all_entities) {
        it->animation_timer = it->anim_timer_advanced(dt);
        while (it->animation_timer >= 1) {
            it->animation_timer -= 1;
        }
    }

    //
    // render
    //

    bool do_tint = true;

    auto draw_light = [&](Vector2 pos, float size, float alpha, Game *game, SDL_Color color, bool oscillate = false, float oscillation_offset = 0) {
        // d = sqrt(A)
        // d = sqrt(pi*r^2)
        // d = sqrt(pi) * r

        alpha = ng::clamp(alpha, 0.0f, 1.0f);

        if (oscillate) {
            auto oscillation = SDL_sinf((t + oscillation_offset) * 0.75f * ng::TAU32);
            {
                auto k = 0.01f;
                size *= (1 - k) + oscillation * k;
            }
            {
                auto k = 0.05f;
                alpha *= (1 - k) + oscillation * k;
            }
        }
        alpha = ng::clamp(alpha, 0.0f, 1.0f);

        if (do_tint) {
            // alpha *= 0.9f;
            // size *= 0.9f;
        }

        // color.r = cast(u8)(color.r / 255.0f * alpha * 255.0f);
        // color.g = cast(u8)(color.g / 255.0f * alpha * 255.0f);
        // color.b = cast(u8)(color.b / 255.0f * alpha * 255.0f);


        Rectangle rect_to_draw;
        auto dimension = SDL_sqrt(ng::TAU32 / 2) * size;
        rect_to_draw.size = v2(1, 1) * dimension;
        rect_to_draw.pos -= rect_to_draw.size / 2;

        rect_to_draw.pos += pos;
        rect_to_draw = rect_world_to_screen(rect_to_draw, game);

        auto rect_sdl = rect_to_sdl(rect_to_draw);

        SDL_SetRenderTarget(renderer, dynamic_lighting_render_target);

        if (dev && keystate[SDL_SCANCODE_F6]) {
            SDL_SetTextureColorMod(light_spot, color.r, color.g, color.b);
            SDL_SetTextureAlphaMod(light_spot, alpha * color.a);
        } else {
            auto color_scale = alpha * (color.a / 255.0f);
            SDL_SetTextureColorMod(light_spot, color.r * color_scale, color.g * color_scale, color.b * color_scale);
            SDL_SetTextureAlphaMod(light_spot, 255);
        }


#if 0
        SDL_SetTextureBlendMode(light_spot, SDL_BLENDMODE_MOD);
#elif 1
        SDL_SetTextureBlendMode(light_spot, SDL_BLENDMODE_ADD);
#else
        SDL_SetTextureBlendMode(light_spot, SDL_BLENDMODE_BLEND);
#endif
        SDL_RenderCopy(renderer, light_spot, null, &rect_sdl);
        SDL_SetRenderTarget(renderer, null);
    };

    auto draw_texture = [](SDL_Renderer *renderer, Game *game, Render_Texture *rt) {
        ng_assert(rt->tex);

        if (rt->w == 0) return;
        if (rt->h == 0) return;

        SDL_Rect src_rect = {};
        src_rect.w = rt->w;
        src_rect.h = rt->h;
        src_rect.x = rt->w * rt->atlas_index;
        src_rect.y = 0;

        SDL_Rect dst_rect = {};
        dst_rect.w = rt->w * game->camera.scale;
        dst_rect.h = rt->h * game->camera.scale;
        auto pos = rt->pos;
        if (rt->flip) {
            pos.x += -rt->offset.x - rt->w;
        } else {
            pos.x += rt->offset.x;
        }
        pos.y += rt->offset.y;
        pos.y += rt->z;
        pos = v2_world_to_screen(pos, game);
        dst_rect.x = pos.x;
        dst_rect.y = pos.y;

        SDL_RendererFlip flip = SDL_FLIP_NONE;
        if (rt->flip) flip = SDL_FLIP_HORIZONTAL;

        SDL_SetTextureColorMod(rt->tex, rt->color.r, rt->color.g, rt->color.b);
        SDL_SetTextureAlphaMod(rt->tex, rt->color.a);

        SDL_RenderCopyEx(renderer, rt->tex, &src_rect, &dst_rect, 0.0, null, flip);
    };

    For(game->enemies) {
        if (it.death_timer > 0) {
            f64 t = it.death_timer / it.DEATH_TIME();
            t = 1 - SDL_pow(2 * t - 1, 2);
            draw_light(it.pos, t * 96, t * 1.0, game, ENEMY_DEATH_LIGHT_COLOR);
        }
    }
    For(game->torches) {
        draw_light(it.pos + v2(0, it.z), it.radius(), 1.3f * it.light_level, game,
                   TORCH_COLOR, true, it.oscillation_offset);
    }
    // if (!game->guy.is_dead) //
    {
        SDL_SetTextureColorMod(light_spot, 255, 255, 255);

        auto radius = health_to_light_radius(game->guy.torch);
        {
            auto shrink_amt =
                SDL_pow(game->guy.stun_torch_shrink_timer / GUY_STUN_TORCH_SHRINK_TIME, 2);
            radius *= 1 - shrink_amt * 0.95f;
        }
        radius = ng::max(radius, 0.0f);

        auto alpha = 1.1f * (0.5f + 0.5f * game->guy.torch);

        if (radius > 0 && alpha > 0) {
            draw_light(game->guy.pos + game->guy.torch_offset() + v2(0, game->guy.z), radius,
                       alpha, game, GUY_TORCH_COLOR, true);
        }
    }
    if (!game->guy.is_dead && game->guy.stun_timer > 0) {
        auto color = ATTACK_IMPACT_PARTICLE_COLOR;
        color.a = 255;
        draw_light(game->guy.pos + v2(0, 16), 32, SDL_sqrt(game->guy.stun_timer / GUY_STUN_TIME), game, color);
    }

    
    auto draw_shadow = [&](Vector2 where) {
        auto rt = game->render_textures.push();
        rt->pos = where;
        rt->tex = game->tex_shadow.tex;

        rt->w = game->tex_shadow.frame_w;
        rt->h = game->tex_shadow.h;
        // rt->z = game->guy.z - 0.1;
        rt->z = -0.1;
        rt->offset = game->tex_shadow.offset;
        // rt->offset.y -= rt->z;

        rt->color = SDL_Color{255, 255, 255, 255};
    };
    draw_shadow(game->guy.pos);
    ng_for(game->enemies) {
        if (it.is_swarm && !it.aggroed) continue;
        draw_shadow(it.pos);
    }

#define SQRT_PI_OVER_2 (1.77245385091f * 0.5f)

    ng_for(game->particles) {
        auto age = t - it.birth_time;
        if (age > it.lifespan) {
            it.scheduled_for_deletion = true;
            continue;
        }

        auto current_pos = it.pos + (it.vel + it.acc * age) * age;
        auto current_z = it.z + (it.vel_z + it.acc_z * age) * age;
        // current_pos = it.pos;
        // current_z = it.z;

        auto vitality = (1 - (age / it.lifespan));

        auto part_size = it.size * (0.5f + 0.5f * vitality);
        auto part_color = it.color;
        part_color.a *= vitality;

        Rectangle aabb;
        aabb.pos = current_pos;
        aabb.w = 1;
        aabb.h = 1;

        bool collided = false;
        auto colliding_tiles = game->level->get_colliding_tiles(aabb);
        defer { colliding_tiles.release(); };
        foreach (&&tile : colliding_tiles) {
            if (tile_is_not_dashable(tile.tile)) {
                collided = true;
                break;
            }
        }
        if (collided) continue;

        // Square has same area as circle of radius (part_size/2)
        // dim = sqrt(A)
        // A = pi * r^2
        // dim = sqrt(pi * r^2)
        // dim = sqrt(pi) * (part_size / 2)

        auto dim = part_size * SQRT_PI_OVER_2;

        if (it.texture.tex == game->tex_particle_fire.tex) //
        {
            dim = it.texture.h;
            if (it.about_to_finish(dt)) {
                it.texture.w = 0; // hack
            }

            // part_color = SDL_Color{255, 192, 144, part_color.a};

        } else {
            it.switch_if_about_to_finish(dt, dummy_texture());
        }

        auto pos = current_pos;
        pos.y += current_z;
        draw_light(pos, part_size * 2, part_color.a * 2 / 255.0f, game, part_color);

        auto rt = game->render_textures.push();
        rt->pos = current_pos;
        rt->z = current_z;

        rt->tex = it.texture.tex;

        auto num_frames = it.texture.w / it.texture.frame_w;
        rt->atlas_index = (1 - vitality) * num_frames;

        rt->w = dim;
        rt->h = dim;
        rt->offset = it.texture.offset;
        // rt->offset.y -= rt->z;

        if (it.texture.tex == game->tex_particle_fire.tex) {
            rt->color = SDL_Color{255, 255, 255, cast(u8)(vitality * 255)};
        } else {
            rt->color = it.color;
            rt->color.a = cast(u8)(vitality * 255);
        }
    }
    if (0) { // occlude a lil bit
        const auto w = game->level->w;
        const auto h = game->level->h;
        for (int j = 0; j < h; j += 1) {
            for (int i = 0; i < w; i += 1) {
                auto idx = i + j * w;
                auto val = game->level->tiles[idx];

                if (!tile_is_wall(val)) continue;

                auto x = i;
                auto y = -(j - 1) + h;

                auto black_tile = rect_to_sdl(
                    rect_world_to_screen(rect(x * 16 - 8, y * 16 + 16, 16, 16), game));

                SDL_SetRenderTarget(renderer, dynamic_lighting_render_target);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderFillRect(renderer, &black_tile);
            }
        }
    }
    SDL_SetRenderTarget(renderer, null);

    { // render entities
        auto cam_in_world = camera_bounds_in_world_space(game);

        auto culled = [&](float x, float y) {
#define CULL_SIZE 96 // @Hack: doesn't work for things bigger than CULL_SIZE obviously
            auto cull_rect = rect(x - CULL_SIZE / 2, y - CULL_SIZE / 2, CULL_SIZE, CULL_SIZE);

            if (rect_intersect(cull_rect, cam_in_world)) return false;

            return true;
        };

        ng_for(all_entities) {
            if (culled(it->pos.x, it->pos.y)) continue;
            if (!it->texture.tex) continue;

            auto rt = game->render_textures.push();
            auto num_frames = it->texture.w / it->texture.frame_w;
            rt->atlas_index = it->animation_timer * num_frames;
            rt->tex = it->texture.tex;
            rt->w = it->texture.frame_w;
            rt->h = it->texture.h;
            rt->pos = it->pos;
            rt->z = it->z;
            rt->offset = it->texture.offset;
            // rt->offset = v2(-rt->w / 2, rt->h);
            rt->flip = it->texture.flip;

            rt->color = SDL_Color{255, 255, 255, 255};
        }

        // iterate over world
        for (int i = 0; i < game->level->h * game->level->h; i += 1) {
            float x = (i % game->level->w) * 16;
            float y = -(i / game->level->w - game->level->h) * 16;

            if (culled(x, y)) continue;

            auto val = game->level->tiles[i];

            auto rt = game->render_textures.push();
            rt->atlas_index = val;
            rt->tex = walls_atlas.tex;
            rt->w = walls_atlas.frame_w;
            rt->h = walls_atlas.h;
            rt->pos.x = x;
            rt->pos.y = y;

            rt->offset.x = -8;
            if (tile_is_floor(val)) {
                rt->z = -16;
                rt->offset.y = 16 + 48;
            } else if (tile_is_below_floor(val)) {
                rt->z = -16;
                rt->offset.y = 32 + 48;
            } else {
                rt->z = 0;
                rt->offset.y = 48;
            }

            rt->color = SDL_Color{255, 255, 255, 255};
        }

        quick_sort(&game->render_textures,
                   [](const Render_Texture &a, const Render_Texture &b) {
                       return a.pos.y - a.z > b.pos.y - b.z;
                   });

        ng_for(game->render_textures) { draw_texture(renderer, game, &it); }

        game->render_textures.clear();
    }

    auto prompt = [&](Vector2 pos, const Texture &t) {
        auto rt = game->render_textures.push();
        rt->pos = pos;
        rt->z = 0;

        rt->tex = t.tex;

        rt->w = t.w;
        rt->h = t.h;
        rt->offset = t.offset;

        rt->color = SDL_Color{255, 255, 255, 96};
    };

    prompt(v2(5, 7) * 16 + v2(0, -1), game->tex_icon_wasd);
    prompt(v2(23, 8) * 16 + v2(0, -1), game->tex_icon_lmb);
    prompt(v2(27, 8) * 16 + v2(0, -1), game->tex_icon_rmb);

    SDL_SetRenderTarget(renderer, null);
    auto draw_render_rect = [](SDL_Renderer *renderer, Rectangle rect, SDL_Color c,
                               SDL_BlendMode blend_mode) {
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(renderer, blend_mode);
        auto temp = rect_to_sdl(rect);
        SDL_RenderFillRect(renderer, &temp);
    };
    if (dev) {
        ng_for(game->render_rects_world) {
            auto rect = it.rect;
            draw_render_rect(renderer, rect_world_to_screen(rect, game), it.color, it.blend_mode);
        }
        ng_for(game->render_rects_screen) {
            draw_render_rect(renderer, it.rect, it.color, it.blend_mode);
        }
        game->render_rects_world.clear();
        game->render_rects_screen.clear();
    }

    SDL_SetRenderTarget(renderer, null);

    SDL_SetTextureBlendMode(dynamic_lighting_render_target, SDL_BLENDMODE_MOD);
    SDL_SetTextureColorMod(dynamic_lighting_render_target, 255, 255, 255);
    SDL_RenderCopy(renderer, dynamic_lighting_render_target, null, null);

#define TINT_SCALAR (18.214285714285714285714285714286)
#define TINT_R (cast(int)(6 * TINT_SCALAR))
#define TINT_G (cast(int)(8 * TINT_SCALAR))
#define TINT_B (cast(int)(14 * TINT_SCALAR))
#define TINT_A (cast(int)(255 / TINT_SCALAR))

    auto tint_rect = SDL_Rect{-10, -10, 9000, 9000}; // hack
    if (do_tint)                                     //
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        SDL_SetRenderDrawColor(renderer, TINT_R, TINT_G, TINT_B, TINT_A);
        SDL_RenderFillRect(renderer, &tint_rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);
        SDL_SetRenderDrawColor(renderer, 208, 208, 218, 255);
        SDL_RenderFillRect(renderer, &tint_rect);
    }
    // else {
    //     SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    //     SDL_SetRenderDrawColor(renderer, TINT_R, TINT_G, TINT_B, TINT_A);
    //     SDL_RenderFillRect(renderer, &tint_rect);
    // }

    {
        auto cursor_rect = rect();
        cursor_rect.pos = input.mouse_pos;
        cursor_rect.size = v2(game->tex_cursor.w, game->tex_cursor.h) * game->camera.scale;
        cursor_rect.pos -= cursor_rect.size / 2;

        auto dest = rect_to_sdl(cursor_rect);

        SDL_RenderCopy(renderer, game->tex_cursor.tex, null, &dest);
    }

    auto fade_rect = SDL_Rect{-10, -10, 9000, 9000}; // hack
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 6, 8, 14,
                               (ng::clamp(1 - screen_fader_alpha(), 0.0f, 1.0f) * 255));
        SDL_RenderFillRect(renderer, &fade_rect);
    }

    bool credits_complete = true;
    auto draw_credits_text = [&](float speed, Texture text, float y_offset, int before_or_after_main_credits, float scale = -1) {
        if (scale < 0) scale = game->camera.scale;
        auto dst =
            rect(game->screen_size / 2, v2(text.w, text.h) * scale);
        dst.pos -= dst.size / 2;
        dst.y += y_offset;
        dst.y -= credits_scroll_pos() * speed;

        // @Hack: some disgusting state coupling goin on here 2018-07-11
        if (before_or_after_main_credits > 0) {
            if (dst.y <= game->screen_size.y / 2 - dst.h / 2) {
                // Render it exactly in the middle.
                dst.y = game->screen_size.y / 2 - dst.h / 2;

            } else {
                credits_complete = false;
            }
        } else if (before_or_after_main_credits < 0) { // nothing extra to do
        } else {
            if (dst.y + dst.h < 0) {
                // nothing extra to do
            } else {
                credits_complete = false;
            }
        }

        SDL_SetTextureColorMod(text.tex, 255 - 6, 255 - 8, 255 - 14);
        SDL_SetTextureAlphaMod(text.tex,
                               ng::clamp(1 - screen_fader_alpha(), 0.0f, 1.0f) * 255);

        auto dest = rect_to_sdl(dst);

        SDL_RenderCopy(renderer, text.tex, null, &dest);
    };
    {
        auto main_credits_offset = credits_text.h / 2 + game->screen_size.y / 2 * 1.05;
        auto the_final_offset = main_credits_offset + credits_text.h / 2 + title_text.h * game->camera.scale / 2;

        float credits_distance = the_final_offset;
        float credits_time = 60.25f;
        float speed = credits_distance / credits_time;

        draw_credits_text(speed, title_text, 0, -1);
        draw_credits_text(speed, credits_text, main_credits_offset, 0, 1.0f);
        draw_credits_text(speed, title_text, the_final_offset, 1);
    }

    if (credits_complete) {
        paused = false;
        // credits_timer = -1000; // even more @Hacky
        // Mix_FadeOutMusic(100);
        Mix_HaltMusic();
        Mix_FadeInMusic(game->mus_ambient, -1, AMBIENT_FADE_MS);
        Mix_ResumeMusic();
        restart(); // @Hacky
    }

#if 0
    if (dt > 0.0) {
        for (int i = 0; i < NUM_SAMPLES; i += 1) {
            sample_points[i].x = i * (1280.0 / NUM_SAMPLES);
            sample_points[i].y = samples_l[i] / 64 + (360 * 1 / 2);
            sample_points[NUM_SAMPLES + i].x = i * (1280.0 / NUM_SAMPLES);
            sample_points[NUM_SAMPLES + i].y = samples_r[i] / 64 + (360 * 3 / 2);
        }
    }
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderDrawPoints(renderer, sample_points, ng::countof(sample_points));
#endif
                                                             
    SDL_RenderPresent(renderer);

    auto del = dt;
    // if (del == 0) del = 1.0f / 30.0f;
    // SDL_Delay(cast(int)(del * 1000));
    SDL_Delay(16);
        
    if (!game->quit) goto restart_frame;

    return 0;
#endif
}

extern "C" {
// int _fltused;
#undef main
extern int main();
void __cdecl mainCRTStartup() { ng::exit(main()); }
void __cdecl WinMainCRTStartup() { mainCRTStartup(); }
#pragma intrinsic(memset)
#pragma intrinsic(memcpy)
#pragma intrinsic(log)
#pragma intrinsic(pow)
#pragma intrinsic(atan) 
#pragma intrinsic(ceil)
#pragma intrinsic(exp)
#pragma intrinsic(sqrt)
#pragma intrinsic(cos)
#pragma intrinsic(sin)   
#pragma intrinsic(acos)
#pragma function(memset)
#pragma function(memcpy)
#pragma function(log)
#pragma function(pow)
#pragma function(atan)  
#pragma function(ceil)
#pragma function(exp)
#pragma function(sqrt) 
#pragma function(cos)
#pragma function(sin)  
#pragma function(acos)
void *__cdecl memset(void *dest, int x, size_t size) {
    return SDL_memset(dest, x, size);
}
void *__cdecl memcpy(void *dest, const void *src, size_t size) {
    return SDL_memcpy(dest, src, size);
}
void *__cdecl memmove(void *dest, const void *src, size_t size) {
    return SDL_memmove(dest, src, size);
}
// What follows is EXTREEEME hackery. Including fooling the type system across object file boundaries :(.
#pragma intrinsic(floor)
#pragma function(floor)
double __cdecl floor(double x) {
    return SDL_floor(x);
}
int __cdecl toupper(int x) {
    return SDL_toupper(x);
}
void __cdecl qsort(void *base, size_t nmemb, size_t size, int (*compare) (const void *, const void *)) {
    return SDL_qsort(base, nmemb, size, compare);
}
__declspec(restrict) void *__cdecl malloc(size_t n) {
    return HeapAlloc(GetProcessHeap(), 0, n);
}
__declspec(restrict) void *__cdecl calloc(size_t num, size_t size) {
    size_t final_amt = num * size;
    if (num != 0 && final_amt / num != size) return 0; // overflow, yay
    return HeapAlloc(GetProcessHeap(), 0x00000008 /* HEAP_ZERO_MEMORY */, final_amt);
}
__declspec(restrict) void *__cdecl realloc(void *p, size_t n) {
    if (!p) return HeapAlloc(GetProcessHeap(), 0, n);
    if (!n) {
        HeapFree(GetProcessHeap(), 0, p);
        return 0;
    }
    return HeapReAlloc(GetProcessHeap(), 0, p, n);
}
double __cdecl ldexp(double x, int exp) {
    union {double d; uint64_t i;} u;
    u.d = x;
    int exponent = ((u.i >> 52) & 0x7ff) + exp;
    if (exponent > 2046) {
        return HUGE_VAL;
    }
    if (exponent <= 0) { // flushes denormals, but meh whatever
        return 0;
    }
    u.i = (u.i & 0x800fffffffffffffULL) | ((uint64_t)exponent << 52ull);
    return u.d;
}
double __cdecl log(double x) {
    return SDL_log(x);
}
double __cdecl pow(double a, double b) {
    return SDL_pow(a, b);
}
double __cdecl atan(double x) {
    return SDL_atan(x);
}
double __cdecl ceil(double x) {
    return SDL_ceil(x);
}
double __cdecl exp(double x) {
    return SDL_pow(2.71828182845904523536028747135266249775724709369995, x);
}
double __cdecl sqrt(double x) {
    return SDL_sqrt(x);
}
double __cdecl cos(double x) {
    return SDL_cos(x);
}
double __cdecl sin(double x) {
    return SDL_sin(x);
}                  
double __cdecl acos(double x) {
    return SDL_acos(x);
}
void __cdecl free(void *p) {
    HeapFree(GetProcessHeap(), 0, p);
}
void __cdecl exit(int) { __debugbreak(); } // Why does a library need to call this?
const void *__cdecl memchr(const void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char *)ptr;
    unsigned char c = value;
    while (num--) {
        if (*p == c) return (void *)p;
        p++;
    }
    return 0;
}
SDL_RWops *__cdecl fopen(const char *file, const char *mode) {
    return SDL_RWFromFile(file, mode);
}
void __cdecl fclose(SDL_RWops *rw) {
    SDL_RWclose(rw);
}
size_t __cdecl fread(void *ptr, size_t size, size_t nmemb, SDL_RWops *stream) {
    return SDL_RWread(stream, ptr, size, nmemb);
}
int __cdecl fseek(SDL_RWops *stream, long int offset, int whence) {
    return SDL_RWseek(stream, offset, whence);
}
long int __cdecl ftell(SDL_RWops *stream) {
    return SDL_RWseek(stream, 0, RW_SEEK_CUR);
}
int *__cdecl _errno() { // This one's especially cute.
    static int i = 0;
    return &i;
}
}
