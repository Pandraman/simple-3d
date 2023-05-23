#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

using namespace sf;
using namespace std;
void drawTexturedShape(const std::vector<sf::Vector2f>& vertices, const std::vector<sf::Vector2f>& texCoords, const sf::Texture& texture, sf::RenderWindow& window)
{
    // Create a vertex array
    sf::VertexArray vertexArray(sf::Quads);

    // Ensure the vertices and texture coordinates have the same size
    if (vertices.size() != texCoords.size())
    {
        // Handle the error or return if the sizes don't match
        return;
    }

    // Get the size of the texture
    sf::Vector2u textureSize = texture.getSize();

    // Populate the vertex array with vertices and converted texture coordinates
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        // Create a vertex with position and converted texture coordinates
        sf::Vertex vertex;
        vertex.position = vertices[i];
        vertex.texCoords.x = texCoords[i].x * static_cast<float>(textureSize.x);
        vertex.texCoords.y = texCoords[i].y * static_cast<float>(textureSize.y);

        // Add the vertex to the vertex array
        vertexArray.append(vertex);
    }

    // Set the texture for the shape using RenderStates
    sf::RenderStates renderStates;
    renderStates.texture = &texture;

    // Draw the textured shape
    window.draw(vertexArray, renderStates);
}

RenderWindow window(VideoMode(1080,720),"Window");

Vector2f cameraRotation;
Vector3f cameraPosition;
void drawRect(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size, Vector3i color) {
    sf::RectangleShape rect(size);
    rect.setPosition(position);
    rect.setFillColor(sf::Color(color.x, color.y, color.z));

    window.draw(rect);
}


Vector3f rotate_y(const Vector3f& pos, const Vector3f& b, float xrot) {
    Vector3f c;
    float cosAngle = cos(xrot);
    float sinAngle = sin(xrot);

    c.x = (pos.x - b.x) * cosAngle + (pos.z - b.z) * sinAngle + b.x;
    c.z = (pos.z - b.z) * cosAngle - (pos.x - b.x) * sinAngle + b.z;
    c.y = pos.y;

    return c;
}










class Face {
public:
    std::vector<sf::Vector3f> positions;
    std::vector<sf::Vector2f> texCoords;
    Vector3f center;

    void calculateCenter() {
    center = Vector3f(0, 0, 0);
    for (const auto& pos : positions) {
        center += pos;
    }
    center /= static_cast<float>(positions.size()); // Convert the size to float for division
}


    void draw(const sf::Texture& texture, sf::RenderWindow& window) const {
        std::vector<sf::Vector2f> vertices;

        for (auto& position : positions) {
            sf::Vector2f position2D = to2D(position);
            vertices.push_back(position2D);
        }

        drawTexturedShape(vertices, texCoords, texture, window);
    }

    sf::Vector2f to2D(const sf::Vector3f& position) const {
        return sf::Vector2f(position.x * (300 / position.z), position.y * (300 / position.z));
    }
};

class Model {
public:
    
    std::vector<Face> faces;
    std::vector<sf::Vector3f> vertices;
    std::vector<sf::Vector2f> texCoords;
    
    bool isLoaded() const {
        return !faces.empty();
    }
    sf::Vector2f to2D(const sf::Vector3f& position) const {
        return sf::Vector2f(position.x * (300 / position.z), position.y * (300 / position.z));
    }
    void loadFromObj(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open OBJ file: " << filename << std::endl;
            return;
        }

        faces.clear();

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;

            if (keyword == "v") {
                sf::Vector3f position;
                iss >> position.x >> position.y >> position.z;
                vertices.push_back(position);
            } else if (keyword == "vt") {
                sf::Vector2f texCoord;
                iss >> texCoord.x >> texCoord.y;
                texCoord.y = 1 - texCoord.y; // Flip the y-coordinate
                texCoords.push_back(texCoord);            
            } else if (keyword == "f") {
                std::string faceVertexStr;
                std::vector<sf::Vector3i> faceVertices;
                while (iss >> faceVertexStr) {
                    std::replace(faceVertexStr.begin(), faceVertexStr.end(), '/', ' ');
                    std::istringstream faceVertexIss(faceVertexStr);
                    sf::Vector3i vertex;
                    faceVertexIss >> vertex.x >> vertex.y >> vertex.z;
                    faceVertices.push_back(vertex);
                }
                if (faceVertices.size() >= 3) {
                    Face face;
                    for (const auto& vertex : faceVertices) {
                        face.positions.push_back(vertices[vertex.x - 1]);
                        face.texCoords.push_back(texCoords[vertex.y - 1]);
                    }
                    face.calculateCenter();
                    faces.push_back(face);
                }
            }
        }

        file.close();
    }
    
    void draw(const sf::Vector3f& cameraPosition, const sf::Vector2f& cameraRotation, sf::Texture& texture, sf::RenderWindow& window) {
        sf::Vector2u windowSize = window.getSize();
        float halfWidth = windowSize.x / 2.0f;
        float halfHeight = windowSize.y / 2.0f;
        sf::View view(sf::FloatRect(-halfWidth, -halfHeight, windowSize.x, windowSize.y));
        window.setView(view);

        std::vector<Face> visibleFaces;

        for (const auto& face : faces) {
            std::vector<sf::Vector3f> modified;
            bool isVisible = false;

            for (const auto& vertex : face.positions) {
                sf::Vector3f pos = vertex;
                sf::Vector3f mod_pos = rotate_y(pos, cameraPosition, cameraRotation.x) - cameraPosition;

                // Check if the vertex is in front of the camera
                sf::Vector3f diff = mod_pos - cameraPosition;
                float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
                if (distance > 1.f && diff.z < 0) {
                    isVisible = true;
                }

                modified.push_back(mod_pos);
            }

            if (isVisible) {
                Face visibleFace;
                visibleFace.positions = modified;
                visibleFace.texCoords = face.texCoords;
                visibleFace.calculateCenter();
                visibleFaces.push_back(visibleFace);
            }
        }

        // Sort the visible faces by distance from camera position
        std::sort(visibleFaces.begin(), visibleFaces.end(), [&cameraPosition](const Face& a, const Face& b) {
            sf::Vector3f diffA = a.center - cameraPosition;
            sf::Vector3f diffB = b.center - cameraPosition;
            float distA = std::sqrt(diffA.x * diffA.x + diffA.y * diffA.y + diffA.z * diffA.z);
            float distB = std::sqrt(diffB.x * diffB.x + diffB.y * diffB.y + diffB.z * diffB.z);
            return distA > distB; // Sort in descending order (farther faces first)
        });

        for (const auto& face : visibleFaces) {
            face.draw(texture, window);
        }
    }
    
};


int main()
{
    
    //SETUP

    Model model;
    model.loadFromObj("Object/suzanne.obj");
    cout << model.isLoaded() << endl;
    Texture text;

    text.loadFromFile("Object/monkey.png");

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            }

    if (Keyboard::isKeyPressed(Keyboard::Key::W)) {
        cameraPosition.z -= .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::S)) {
        cameraPosition.z += .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::A)) {
        cameraPosition.x += .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::D)) {
        cameraPosition.x -= .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::LShift)) {
        cameraPosition.y -= .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::Space)) {
        cameraPosition.y += .01;
    }

    if (Keyboard::isKeyPressed(Keyboard::Key::Up)) {
        cameraRotation.y += .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::Down)) {
        cameraRotation.y -= .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::Left)) {
        cameraRotation.x += .01;
    }
    if (Keyboard::isKeyPressed(Keyboard::Key::Right)) {
        cameraRotation.x -= .01;
    }




        window.clear();





        model.draw(cameraPosition, cameraRotation, text, window);

        window.display();
    }return 0;}