#version 330 core
in vec3 normal;
in vec3 position;
in vec2 texcoord;

uniform sampler2D tex;

uniform vec3 lightColor;
uniform vec3 lightPos;

uniform vec3 viewPos;

out vec4 color;

void main() {
	float ambientStrength = 0.5f;
	
    vec3 ambient = ambientStrength * lightColor;
    
	vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - position);

	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

	float specularStrength = 0.5f;
    vec3 viewDir = normalize(viewPos - position);
    vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec4 result = vec4(ambient + diffuse + specular, 1.0f) * texture(tex, texcoord);
    color = result;
}
