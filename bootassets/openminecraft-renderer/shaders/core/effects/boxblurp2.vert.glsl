#version 330 core
#extension GL_ARB_separate_shader_objects : enable

#vertex

layout(location = 0) out vec2 outTexCoord;

void main() {
    gl_Position = vec4(inPosition, 1.0);
#ifdef VULKAN
    gl_Position.y = -gl_Position.y;
#endif
    outTexCoord = inTexCoord;
}
