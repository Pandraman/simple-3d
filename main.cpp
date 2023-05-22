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

Vector2i cameraRotation;
void drawRect(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size, Vector3i color) {
    sf::RectangleShape rect(size);
    rect.setPosition(position);
    rect.setFillColor(sf::Color(color.x, color.y, color.z));

    window.draw(rect);
}


Vector3i rotate(float x, float y, float z, float xrot, float yrot, float zrot) {
    float X = x;
    float Y = y;
    float Z = z;

    //xrot
    Y = (y*sin(xrot)+z*cos(xrot));
    Z = (z*sin(xrot)+y*cos(xrot));

    // yrot
    X = (x*sin(yrot)+Z*cos(yrot));
    Z = (Z*sin(yrot)+x*cos(yrot));

    //zrot
    X = (X*sin(zrot)+Y*cos(zrot));
    Y = (Y*sin(zrot)+X*cos(zrot));

    return Vector3i(X,Y,Z);
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


    void draw(const sf::Texture& texture, sf::RenderWindow& window, float) const {
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
    
    Vector3f rotate(const Vector3f& point, float xrot, float yrot, float zrot) {
        float x = point.x;
        float y = point.y;
        float z = point.z;

        // xrot
        float newY = y * std::cos(xrot) - z * std::sin(xrot);
        float newZ = y * std::sin(xrot) + z * std::cos(xrot);

        // yrot
        float newX = x * std::cos(yrot) + newZ * std::sin(yrot);
        newZ = -x * std::sin(yrot) + newZ * std::cos(yrot);

        // zrot
        float newX2 = newX * std::cos(zrot) - newY * std::sin(zrot);
        float newY2 = newX * std::sin(zrot) + newY * std::cos(zrot);

        return Vector3f(newX2, newY2, newZ);
    }

    void rotate(float xrot, float yrot, float zrot) {
        for (auto& face : faces) {
            for (auto& position : face.positions) {
                position = rotate(position, xrot, yrot, zrot);
            }
        }
    }

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
    
    void draw(const sf::Vector3f& cameraPosition, sf::Texture& texture, sf::RenderWindow& window) {
        // Calculate the center of each face and face normals
        
        sf::Vector2u windowSize = window.getSize();
        float halfWidth = windowSize.x / 2.0f;
        float halfHeight = windowSize.y / 2.0f;
        sf::View view(sf::FloatRect(-halfWidth, -halfHeight, windowSize.x, windowSize.y));
        window.setView(view);
        
        for (auto& face : faces) {
            face.calculateCenter();
        }

        // Sort the faces by distance from camera position
        std::sort(faces.begin(), faces.end(), [&cameraPosition](const Face& a, const Face& b) {
            sf::Vector3f diffA = a.center - cameraPosition;
            sf::Vector3f diffB = b.center - cameraPosition;
            float distA = std::sqrt(diffA.x * diffA.x + diffA.y * diffA.y + diffA.z * diffA.z);
            float distB = std::sqrt(diffB.x * diffB.x + diffB.y * diffB.y + diffB.z * diffB.z);
            return distA > distB; // Sort in descending order (farther faces first)
        });


        for (auto& face : faces) {
        for (auto& position : face.positions) {
            // Apply camera rotation around the y-axis (yaw)
            float x = position.x - cameraPosition.x;
            float z = position.z - cameraPosition.z;
            float rotatedX = x * std::cos(cameraRotation.y) - z * std::sin(cameraRotation.y);
            float rotatedZ = x * std::sin(cameraRotation.y) + z * std::cos(cameraRotation.y);
            position.x = rotatedX + cameraPosition.x;
            position.z = rotatedZ + cameraPosition.z;
            
            // Apply camera rotation around the x-axis (pitch)
            float y = position.y - cameraPosition.y;
            float rotatedY = y * std::cos(cameraRotation.x) - rotatedZ * std::sin(cameraRotation.x);
            float rotatedZ2 = y * std::sin(cameraRotation.x) + rotatedZ * std::cos(cameraRotation.x);
            position.y = rotatedY + cameraPosition.y;
            position.z = rotatedZ2 + cameraPosition.z;
        }
    }


        // Draw the sorted faces
        for (const auto& face : faces) {
        std::vector<sf::Vector2f> vertices;

        for (const auto& position : face.positions) {
            sf::Vector3f modifiedPosition = position - cameraPosition;
            modifiedPosition.z *= -1;
            sf::Vector2f position2D = face.to2D(modifiedPosition);
            
            vertices.push_back(position2D);
        }

        // Draw the face with the converted 2D vertices
        drawTexturedShape(vertices, face.texCoords, texture, window);
        }
    }
    
};




sf::Vector3f cameraPosition(0.0f, 0.0f, 0.0f);
sf::Vector2i prevMousePos(0, 0);
sf::Vector2i mouseDelta(0, 0);

void updateCameraRotation(const sf::Vector2i& mouseDelta) {
    float rotationSpeed = 0.1f;

    // Update camera rotation based on mouse movement
    float yaw = mouseDelta.x * rotationSpeed;
    float pitch = mouseDelta.y * rotationSpeed;

    // Apply rotation to camera
    cameraRotation.x += pitch/20;
    cameraRotation.y += yaw/20;

    // Clamp pitch to avoid flipping upside down
    constexpr float maxPitch = 89.0f;
    constexpr float minPitch = -89.0f;
}





int main()
{
    
    //SETUP

    
    bool isWKeyPressed = false;
    bool isAKeyPressed = false;
    bool isSKeyPressed = false;
    bool isDKeyPressed = false;
    bool isSpaceKeyPressed = false;
    bool isShiftKeyPressed = false;
    



    Model model;
    model.loadFromObj("suzanne.obj");
    cout << model.isLoaded() << endl;
    Texture text;
    float angleY = 45; // Rotation angle in degrees on the Y axis
    float angleX = 45; // Rotation angle in degrees on the X axis

    float radiansY = angleY * (3.14159f / 180.0f); // Convert angle to radians
    float radiansX = angleX * (3.14159f / 180.0f); // Convert angle to radians

    text.loadFromFile("D:\\Desktop/Suzanne/monkey.png");
    sf::Vector3f cameraPosition(0.0f, 0.0f, -10.0f); // Example camera position

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::W) {
                    isWKeyPressed = true;
                }
                else if (event.key.code == sf::Keyboard::A) {
                    isAKeyPressed = true;
                }
                else if (event.key.code == sf::Keyboard::S) {
                    isSKeyPressed = true;
                }
                else if (event.key.code == sf::Keyboard::D) {
                    isDKeyPressed = true;
                }
                else if (event.key.code == sf::Keyboard::Space) {
                    isSpaceKeyPressed = true;
                }
                else if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
                    isShiftKeyPressed = true;
                }
            }
            else if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::W) {
                    isWKeyPressed = false;
                }
                else if (event.key.code == sf::Keyboard::A) {
                    isAKeyPressed = false;
                }
                else if (event.key.code == sf::Keyboard::S) {
                    isSKeyPressed = false;
                }
                else if (event.key.code == sf::Keyboard::D) {
                    isDKeyPressed = false;
                }
                else if (event.key.code == sf::Keyboard::Space) {
                    isSpaceKeyPressed = false;
                }
                else if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
                    isShiftKeyPressed = false;
                }
            }
            else if (event.type == sf::Event::MouseMoved) {
                // Calculate mouse delta based on previous position
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                mouseDelta = mousePos - prevMousePos;
                updateCameraRotation(mouseDelta);
                prevMousePos = mousePos;

                // Update camera rotation based on mouse input

                // Store current mouse position as previous position for the next frame
            }
            }
        window.clear();
        


        // Handle the input actions based on the key states
        if (isWKeyPressed) {
            cameraPosition.z += .005;
        }
        if (isAKeyPressed) {
            cameraPosition.x += .005;
        }
        if (isSKeyPressed) {
            cameraPosition.z -= .005;
        }
        if (isDKeyPressed) {
            cameraPosition.x -= .005;
        }
        if (isSpaceKeyPressed) {
            cameraPosition.y += .005;
        }
        if (isShiftKeyPressed) {
            cameraPosition.y -= .005;
        }
        
       
       
       
        model.draw(cameraPosition, text, window);
        //std::vector<Vector2f> verts = {a.To2D(), b.To2D(), d.To2D(), c.To2D()};
        //std::vector<Vector2f> tex = {Vector2f(0,0),Vector2f(1,0),Vector2f(1,1),Vector2f(0,1)};
        //sf::Texture Tex;
        //Tex.loadFromFile("D:/Desktop/Suzanne/monkey.png");
        //drawTexturedShape(verts, tex, Tex, window);
        
        mouseDelta = Vector2i(0,0);
    

        
        


        window.display();
    }

    return 0;
}