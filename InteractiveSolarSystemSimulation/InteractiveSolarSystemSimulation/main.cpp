#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)

#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>
#include "stb_image.h"

const float PI = 3.14159265358979323846f;

// Shaders
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"layout (location = 2) in vec2 aTexCoords;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform mat3 normalMatrix;\n"
"out vec3 FragPos;\n"
"out vec3 Normal;\n"
"out vec2 TexCoords;\n"
"void main()\n"
"{\n"
"   FragPos = vec3(model * vec4(aPos, 1.0));\n"
"   Normal = normalMatrix * aNormal;\n"
"   TexCoords = aTexCoords;\n"
"   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"in vec2 TexCoords;\n"
//"uniform vec3 objectColor;\n"
"uniform sampler2D texture_diffuse;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 viewPos;\n"
"void main()\n"
"{\n"
"   vec3 objectColor = texture(texture_diffuse, TexCoords).rgb;\n"
"   // Ambient\n"
"   float ambientStrength = 0.1;\n"
"   vec3 ambient = ambientStrength * lightColor;\n"
"   // Diffuse\n"
"   vec3 norm = normalize(Normal);\n"
"   vec3 lightDir = normalize(lightPos - FragPos);\n"
"   float diff = max(dot(norm, lightDir), 0.0);\n"
"   vec3 diffuse = diff * lightColor;\n"
"   // Specular\n"
"   float specularStrength = 0.5;\n"
"   vec3 viewDir = normalize(viewPos - FragPos);\n"
"   vec3 reflectDir = reflect(-lightDir, norm);\n"
"   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
"   vec3 specular = specularStrength * spec * lightColor;\n"
"   vec3 result = (ambient + diffuse + specular) * objectColor;\n"
"   FragColor = vec4(result, 1.0);\n"
"}\n\0";

// Shape types
enum ShapeType {SPHERE};

// 3D Shape structure
struct Shape3D {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    // Global transformations
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 color;

    // Local transformations
    glm::vec3 localPosition;
    glm::vec3 localRotation;
    glm::vec3 localScale;

    float animationPhase;
    ShapeType type;

    Shape3D() : VAO(0), VBO(0), EBO(0), position(0.0f), rotation(0.0f),
        scale(1.0f), color(1.0f), localPosition(0.0f),
        localRotation(0.0f), localScale(1.0f), animationPhase(0.0f),
        type(SPHERE) {
    }
};

// Forward declarations
void createSphere(Shape3D& shape);
void setupShapeBuffers(Shape3D& shape);
void initializeShapes();
void updateOrbitAnimation(float deltaTime);
void updateHierarchicalAnimation(float deltaTime);
void updateRotationAnimation(float deltaTime);
void drawGrid(unsigned int program);
void drawOrigin(unsigned int program);
void displayInfo();
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int compileShader(unsigned int type, const char* source);
glm::mat4 createTransform3D(const Shape3D& shape);
glm::mat4 createLocalTransform3D(const Shape3D& shape);

// Global variables
std::vector<Shape3D> shapes;
int currentShape = 0;
bool showGrid = true;
bool showOrigin = true;
bool orbitAnimation = false;
bool hierarchicalAnimation = false;
bool rotationAnimation = false;
float globalTime = 0.0f;

enum TransformMode { GLOBAL_3D, LOCAL_3D, HIERARCHICAL_3D };
TransformMode currentMode = GLOBAL_3D;

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 1.0f;
float fov = 45.0f;

// Lighting
glm::vec3 lightPos = glm::vec3(3.0f, 3.0f, 3.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

const float MOVE_SPEED = 0.1f;
const float ROTATE_SPEED = 2.0f;
const float SCALE_SPEED = 0.05f;

// Utility functions
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed: " << infoLog << std::endl;
        return 0;
    }
    return shader;
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;

    // Flips the texture on the Y axis so the image isn't upside down on the sphere
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Texture parameters for wrapping and filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Matrix calculations
glm::mat4 createTransform3D(const Shape3D& shape) {
    glm::mat4 model = glm::mat4(1.0f);

    // Apply transformations in T * R * S order
    model = glm::translate(model, shape.position);
    model = glm::rotate(model, glm::radians(shape.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(shape.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(shape.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, shape.scale);

    return model;
}

glm::mat4 createLocalTransform3D(const Shape3D& shape) {
    glm::mat4 localModel = glm::mat4(1.0f);

    localModel = glm::translate(localModel, shape.localPosition);
    localModel = glm::rotate(localModel, glm::radians(shape.localRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    localModel = glm::rotate(localModel, glm::radians(shape.localRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    localModel = glm::rotate(localModel, glm::radians(shape.localRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    localModel = glm::scale(localModel, shape.localScale);

    return localModel;
}

// Shape creation functions
void createSphere(Shape3D& shape) {
    shape.vertices.clear();
    shape.indices.clear();

    const int stacks = 16;
    const int slices = 32;
    const float radius = 0.5f;

    // Generate vertices
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2 - i * PI / stacks;
        float xy = radius * cos(stackAngle);
        float z = radius * sin(stackAngle);

        for (int j = 0; j <= slices; ++j) {
            float sectorAngle = j * 2 * PI / slices;

            float x = xy * cos(sectorAngle);
            float y = xy * sin(sectorAngle);

            shape.vertices.push_back(x);
            shape.vertices.push_back(y);
            shape.vertices.push_back(z);

            shape.vertices.push_back(x / radius);
            shape.vertices.push_back(y / radius);
            shape.vertices.push_back(z / radius);

            // Texture Coordinates (U, V)
            float u = static_cast<float>(j) / slices;
            float v = static_cast<float>(i) / stacks;
            shape.vertices.push_back(u);
            shape.vertices.push_back(v);
        }
    }

    // Generate indices
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (slices + 1);
        int k2 = k1 + slices + 1;

        for (int j = 0; j < slices; ++j, ++k1, ++k2) {
            if (i != 0) {
                shape.indices.push_back(static_cast<unsigned int>(k1));
                shape.indices.push_back(static_cast<unsigned int>(k2));
                shape.indices.push_back(static_cast<unsigned int>(k1 + 1));
            }

            if (i != (stacks - 1)) {
                shape.indices.push_back(static_cast<unsigned int>(k1 + 1));
                shape.indices.push_back(static_cast<unsigned int>(k2));
                shape.indices.push_back(static_cast<unsigned int>(k2 + 1));
            }
        }
    }

    shape.type = SPHERE;
}

// OpenGL buffer setup
void setupShapeBuffers(Shape3D& shape) {
    glGenVertexArrays(1, &shape.VAO);
    glGenBuffers(1, &shape.VBO);
    glGenBuffers(1, &shape.EBO);

    glBindVertexArray(shape.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, shape.VBO);
    glBufferData(GL_ARRAY_BUFFER, shape.vertices.size() * sizeof(float),
        shape.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, shape.indices.size() * sizeof(unsigned int),
        shape.indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture Coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

// Initialize all shapes (This is where the planets can be modified)
void initializeShapes() {
    shapes.clear();
    shapes.resize(9);

    // Sun
    createSphere(shapes[0]);
    shapes[0].position = glm::vec3(-15.0f, 0.0f, 0.0f);
    shapes[0].scale = glm::vec3(26.16f); // Scaled for Sun
    setupShapeBuffers(shapes[0]);

    // Mercury
    createSphere(shapes[1]);
    shapes[1].position = glm::vec3(2.0f, 0.3f, 0.0f);
    shapes[1].scale = glm::vec3(0.4f); // Scaled for Mercury
    setupShapeBuffers(shapes[1]);

    // Venus
    createSphere(shapes[2]);
    shapes[2].position = glm::vec3(4.0f, 0.3f, 0.0f);
    shapes[2].scale = glm::vec3(1.15f); // Scaled for Venus
    setupShapeBuffers(shapes[2]);

    // Earth
    createSphere(shapes[3]);
    //shapes[0].color = glm::vec3(0.3f, 0.3f, 1.0f);
    shapes[3].position = glm::vec3(6.0f, 0.3f, 0.0f);
    shapes[3].scale = glm::vec3(1.2f); // Scaled for Earth
    setupShapeBuffers(shapes[3]);

    // Mars
    createSphere(shapes[4]);
    shapes[4].position = glm::vec3(8.0f, 0.3f, 0.0f);
    shapes[4].scale = glm::vec3(0.6f); // Scaled for Mars
    setupShapeBuffers(shapes[4]);

    // Jupiter
    createSphere(shapes[5]);
    shapes[5].position = glm::vec3(14.0f, 0.3f, 0.0f);
    shapes[5].scale = glm::vec3(9.6f); // Scaled for Jupiter
    setupShapeBuffers(shapes[5]);

    // Saturn
    createSphere(shapes[6]);
    shapes[6].position = glm::vec3(20.0f, 0.3f, 0.0f);
    shapes[6].scale = glm::vec3(8.5f); // Scaled for Saturn
    setupShapeBuffers(shapes[6]);

    // Uranus
    createSphere(shapes[7]);
    shapes[7].position = glm::vec3(26.0f, 0.3f, 0.0f);
    shapes[7].scale = glm::vec3(4.8f); // Scaled for Uranus
    setupShapeBuffers(shapes[7]);

    // Neptune
    createSphere(shapes[8]);
    shapes[8].position = glm::vec3(28.0f, 0.3f, 0.0f);
    shapes[8].scale = glm::vec3(4.6f); // Scaled for Neptune
    setupShapeBuffers(shapes[8]);
}

// Animation functions
void updateOrbitAnimation(float deltaTime) {
    if (orbitAnimation) {
        globalTime += deltaTime;
        for (size_t i = 0; i < shapes.size(); ++i) {
            float time = globalTime + shapes[i].animationPhase;
            float radius = 3.0f;
            float speed = 0.5f + static_cast<float>(i) * 0.2f;

            shapes[i].position.x = radius * cos(time * speed);
            shapes[i].position.z = radius * sin(time * speed);
            shapes[i].position.y = sin(time * speed * 2) * 0.5f;

            shapes[i].rotation.y = time * 30.0f * (i + 1);
        }
    }
}

void updateHierarchicalAnimation(float deltaTime) {
    if (hierarchicalAnimation) {
        globalTime += deltaTime;
        float parentRotation = globalTime * 20.0f;

        for (size_t i = 0; i < shapes.size(); ++i) {
            float time = globalTime + shapes[i].animationPhase;

            shapes[i].rotation.y = parentRotation;
            shapes[i].localRotation.x = time * 45.0f * (static_cast<float>(i) + 1.0f);
            shapes[i].localRotation.z = time * 30.0f * (static_cast<float>(i) + 1.0f);

            shapes[i].localPosition.x = 0.5f * sin(time * 2.0f);
            shapes[i].localPosition.z = 0.3f * cos(time * 3.0f);
        }
    }
}

void updateRotationAnimation(float deltaTime) {
    if (rotationAnimation) {
        globalTime += deltaTime;
        for (size_t i = 0; i < shapes.size(); ++i) {
            float time = globalTime + shapes[i].animationPhase;

            shapes[i].rotation.x = time * 45.0f + static_cast<float>(i) * 30.0f;
            shapes[i].rotation.y = time * 60.0f + static_cast<float>(i) * 45.0f;
            shapes[i].rotation.z = time * 30.0f + static_cast<float>(i) * 60.0f;

            float scale = 0.8f + 0.4f * sin(time * 3.0f + static_cast<float>(i));
            shapes[i].scale = glm::vec3(scale);
        }
    }
}

// Grid and coordinate system
void drawGrid(unsigned int program) {
    if (!showGrid) return;

    std::vector<float> gridVertices;

    // Grid lines in XZ plane
    for (int i = -10; i <= 10; ++i) {
        float pos = static_cast<float>(i) * 0.5f;
        // X lines
        gridVertices.insert(gridVertices.end(), { pos, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f });
        gridVertices.insert(gridVertices.end(), { pos, 0.0f, 5.0f, 0.0f, 1.0f, 0.0f });
        // Z lines
        gridVertices.insert(gridVertices.end(), { -5.0f, 0.0f, pos, 0.0f, 1.0f, 0.0f });
        gridVertices.insert(gridVertices.end(), { 5.0f, 0.0f, pos, 0.0f, 1.0f, 0.0f });
    }

    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float),
        gridVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(program, "objectColor"), 0.3f, 0.3f, 0.3f);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(gridVertices.size() / 6));

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
}

void drawOrigin(unsigned int program) {
    if (!showOrigin) return;

    float axes[] = {
        // X-axis (red)
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        // Y-axis (green)
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        0.0f, 2.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        // Z-axis (blue)
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 2.0f,  0.0f, 0.0f, 1.0f
    };

    unsigned int axesVAO, axesVBO;
    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // X-axis (red)
    glUniform3f(glGetUniformLocation(program, "objectColor"), 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Y-axis (green)
    glUniform3f(glGetUniformLocation(program, "objectColor"), 0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_LINES, 2, 2);

    // Z-axis (blue)
    glUniform3f(glGetUniformLocation(program, "objectColor"), 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);

    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);
}

// Display information
void displayInfo() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    std::cout << "=== Interactive Solar System Simulation ===" << std::endl;
    std::cout << "Current Shape: " << currentShape + 1 << "/" << shapes.size() << " (";

    switch (shapes[currentShape].type) {
    case SPHERE: std::cout << "Sphere"; break;
    }

    std::cout << ")" << std::endl;
    std::cout << "Transform Mode: ";

    switch (currentMode) {
    case GLOBAL_3D: std::cout << "GLOBAL"; break;
    case LOCAL_3D: std::cout << "LOCAL"; break;
    case HIERARCHICAL_3D: std::cout << "HIERARCHICAL"; break;
    }

    std::cout << std::endl;

    const auto& shape = shapes[currentShape];
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n--- CURRENT SHAPE TRANSFORMATIONS ---" << std::endl;
    std::cout << "Position: (" << shape.position.x << ", " << shape.position.y << ", " << shape.position.z << ")" << std::endl;
    std::cout << "Rotation: (" << shape.rotation.x << "°, " << shape.rotation.y << "°, " << shape.rotation.z << "°)" << std::endl;
    std::cout << "Scale: (" << shape.scale.x << ", " << shape.scale.y << ", " << shape.scale.z << ")" << std::endl;
    std::cout << "Local Position: (" << shape.localPosition.x << ", " << shape.localPosition.y << ", " << shape.localPosition.z << ")" << std::endl;
    std::cout << "Local Rotation: (" << shape.localRotation.x << "°, " << shape.localRotation.y << "°, " << shape.localRotation.z << "°)" << std::endl;
    std::cout << "Local Scale: (" << shape.localScale.x << ", " << shape.localScale.y << ", " << shape.localScale.z << ")" << std::endl;

    std::cout << "\n--- CAMERA ---" << std::endl;
    std::cout << "Position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
    std::cout << "FOV: " << fov << "°" << std::endl;

    std::cout << "\n--- ANIMATION ---" << std::endl;
    std::cout << "Orbit Animation: " << (orbitAnimation ? "ON" : "OFF") << std::endl;
    std::cout << "Hierarchical Animation: " << (hierarchicalAnimation ? "ON" : "OFF") << std::endl;
    std::cout << "Rotation Animation: " << (rotationAnimation ? "ON" : "OFF") << std::endl;

    std::cout << "\n--- DISPLAY ---" << std::endl;
    std::cout << "Grid: " << (showGrid ? "VISIBLE" : "HIDDEN") << std::endl;
    std::cout << "Origin: " << (showOrigin ? "VISIBLE" : "HIDDEN") << std::endl;

    std::cout << "\n--- CONTROLS ---" << std::endl;
    std::cout << "TAB: Switch shapes | M: Change transform mode" << std::endl;
    std::cout << "WASD: Move XZ | RF: Move Y | QE: Rotate Y | ZC: Rotate X | VB: Rotate Z" << std::endl;
    std::cout << "TY: Scale | Arrow Keys: Camera movement | +/-: FOV" << std::endl;
    std::cout << "1: Orbit animation | 2: Hierarchical animation | 3: Rotation animation" << std::endl;
    std::cout << "G: Toggle grid | O: Toggle origin" << std::endl;
    std::cout << "0: Reset current shape | ESC: Exit" << std::endl;
    std::cout << "=======================================================" << std::endl;
}

// Input handling
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        Shape3D& shape = shapes[currentShape];

        switch (key) {
        case GLFW_KEY_TAB:
            if (action == GLFW_PRESS) {
                currentShape = (currentShape + 1) % shapes.size();
            }
            break;

        case GLFW_KEY_M:
            if (action == GLFW_PRESS) {
                currentMode = static_cast<TransformMode>((currentMode + 1) % 3);
            }
            break;

            // Movement controls
        case GLFW_KEY_W:
            if (currentMode == LOCAL_3D) shape.localPosition.z -= MOVE_SPEED;
            else shape.position.z -= MOVE_SPEED;
            break;
        case GLFW_KEY_S:
            if (currentMode == LOCAL_3D) shape.localPosition.z += MOVE_SPEED;
            else shape.position.z += MOVE_SPEED;
            break;
        case GLFW_KEY_A:
            if (currentMode == LOCAL_3D) shape.localPosition.x -= MOVE_SPEED;
            else shape.position.x -= MOVE_SPEED;
            break;
        case GLFW_KEY_D:
            if (currentMode == LOCAL_3D) shape.localPosition.x += MOVE_SPEED;
            else shape.position.x += MOVE_SPEED;
            break;
        case GLFW_KEY_R:
            if (currentMode == LOCAL_3D) shape.localPosition.y += MOVE_SPEED;
            else shape.position.y += MOVE_SPEED;
            break;
        case GLFW_KEY_F:
            if (currentMode == LOCAL_3D) shape.localPosition.y -= MOVE_SPEED;
            else shape.position.y -= MOVE_SPEED;
            break;

            // Rotation controls
        case GLFW_KEY_Q:
            if (currentMode == LOCAL_3D) shape.localRotation.y += ROTATE_SPEED;
            else shape.rotation.y += ROTATE_SPEED;
            break;
        case GLFW_KEY_E:
            if (currentMode == LOCAL_3D) shape.localRotation.y -= ROTATE_SPEED;
            else shape.rotation.y -= ROTATE_SPEED;
            break;
        case GLFW_KEY_Z:
            if (currentMode == LOCAL_3D) shape.localRotation.x += ROTATE_SPEED;
            else shape.rotation.x += ROTATE_SPEED;
            break;
        case GLFW_KEY_C:
            if (currentMode == LOCAL_3D) shape.localRotation.x -= ROTATE_SPEED;
            else shape.rotation.x -= ROTATE_SPEED;
            break;
        case GLFW_KEY_V:
            if (currentMode == LOCAL_3D) shape.localRotation.z += ROTATE_SPEED;
            else shape.rotation.z += ROTATE_SPEED;
            break;
        case GLFW_KEY_B:
            if (currentMode == LOCAL_3D) shape.localRotation.z -= ROTATE_SPEED;
            else shape.rotation.z -= ROTATE_SPEED;
            break;

            // Scale controls
        case GLFW_KEY_T:
            if (currentMode == LOCAL_3D) shape.localScale += SCALE_SPEED;
            else shape.scale += SCALE_SPEED;
            break;
        case GLFW_KEY_Y:
            if (currentMode == LOCAL_3D) {
                shape.localScale -= SCALE_SPEED;
                if (shape.localScale.x < 0.1f) shape.localScale = glm::vec3(0.1f);
            }
            else {
                shape.scale -= SCALE_SPEED;
                if (shape.scale.x < 0.1f) shape.scale = glm::vec3(0.1f);
            }
            break;

            // Camera controls
        case GLFW_KEY_UP:
            cameraPos += cameraSpeed * cameraFront;
            break;
        case GLFW_KEY_DOWN:
            cameraPos -= cameraSpeed * cameraFront;
            break;
        case GLFW_KEY_LEFT:
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            break;
        case GLFW_KEY_RIGHT:
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            break;

            // FOV controls
        case GLFW_KEY_KP_ADD:
        case GLFW_KEY_EQUAL:
            fov -= 2.0f;
            if (fov < 10.0f) fov = 10.0f;
            break;
        case GLFW_KEY_KP_SUBTRACT:
        case GLFW_KEY_MINUS:
            fov += 2.0f;
            if (fov > 120.0f) fov = 120.0f;
            break;

            // Animation controls
        case GLFW_KEY_1:
            if (action == GLFW_PRESS) {
                orbitAnimation = !orbitAnimation;
                if (orbitAnimation) {
                    hierarchicalAnimation = false;
                    rotationAnimation = false;
                }
            }
            break;
        case GLFW_KEY_2:
            if (action == GLFW_PRESS) {
                hierarchicalAnimation = !hierarchicalAnimation;
                if (hierarchicalAnimation) {
                    orbitAnimation = false;
                    rotationAnimation = false;
                }
            }
            break;
        case GLFW_KEY_3:
            if (action == GLFW_PRESS) {
                rotationAnimation = !rotationAnimation;
                if (rotationAnimation) {
                    orbitAnimation = false;
                    hierarchicalAnimation = false;
                }
            }
            break;

            // Display toggles
        case GLFW_KEY_G:
            if (action == GLFW_PRESS) showGrid = !showGrid;
            break;
        case GLFW_KEY_O:
            if (action == GLFW_PRESS) showOrigin = !showOrigin;
            break;

            // Reset
        case GLFW_KEY_0:
            shape.position = glm::vec3(0.0f);
            shape.rotation = glm::vec3(0.0f);
            shape.scale = glm::vec3(1.0f);
            shape.localPosition = glm::vec3(0.0f);
            shape.localRotation = glm::vec3(0.0f);
            shape.localScale = glm::vec3(1.0f);
            orbitAnimation = false;
            hierarchicalAnimation = false;
            rotationAnimation = false;
            break;

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        }
    }
}

// Main function
int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "Lab06 - 3D Graphics & Homogeneous Coordinates", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (glewInit() != GLEW_OK) return -1;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    // Setup shaders
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int program = glCreateProgram();

    // Planet Textures
    unsigned int sunTexture = loadTexture("sun.jpg");
    unsigned int mercuryTexture = loadTexture("mercury.jpg");
    unsigned int venusTexture = loadTexture("venus.jpg");
    unsigned int earthTexture = loadTexture("earth.jpg");
    unsigned int marsTexture = loadTexture("mars.jpg");
    unsigned int jupiterTexture = loadTexture("jupiter.jpg");
    unsigned int saturnTexture = loadTexture("saturn.jpg");
    unsigned int uranusTexture = loadTexture("uranus.jpg");
    unsigned int neptuneTexture = loadTexture("neptune.jpg");

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Initialize shapes
    initializeShapes();

    // Set animation phases
    for (size_t i = 0; i < shapes.size(); ++i) {
        shapes[i].animationPhase = static_cast<float>(i) * PI / 3.0f;
    }

    // Render loop
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update animations
        updateOrbitAnimation(deltaTime);
        updateHierarchicalAnimation(deltaTime);
        updateRotationAnimation(deltaTime);

        displayInfo();

        // Clear screen
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        // Set up matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), 1200.0f / 900.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Set lighting uniforms
        glUniform3fv(glGetUniformLocation(program, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(program, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(cameraPos));

        // Draw grid and origin
        drawGrid(program);
        drawOrigin(program);

        // Draw all shapes
        for (size_t i = 0; i < shapes.size(); ++i) {
            const auto& shape = shapes[i];

            glActiveTexture(GL_TEXTURE0);
            // Here is where it selects the right texture for each planet
            if (i == 0) {
                glBindTexture(GL_TEXTURE_2D, sunTexture);
            }
            else if (i == 1) {
                glBindTexture(GL_TEXTURE_2D, mercuryTexture);
            }
            else if (i == 2) {
                glBindTexture(GL_TEXTURE_2D, venusTexture);
            }
            else if (i == 3) {
                glBindTexture(GL_TEXTURE_2D, earthTexture);
            }
            else if (i == 4) {
                glBindTexture(GL_TEXTURE_2D, marsTexture);
            }
            else if (i == 5) {
                glBindTexture(GL_TEXTURE_2D, jupiterTexture);
            }
            else if (i == 6) {
                glBindTexture(GL_TEXTURE_2D, saturnTexture);
            }
            else if (i == 7) {
                glBindTexture(GL_TEXTURE_2D, uranusTexture);
            }
            else if (i == 8) {
                glBindTexture(GL_TEXTURE_2D, neptuneTexture);
            }
            glUniform1i(glGetUniformLocation(program, "texture_diffuse"), 0);

            glm::mat4 globalTransform = createTransform3D(shape);
            glm::mat4 localTransform = createLocalTransform3D(shape);
            glm::mat4 finalTransform;

            if (currentMode == HIERARCHICAL_3D) {
                finalTransform = globalTransform * localTransform;
            }
            else if (currentMode == LOCAL_3D && i == currentShape) {
                finalTransform = localTransform;
            }
            else {
                finalTransform = globalTransform;
            }

            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(finalTransform));

            // Calculate normal matrix
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(finalTransform)));
            glUniformMatrix3fv(glGetUniformLocation(program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

            if (i == currentShape) {
                glm::vec3 highlightColor = shape.color * 1.5f;
                glUniform3f(glGetUniformLocation(program, "objectColor"),
                    highlightColor.r, highlightColor.g, highlightColor.b);
            }
            else {
                glUniform3f(glGetUniformLocation(program, "objectColor"),
                    shape.color.r, shape.color.g, shape.color.b);
            }

            glBindVertexArray(shape.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(shape.indices.size()), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (auto& shape : shapes) {
        glDeleteVertexArrays(1, &shape.VAO);
        glDeleteBuffers(1, &shape.VBO);
        glDeleteBuffers(1, &shape.EBO);
    }

    glfwTerminate();
    return 0;
}
