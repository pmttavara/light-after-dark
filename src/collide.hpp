struct Collide_State {
    Vector2 pos = {};
    Vector2 delta = {};
    Rectangle collider_left = {};
    Rectangle collider_right = {};
    Rectangle collider_top = {};
    Rectangle collider_bottom = {};
};
struct Level_Collision_Test_Result {
    bool did_collide = false;
};
struct Collide_Result {
    Collide_State new_state = {};
    Level_Collision_Test_Result bottom = {}, top = {}, right = {}, left = {};
};

Collide_Result collide(Collide_State collide_state, Level *level);

struct Collision_Resolution_Data {
    f32 tile_offset = 0;
    f32 offset_from_collider = 0;
};
void collide_directional(f32 *pos_coord, f32 *delta_coord, Collision_Resolution_Data res_data,
                         Level_Collision_Test_Result *collision_test_result, Rectangle aabb,
                         Level *level, f32 delta, bool is_a_horizontal_check,
                         bool checking_direction_is_negative, bool (&collision_checker)(int));
Collide_Result collide(Collide_State input_state, Level *level, bool (&collision_checker)(int)) {
    Collide_Result result = {};
    result.new_state = input_state;
    auto state = &result.new_state;
    f32 delta_x = state->delta.x;
    f32 delta_y = state->delta.y;
    auto c_bottom = [&](f32 delta) {
        auto pos_coord = &state->pos.y;
        auto delta_coord = &state->delta.y;
        auto aabb = rect(state->collider_bottom.x + state->pos.x,
                         state->collider_bottom.y + state->pos.y + delta, state->collider_bottom.w,
                         state->collider_bottom.h - delta);
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 16;
        res_data.offset_from_collider = state->collider_bottom.b();
        collide_directional(pos_coord, delta_coord, res_data, &result.bottom, aabb, level, delta,
                            false, true, collision_checker);
    };
    auto c_top = [&](f32 delta) {
        auto pos_coord = &state->pos.y;
        auto delta_coord = &state->delta.y;
        auto aabb = rect(state->collider_top.x + state->pos.x, state->collider_top.y + state->pos.y,
                         state->collider_top.w, state->collider_top.h + delta);
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 0;
        res_data.offset_from_collider = state->collider_top.t();
        collide_directional(pos_coord, delta_coord, res_data, &result.top, aabb, level, delta,
                            false, false, collision_checker);
    };
    auto c_right = [&](f32 delta) {
        auto pos_coord = &state->pos.x;
        auto delta_coord = &state->delta.x;
        auto aabb =
            rect(state->collider_right.x + state->pos.x, state->collider_right.y + state->pos.y,
                 state->collider_right.w + delta, state->collider_right.h);
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = -8;
        res_data.offset_from_collider = state->collider_right.r();
        collide_directional(pos_coord, delta_coord, res_data, &result.right, aabb, level, delta,
                            true, false, collision_checker);
    };
    auto c_left = [&](f32 delta) {
        auto pos_coord = &state->pos.x;
        auto delta_coord = &state->delta.x;
        auto aabb = rect(state->collider_left.x + state->pos.x + delta,
                         state->collider_left.y + state->pos.y, state->collider_left.w - delta,
                         state->collider_left.h);
        Collision_Resolution_Data res_data = {};
        res_data.tile_offset = 8;
        res_data.offset_from_collider = state->collider_left.l();
        collide_directional(pos_coord, delta_coord, res_data, &result.left, aabb, level, delta,
                            true, true, collision_checker);
    };
    if (delta_y > 0) {
        c_top(delta_y);
        c_bottom(0.0f);
    } else {
        c_bottom(delta_y);
        c_top(0.0f);
    }
    if (delta_x > 0) {
        c_right(delta_x);
        c_left(0.0f);
    } else {
        c_left(delta_x);
        c_right(0.0f);
    }
    return result;
}
void collide_directional(f32 *pos_coord, f32 *delta_coord, Collision_Resolution_Data res_data,
                         Level_Collision_Test_Result *collision_test_result, Rectangle aabb,
                         Level *level, f32 delta, bool is_a_horizontal_check,
                         bool checking_direction_is_negative, bool (&collision_checker)(int)) {
    auto collisions = level->get_colliding_tiles(aabb);
    defer { collisions.release(); };

    Collision_Info first_ci = {};

    bool collided = false;
    ng_for(collisions) {
        if (collision_checker(it.tile)) {
            collided = true;
            first_ci = it;
            break;
        }
    }

    collision_test_result->did_collide = collided;

    if (collided) {
        bool should_zero_delta =
            (checking_direction_is_negative ? (*delta_coord < 0.0f) : (*delta_coord > 0.0f));
        if (should_zero_delta) {
            *delta_coord = 0.0f;
        }
        *pos_coord =
            (((is_a_horizontal_check ? first_ci.x : first_ci.y) * 16) + res_data.tile_offset) -
            res_data.offset_from_collider;
    } else {
        *pos_coord += delta;
    }
}
