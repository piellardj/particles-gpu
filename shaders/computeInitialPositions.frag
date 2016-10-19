#version 130


uniform vec2 bufferSize;


__UTILS.GLSL__


void main()
{
    gl_FragColor = coordsToColor(vec2(1.0,-1.0)*(gl_FragCoord.xy - bufferSize/2.0),
                                 MAX_POSITION);
}
