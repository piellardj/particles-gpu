#version 130


uniform sampler2D oldPositions;
uniform sampler2D velocities;

uniform vec2 bufferSize;

uniform float dt;

__UTILS.GLSL__


void main()
{
    vec2 coordsOnBuffer = gl_FragCoord.xy / bufferSize;
    
    /* Retrieving of position and velocity from texture buffers */
    vec2 position = colorToCoords(texture2D(oldPositions, coordsOnBuffer),
                                  MAX_POSITION);
    vec2 velocity = colorToCoords(texture2D(velocities, coordsOnBuffer),
                                  MAX_SPEED);

    gl_FragColor = coordsToColor(position + dt*velocity, MAX_POSITION);
}
