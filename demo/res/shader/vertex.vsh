#ifdef GL_ES
// Set default precision to medium
precision highp int;
precision highp float;
#endif
attribute highp vec4 model_coords;
attribute highp vec4 tex_coords;
uniform highp mat4 mvp_matrix;
varying highp vec4 v_tex_coords;
void main(void)
{
    gl_Position = mvp_matrix * model_coords;
    v_tex_coords = tex_coords;
}
