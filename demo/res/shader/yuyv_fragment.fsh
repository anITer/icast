precision mediump float;
uniform sampler2D color_map;
uniform sampler2D uv_color_map;
varying highp vec4 v_tex_coords;

void main(void)
{
    vec2 uv = texture2D(uv_color_map, v_tex_coords.st).ga;
    vec3 yuv = vec3(texture2D(color_map, v_tex_coords.st).r, uv.s, uv.t);
    vec3 rgb = mat3(1.0,  1.0,   1.0,
                    0.0, -0.343, 1.765,
                    1.4, -0.711, 0.0) * (yuv - vec3(0.0, 0.5, 0.5));
    gl_FragColor = vec4(rgb, 1.0);
}
