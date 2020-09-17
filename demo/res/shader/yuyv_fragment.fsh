#ifdef GL_ES
// Set default precision to medium
precision highp int;
precision highp float;
#endif
uniform sampler2D color_map;
uniform float tex_width;
varying highp vec4 v_tex_coords;
void main(void)
{
    float is_right = floor(mod(v_tex_coords.x / tex_width, 2.0));

    // Y1UY2V ---> Y1UY2VY1UY2V, for GL_NEAREST filter
    vec4 rgba = texture2D(color_map, v_tex_coords.st);
    vec3 yuv = mix(rgba.rga, rgba.bga, is_right) - vec3(0.0, 0.5, 0.5);

    // Y1UY2V ---> Y1Y1UUY2Y2VV, for GL_LINEAR filter
//    vec4 y1u = texture2D(color_map, v_tex_coords.st - is_right * vec2(tex_width, 0.0));
//    vec4 y2v = texture2D(color_map, v_tex_coords.st + (1.0 - is_right) * vec2(tex_width, 0.0));
//    vec3 yuv = vec3(mix(y1u.r, y2v.r, is_right) , y1u.a, y2v.a) - vec3(0.0, 0.5, 0.5);
    vec3 rgb = mat3(1.0, 1.0, 1.0,
                    0.0, -0.343, 1.765,
                    1.4, -0.711, 0.0) * yuv;
    gl_FragColor = vec4(rgb, 1.0);
//    gl_FragColor = texture2D(qt_Texture0, qt_TexCoord0.st).bgra;
}
