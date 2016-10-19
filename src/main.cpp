#include <cstdlib>
#include <cmath>
#include <iostream>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics.hpp>

#include <GL/glew.h>

#include "Particles.hpp"
#include "Camera.hpp"

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Particles", sf::Style::Default);
    std::cout << "openGL version:" << window.getSettings().majorVersion << "." << window.getSettings().minorVersion << std::endl << std::endl;
    if (window.getSettings().majorVersion < 3) {
        std::cerr << "This program requires OpenGL 3+." << std::endl << std::endl;
        return EXIT_FAILURE;
    }

    //window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    Camera camera(window.getSize().x, window.getSize().y,
                  glm::vec2(0.f,0.f), 1.f);
    float cameraSpeed = 300.f;
    float cameraZoomSpeed = 3.f;

    glewInit();

    Particles particles("rc/pic.bmp");

    float total = 0.f;
    int loops = 0;
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                break;
                case sf::Event::Resized:
                    glViewport(0, 0, event.size.width, event.size.height);
                    camera.setScreenSize(window.getSize().x, window.getSize().y);
                break;
                case sf::Event::KeyReleased:
                    if (event.key.code == sf::Keyboard::R) {
                        particles.initialize();
                    }
                break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        particles.setMagnetState(true);
                    }
                break;
                case sf::Event::MouseButtonReleased:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        particles.setMagnetState(false);
                    }
                break;
                default:
                    break;
            }
        }

        {
            sf::Vector2i movement;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
                movement.y++;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
                movement.y--;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))
                movement.x--;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
                movement.x++;

            if (movement.x != 0 || movement.y != 0)
                camera.moveInPixels(glm::normalize(glm::vec2(movement.x,movement.y)) * cameraSpeed * clock.getElapsedTime().asSeconds());
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            camera.zoom( pow(1.f/cameraZoomSpeed, clock.getElapsedTime().asSeconds()) );
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))
            camera.zoom( pow(cameraZoomSpeed, clock.getElapsedTime().asSeconds()) );

        particles.setMagnetPosition(camera.pixelToCoords(sf::Mouse::getPosition(window)));
        particles.computeNewPositions( clock.getElapsedTime());
        clock.restart();

        particles.draw(window, camera);

        window.display();

       // std::cout << 1.f / clock.getElapsedTime().asSeconds() << std::endl;
        ++loops;
        total += clock.getElapsedTime().asSeconds();
    }

    std::cout << "average fps: " << static_cast<float>(loops) / total << std::endl;

    return EXIT_SUCCESS;
}
