#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

out vec2 fragTexCoord;
out vec3 fragNormal;

uniform mat4 mvp;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragNormal = vertexNormal;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}