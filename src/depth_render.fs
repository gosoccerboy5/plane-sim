//taken from the depth rendering example from raylib

#version 330

precision mediump float;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;

// Input uniform values
uniform sampler2D depthTexture;
uniform sampler2D colorTexture;
uniform bool flipY;

float nearPlane = 0.1;
float farPlane = 10000.0;

out vec4 FragColor;

void main() {
    // Handle potential Y-flipping
    vec2 texCoord = fragTexCoord;
    if (flipY) texCoord.y = 1.0 - texCoord.y;

    // Sample depth texture
    float depth = texture(depthTexture, texCoord).r;
    vec4 color = texture(colorTexture, texCoord);

    // Linearize depth
    float linearDepth = (2.0*nearPlane)/(farPlane + nearPlane - depth*(farPlane - nearPlane));
    
    // Output final color
    //gl_FragColor = vec4(vec3(linearDepth), 1.0);
    
    //float colorMultiplier = max(linearDepth*-0.15+1.0, 0.5);
    //gl_FragColor = vec4(color.r*colorMultiplier, color.g*colorMultiplier, color.b*colorMultiplier, max(1.0-linearDepth/10.0, 0.3)); //fog effect :D
    
    //max(linearDepth*-0.15+1.0, 0.5);
    if (linearDepth >= 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    } else {
        float colorMultiplier = linearDepth*-0.5+1.0;//sqrt(linearDepth)*.5+.6;
        colorMultiplier = clamp(colorMultiplier, 0.5, 1.0);
        FragColor = vec4(color.r*colorMultiplier, color.g*colorMultiplier, color.b*colorMultiplier, 1.0);
    }
    
}