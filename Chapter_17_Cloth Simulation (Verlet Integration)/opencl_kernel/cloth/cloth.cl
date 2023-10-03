
#pragma pack(1)

enum STICK_GROUP_ID
{
    HORIZONTAL_DISTANCE_1_EVEN = 0,
    HORIZONTAL_DISTANCE_1_ODD,
    VERTICAL_DISTANCE_1_EVEN,
    VERTICAL_DISTANCE_1_ODD,
    DIAGONAL_DISTANCE_1_EVEN,
    DIAGONAL_DISTANCE_1_ODD,

    HORIZONTAL_DISTANCE_2_EVEN,
    HORIZONTAL_DISTANCE_2_ODD,
    VERTICAL_DISTANCE_2_EVEN,
    VERTICAL_DISTANCE_2_ODD,
    DIAGONAL_DISTANCE_2_EVEN,
    DIAGONAL_DISTANCE_2_ODD,
};

struct Stick
{
    uint p0;
    uint p1;
};

float4 mat_vec_mul( float16 mat, float4 vec)
{
    // variable declaration
    float4 ret_val = (float4)( 0.0f);
    
    // code
    /*
        [ m00 m01 m02 m03] [ v00]
        [ m10 m11 m12 m13] [ v10]
        [ m20 m21 m22 m23] [ v20]
        [ m30 m31 m32 m33] [ v30]
     */

    ret_val.x = (mat.s0 * vec.x) + (mat.s1 * vec.y) + (mat.s2 * vec.z) + (mat.s3 * vec.w);
    ret_val.y = (mat.s4 * vec.x) + (mat.s5 * vec.y) + (mat.s6 * vec.z) + (mat.s7 * vec.w);
    ret_val.z = (mat.s8 * vec.x) + (mat.s9 * vec.y) + (mat.sa * vec.z) + (mat.sb * vec.w);
    ret_val.w = (mat.sc * vec.x) + (mat.sd * vec.y) + (mat.se * vec.z) + (mat.sf * vec.w);

    return ret_val;
}


__kernel void update_vertices(
    __global float4 *p_position, __global float4 *p_old_position,
    __global bool *p_fix_vertices, float4 gravity, float friction,
    uint vertices_count
)
{
    // variable declaration
    float4 velocity = (float4)(0.0f);

    // code
    int index = get_global_id(0);
    if( index > (vertices_count - 1))
    {
        return;
    }

    if( p_fix_vertices[index])
    {
        return;
    }

    float4 pos = p_position[index];
    float4 old_pos = p_old_position[index];

    velocity = (pos - old_pos) * friction;

    old_pos = pos;
    pos = pos + velocity;
    pos = pos + gravity;

    p_position[index] = (float4)(pos.xyz, 1.0f);
    p_old_position[index] = (float4)(old_pos.xyz, 1.0f);
}


__kernel void constraint_vertices(
    __global float4 *p_position, __global float4 *p_old_position,
    __global bool *p_fix_vertices, float friction, float bounce_damping,
    float3 bound_dimension, uint vertices_count
)
{
    // variable declaration
    float4 velocity = (float4)(0.0f);

    // code
    int index = get_global_id(0);
    if( index > (vertices_count - 1))
    {
        return;
    }

    if( p_fix_vertices[index])
    {
        return;
    }

    float4 pos = p_position[index];
    float4 old_pos = p_old_position[index];

    velocity = (pos - old_pos) * friction;

    if( pos.x > bound_dimension.x)
    {
        pos.x = bound_dimension.x;
        old_pos.x = pos.x + velocity.x * bounce_damping;
    }
    else if( pos.x < -bound_dimension.x)
    {
        pos.x = -bound_dimension.x;
        old_pos.x = pos.x + velocity.x * bounce_damping;
    }


    if( pos.y > bound_dimension.y)
    {
        pos.y = bound_dimension.y;
        old_pos.y = pos.y + velocity.y * bounce_damping;
    }
    else if( pos.y < -bound_dimension.y)
    {
        pos.y = -bound_dimension.y;
        old_pos.y = pos.y + velocity.y * bounce_damping;
    }


    if( pos.z > bound_dimension.z)
    {
        pos.z = bound_dimension.z;
        old_pos.z = pos.z + velocity.z * bounce_damping;
    }
    else if( pos.z < -bound_dimension.z)
    {
        pos.z = -bound_dimension.z;
        old_pos.z = pos.z + velocity.z * bounce_damping;
    }

    p_position[index] = pos;
    p_old_position[index] = old_pos;
}


__kernel void update_sticks(
    __global float4 *p_position, __global bool *p_fix_vertices,
    __global struct Stick *p_sticks, float current_group_distance,
    unsigned int sticks_count
)
{
    // code
    int index = get_global_id(0);

    if( index > (sticks_count - 1))
    {
        return;
    }

    uint p0_index = p_sticks[index].p0;
    uint p1_index = p_sticks[index].p1;

    float4 p0 = p_position[ p0_index];
    float4 p1 = p_position[ p1_index];

    float4 dp = p1 - p0;
    float dist = length( dp);
    float difference = current_group_distance - dist;
    float percent = difference / dist / 2.0f;

    float4 offset = dp * percent;

    if( ! p_fix_vertices[p0_index])
    {
        p0 = p0 - offset;
    }

    if( ! p_fix_vertices[p1_index])
    {
        p1 = p1 + offset;
    }

    p0.w = p1.w = 1.0f;

    p_position[p0_index] = p0;
    p_position[p1_index] = p1;
}


//Normal Calculation
__kernel void normal_calculation(
    __global float4 *p_normal,
    __global float4 *p_position,
    unsigned int width,
    unsigned int height
)
{
    uint index = get_global_id( 0);

    uint x = index % width;
    uint y = index / width;

    float3 position = p_position[index].xyz;
    float3 normal = (float3)( 0.0f, 0.0f, 0.0f);

    if( x > 0)
    {
        float3 left = p_position[ index - 1].xyz;

        if( y > 0)
        {
            float3 down = p_position[index - width].xyz;
            // normal += normalize( cross( left - position, down - position));
            normal += normalize( cross( down - position, left - position));
        }

        if( y < (height - 1))
        {
            float3 up = p_position[index + width].xyz;
            // normal += normalize( cross( up - position, left - position));
            normal += normalize( cross( left - position, up - position));
        }
    }

    if( x < (width - 1))
    {
        float3 right = p_position[ index + 1].xyz;

        if( y > 0)
        {
            float3 down = p_position[index - width].xyz;
            // normal += normalize( cross( down - position, right - position));
            normal += normalize( cross( right - position, down - position));
        }

        if( y < (height - 1))
        {
            float3 up = p_position[index + width].xyz;
            // normal += normalize( cross( right - position, up - position));
            normal += normalize( cross( up - position, right - position));
        }
    }

    p_normal[index].xyz = normalize( normal);
}
