#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main()
{
    vec4 texelColor =
        texture(texture0, fragTexCoord);

    // finalColor =
    //     texelColor *
    //     fragColor *
    //     colDiffuse;

    finalColor =
    vec4(
        texelColor.r,
        texelColor.g * 0.25,
        texelColor.b * 1,
        texelColor.a
    );
}