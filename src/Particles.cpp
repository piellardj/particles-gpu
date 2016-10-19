#include "Particles.hpp"

#include <cmath>
#include <algorithm>
#include <exception>

#include <iostream>

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "GLCheck.hpp"
#include "Utilities.hpp"

Particles::Particles(std::string const& imagePath):
            _maxSpeed(10.f),
            _attraction (0.f),
            _friction (0.99f),
            _magnetPosition(sf::Vector2f(0.f, 0.f)),
            _currentBufferIndex (0),
            _colorBufferID(0),
            _texCoordBufferID(0)
{
    /* Loading of the image */
    sf::Image image;
    if (!image.loadFromFile(imagePath))
        throw std::runtime_error("unable to open " + imagePath);
    _buffersSize = image.getSize();

    /* Allocation of buffers */
    for (sf::RenderTexture &positionBuffer : _positions) {
        if (!positionBuffer.create(getBuffersSize().x, getBuffersSize().y))
            throw std::runtime_error("unable to create positions buffer");
    }
    for (sf::RenderTexture &velocityBuffer : _velocities) {
        if (!velocityBuffer.create(getBuffersSize().x, getBuffersSize().y))
            throw std::runtime_error("unable to create velocities buffer");
    }

    /* Loading of the shaders */
    std::string fragmentShader, vertexShader, utils;
    loadFile("shaders/utils.glsl", utils);

    loadFile("shaders/computeInitialPositions.frag", fragmentShader);
    searchAndReplace("__UTILS.GLSL__", utils, fragmentShader);
    if (!_computeInitialPositionsShader.loadFromMemory(fragmentShader, sf::Shader::Fragment))
        throw std::runtime_error("unable to load shader shaders/computeInitialPositions.frag");

    loadFile("shaders/computeInitialVelocities.frag", fragmentShader);
    searchAndReplace("__UTILS.GLSL__", utils, fragmentShader);
    if (!_computeInitialVelocitiesShader.loadFromMemory(fragmentShader, sf::Shader::Fragment))
        throw std::runtime_error("unable to load shader shaders/computeInitialVelocities.frag");

    loadFile("shaders/updateVelocity.frag", fragmentShader);
    searchAndReplace("__UTILS.GLSL__", utils, fragmentShader);
    if (!_updateVelocityShader.loadFromMemory(fragmentShader, sf::Shader::Fragment))
        throw std::runtime_error("unable to load shader shaders/updateVelocity.frag");

    loadFile("shaders/updatePosition.frag", fragmentShader);
    searchAndReplace("__UTILS.GLSL__", utils, fragmentShader);
    if (!_updatePositionShader.loadFromMemory(fragmentShader, sf::Shader::Fragment))
        throw std::runtime_error("unable to load shader shaders/updatePosition.frag");

    loadFile("shaders/displayParticles.vert", vertexShader);
    searchAndReplace("__UTILS.GLSL__", utils, vertexShader);
    loadFile("shaders/displayParticles.frag", fragmentShader);
    searchAndReplace("__UTILS.GLSL__", utils, fragmentShader);

    if (!_displayVerticesShader.loadFromMemory(vertexShader, fragmentShader))
        throw std::runtime_error("unable to load shader shaders/displayParticles.frag or shaders/displayParticles.vert");


    /* Create VBO */
    std::vector< glm::vec4 > colors (getNbParticles());
    std::vector< glm::vec2 > texCoords (getNbParticles());

    for (unsigned int i = 0 ; i < getNbParticles() ; ++i) {
        sf::Vector2i pos (i % getBuffersSize().x, i / getBuffersSize().x);
        sf::Color col (image.getPixel(pos.x, pos.y));

        colors[i] = glm::vec4(col.r, col.g, col.b, col.a) / 255.f;
        texCoords[i] = glm::vec2(static_cast<float>(pos.x) / static_cast<float>(getBuffersSize().x),
                              static_cast<float>(pos.y) / static_cast<float>(getBuffersSize().y));
    }

    /* Activate buffer and send data to the graphics card */
    GLCHECK(glGenBuffers(1, &_colorBufferID)); //colors
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER,_colorBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, colors.size()*sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW));

    GLCHECK(glGenBuffers(1, &_texCoordBufferID)); //corresponding texture pixel coordinates
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _texCoordBufferID));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, texCoords.size()*sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW));

    initialize();
}

Particles::~Particles()
{
    if (_colorBufferID != 0)
        GLCHECK(glDeleteBuffers(1, &_colorBufferID));
    if (_texCoordBufferID != 0)
        GLCHECK(glDeleteBuffers(1, &_texCoordBufferID));
}

unsigned int Particles::getNbParticles() const
{
    return getBuffersSize().x * getBuffersSize().y;
}

sf::Vector2u const& Particles::getBuffersSize() const
{
    return _buffersSize;
}

void Particles::initialize()
{
    /* Blending disabled, all four components replaced */
    sf::RenderStates noBlending(sf::BlendMode(sf::BlendMode::One, sf::BlendMode::Zero));
    sf::Vector2f bufferSize = sf::Vector2f(_buffersSize.x, _buffersSize.y);
    sf::RectangleShape square(sf::Vector2f(getBuffersSize().x, getBuffersSize().y));

    /* Positions */
    _computeInitialPositionsShader.setParameter("bufferSize", bufferSize);
    noBlending.shader = &_computeInitialPositionsShader;
    for (sf::RenderTexture &texture : _positions)
        texture.draw (square, noBlending);

    /* Velocities */
    noBlending.shader = &_computeInitialVelocitiesShader;
    for (sf::RenderTexture &texture : _velocities)
        texture.draw (square, noBlending);
}

void Particles::setMagnetState (bool activation)
{
    _attraction = (activation) ? 50.f : 0.f;
}

void Particles::setMagnetPosition(sf::Vector2f const& position)
{
    _magnetPosition = position;
}

void Particles::computeNewPositions(sf::Time const& dtime)
{
    /* Blending disabled, all four components replaced */
    sf::RenderStates renderStates(sf::BlendMode(sf::BlendMode::One, sf::BlendMode::Zero));

    int nextBufferIndex = (_currentBufferIndex + 1) % 2;

    sf::RectangleShape square(sf::Vector2f(getBuffersSize().x, getBuffersSize().y));
    sf::Vector2f bufferSize = sf::Vector2f(_buffersSize.x, _buffersSize.y);
    float dt = 30.f * dtime.asSeconds();
/**/
    _updateVelocityShader.setParameter("positions", _positions[_currentBufferIndex].getTexture());
    _updateVelocityShader.setParameter("oldVelocities", _velocities[_currentBufferIndex].getTexture());
    _updateVelocityShader.setParameter("bufferSize", bufferSize);
    _updateVelocityShader.setParameter("dt", dt);
    _updateVelocityShader.setParameter("mouse", _magnetPosition);
    _updateVelocityShader.setParameter("maxSpeed", _maxSpeed);
    _updateVelocityShader.setParameter("friction", std::pow(_friction, dt));
    _updateVelocityShader.setParameter("attraction", _attraction);
    renderStates.shader = &_updateVelocityShader;
    _velocities[nextBufferIndex].draw (square, renderStates);

    _updatePositionShader.setParameter("oldPositions", _positions[_currentBufferIndex].getTexture());
    _updatePositionShader.setParameter("velocities", _velocities[nextBufferIndex].getTexture());
    _updatePositionShader.setParameter("bufferSize", bufferSize);
    _updatePositionShader.setParameter("dt", dt);
    renderStates.shader = &_updatePositionShader;
    _positions[nextBufferIndex].draw (square, renderStates);

    _currentBufferIndex = nextBufferIndex;
}

void Particles::draw(sf::RenderWindow &window, Camera const& camera) const
{
    window.setActive(true);

    GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    _displayVerticesShader.setParameter("positions", _positions[_currentBufferIndex].getTexture());
    sf::Shader::bind(&_displayVerticesShader);

    /* First we retrieve the shader program's, Attributes' and Uniforms' ID */
    GLuint displayShaderID = 0;
    GLCHECK(displayShaderID = _displayVerticesShader.getNativeHandle());

    GLuint texCoordAttributeID = 0, colorAttributeID = 0, viewMatrixUniformID = 0;
    GLCHECK(texCoordAttributeID = glGetAttribLocation(displayShaderID, "coordsOnBuffer"));
    GLCHECK(colorAttributeID = glGetAttribLocation(displayShaderID, "color"));
    GLCHECK(viewMatrixUniformID = glGetUniformLocation(displayShaderID, "viewMatrix"));

//    std::cout << "shaderID : " << displayShaderID << std::endl;
//    std::cout << "texCoord ; color ; viewMatrix  ->  " << texCoordAttributeID << " ; " << colorAttributeID << " ; " << viewMatrixUniformID << std::endl;

    /* Sending the view matrix */
    GLCHECK(glUniformMatrix3fv(viewMatrixUniformID, 1, GL_FALSE, &camera.getViewMatrix()[0][0]));

    /* Enabling texture coordinates buffer */
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _texCoordBufferID));
    GLCHECK(glEnableVertexAttribArray(texCoordAttributeID));
    GLCHECK(glVertexAttribPointer(texCoordAttributeID, 2, GL_FLOAT, GL_FALSE, 0, (void*)0));

    /* Enabling color buffer */
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, _colorBufferID));
    GLCHECK(glEnableVertexAttribArray(colorAttributeID));
    GLCHECK(glVertexAttribPointer(colorAttributeID, 4, GL_FLOAT, GL_FALSE, 0, (void*)0));

    GLCHECK(glPointSize(1.f));

    /* Actual drawing */
    GLCHECK(glDrawArrays(GL_POINTS, 0, getNbParticles()));

    /* Don't forget to unbind buffers */
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));


//    window.setActive(true);
//    window.pushGLStates();
//
//    sf::Sprite sprite;
//    sprite.setTexture(_positions[_currentBufferIndex].getTexture());
//    sprite.setPosition(10,10);
//    window.draw(sprite);
//
//    window.display();
//    window.popGLStates();
}
