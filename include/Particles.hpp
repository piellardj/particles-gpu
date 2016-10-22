#ifndef PARTICLES_HPP_INCLUDED
#define PARTICLES_HPP_INCLUDED

#include <array>

#include <GL/glew.h>
#include "glm.hpp"

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/OpenGL.hpp>

#include "Camera.hpp"


/* Class for handling particles that can be moved with the mouse.
 * Stores the particles' positions and velocities on a texture in GPU memory */
class Particles
{
    public:
        Particles(std::string const& image);
        ~Particles();

        unsigned int getNbParticles() const;
        sf::Vector2u const& getBuffersSize() const;

        /* Centers particles with zero initial speed */
        void initialize();

        /* Activation and movement of the magnet attracting the particles */
        void setMagnetState (bool activation);
        void setMagnetPosition(sf::Vector2f const& position);

        void computeNewPositions(sf::Time const& dt);

        void draw(sf::RenderWindow &window, Camera const& camera) const;

    private:
        float _maxSpeed;
        float _attraction;
        float _friction;

        sf::Vector2f _magnetPosition;

        sf::Vector2u _buffersSize;

        int _currentBufferIndex; //0 or 1 alternatively
        std::array<sf::RenderTexture, 2> _positions;
        std::array<sf::RenderTexture, 3> _velocities;

        sf::Shader _computeInitialPositionsShader;
        sf::Shader _computeInitialVelocitiesShader;
        sf::Shader _updateVelocityShader;
        sf::Shader _updatePositionShader;
        mutable sf::Shader _displayVerticesShader;

        GLuint _colorBufferID;
        GLuint _texCoordBufferID;
};

#endif // PARTICLES_HPP_INCLUDED
