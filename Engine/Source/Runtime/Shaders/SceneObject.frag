#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera;

layout(binding = 1) uniform ObjectBuffer {
    mat4 model;
    vec4 color;
    int mode; // 0 = Lit, 1 = Unlit, 2 = Wireframe
} object;

void main() {
    if (object.mode == 1 || object.mode == 2) {
        outColor = object.color;
        return;
    }
    
    // Simple directional lighting
    vec3 normal = normalize(outNormal);
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3)); // Light direction
    vec3 viewDir = normalize(camera.pos - inWorldPos);
    
    // Ambient
    float ambientStr = 0.25;
    vec3 ambient = ambientStr * vec3(1.0);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.85);
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = 0.2 * spec * vec3(1.0);
    
    vec3 finalLight = (ambient + diffuse + specular) * object.color.rgb;
    outColor = vec4(finalLight, object.color.a);
}
