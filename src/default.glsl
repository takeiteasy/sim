@vs vs
uniform vs_params {
    mat4 view_matrix;
    mat4 texture_matrix;
    mat4 projection_matrix;
};
in vec4 position;
in vec4 normal;
in vec2 texcoord0;
in vec4 color0;

in vec4 inst_mat_xxxx;
in vec4 inst_mat_yyyy;
in vec4 inst_mat_zzzz;
in vec4 inst_mat_wwww;

out vec4 uv;
out vec4 color;

mat4 make_matrix(vec4 x, vec4 y, vec4 z, vec4 w) {
    mat4 m;
    m[0][0] = x.x;
    m[1][0] = x.y;
    m[2][0] = x.z;
    m[3][0] = x.w;
    m[0][1] = y.x;
    m[1][1] = y.y;
    m[2][1] = y.z;
    m[3][1] = y.w;
    m[0][2] = z.x;
    m[1][2] = z.y;
    m[2][2] = z.z;
    m[3][2] = z.w;
    m[0][3] = w.x;
    m[1][3] = w.y;
    m[2][3] = w.z;
    m[3][3] = w.w;
    return m;
}


void main() {
    mat4 model_matrix = make_matrix(inst_mat_xxxx,
                                    inst_mat_yyyy,
                                    inst_mat_zzzz,
                                    inst_mat_wwww);
	gl_Position = (model_matrix * view_matrix * projection_matrix) * position;
//	gl_PointSize = psize;
	uv = texture_matrix * vec4(texcoord0, 0.0, 1.0);
	color = color0;
}
@end

@fs fs
uniform sampler2D texture0;

in vec4 uv;
in vec4 color;

out vec4 frag_color;

void main() {
	frag_color = texture(texture0, uv.xy) * color;
}
@end

@program sim vs fs
