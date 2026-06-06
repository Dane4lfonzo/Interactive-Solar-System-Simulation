#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4267)
using namespace std;

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
"uniform vec3 objectColor;\n"
"uniform sampler2D texture_diffuse;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 viewPos;\n"
"uniform bool isSun;\n"
"uniform bool useTexture;\n"
"void main()\n"
"{\n"
"   vec3 objectColor = texture(texture_diffuse, TexCoords).rgb;\n"
"   if (useTexture) {\n"
"       objectColor = texture(texture_diffuse, TexCoords).rgb;\n"
"   }\n"
"   if (isSun) {\n"
"       FragColor = vec4(objectColor, 1.0);\n" // Sun glows at 100% brightness, no shadows
"   } else {\n"
"   // Ambient\n"
"   float ambientStrength = 0.1;\n"
"   vec3 ambient = ambientStrength * lightColor;\n"
"   // Diffuse\n"
"   vec3 norm = normalize(Normal);\n"
"   //Attenuation\n"
"   vec3 lightDir = normalize(lightPos - FragPos);\n"
"   float distance = length(lightPos - FragPos);\n"
"   float constant = 1.0;\n"
"   float linear = 0.002;\n"
"   float quadratic = 0.00005;\n"
"   float attenuation = 1.0 /"
"   (constant + linear * distance + quadratic * distance * distance);\n"
"   float diff = max(dot(norm, lightDir), 0.0);\n"
"   vec3 diffuse = diff * lightColor * attenuation;;\n"
"   // Specular\n"
"   float specularStrength = 0.5;\n"
"   vec3 viewDir = normalize(viewPos - FragPos);\n"
"   vec3 reflectDir = reflect(-lightDir, norm);\n"
"   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
"   vec3 specular = specularStrength * spec * lightColor * attenuation;\n"
"   vec3 result = (ambient + diffuse + specular) * objectColor;\n"
"   FragColor = vec4(result, 1.0);\n"
"   }\n"
"   }\0";

const char* skyboxVertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"out vec3 TexCoords;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    TexCoords = aPos;\n"
// Remove translation from the view matrix so the skybox stays centered on camera
"    mat4 staticView = mat4(mat3(view));\n"
"    vec4 pos = projection * staticView * vec4(aPos, 1.0);\n"
// Forces depth value to 1.0 (maximum depth) so it renders behind objects
"    gl_Position = pos.xyww;\n"
"}\0";

const char* skyboxFragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 TexCoords;\n"
"uniform samplerCube skybox;\n" // samplerCube instead of sampler2D
"void main()\n"
"{\n"
"    FragColor = texture(skybox, TexCoords);\n"
"}\0";

// Shape types
enum ShapeType { SPHERE };

enum CameraMode { FREECAM, TRACKING };
CameraMode currentCameraMode = FREECAM;
glm::vec3 trackingOffset = glm::vec3(0.0f, 5.0f, 10.0f);

// 3D Shape structure
struct Shape3D {
    vector<float> vertices;
    vector<unsigned int> indices;
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

    // Moon Properties
    bool isMoon = false;
    size_t parentPlanetIndex = 0;
    float moonOrbitRadius = 0.0f;
    float moonOrbitSpeed = 0.0f;

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
void setupSkybox();
void updateOrbitAnimation(float deltaTime);
void drawGrid(unsigned int program);
void drawOrigin(unsigned int program);
void displayInfo();
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int compileShader(unsigned int type, const char* source);
glm::mat4 createTransform3D(const Shape3D& shape);
glm::mat4 createLocalTransform3D(const Shape3D& shape);

// Global variables
vector<Shape3D> shapes;
int currentShape = 0;
bool showGrid = true;
bool showOrigin = true;
bool orbitAnimation = false;
float globalTime = 0.0f;
unsigned int skyboxVAO, skyboxVBO;
unsigned int skyboxTexture;
unsigned int skyboxShader;
float skyboxVertices[] = {
    // Positions          
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
};

struct RingParticle {
    glm::vec3 relativePosition;
    float orbitRadius;
    float orbitSpeed;
    float currentAngle;
};

const int NUM_PARTICLES = 8000;
vector<RingParticle> saturnRings;
unsigned int ringVAO, ringVBO;


// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 direction;

float yaw = -90.0f;     // Initialized to -90.0f so it points straight down the -Z axis
float pitch = 0.0f;     // 0.0f means looking perfectly level at the horizon
bool firstMouse = true;     // Detects mouse
float lastX = 600.0f;       // Center X of 1200 width window
float lastY = 450.0f;       // Center Y of 900 height window

float cameraSpeed = 3.0f;
float fov = 45.0f;
bool mouseEnabled = true;

// Lighting
glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);
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
        cout << "Shader compilation failed: " << infoLog << endl;
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
        cout << "Texture failed to load at path: " << path << endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
            // Target targets are sequential: GL_TEXTURE_CUBE_MAP_POSITIVE_X, NEGATIVE_X, etc.
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            cout << "Cubemap texture failed to load at path: " << faces[i] << endl;
            stbi_image_free(data);
        }
    }

    // Cubemap parameters required for seamless corners
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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
        float xz = radius * cos(stackAngle);
        float y = radius * sin(stackAngle);

        for (int j = 0; j <= slices; ++j) {
            float sectorAngle = j * 2 * PI / slices;

            float x = xz * cos(sectorAngle);
            float z = xz * sin(sectorAngle);

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
    shapes[0].position = glm::vec3(0.0f, 0.0f, 0.0f);
    shapes[0].scale = glm::vec3(26.16f); // Scaled for Sun
    setupShapeBuffers(shapes[0]);

    // Mercury
    createSphere(shapes[1]);
    shapes[1].position = glm::vec3(32.0f, 0.3f, 0.0f);
    shapes[1].scale = glm::vec3(0.2f); // Scaled for Mercury
    setupShapeBuffers(shapes[1]);

    // Venus
    createSphere(shapes[2]);
    shapes[2].position = glm::vec3(40.0f, 0.3f, 0.0f);
    shapes[2].scale = glm::vec3(0.575f); // Scaled for Venus
    setupShapeBuffers(shapes[2]);

    // Earth
    createSphere(shapes[3]);
    shapes[3].position = glm::vec3(50.0f, 0.3f, 0.0f);
    shapes[3].scale = glm::vec3(0.6f); // Scaled for Earth
    setupShapeBuffers(shapes[3]);

    // Mars
    createSphere(shapes[4]);
    shapes[4].position = glm::vec3(60.0f, 0.3f, 0.0f);
    shapes[4].scale = glm::vec3(0.3f); // Scaled for Mars
    setupShapeBuffers(shapes[4]);

    // Jupiter
    createSphere(shapes[5]);
    shapes[5].position = glm::vec3(76.0f, 0.3f, 0.0f);
    shapes[5].scale = glm::vec3(4.8f); // Scaled for Jupiter
    setupShapeBuffers(shapes[5]);

    // Saturn
    createSphere(shapes[6]);
    shapes[6].position = glm::vec3(96.0f, 0.3f, 0.0f);
    shapes[6].scale = glm::vec3(4.25f); // Scaled for Saturn
    setupShapeBuffers(shapes[6]);

    // Uranus
    createSphere(shapes[7]);
    shapes[7].position = glm::vec3(116.0f, 0.3f, 0.0f);
    shapes[7].scale = glm::vec3(2.4f); // Scaled for Uranus
    setupShapeBuffers(shapes[7]);

    // Neptune
    createSphere(shapes[8]);
    shapes[8].position = glm::vec3(136.0f, 0.3f, 0.0f);
    shapes[8].scale = glm::vec3(2.3f); // Scaled for Neptune
    setupShapeBuffers(shapes[8]);


    saturnRings.resize(NUM_PARTICLES);
    float innerRadius = 5.5f;
    float outerRadius = 10.0f;

    for (int i = 0; i < NUM_PARTICLES; ++i) {
        float r = innerRadius + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (outerRadius - innerRadius)));

        float angle = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * PI)));

        saturnRings[i].orbitRadius = r;
        saturnRings[i].currentAngle = angle;

        saturnRings[i].orbitSpeed = 2.0f / sqrt(r);

        saturnRings[i].relativePosition.x = r * cos(angle);
        saturnRings[i].relativePosition.y = 0.0f;
        saturnRings[i].relativePosition.z = r * sin(angle);
    }

    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);

    glBindVertexArray(ringVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);

    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    struct MoonDef {
        size_t parentIndex; // Which planet this moon belongs to
        float orbitRadius;
        float size;
        float speed;
    };

    std::vector<MoonDef> moonDefinitions = {
        {3, 1.5f, 0.15f, 2.0f},   // Earth: Moon
        {4, 0.8f, 0.06f, 2.5f},   // Mars: Phobos
        {4, 1.2f, 0.04f, 1.8f},   // Mars: Deimos
        {5, 6.0f, 0.40f, 1.5f},   // Jupiter: Io
        {5, 9.0f, 0.60f, 0.9f},   // Jupiter: Ganymede
        {5, 11.0f, 0.45f, 0.6f},  // Jupiter: Callisto
        {6, 5.5f, 0.35f, 1.1f},   // Saturn: Titan
        {6, 3.5f, 0.10f, 0.9f},   // Saturn: Rhea
        {6, 1.3f, 0.04f, 0.6f},   // Saturn: Mimas
        {7, 3.5f, 0.32f, 1.3f},   // Uranus: Titania
        {7, 3.5f, 0.30f, 1.7f},   // Uranus: Oberon
        {7, 3.0f, 0.15f, 1.0f},   // Uranus: Umbrial
        {8, 3.2f, 0.25f, 1.2f}    // Neptune: Triton
    };

    // Resize the shapes vector to hold 9 planets + our 10 new moons
    size_t planetCount = shapes.size();
    shapes.resize(planetCount + moonDefinitions.size());

    for (size_t i = 0; i < moonDefinitions.size(); ++i) {
        size_t moonIdx = planetCount + i;
        const auto& def = moonDefinitions[i];

        createSphere(shapes[moonIdx]);
        setupShapeBuffers(shapes[moonIdx]);

        shapes[moonIdx].isMoon = true;
        shapes[moonIdx].parentPlanetIndex = def.parentIndex;
        shapes[moonIdx].moonOrbitRadius = def.orbitRadius;
        shapes[moonIdx].moonOrbitSpeed = def.speed;
        shapes[moonIdx].scale = glm::vec3(def.size);
        shapes[moonIdx].animationPhase = static_cast<float>(i) * (PI / 2.0f); // offset starting angles
    }
}

// Animation functions
void updateOrbitAnimation(float deltaTime) {
    if (orbitAnimation) {
        globalTime += deltaTime;

        // Keep the Sun (index 0) locked at the origin
        shapes[0].position = glm::vec3(0.0f);

        for (size_t i = 1; i < 9; ++i) {
            float time = globalTime + shapes[i].animationPhase;

            float radius = 0.0f;
            float speed = 1.0f;

            switch (i) {
            case 1: radius = 32.0f; speed = 1.20f; break;  // Mercury (Fastest)
            case 2: radius = 40.0f; speed = 0.90f; break;  // Venus
            case 3: radius = 50.0f; speed = 0.75f; break;  // Earth
            case 4: radius = 60.0f; speed = 0.60f; break;  // Mars
            case 5: radius = 76.0f; speed = 0.40f; break;  // Jupiter
            case 6: radius = 96.0f; speed = 0.25f; break;  // Saturn
            case 7: radius = 116.0f; speed = 0.15f; break;  // Uranus
            case 8: radius = 136.0f; speed = 0.08f; break;  // Neptune (Slowest)
            }

            // Apply perfect circular math on the XZ plane
            shapes[i].position.x = radius * cos((time / 4) * speed);
            shapes[i].position.y = 0.3f; // Flat placeholder Y offset
            shapes[i].position.z = radius * sin((time / 4) * speed);
        }
        // Make moons
        for (size_t i = 9; i < shapes.size(); ++i) {
            if (shapes[i].isMoon) {
                float time = globalTime + shapes[i].animationPhase;
                size_t parentIdx = shapes[i].parentPlanetIndex;

                // Calculate position around the parent planet
                float localX = shapes[i].moonOrbitRadius * cos(time * shapes[i].moonOrbitSpeed);
                float localZ = shapes[i].moonOrbitRadius * sin(time * shapes[i].moonOrbitSpeed);

                // Add the parent's current position to get absolute world position
                shapes[i].position.x = shapes[parentIdx].position.x + localX;
                shapes[i].position.y = shapes[parentIdx].position.y; // Keep them on the same plane
                shapes[i].position.z = shapes[parentIdx].position.z + localZ;
            }
        }

        // Axial rotation
        // Planet Rotation speed
        std::vector<float> rotationSpeeds = {
            7.0f,   // Sun
            10.0f,   // Mercury
            -6.0f,   // Venus (retrograde)
            30.0f,   // Earth
            24.0f,   // Mars
            70.0f,   // Jupiter
            60.0f,   // Saturn
            -40.0f,  // Uranus (retrograde-ish)
            45.0f    // Neptune
        };

        // Rotate Planets
        for (size_t i = 0; i < 9; ++i) {
            float time = globalTime + shapes[i].animationPhase;
            // planet self rotation
            shapes[i].rotation.y += rotationSpeeds[i] * deltaTime;
        }

        // Rotate Moons 
        for (size_t i = 9; i < shapes.size(); ++i) {
            shapes[i].rotation.y += 50.0f * deltaTime;
        }
        
    }
}

void updateSaturnRings(float deltaTime) {
    if (orbitAnimation) {
        for (int i = 0; i < NUM_PARTICLES; ++i) {
            saturnRings[i].currentAngle += saturnRings[i].orbitSpeed * deltaTime;

            float r = saturnRings[i].orbitRadius;
            float angle = saturnRings[i].currentAngle;

            saturnRings[i].relativePosition.x = r * cos(angle);
            saturnRings[i].relativePosition.z = r * sin(angle);
        }
    }
}

// Grid and coordinate system
void drawGrid(unsigned int program) {
    if (!showGrid) return;

    vector<float> gridVertices;

    // Grid lines in XZ plane
    for (int i = -30; i <= 30; ++i) {
        float pos = static_cast<float>(i) * 0.5f;
        // X lines
        gridVertices.insert(gridVertices.end(), { pos, 0.0f, -15.0f, 0.0f, 1.0f, 0.0f });
        gridVertices.insert(gridVertices.end(), { pos, 0.0f, 15.0f, 0.0f, 1.0f, 0.0f });
        // Z lines
        gridVertices.insert(gridVertices.end(), { -15.0f, 0.0f, pos, 0.0f, 1.0f, 0.0f });
        gridVertices.insert(gridVertices.end(), { 15.0f, 0.0f, pos, 0.0f, 1.0f, 0.0f });
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

void drawSaturnRings(unsigned int program) {
    glm::vec3 saturnPos = shapes[6].position;

    vector<glm::vec3> globalParticlePositions(NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES; ++i) {
        globalParticlePositions[i] = saturnPos + saturnRings[i].relativePosition;
    }

    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_PARTICLES * sizeof(glm::vec3), globalParticlePositions.data());

    glm::mat4 identityModel = glm::mat4(1.0f); // Position is already computed in world-space
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(identityModel));

    // Give the rings a dusty golden/rock color
    glUniform3f(glGetUniformLocation(program, "objectColor"), 0.85f, 0.75f, 0.6f);
    glUniform1i(glGetUniformLocation(program, "useTexture"), false); // Tell shader not to use texture
    //glUniform1i(glGetUniformLocation(program, "isSun"), false);

    glPointSize(2.0f); // Sets the pixel width of each point
    glBindVertexArray(ringVAO);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
}

void setupSkybox() {
    // 1. Compile Shader
    unsigned int vs = compileShader(GL_VERTEX_SHADER, skyboxVertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, skyboxFragmentShaderSource);
    skyboxShader = glCreateProgram();
    glAttachShader(skyboxShader, vs);
    glAttachShader(skyboxShader, fs);
    glLinkProgram(skyboxShader);

    // 2. Setup Buffers
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 3. Define textures (Order matters!)
    vector<string> faces{
        "NebulaRT.png",  // +X
        "NebulaLF.png",   // -X
        "NebulaUP.png",    // +Y
        "NebulaDN.png", // -Y
        "NebulaFT.png",  // +Z
        "NebulaBK.png"    // -Z
    };
    skyboxTexture = loadCubemap(faces);
}

// Display information
void displayInfo() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    cout << "=== Interactive Solar System Simulation ===" << endl;
    cout << "Current Shape: " << currentShape + 1 << "/9" << " (";

    switch (currentShape) {
    case (0): 
        cout << "SUN";
        break;
    case (1): 
        cout << "MERCURY";
        break;
    case (2):
        cout << "VENUS";
        break;
    case (3):
        cout << "EARTH";
        break;
    case (4):
        cout << "MARS";
        break;
    case (5):
        cout << "JUPITER";
        break;
    case (6):
        cout << "SATURN";
        break;
    case (7):
        cout << "URANUS";
        break;
    case (8):
        cout << "NEPTUNE";
        break;
    }

    cout << ")" << endl;
    cout << "Transform Mode: GLOBAL";

    cout << endl;

    const auto& shape = shapes[currentShape];
    cout << fixed << setprecision(2);

    cout << "\n--- CURRENT SHAPE TRANSFORMATIONS ---" << endl;
    cout << "Position: (" << shape.position.x << ", " << shape.position.y << ", " << shape.position.z << ")" << endl;
    cout << "Rotation: (" << shape.rotation.x << "°, " << shape.rotation.y << "°, " << shape.rotation.z << "°)" << endl;
    cout << "Scale: (" << shape.scale.x << ", " << shape.scale.y << ", " << shape.scale.z << ")" << endl;

    cout << "\n--- CAMERA ---" << endl;
    cout << "Position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << endl;
    cout << "FOV: " << fov << "°" << endl;

    cout << "\n--- ANIMATION ---" << endl;
    cout << "Orbit Animation: " << (orbitAnimation ? "ON" : "OFF") << endl;

    cout << "\n--- DISPLAY ---" << endl;
    cout << "Grid: " << (showGrid ? "VISIBLE" : "HIDDEN") << endl;
    cout << "Origin: " << (showOrigin ? "VISIBLE" : "HIDDEN") << endl;

    cout << "\n--- CONTROLS ---" << endl;
    cout << "P: Toggle Mouse | TAB: Lock Camera to planets | Q: Enable FreeCam " << endl;
    cout << "WASD: Move Camera | JL: Rotate Camera X | IK: Rotate Camera Y" << endl;
    cout << "+/-: FOV" << endl;
    cout << "SPACE: Resume/Pause Animation" << endl;
    cout << "G: Toggle grid" << endl;
    cout << "ESC: Exit" << endl;
    cout << "=======================================================" << endl;
}

// Detect user's mouse input
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (mouseEnabled)
        return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Calculate how far the mouse moved since the last frame
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    // Multiply by a sensitivity setting so it's not too twitchy
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Update global yaw and pitch variables
    yaw += xoffset;
    pitch += yoffset;

    // Safety clamp so the screen doesn't flip upside down
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Recalculate cameraFront immediately based on the new mouse angles
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void toggleMouse(GLFWwindow* window)
{
    mouseEnabled = !mouseEnabled;

    if (mouseEnabled)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Prevent camera jump
        firstMouse = true;
    }
}

// Input handling
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        Shape3D& shape = shapes[currentShape];

        switch (key) {
        case GLFW_KEY_P:
            if (action == GLFW_PRESS) {
                toggleMouse(window);
            }
            break;

        case GLFW_KEY_TAB:
            if (action == GLFW_PRESS) {
                // Keep the camera cycling strictly through the 9 main planets (0 to 8)
                currentShape = (currentShape + 1) % 9;
                currentCameraMode = TRACKING;
            }
            break;

        case GLFW_KEY_Q:
            if (action == GLFW_PRESS) {
                currentCameraMode = FREECAM;
            }
            break;

        // Camera controls
        if (currentCameraMode == FREECAM) {
            case GLFW_KEY_W:
                cameraPos += cameraSpeed * cameraFront;
                break;
            case GLFW_KEY_S:
                cameraPos -= cameraSpeed * cameraFront;
                break;
            case GLFW_KEY_A:
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                break;
            case GLFW_KEY_D:
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
                break;
        }

            // Camera Rotation
        case GLFW_KEY_I: { // tilt up
            pitch += ROTATE_SPEED;
            if (pitch > 89.0f) pitch = 89.0f; // Safety cap to avoid gimbal lock

            // Calculate new front direction immediately
            direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            direction.y = sin(glm::radians(pitch));
            direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(direction);
            break;
        }

        case GLFW_KEY_K: { //tilt down
            pitch -= ROTATE_SPEED;
            if (pitch < -89.0f) pitch = -89.0f; // Safety cap to avoid gimbal lock

            // Calculate new front direction immediately
            direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            direction.y = sin(glm::radians(pitch));
            direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(direction);
            break;
        }

        // Rotate LEFT 
        case GLFW_KEY_J:
            yaw -= ROTATE_SPEED; // Decrement yaw to turn left

            direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            direction.y = sin(glm::radians(pitch));
            direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(direction);
            break;

        // Rotate RIGHT 
        case GLFW_KEY_L:
            yaw += ROTATE_SPEED; // Increment yaw to turn right

            direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            direction.y = sin(glm::radians(pitch));
            direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(direction);
            break;

            // FOV controls
        case GLFW_KEY_EQUAL:
            fov -= 2.0f;
            if (fov < 10.0f) fov = 10.0f;
            break;
        case GLFW_KEY_MINUS:
            fov += 2.0f;
            if (fov > 120.0f) fov = 120.0f;
            break;

            // Animation controls
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS) {
                orbitAnimation = !orbitAnimation;
            }
            break;

            // Display toggles
        case GLFW_KEY_G:
            if (action == GLFW_PRESS) showGrid = !showGrid;
            break;
        case GLFW_KEY_O:
            if (action == GLFW_PRESS) showOrigin = !showOrigin;
            break;

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        }
    }
}

void updateCamera(float deltaTime) {
    if (mouseEnabled)
        return;

    if (currentCameraMode == TRACKING) {
        const Shape3D& targetPlanet = shapes[currentShape];

        float camDistance = 8.0f;

        // Calculate dynamic offset vector using the mouse yaw and pitch angles
        glm::vec3 offset;
        offset.x = camDistance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        offset.y = camDistance * sin(glm::radians(pitch));
        offset.z = camDistance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

        // Position the camera relative to the planet's moving coordinates
        cameraPos = targetPlanet.position + offset;

        // Forces the camera to look straight back at the center of the planet
        cameraFront = glm::normalize(targetPlanet.position - cameraPos);
    }
}

// Main function
int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "Interactive Solar System Simulation", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) return -1;

    // Initialize shapes
    initializeShapes();
    setupSkybox();

    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hides and traps cursor
    glfwSetCursorPosCallback(window, mouse_callback);            // Registers the mouse function
  
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    // Setup shaders
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    int success;
    char infoLog[512];

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "PROGRAM LINK ERROR:\n" << infoLog << endl;
    }

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

    std::vector<unsigned int> planetTextures = {
        sunTexture, mercuryTexture, venusTexture, earthTexture,
        marsTexture, jupiterTexture, saturnTexture, uranusTexture, neptuneTexture
    };

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
        updateSaturnRings(deltaTime);

        // Focus camera on planets when pressing TAB
        updateCamera(deltaTime);

        displayInfo();

        if (!shapes.empty()) {
            lightPos = shapes[0].position; // Securely shadows the sun even when moving!
        }

        // Clear screen
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        // Set up matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), 1200.0f / 900.0f, 0.1f, 1000.0f);

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

            // Make the sun shadeless
            if (i == 0) {
                glUniform1i(glGetUniformLocation(program, "isSun"), true);
            }
            else {
                glUniform1i(glGetUniformLocation(program, "isSun"), false);
            }

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
            else {
                // FALLBACK FOR MOONS: Use the mercury texture as a placeholder moon rock texture
                glBindTexture(GL_TEXTURE_2D, mercuryTexture);
            }
            glUniform1i(glGetUniformLocation(program, "texture_diffuse"), 0);

            // Tell the fragment shader to actively use the texture instead of solid colors
            glUniform1i(glGetUniformLocation(program, "useTexture"), 1);

            drawSaturnRings(program);

            glm::mat4 globalTransform = createTransform3D(shape);
            glm::mat4 finalTransform = globalTransform;
            
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


        glDepthFunc(GL_LEQUAL);

        glUseProgram(skyboxShader);

        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Bind the cubemap
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glUniform1i(glGetUniformLocation(skyboxShader, "skybox"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // Restore default depth test function


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
