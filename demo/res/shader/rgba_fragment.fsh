precision mediump float;
uniform sampler2D color_map;
uniform float tex_width;
varying highp vec4 v_tex_coords;
void main(void)
{
    gl_FragColor = texture2D(color_map, v_tex_coords.st).bgra;
}
