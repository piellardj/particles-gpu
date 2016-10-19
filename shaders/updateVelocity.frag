#version 130


uniform sampler2D positions;
uniform sampler2D oldVelocities;

uniform vec2 bufferSize;

uniform float dt;

uniform vec2 mouse;

uniform float maxSpeed;
uniform float friction;
uniform float attraction;


__UTILS.GLSL__

/* Acceleration is proportionnal to 1 / distance */
vec2 getAcceleration(const vec2 coordsOnBuffer)
{
    vec2 position = colorToCoords(texture2D(positions, coordsOnBuffer),
                                  MAX_POSITION);
    vec2 toMouse = mouse - position;
    
    float squaredDistance = dot(toMouse, toMouse);
    
    return attraction * toMouse / squaredDistance;
}

vec2 getNewVelocity(const vec2 coordsOnBuffer)
{
    vec2 velocity = colorToCoords(texture2D(oldVelocities, coordsOnBuffer),
                                  MAX_SPEED);
    vec2 acceleration = getAcceleration(coordsOnBuffer);
    
    //add current acceleration
    velocity = velocity + dt * acceleration;
    
    //speed cannot be greater than maxSpeed
    velocity = velocity * min(1.0, maxSpeed/length(velocity));
    
    return velocity * friction;
}

void main()
{
   vec2 coordsOnBuffer = gl_FragCoord.xy / bufferSize;

    gl_FragColor = coordsToColor(getNewVelocity(coordsOnBuffer),
                                 MAX_SPEED);
}
