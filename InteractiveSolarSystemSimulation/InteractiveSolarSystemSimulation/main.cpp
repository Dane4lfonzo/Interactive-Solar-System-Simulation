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
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
"uniform samplerCube skybox; // ADD THIS UNIFORM\n" // <--- 
"uniform vec3 lightPos;\n"
"uniform vec3 lightPos2;\n"
"uniform vec3 lightPos3;\n"
"uniform vec3 lightPos4;\n"
"uniform vec3 lightPos5;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 viewPos;\n"
"uniform bool isSun;\n"
"uniform bool useTexture;\n"
"uniform bool isBlackHole;\n"
"uniform vec3 blackHoleCenter;\n"
"uniform float bhRadius;\n"
"void main()\n"
"{\n"
"   if (isBlackHole) {\n"
"       vec3 viewDir = normalize(FragPos - viewPos);\n"
"       vec3 toCenter = normalize(blackHoleCenter - FragPos);\n"
"       float alignment = dot(viewDir, toCenter);\n"
"       \n"
"       // 1. Crisp Event Horizon Core\n"
"       if (alignment > 0.995) {\n"
"           FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
"           return;\n"
"       }\n"
"       \n"
"       // 2. High-intensity narrow accretion glow\n"
"       float glowFactor = pow(alignment, 40.0);\n"
"       vec3 accretionGlow = vec3(0.9, 0.4, 0.1) * glowFactor * 2.0;\n"
"       \n"
"       // 3. 3D Gravitational Lensing Warp\n"
"       // We warp the actual 3D view vector instead of 2D coordinates\n"
"       float warp = pow(alignment, 4.0) * 0.5;\n"
"       vec3 warpedViewDir = viewDir + toCenter * warp;\n"
"       \n"
"       // Sample the 6-sided skybox using our bent 3D vector!\n"
"       vec3 sceneColor = texture(skybox, normalize(warpedViewDir)).rgb;\n"
"       \n"
"       FragColor = vec4(sceneColor + accretionGlow, 1.0);\n"
"       return;\n"
"   }\n"
"   vec3 finalColor = texture(texture_diffuse, TexCoords).rgb;\n"
"   if (isSun) {\n"
"       FragColor = vec4(finalColor, 1.0);\n" // Suns glow at 100% brightness
"   } else {\n"
"       vec3 norm = normalize(Normal);\n"
"       vec3 viewDir = normalize(viewPos - FragPos);\n"
"       float ambientStrength = 0.1;\n"
"       vec3 ambient = ambientStrength * lightColor;\n"
"       float constant = 1.0;\n"
"       float linear = 0.002;\n"
"       float quadratic = 0.00005;\n"
"       float specularStrength = 0.5;\n"
"       \n"
"       // === LIGHT SOURCE 1 (Sun 1) ===\n"
"       vec3 lightDir1 = normalize(lightPos - FragPos);\n"
"       float dist1 = length(lightPos - FragPos);\n"
"       float att1 = 1.0 / (constant + linear * dist1 + quadratic * dist1 * dist1);\n"
"       float diff1 = max(dot(norm, lightDir1), 0.0);\n"
"       vec3 diffuse1 = diff1 * lightColor * att1;\n"
"       vec3 reflectDir1 = reflect(-lightDir1, norm);\n"
"       float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), 32);\n"
"       vec3 specular1 = specularStrength * spec1 * lightColor * att1;\n"
"       \n"
"       // === LIGHT SOURCE 2 (Sun 2 / shapes[9]) ===\n"
"       vec3 lightDir2 = normalize(lightPos2 - FragPos);\n"
"       float dist2 = length(lightPos2 - FragPos);\n"
"       float att2 = 1.0 / (constant + linear * dist2 + quadratic * dist2 * dist2);\n"
"       float diff2 = max(dot(norm, lightDir2), 0.0);\n"
"       vec3 diffuse2 = diff2 * lightColor * att2;\n"
"       vec3 reflectDir2 = reflect(-lightDir2, norm);\n"
"       float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), 32);\n"
"       vec3 specular2 = specularStrength * spec2 * lightColor * att2;\n"
"       \n"
"       // === LIGHT SOURCE 3 (Sun 3 / shapes[13]) ===\n"
"       vec3 lightDir3 = normalize(lightPos3 - FragPos);\n"
"       float dist3 = length(lightPos3 - FragPos);\n"
"       float att3 = 1.0 / (constant + linear * dist3 + quadratic * dist3 * dist3);\n"
"       float diff3 = max(dot(norm, lightDir3), 0.0);\n"
"       vec3 diffuse3 = diff3 * lightColor * att3;\n"
"       vec3 reflectDir3 = reflect(-lightDir3, norm);\n"
"       float spec3 = pow(max(dot(viewDir, reflectDir3), 0.0), 32);\n"
"       vec3 specular3 = specularStrength * spec3 * lightColor * att3;\n"
"       \n"
"       // === LIGHT SOURCE 4 (Sun 4 / shapes[19]) ===\n"
"       vec3 lightDir4 = normalize(lightPos4 - FragPos);\n"
"       float dist4 = length(lightPos4 - FragPos);\n"
"       float att4 = 1.0 / (constant + linear * dist4 + quadratic * dist4 * dist4);\n"
"       float diff4 = max(dot(norm, lightDir4), 0.0);\n"
"       vec3 diffuse4 = diff4 * lightColor * att4;\n"
"       vec3 reflectDir4 = reflect(-lightDir4, norm);\n"
"       float spec4 = pow(max(dot(viewDir, reflectDir4), 0.0), 32);\n"
"       vec3 specular4 = specularStrength * spec4 * lightColor * att4;\n"
"       \n"
"       // === LIGHT SOURCE 5 (Sun 5 / shapes[18]) ===\n"
"       vec3 lightDir5 = normalize(lightPos5 - FragPos);\n"
"       float dist5 = length(lightPos5 - FragPos);\n"
"       float att5 = 1.0 / (constant + linear * dist5 + quadratic * dist5 * dist5);\n"
"       float diff5 = max(dot(norm, lightDir5), 0.0);\n"
"       vec3 diffuse5 = diff5 * lightColor * att5;\n"
"       vec3 reflectDir5 = reflect(-lightDir5, norm);\n"
"       float spec5 = pow(max(dot(viewDir, reflectDir5), 0.0), 32);\n"
"       vec3 specular5 = specularStrength * spec5 * lightColor * att5;\n"
"       \n"
"       // Combine ambient + both light sources\n"
"       vec3 result = (ambient + diffuse1 + specular1 + diffuse2 + specular2 + diffuse3 + specular3 + diffuse4 + specular4 + diffuse5 + specular5) * finalColor;\n"
"       FragColor = vec4(result, 1.0);\n"
"   }\n"
"}\0";

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

enum CameraMode { FREECAM, TRACKING, SPACESHIP_DRIVE };
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
bool loadOBJ(Shape3D& shape, const string& path);
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

// Global variables
vector<Shape3D> shapes;
Shape3D spaceship;
int currentShape = 0;
int numOfSpheres = 23;
bool showGrid = true;
bool showOrigin = true;
bool orbitAnimation = false;
float globalTime = 0.0f;
float timeSlowFactor = 8.0f;
unsigned int skyboxVAO, skyboxVBO;
unsigned int skyboxTexture;
unsigned int skyboxShader;
unsigned int spaceshipTexture;

struct Satellite {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    int targetPlanetIndex;
    bool isOrbiting;
    float orbitAngle;
    float orbitRadius;
    float orbitSpeed;
};

// Globals for Satellites
std::vector<Satellite> launchedSatellites;
Shape3D satelliteModel;
unsigned int satelliteTexture;


bool isAutoPilotActive = false;
int autoPilotTargetIndex = -1;
float autoPilotSpeed = 150.0f;
float arrivalThreshold = 15.0f; 

bool isOrbitingTarget = false;
float orbitAngle = 0.0f;       // Tracks the spaceship's progress around the planet
float orbitRadius = 15.0f;     // How far away from the planet center you want to orbit
float orbitSpeed = 1.5f;       // How fast the ship circles the planet


struct SprayParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float lifetime;
    float maxLifetime;
};

// Terraforming States
bool isTerraformingActive = false;
int terraformTargetIndex = -1;
float terraformProgress = 0.0f; // Goes from 0.0 to 1.0
float terraformSpeed = 0.2f;    // Controls how fast it completes (e.g., 5 seconds)

// Particle Spray Globals
std::vector<SprayParticle> sprayParticles;
unsigned int sprayVAO, sprayVBO;
const int MAX_SPRAY_PARTICLES = 500;

// Textures for Terraforming target states
unsigned int terraformedTexture; // Load a target texture (e.g., green/blue earth-like)

// Planet Position Global Variables (mainly x-axis)
float SunPosX = 4500.0f;
float MercuryPosX = 82.0f;
float VenusPosX = 90.0f;
float EarthPosX = 100.0f;
float MarsPosX = 110.0f;
float JupiterPosX = 126.0f;
float SaturnPosX = 166.0f;
float UranusPosX = 206.0f;
float NeptunePosX = 226.0f;

float HeliosPosX = 6000.0f; // 2nd Sun
float TerraPosX = 100.0f;
float SolarisPosX = 200.0f;
float CastoricePosX = 250.0f;

float SolPosX = -500.0f;  // 3rd Sun
float YharonPosX = 100.0f;
float GollumPosX = 200.0f;
float DalekPosX = 250.0f;
float BjornePosX = 350.0f;

float centerOfStarsPosX = 8500.0f;

float SuryaPosX = 500.0f; // 4th Sun
float DharonPosX = 67.0f;
float CubeoPosX = 169.0f;

float BlackHolePosX = 0.0f;

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

const int NUM_PARTICLES_SATURN = 8000;
vector<RingParticle> saturnRings;
unsigned int ringVAO, ringVBO;

const int NUM_PARTICLES_BHOLE = 32000;
vector<RingParticle> bholeRings;
unsigned int bholeRingVAO, bholeRingVBO;


// Camera variables
glm::vec3 cameraPos = glm::vec3(HeliosPosX + 100.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 direction;
glm::vec3 spaceshipCamera = glm::vec3(0.0f, 0.0f, -1.0f);

float yaw = -90.0f;     // Initialized to -90.0f so it points straight down the -Z axis
float pitch = 0.0f;     // 0.0f means looking perfectly level at the horizon
bool firstMouse = true;     // Detects mouse
float lastX = 600.0f;       // Center X of 1200 width window
float lastY = 450.0f;       // Center Y of 900 height window

float cameraSpeed = 20.0f; // Old speed was 3.0f
float spaceshipSpeed = 5.0f;
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
bool loadOBJ(Shape3D& shape, const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        cout << "Failed to open OBJ file: " << path << endl;
        return false;
    }

    vector<glm::vec3> temp_positions;
    vector<glm::vec3> temp_normals;
    vector<glm::vec2> temp_uvs;

    // To match unique combinations of v/vt/vn positions to an unified index tracking system
    map<string, unsigned int> uniqueVertices;

    shape.vertices.clear();
    shape.indices.clear();

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (type == "vn") {
            glm::vec3 norm;
            ss >> norm.x >> norm.y >> norm.z;
            temp_normals.push_back(norm);
        }
        else if (type == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (type == "f") {
            std::vector<unsigned int> faceIndices;
            string vertexData;

            // Read all available vertex tokens on this face line (handles triangles or quads)
            while (ss >> vertexData) {
                if (uniqueVertices.count(vertexData) == 0) {
                    stringstream vss(vertexData);
                    string vIdxStr, vtIdxStr, vnIdxStr;

                    getline(vss, vIdxStr, '/');
                    getline(vss, vtIdxStr, '/');
                    getline(vss, vnIdxStr, '/');

                    int vIdx = !vIdxStr.empty() ? stoi(vIdxStr) - 1 : -1;
                    int vtIdx = !vtIdxStr.empty() ? stoi(vtIdxStr) - 1 : -1;
                    int vnIdx = !vnIdxStr.empty() ? stoi(vnIdxStr) - 1 : -1;

                    unsigned int newIndex = static_cast<unsigned int>(shape.vertices.size() / 8);

                    // Push position
                    if (vIdx >= 0 && vIdx < temp_positions.size()) {
                        shape.vertices.push_back(temp_positions[vIdx].x);
                        shape.vertices.push_back(temp_positions[vIdx].y);
                        shape.vertices.push_back(temp_positions[vIdx].z);
                    }
                    else {
                        shape.vertices.insert(shape.vertices.end(), { 0.0f, 0.0f, 0.0f });
                    }

                    // Push normal
                    if (vnIdx >= 0 && vnIdx < temp_normals.size()) {
                        shape.vertices.push_back(temp_normals[vnIdx].x);
                        shape.vertices.push_back(temp_normals[vnIdx].y);
                        shape.vertices.push_back(temp_normals[vnIdx].z);
                    }
                    else {
                        shape.vertices.insert(shape.vertices.end(), { 0.0f, 0.0f, 1.0f });
                    }

                    // Push UV
                    if (vtIdx >= 0 && vtIdx < temp_uvs.size()) {
                        shape.vertices.push_back(temp_uvs[vtIdx].x);
                        shape.vertices.push_back(temp_uvs[vtIdx].y);
                    }
                    else {
                        shape.vertices.insert(shape.vertices.end(), { 0.0f, 0.0f });
                    }

                    uniqueVertices[vertexData] = newIndex;
                }
                faceIndices.push_back(uniqueVertices[vertexData]);
            }

            // Triangulate the face data
            // If it's a triangle (3 indices): outputs 0, 1, 2
            // If it's a quad (4 indices): outputs 0, 1, 2 AND 0, 2, 3
            for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                shape.indices.push_back(faceIndices[0]);
                shape.indices.push_back(faceIndices[i]);
                shape.indices.push_back(faceIndices[i + 1]);
            }
        }
    }
    shape.type = SPHERE;
    return true;
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
    shapes.resize(numOfSpheres);

    const string objPath = "sphere.obj";

    for (size_t i = 0; i < numOfSpheres; ++i) {
        loadOBJ(shapes[i], objPath);
    }

    // Positions & scales for planets
    shapes[18].position = glm::vec3(centerOfStarsPosX, 0.0f, 0.0f); shapes[18].scale = glm::vec3(10.0f);
    // This is the Center Point that will help make the two stars (Sol and Surya) orbit around eachother (hopefully)

    shapes[0].position = glm::vec3(SunPosX, 0.0f, 0.0f);    shapes[0].scale = glm::vec3(26.16f); // Sun
    shapes[1].position = glm::vec3(MercuryPosX + shapes[0].position.x, 0.3f, 0.0f);   shapes[1].scale = glm::vec3(0.2f);   // Mercury
    shapes[2].position = glm::vec3(VenusPosX + shapes[0].position.x, 0.3f, 0.0f);   shapes[2].scale = glm::vec3(0.575f); // Venus
    shapes[3].position = glm::vec3(EarthPosX + shapes[0].position.x, 0.3f, 0.0f);   shapes[3].scale = glm::vec3(0.6f);   // Earth
    shapes[4].position = glm::vec3(MarsPosX + shapes[0].position.x, 0.3f, 0.0f);   shapes[4].scale = glm::vec3(0.3f);   // Mars
    shapes[5].position = glm::vec3(JupiterPosX + shapes[0].position.x, 0.3f, 0.0f);   shapes[5].scale = glm::vec3(4.8f);   // Jupiter
    shapes[6].position = glm::vec3(SaturnPosX + shapes[0].position.x, 0.3f, 0.0f);   shapes[6].scale = glm::vec3(4.25f);  // Saturn
    shapes[7].position = glm::vec3(UranusPosX + shapes[0].position.x, 0.3f, 0.0f);  shapes[7].scale = glm::vec3(2.4f);   // Uranus
    shapes[8].position = glm::vec3(NeptunePosX + shapes[0].position.x, 0.3f, 0.0f);  shapes[8].scale = glm::vec3(2.3f);   // Neptune 
    // Above is Solar System 1, increment each planet X-pos with '+ shapes[0].position.x' to get Sun's relative position
    shapes[9].position = glm::vec3(HeliosPosX, 0.0f, 0.0f); shapes[9].scale = glm::vec3(50.0f);
    shapes[10].position = glm::vec3(TerraPosX + shapes[9].position.x, 0.3f, 0.0f); shapes[10].scale = glm::vec3(5.0f);
    shapes[11].position = glm::vec3(SolarisPosX + shapes[9].position.x, 0.3f, 0.0f); shapes[11].scale = glm::vec3(2.0f);
    shapes[12].position = glm::vec3(CastoricePosX + shapes[9].position.x, 0.3f, 0.0f); shapes[12].scale = glm::vec3(3.0f);
    // Above is Solar System 2 
    shapes[13].position = glm::vec3(SolPosX + shapes[18].position.x, 0.0f, 0.0f); shapes[13].scale = glm::vec3(15.0f);
    shapes[14].position = glm::vec3(YharonPosX + shapes[13].position.x, 0.3f, 0.0f); shapes[14].scale = glm::vec3(2.0f);
    shapes[15].position = glm::vec3(GollumPosX + shapes[13].position.x, 0.3f, 0.0f); shapes[15].scale = glm::vec3(2.0f);
    shapes[16].position = glm::vec3(DalekPosX + shapes[13].position.x, 0.3f, 0.0f); shapes[16].scale = glm::vec3(2.0f);
    shapes[17].position = glm::vec3(BjornePosX + shapes[13].position.x, 0.3f, 0.0f); shapes[17].scale = glm::vec3(2.0f);
    // Above is Solar System 3
    shapes[19].position = glm::vec3(SuryaPosX + shapes[18].position.x, 0.0f, 0.0f); shapes[19].scale = glm::vec3(20.0f);
    shapes[20].position = glm::vec3(DharonPosX + shapes[19].position.x, 0.3f, 0.0f); shapes[20].scale = glm::vec3(2.4f);
    shapes[21].position = glm::vec3(CubeoPosX + shapes[19].position.x, 0.3f, 0.0f); shapes[21].scale = glm::vec3(4.8f);
    // Above is Solar System 4
    shapes[22].position = glm::vec3(BlackHolePosX, 0.0f, 0.0f); shapes[22].scale = glm::vec3(1000.0f);
    

    for (size_t i = 0; i < numOfSpheres; ++i) {
        setupShapeBuffers(shapes[i]);
    }

    saturnRings.resize(NUM_PARTICLES_SATURN);
    float innerRadius = 5.5f;
    float outerRadius = 10.0f;

    for (int i = 0; i < NUM_PARTICLES_SATURN; ++i) {
        float r = innerRadius + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (outerRadius - innerRadius)));

        float angle = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * PI)));

        saturnRings[i].orbitRadius = r;
        saturnRings[i].currentAngle = angle;

        saturnRings[i].orbitSpeed = 2.0f / sqrt(r);

        saturnRings[i].relativePosition.x = r * cos(angle);
        saturnRings[i].relativePosition.y = 0.0f;
        saturnRings[i].relativePosition.z = r * sin(angle);
    }

    bholeRings.resize(NUM_PARTICLES_BHOLE);
    float bholeInnerRadius = 1000.0f;
    float bholeOuterRadius = 3000.0f;

    for (int i = 0; i < NUM_PARTICLES_BHOLE; ++i) {
        float r = bholeInnerRadius + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (bholeOuterRadius - bholeInnerRadius)));

        float angle = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f * PI)));

        bholeRings[i].orbitRadius = r;
        bholeRings[i].currentAngle = angle;

        bholeRings[i].orbitSpeed = 4.0f / sqrt(r);

        bholeRings[i].relativePosition.x = r * cos(angle);
        bholeRings[i].relativePosition.y = 0.0f;
        bholeRings[i].relativePosition.z = r * sin(angle);
    }

    // 1. Configure Saturn's VAO
    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);
    glBindVertexArray(ringVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES_SATURN * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttrib4f(1, 0.85f, 0.75f, 0.6f, 1.0f);

    glBindVertexArray(0); // Safely unbind

    // 2. Configure Black Hole's VAO
    glGenVertexArrays(1, &bholeRingVAO);
    glGenBuffers(1, &bholeRingVBO);
    glBindVertexArray(bholeRingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bholeRingVBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES_BHOLE * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); // Safely unbind
    


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

        loadOBJ(shapes[moonIdx], objPath);
        setupShapeBuffers(shapes[moonIdx]);

        shapes[moonIdx].isMoon = true;
        shapes[moonIdx].parentPlanetIndex = def.parentIndex;
        shapes[moonIdx].moonOrbitRadius = def.orbitRadius;
        shapes[moonIdx].moonOrbitSpeed = def.speed;
        shapes[moonIdx].scale = glm::vec3(def.size);
        shapes[moonIdx].animationPhase = static_cast<float>(i) * (PI / 2.0f); // offset starting angles
    }

    const string spaceshipPath = "spaceship.obj";
    if (loadOBJ(spaceship, spaceshipPath)) {
        setupShapeBuffers(spaceship);

        // Spawn next to Earth
        spaceship.position = glm::vec3(shapes[3].position.x + 5.0f, 0.3f, shapes[3].position.z + 2.0f);
        spaceship.scale = glm::vec3(0.05f);
        spaceship.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

        // Texture Mapping
        stbi_set_flip_vertically_on_load(false);
        spaceshipTexture = loadTexture("Material.001_Base_color.jpg");
        stbi_set_flip_vertically_on_load(true); // Turning vertical flip on/off specifically for spaceship textures so it doesnt break the planet texture

        const string satellitePath = "satellite.obj"; 
        if (loadOBJ(satelliteModel, satellitePath)) {
            setupShapeBuffers(satelliteModel);
            satelliteModel.scale = glm::vec3(0.02f); // Adjust scale as needed

            stbi_set_flip_vertically_on_load(false);
            satelliteTexture = loadTexture("satellite_albedo.jpg"); 
            stbi_set_flip_vertically_on_load(true);
        }
    }
}

// Animation functions
void updateOrbitAnimation(float deltaTime) {
    if (orbitAnimation) {
        globalTime += deltaTime;

        // Keep the Sun (index 0) locked at the origin
        //shapes[0].position = glm::vec3(0.0f);

        for (size_t i = 0; i < numOfSpheres; ++i) {
            float time = (globalTime + shapes[i].animationPhase) / timeSlowFactor;

            float radius = 0.0f;
            float speed = 1.0f;

            switch (i) {
            case 0: radius = SunPosX; speed = 4.0f; break; // Sun
            case 1: radius = MercuryPosX; speed = 1.20f; break;  // Mercury (Fastest)
            case 2: radius = VenusPosX; speed = 0.90f; break;  // Venus
            case 3: radius = EarthPosX; speed = 0.75f; break;  // Earth
            case 4: radius = MarsPosX; speed = 0.60f; break;  // Mars
            case 5: radius = JupiterPosX; speed = 0.40f; break;  // Jupiter
            case 6: radius = SaturnPosX; speed = 0.25f; break;  // Saturn
            case 7: radius = UranusPosX; speed = 0.15f; break;  // Uranus
            case 8: radius = NeptunePosX; speed = 0.08f; break;  // Neptune (Slowest)
            case 9: radius = HeliosPosX; speed = 2.0f; break;
            case 10: radius = TerraPosX; speed = 0.5f; break; 
            case 11: radius = SolarisPosX; speed = 0.7f; break;
            case 12: radius = CastoricePosX; speed = 0.4; break;
            case 13: radius = SolPosX; speed = 8.0f; break; // Sun 3
            case 14: radius = YharonPosX; speed = 0.90f; break;
            case 15: radius = GollumPosX; speed = 0.60f; break;
            case 16: radius = DalekPosX; speed = 1.20f; break;
            case 17: radius = BjornePosX; speed = 0.40f; break;
            case 18: radius = centerOfStarsPosX; speed = 2.0f; break;
            case 19: radius = SuryaPosX; speed = 8.0f; break; // Sun 4
            case 20: radius = DharonPosX; speed = 1.20f; break;
            case 21: radius = CubeoPosX; speed = 0.75f; break;
            }

            // 1. Calculate the pure circular math on the XZ plane relative to the center
            float localX = radius * cos((time / 4) * speed);
            float localZ = radius * sin((time / 4) * speed);

            if (i == 0) {
                // The Sun moves purely based on its own circle path
                shapes[0].position.x = localX;
                shapes[0].position.y = 0.3f;
                shapes[0].position.z = localZ;
            }
            else if (i == 9) {
                shapes[9].position.x = localX;
                shapes[9].position.y = 0.3f;
                shapes[9].position.z = localZ;
            }
            else if (i == 18) {
                shapes[18].position.x = localX;
                shapes[18].position.y = 0.3f;
                shapes[18].position.z = localZ;
            }
            else if (i < 9 && !(i >= 9)) {
                // Identical to Moon logic: Add the Sun's position to the planet's local circular position
                shapes[i].position.x = shapes[0].position.x + localX;
                shapes[i].position.y = 0.3f;
                shapes[i].position.z = shapes[0].position.z + localZ;
            }
            else if (i < 13 && !(i >= 13)) {
                shapes[i].position.x = shapes[9].position.x + localX;
                shapes[i].position.y = 0.3f;
                shapes[i].position.z = shapes[9].position.z + localZ;
            }
            else if (i == 13) { // For orbiting the two star around eachother
                shapes[i].position.x = shapes[18].position.x + localX;
                shapes[i].position.y = 0.3f;
                shapes[i].position.z = shapes[18].position.z + localZ;
            }
            else if (i == 19) {
                shapes[i].position.x = shapes[18].position.x + localX;
                shapes[i].position.y = 0.3f;
                shapes[i].position.z = shapes[18].position.z + localZ;
            }
            else if (i >= 14 && i <= 17) {
                // Anchor Yharon, Gollum, Dalek, and Bjorne around Sol (shapes[13])
                shapes[i].position.x = shapes[13].position.x + localX;
                shapes[i].position.y = 0.3f;
                shapes[i].position.z = shapes[13].position.z + localZ;
            }
            else if (i == 20 || i == 21) {
                // Anchor your new planets around Surya (shapes[19])
                shapes[i].position.x = shapes[19].position.x + localX;
                shapes[i].position.y = 0.3f;
                shapes[i].position.z = shapes[19].position.z + localZ;
            }
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
        std::vector<float> rotationSpeeds = {
            7.0f,   // Sun
            10.0f,   // Mercury
            -6.0f,   // Venus (retrograde)
            30.0f,   // Earth
            24.0f,   // Mars
            70.0f,   // Jupiter
            60.0f,   // Saturn
            -40.0f,  // Uranus (retrograde-ish)
            45.0f,    // Neptune

            14.0f,   // Helios
            60.0f,   // Terra
            25.0f,   // Solaris
            45.0f,   // Castorice

            120.0f, // Sol
            66.0f,  // Yharon
            76.0f,  // Gollum
            89.0f,  // Dalek
            27.0f,  // Bjorne

            160.0f, // Center Star

            180.0f,  // Surya
            132.0f,  // Dharon
            99.0f,    // Cubeo

            200.0f  // Black Hole
        };

        // Rotate Planets and moons
        for (size_t i = 0; i < shapes.size(); ++i) {
            float speed = 0.0f;

            if (i < rotationSpeeds.size()) {
                // If it's a primary system body, use its custom speed
                speed = rotationSpeeds[i];
            }
            else {
                // Safely handles all moons dynamically, no matter how many you add!
                speed = 50.0f;
            }

            shapes[i].rotation.y += speed * deltaTime;
        }
    }
}

void updateSaturnRings(float deltaTime) {
    if (orbitAnimation) {
        for (int i = 0; i < NUM_PARTICLES_SATURN; ++i) {
            saturnRings[i].currentAngle += saturnRings[i].orbitSpeed * deltaTime;

            float r = saturnRings[i].orbitRadius;
            float angle = saturnRings[i].currentAngle;

            saturnRings[i].relativePosition.x = r * cos(angle);
            saturnRings[i].relativePosition.z = r * sin(angle);
        }
    }
}

void updateBholeRings(float deltaTime) {
    if (orbitAnimation) {
        for (int i = 0; i < NUM_PARTICLES_BHOLE; ++i) {
            bholeRings[i].currentAngle += bholeRings[i].orbitSpeed * deltaTime;

            float r = bholeRings[i].orbitRadius;
            float angle = bholeRings[i].currentAngle;

            bholeRings[i].relativePosition.x = r * cos(angle);
            bholeRings[i].relativePosition.z = r * sin(angle);
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

    vector<glm::vec3> globalParticlePositions(NUM_PARTICLES_SATURN);
    for (int i = 0; i < NUM_PARTICLES_SATURN; ++i) {
        globalParticlePositions[i] = saturnPos + saturnRings[i].relativePosition;
    }

    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_PARTICLES_SATURN * sizeof(glm::vec3), globalParticlePositions.data());

    glm::mat4 identityModel = glm::mat4(1.0f); // Position is already computed in world-space
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(identityModel));

    // Give the rings a dusty golden/rock color
    glUniform1i(glGetUniformLocation(program, "useTexture"), false); 
    glUniform1i(glGetUniformLocation(program, "isSun"), true);
    glUniform3f(glGetUniformLocation(program, "objectColor"), 0.85f, 0.75f, 0.6f);
    
    

    glPointSize(2.0f); // Sets the pixel width of each point
    glBindVertexArray(ringVAO);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES_SATURN);

    //glUniform1i(glGetUniformLocation(program, "isSun"), false);
    glBindVertexArray(0);
}

void drawBholeRings(unsigned int program) {
    glm::vec3 saturnPos = shapes[22].position;

    vector<glm::vec3> globalParticlePositions(NUM_PARTICLES_BHOLE);
    for (int i = 0; i < NUM_PARTICLES_BHOLE; ++i) {
        globalParticlePositions[i] = saturnPos + bholeRings[i].relativePosition;
    }

    glBindBuffer(GL_ARRAY_BUFFER, bholeRingVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_PARTICLES_BHOLE * sizeof(glm::vec3), globalParticlePositions.data());

    glm::mat4 identityModel = glm::mat4(1.0f); // Position is already computed in world-space
    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(identityModel));

    // Give the rings a dusty golden/rock color
    glUniform1i(glGetUniformLocation(program, "useTexture"), false); // Tell shader not to use texture
    glUniform1i(glGetUniformLocation(program, "isSun"), true);
    glUniform3f(glGetUniformLocation(program, "objectColor"), 0.85f, 0.75f, 0.6f);
    

    glPointSize(2.0f); // Sets the pixel width of each point
    glBindVertexArray(bholeRingVAO);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES_BHOLE);

    //glUniform1i(glGetUniformLocation(program, "isSun"), false);
    glBindVertexArray(0);
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
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
        return;

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
                currentShape = (currentShape + 1) % numOfSpheres;
                currentCameraMode = TRACKING;
            }
            break;

        case GLFW_KEY_0:
            if (action == GLFW_PRESS) {
                currentCameraMode = SPACESHIP_DRIVE;
            }
            break;

        case GLFW_KEY_Q:
            if (action == GLFW_PRESS) {
                currentCameraMode = FREECAM;
            }
            break;

        // Camera controls
        case GLFW_KEY_W:
            if (currentCameraMode == FREECAM) {
                cameraPos += cameraSpeed * cameraFront;
            }
            break;
        case GLFW_KEY_S:
            if (currentCameraMode == FREECAM) {
                cameraPos -= cameraSpeed * cameraFront;
            }
            break;
        case GLFW_KEY_A:
            if (currentCameraMode == FREECAM) {
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            }
            break;
        case GLFW_KEY_D:
            if (currentCameraMode == FREECAM) {
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            }
            break;

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
            if (currentCameraMode != SPACESHIP_DRIVE) {
                fov -= 2.0f;
                if (fov < 10.0f) fov = 10.0f;
            }
            break;
        case GLFW_KEY_MINUS:
            if (currentCameraMode != SPACESHIP_DRIVE) {
                fov += 2.0f;
                if (fov > 120.0f) fov = 120.0f;
            }
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

void processSpaceshipInput(GLFWwindow* window, float deltaTime) {
    if (currentCameraMode != SPACESHIP_DRIVE) return;

    if (isAutoPilotActive) {
        // If auto-pilot is driving, only check for emergency cancel input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            isAutoPilotActive = false;
            autoPilotTargetIndex = -1;
            std::cout << "Auto-Pilot canceled via ESC key." << std::endl;
        }
        return; // Skip normal manual controls
    }

    // 1. Shift for Speed Boost
    float currentSpeed = spaceshipSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        currentSpeed *= 30.0f;
    }



    // 2. A and D rotate the physical spaceship model structure left and right
    float spaceshipRotationSpeed = 60.0f;
    float maxTiltAngle = 30.0f; // Maximum angle the ship will lean (in degrees)
    float targetTilt = 0.0f;    // Level by default

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        spaceship.rotation.y += spaceshipRotationSpeed * deltaTime;
        targetTilt = -maxTiltAngle; // Tilt left
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        spaceship.rotation.y -= spaceshipRotationSpeed * deltaTime;
        targetTilt = maxTiltAngle; // Tilt right
    }

    // Smoothly interpolate current roll (Z axis) towards the target tilt
    // 10.0f controls how fast it snaps/leans into the position. Higher = snappier.
    spaceship.rotation.z = glm::mix(spaceship.rotation.z, targetTilt, 10.0f * deltaTime);

    // 3. Update the direction vector using the updated ship rotation angle
    float yawRadians = glm::radians(spaceship.rotation.y - 90.0f);
    spaceshipCamera.x = std::cos(-yawRadians);
    spaceshipCamera.z = std::sin(-yawRadians);
    spaceshipCamera = glm::normalize(spaceshipCamera);

    // 4. W and S move along that vector path
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        spaceship.position += spaceshipCamera * currentSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        spaceship.position -= spaceshipCamera * currentSpeed * deltaTime;
    }

    // 5. Ascend (=) / Descend (-) tracking
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        spaceship.position.y += currentSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        spaceship.position.y -= currentSpeed * deltaTime;
    }

    // 6. Camera Offset - Position locks behind
    glm::vec3 cameraBehindOffset = -spaceshipCamera * 4.0f;
    cameraBehindOffset.y = 1.05f; // This here is the camera angle

    cameraPos = spaceship.position + cameraBehindOffset;
    cameraFront = glm::normalize(spaceship.position - cameraPos);
}

void updateCamera(float deltaTime) {
    if (mouseEnabled)
        return;

    if (currentCameraMode == TRACKING) {
        const Shape3D& targetPlanet = shapes[currentShape];

        float camDistance = 320.0f; // 32.0f

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

void TopBarMenu(GLFWwindow* window) {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    ImGui::GetIO().AddMousePosEvent(static_cast<float>(mouseX), static_cast<float>(mouseY));

    // Create the Main Top Menu Bar
    if (ImGui::BeginMainMenuBar()) {

        // 1. Check if we are in DRIVE mode
        bool isDriveMode = (currentCameraMode == SPACESHIP_DRIVE);

        // 2. Pass 'isDriveMode' as the enabled flag here so the menu grays out when not in drive mode
        if (ImGui::BeginMenu("Spaceship Control Panel", isDriveMode)) {

            if (ImGui::BeginMenu("Auto-Pilot Target")) {
                const char* planetNames[] = {
                    "Sun", "Mercury", "Venus", "Earth", "Mars",
                    "Jupiter", "Saturn", "Uranus", "Neptune", "Helios",
                    "Terra", "Solaris", "Castorice", "Sol", "Yharon",
                    "Gollum", "Dalek", "Bjorne", "centerOfStars", "Surya",
                    "Dharon", "Cubeo", "BlackHole"
                };

                // Limit the loop strictly to the 23 primary elements (Indices 0 to 22)
                size_t maxElements = std::min(shapes.size(), static_cast<size_t>(23));
                for (size_t i = 0; i < maxElements; ++i) {
                    // Strict index exclusion rule
                    if (i == 0 || i == 9 || i == 13 || i == 18 || i == 19 || i == 22) {
                        continue;
                    }

                    std::string displayName = planetNames[i];
                    std::string menuLabel = displayName;
                    bool isSelected = (isAutoPilotActive && autoPilotTargetIndex == static_cast<int>(i));

                    if (ImGui::MenuItem(menuLabel.c_str(), nullptr, isSelected)) {
                        isAutoPilotActive = true;
                        autoPilotTargetIndex = static_cast<int>(i);
                        std::cout << "Auto-Pilot engaged towards: " << displayName << " (Index " << i << ")" << std::endl;
                    }
                }

                if (isAutoPilotActive) {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Disengage Auto-Pilot")) {
                        isAutoPilotActive = false;
                        isOrbitingTarget = false; // <-- Clear orbital lock state here
                        autoPilotTargetIndex = -1;
                        std::cout << "Auto-Pilot disengaged manually." << std::endl;
                    }
                }

                ImGui::EndMenu(); // Closes "Auto-Pilot Target"
            }

            if (ImGui::BeginMenu("Launch Satellite Target")) {
                const char* planetNames[] = {
                    "Sun", "Mercury", "Venus", "Earth", "Mars",
                    "Jupiter", "Saturn", "Uranus", "Neptune", "Helios",
                    "Terra", "Solaris", "Castorice", "Sol", "Yharon",
                    "Gollum", "Dalek", "Bjorne", "centerOfStars", "Surya",
                    "Dharon", "Cubeo", "BlackHole"
                };

                size_t maxElements = std::min(shapes.size(), static_cast<size_t>(23));
                for (size_t i = 0; i < maxElements; ++i) {
                    if (i == 0 || i == 9 || i == 13 || i == 18 || i == 19 || i == 22) {
                        continue;
                    }

                    std::string displayName = planetNames[i];
                    std::string menuLabel = "Launch to " + displayName;

                    if (ImGui::MenuItem(menuLabel.c_str())) {
                        Satellite sat;
                        sat.position = spaceship.position;
                        sat.rotation = glm::vec3(0.0f);
                        sat.scale = satelliteModel.scale;
                        sat.targetPlanetIndex = static_cast<int>(i);
                        sat.isOrbiting = false;
                        sat.orbitAngle = 0.0f;
                        sat.orbitRadius = shapes[i].scale.x * 2.0f;
                        sat.orbitSpeed = 2.0f;

                        launchedSatellites.push_back(sat);
                        std::cout << "Satellite launched towards: " << displayName << std::endl;
                    }
                }
                ImGui::EndMenu(); // Closes "Launch Satellite Target"
            }

            // --- Shifted Terraform Planet inside the Spaceship Control Panel dropdown list ---
            bool canTerraform = (isDriveMode && isOrbitingTarget && autoPilotTargetIndex >= 0);

            if (ImGui::MenuItem("Terraform Planet", nullptr, isTerraformingActive, canTerraform)) {
                isTerraformingActive = !isTerraformingActive;
                if (isTerraformingActive) {
                    terraformTargetIndex = autoPilotTargetIndex;
                    terraformProgress = 0.0f;
                    std::cout << "Terraforming sequence engaged on planet index " << terraformTargetIndex << std::endl;
                }
                else {
                    std::cout << "Terraforming sequence aborted." << std::endl;
                }
            }

            // Tooltip if hovering the option while disabled inside the list
            if (!canTerraform && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Requires Auto-Pilot to be locked into an orbital path around a planet.");
            }

            ImGui::EndMenu(); // Closes "Spaceship Control Panel"
        }
        else {
            // Show the tooltip if the user hovers over the disabled main "Spaceship Control Panel" button
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Switch to SPACESHIP_DRIVE camera mode to access controls.");
            }
        }

        ImGui::EndMainMenuBar();
    }

    // Rendering ImGui data to the screen
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

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
    unsigned int heliosTexture = loadTexture("whitesun.jpg");
    unsigned int solTexture = loadTexture("bluesun.jpg");
    unsigned int suryaTexture = loadTexture("redsun.jpg");
    unsigned int centreStarTexture = loadTexture("lightorangesun.jpg");
    unsigned int blackTexture = loadTexture("Black_colour.jpg");
    unsigned int terraformedTexture = loadTexture("earth.jpg");

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

    glGenVertexArrays(1, &sprayVAO);
    glGenBuffers(1, &sprayVBO);
    glBindVertexArray(sprayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sprayVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_SPRAY_PARTICLES * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Update animations
        updateOrbitAnimation(deltaTime);
        updateSaturnRings(deltaTime);
        updateBholeRings(deltaTime);

        // Focus camera on planets when pressing TAB
        updateCamera(deltaTime);

        displayInfo();

        if (!shapes.empty()) {
            lightPos = shapes[0].position; // Securely shadows the sun even when moving!
        }

        // Spaceship control input
        processSpaceshipInput(window, deltaTime);

        // Auto-Pilot Flight Execution
        if (isAutoPilotActive && currentCameraMode == SPACESHIP_DRIVE && autoPilotTargetIndex >= 0 && autoPilotTargetIndex < static_cast<int>(shapes.size())) {
            glm::vec3 targetPos = shapes[autoPilotTargetIndex].position;

            if (!isOrbitingTarget) {
                // --- APPROACH PHASE ---
                glm::vec3 direction = targetPos - spaceship.position;
                float distance = glm::length(direction);

                if (distance > arrivalThreshold) {
                    direction = glm::normalize(direction);

                    // Move the physical spaceship forward
                    spaceship.position += direction * autoPilotSpeed * deltaTime;
                    if (fov < 120) {
                        fov += 1;
                    }

                    // Synchronize your ship's horizontal rotation
                    float targetYawRadians = atan2(-direction.z, direction.x);
                    spaceship.rotation.y = glm::degrees(targetYawRadians) + 90.0f;
                    spaceship.rotation.z = glm::mix(spaceship.rotation.z, 0.0f, 10.0f * deltaTime);

                    spaceshipCamera = direction;
                }
                else {
                    // Threshold reached! Transition seamlessly to orbiting phase
                    isOrbitingTarget = true;

                    // Calculate starting angle based on where the ship approached from
                    glm::vec3 approachVec = spaceship.position - targetPos;
                    orbitAngle = atan2(approachVec.z, approachVec.x);
                    std::cout << "Auto-Pilot arrival achieved. Transitioning to orbital lock!" << std::endl;
                }
            }

            if (isOrbitingTarget) {
                // --- ORBITAL LOCK PHASE ---
                // Advance the angle over time
                orbitAngle += orbitSpeed * deltaTime;

                if (fov > 45) {
                    fov -= 5;
                }

                // Calculate the new relative orbital coordinates around the moving planet
                glm::vec3 nextOrbitPos;
                nextOrbitPos.x = targetPos.x + std::cos(orbitAngle) * orbitRadius;
                nextOrbitPos.y = targetPos.y;
                nextOrbitPos.z = targetPos.z + std::sin(orbitAngle) * orbitRadius;

                // Calculate the movement direction vector for orientation mapping
                glm::vec3 orbitMovementDirection = glm::normalize(nextOrbitPos - spaceship.position);

                // Update physical spaceship location
                spaceship.position = nextOrbitPos;

                // Turn the ship to dynamically look forward into its circular trajectory curve
                float targetYawRadians = atan2(-orbitMovementDirection.z, orbitMovementDirection.x);
                spaceship.rotation.y = glm::degrees(targetYawRadians) + 90.0f;
                spaceship.rotation.z = glm::mix(spaceship.rotation.z, 0.0f, 10.0f * deltaTime);

                // Synchronize tracking vector to match the movement tangent line
                spaceshipCamera = orbitMovementDirection;
            }

            // --- CAMERA LOCK TRACKING (WITH DYNAMIC ORBIT ANGLE) ---
            cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

            if (isOrbitingTarget) {
                // Find the perpendicular vector pointing out to the spaceship's left side
                glm::vec3 spaceshipLeftVec = glm::normalize(glm::cross(cameraUp, spaceshipCamera));

                // Position the camera 4.0 units away from the left wing, elevated by 1.05 on the Y axis
                glm::vec3 cameraSideOffset = spaceshipLeftVec * 4.0f;
                cameraSideOffset.y = 1.05f;

                cameraPos = spaceship.position + cameraSideOffset;
            }
            else {
                // Standard approach camera angle (chasing from straight behind)
                glm::vec3 cameraBehindOffset = -spaceshipCamera * 4.0f;
                cameraBehindOffset.y = 1.05f;

                cameraPos = spaceship.position + cameraBehindOffset;
            }

            // Always keep the camera front pinned directly onto the spaceship's center position
            cameraFront = glm::normalize(spaceship.position - cameraPos);

        }
        else {
            if (isAutoPilotActive || isOrbitingTarget) {
                isAutoPilotActive = false;
                isOrbitingTarget = false;
                autoPilotTargetIndex = -1;
            }
        }

        // Satellite Flight Execution
        float satSpeed = 200.0f; // Speed of the satellite mid-transit
        float satArrivalThreshold = 5.0f;

        for (auto& sat : launchedSatellites) {
            glm::vec3 targetPlanetPos = shapes[sat.targetPlanetIndex].position;

            if (!sat.isOrbiting) {
                // --- TRANSIT PHASE ---
                glm::vec3 direction = targetPlanetPos - sat.position;
                float distance = glm::length(direction);

                if (distance > sat.orbitRadius + satArrivalThreshold) {
                    direction = glm::normalize(direction);
                    sat.position += direction * satSpeed * deltaTime;

                    // Point the satellite along its flight path vector
                    float targetYaw = atan2(-direction.z, direction.x);
                    sat.rotation.y = glm::degrees(targetYaw) + 90.0f;
                }
                else {
                    // --- ARRIVAL AT PLANET ---
                    sat.isOrbiting = true;
                    glm::vec3 approachVec = sat.position - targetPlanetPos;
                    sat.orbitAngle = atan2(approachVec.z, approachVec.x);
                }
            }
            else {
                // --- PERMANENT ORBITAL PHASE ---
                sat.orbitAngle += sat.orbitSpeed * deltaTime;

                sat.position.x = targetPlanetPos.x + std::cos(sat.orbitAngle) * sat.orbitRadius;
                sat.position.y = targetPlanetPos.y;
                sat.position.z = targetPlanetPos.z + std::sin(sat.orbitAngle) * sat.orbitRadius;

                // Keep it facing tangentially forward relative to its orbit path curve
                sat.rotation.y = glm::degrees(-sat.orbitAngle);
            }
        }

        // Terraforming Execution 
        if (isTerraformingActive && terraformTargetIndex >= 0) {
            // 1. Progress tracking
            terraformProgress += terraformSpeed * deltaTime;
            if (terraformProgress > 1.0f) terraformProgress = 1.0f;

            // 2. Scale Modification (Gradually grow scale by +50% over time as an example)
            // You can customize the base vs. target scale here
            Shape3D& planet = shapes[terraformTargetIndex];
            glm::vec3 baseScale = planet.localScale; // Grab or define initial scale configuration
            glm::vec3 targetScale = glm::vec3(planet.scale.x * 1.001f); // Micro adjustment or linear mix

            // Gradual sizing shift example:
            planet.scale += glm::vec3(0.5f * deltaTime);

            // 3. Emit Particles from Right Wing
            // Perpendicular vector pointing out to the spaceship's right side:
            glm::vec3 spaceshipRightVec = glm::normalize(glm::cross(spaceshipCamera, cameraUp));
            glm::vec3 rightWingPos = spaceship.position + (spaceshipRightVec * 0.5f);

            // Vector pointing from right wing directly towards the center of the planet
            glm::vec3 toPlanetVector = glm::normalize(shapes[terraformTargetIndex].position - rightWingPos);

            // Spawn new particles rapidly
            for (int i = 0; i < 5; ++i) {
                if (sprayParticles.size() < MAX_SPRAY_PARTICLES) {
                    SprayParticle p;
                    p.position = rightWingPos;

                    // Add slight random spread to the trajectory stream
                    float spreadX = ((rand() % 100) / 100.0f) - 0.5f;
                    float spreadY = ((rand() % 100) / 100.0f) - 0.5f;
                    float spreadZ = ((rand() % 100) / 100.0f) - 0.5f;

                    p.velocity = (toPlanetVector * 40.0f) + glm::vec3(spreadX, spreadY, spreadZ) * 5.0f;
                    p.lifetime = 0.0f;
                    p.maxLifetime = 0.8f; // Despawn quickly upon approaching target area
                    sprayParticles.push_back(p);
                }
            }
        }

        // 4. Update existing spray particles physics loop
        for (size_t i = 0; i < sprayParticles.size(); ) {
            sprayParticles[i].position += sprayParticles[i].velocity * deltaTime;
            sprayParticles[i].lifetime += deltaTime;

            if (sprayParticles[i].lifetime >= sprayParticles[i].maxLifetime) {
                sprayParticles.erase(sprayParticles.begin() + i);
            }
            else {
                ++i;
            }
        }

        // Safety fallback: if orbital breaks away, stop terraforming sequence
        if (!isOrbitingTarget) {
            isTerraformingActive = false;
        }

        // Clear screen
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        // Set up matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), 1200.0f / 900.0f, 0.1f, 10000000.0f);

        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Set lighting uniforms
        glUniform3fv(glGetUniformLocation(program, "lightPos"), 1, glm::value_ptr(shapes[0].position));
        glUniform3fv(glGetUniformLocation(program, "lightPos2"), 1, glm::value_ptr(shapes[9].position));
        glUniform3fv(glGetUniformLocation(program, "lightPos3"), 1, glm::value_ptr(shapes[13].position));
        glUniform3fv(glGetUniformLocation(program, "lightPos4"), 1, glm::value_ptr(shapes[19].position));
        glUniform3fv(glGetUniformLocation(program, "lightPos5"), 1, glm::value_ptr(shapes[18].position));
        glUniform3fv(glGetUniformLocation(program, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(cameraPos));

        // Draw grid and origin
        drawGrid(program);
        drawOrigin(program);

        // Draw all shapes
        for (size_t i = 0; i < shapes.size(); ++i) {
            const auto& shape = shapes[i];

            // Make the sun shadeless
            if (i == 0 || i == 9 || i == 13 || i == 18 || i == 19) {
                glUniform1i(glGetUniformLocation(program, "isSun"), true);
            }
            else {
                glUniform1i(glGetUniformLocation(program, "isSun"), false);
            }

            glActiveTexture(GL_TEXTURE0);

            if (i == terraformTargetIndex && terraformProgress >= 0.8f) {
                glBindTexture(GL_TEXTURE_2D, terraformedTexture); // Swap over to lush planet asset!
            }
            else {
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
                else if (i == 9) {
                    glBindTexture(GL_TEXTURE_2D, heliosTexture);
                }
                else if (i == 10) {
                    glBindTexture(GL_TEXTURE_2D, neptuneTexture);
                }
                else if (i == 11) {
                    glBindTexture(GL_TEXTURE_2D, earthTexture);
                }
                else if (i == 12) {
                    glBindTexture(GL_TEXTURE_2D, saturnTexture);
                }
                else if (i == 13) {
                    glBindTexture(GL_TEXTURE_2D, solTexture);
                }
                else if (i == 14) {
                    glBindTexture(GL_TEXTURE_2D, marsTexture);
                }
                else if (i == 15) {
                    glBindTexture(GL_TEXTURE_2D, venusTexture);
                }
                else if (i == 16) {
                    glBindTexture(GL_TEXTURE_2D, uranusTexture);
                }
                else if (i == 17) {
                    glBindTexture(GL_TEXTURE_2D, mercuryTexture);
                }
                else if (i == 18) {
                    glBindTexture(GL_TEXTURE_2D, centreStarTexture);
                }
                else if (i == 19) {
                    glBindTexture(GL_TEXTURE_2D, suryaTexture);
                }
                else if (i == 20) {
                    glBindTexture(GL_TEXTURE_2D, saturnTexture);
                }
                else if (i == 21) {
                    glBindTexture(GL_TEXTURE_2D, jupiterTexture);
                }
                else if (i == 22) {
                    glBindTexture(GL_TEXTURE_2D, blackTexture);
                }
                else {
                    // FALLBACK FOR MOONS: Use the mercury texture as a placeholder moon rock texture
                    glBindTexture(GL_TEXTURE_2D, mercuryTexture);
                }
            }


            if (i == 22) {
                glUniform1i(glGetUniformLocation(program, "isBlackHole"), true);
                glUniform3fv(glGetUniformLocation(program, "blackHoleCenter"), 1, glm::value_ptr(shapes[22].position));

                // Bind your 6-sided Skybox Cubemap texture ID here
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture); // <-- Replace with your actual skybox texture variable
                glUniform1i(glGetUniformLocation(program, "skybox"), 1);
            }
            else {
                glUniform1i(glGetUniformLocation(program, "isBlackHole"), false);
            }

            glUniform1i(glGetUniformLocation(program, "texture_diffuse"), 0);

            // Tell the fragment shader to actively use the texture instead of solid colors
            glUniform1i(glGetUniformLocation(program, "useTexture"), 1);

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

        drawSaturnRings(program);
        drawBholeRings(program);

        if (!sprayParticles.empty()) {
            glUseProgram(program);

            std::vector<glm::vec3> activePositions;
            for (const auto& p : sprayParticles) {
                activePositions.push_back(p.position);
            }

            glBindBuffer(GL_ARRAY_BUFFER, sprayVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, activePositions.size() * sizeof(glm::vec3), activePositions.data());

            glm::mat4 identityModel = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(identityModel));

            glUniform1i(glGetUniformLocation(program, "useTexture"), false);
            glUniform1i(glGetUniformLocation(program, "isSun"), true); // Shading bypassed so color stays bright neon

            // Neon Teal/Green energy blast beam effect color
            glUniform3f(glGetUniformLocation(program, "objectColor"), 0.0f, 1.0f, 0.7f);

            glPointSize(4.0f); // Make them distinctly visible
            glBindVertexArray(sprayVAO);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(activePositions.size()));
            glBindVertexArray(0);
        }

        if (spaceship.VAO != 0) {
            glUseProgram(program); // Your main shader program
            glBindVertexArray(spaceship.VAO);

            // 1. Bind the spaceship texture to Texture Unit 0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, spaceshipTexture);
            glUniform1i(glGetUniformLocation(program, "texture_diffuse"), 0); // Tells sampler to use unit 0
            glUniform1i(glGetUniformLocation(program, "useTexture"), true);   // Turn on texture mapping!

            // 2. Transformations
            glm::mat4 spaceshipModelMatrix = createTransform3D(spaceship);
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(spaceshipModelMatrix));

            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(spaceshipModelMatrix)));
            glUniformMatrix3fv(glGetUniformLocation(program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

            glUniform1i(glGetUniformLocation(program, "isSun"), false);

            // 3. Draw
            glBindVertexArray(spaceship.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(spaceship.indices.size()), GL_UNSIGNED_INT, 0);
        }

        if (satelliteModel.VAO != 0 && !launchedSatellites.empty()) {
            glUseProgram(program);
            glBindVertexArray(satelliteModel.VAO);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, satelliteTexture);
            glUniform1i(glGetUniformLocation(program, "texture_diffuse"), 0);
            glUniform1i(glGetUniformLocation(program, "useTexture"), true);
            glUniform1i(glGetUniformLocation(program, "isSun"), false);
            glUniform1i(glGetUniformLocation(program, "isBlackHole"), false);

            for (const auto& sat : launchedSatellites) {
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, sat.position);
                modelMatrix = glm::rotate(modelMatrix, glm::radians(sat.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                modelMatrix = glm::rotate(modelMatrix, glm::radians(sat.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                modelMatrix = glm::rotate(modelMatrix, glm::radians(sat.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                modelMatrix = glm::scale(modelMatrix, sat.scale);

                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
                glUniformMatrix3fv(glGetUniformLocation(program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

                glUniform3f(glGetUniformLocation(program, "objectColor"), 1.0f, 1.0f, 1.0f);

                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(satelliteModel.indices.size()), GL_UNSIGNED_INT, 0);
            }
            glBindVertexArray(0);
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

        TopBarMenu(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (auto& shape : shapes) {
        glDeleteVertexArrays(1, &shape.VAO);
        glDeleteBuffers(1, &shape.VBO);
        glDeleteBuffers(1, &shape.EBO);
    }
    glDeleteVertexArrays(1, &satelliteModel.VAO);
    glDeleteBuffers(1, &satelliteModel.VBO);
    glDeleteBuffers(1, &satelliteModel.EBO);

    glDeleteVertexArrays(1, &sprayVAO);
    glDeleteBuffers(1, &sprayVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
