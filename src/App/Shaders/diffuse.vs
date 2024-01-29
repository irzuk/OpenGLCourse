#version 330 core
layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 morphing;

out vec3 normal;
out vec3 position;
out vec2 texcoord;

vec3 morping_calc(vec3 pos){
	return mix(pos, normalize(pos), morphing.x);
}

void main(){
	gl_Position = projection * view * model * vec4(morping_calc(in_vertex), 1);
	position = vec3(model * vec4(morping_calc(in_vertex), 1.0f));

	normal = normalize(mat3(transpose(inverse(model))) * mix(in_normal, normalize(in_vertex), morphing.x));

	texcoord = in_texcoord;
}
