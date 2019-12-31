#ifndef MINECRAFTPP_VEC3_HPP
#define MINECRAFTPP_VEC3_HPP

namespace minecraftpp {
    class vec3 {
    public:
        float x;
        float y;
        float z;

        vec3(): x(0.0f), y(0.0f), z(0.0f) {}
        vec3(float x, float y, float z): x(x), y(y), z(z) {}
    };

    vec3 operator+(vec3 const v1, vec3 const v2) {
        return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
    }

    vec3 operator-(vec3 const v1, vec3 const v2) {
        return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
    }
}

#endif // !MINECRAFTPP_VEC3_HPP
