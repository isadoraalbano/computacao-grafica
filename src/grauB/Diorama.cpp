#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Bibliotecas do OpenGL e Janela
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;
using namespace glm;

const GLuint WIDTH = 1000, HEIGHT = 1000;
const int MAX_LIGHTS = 8; 

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
    vec3 Position, Front, Up, Right, WorldUp;
    float Yaw, Pitch, MovementSpeed, MouseSensitivity, Zoom, NearPlane, FarPlane;

    Camera(vec3 position = vec3(0.0f, 2.0f, 8.0f), vec3 up = vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = -12.0f)
        : Front(vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(3.5f), MouseSensitivity(0.05f), Zoom(45.0f), NearPlane(0.1f), FarPlane(100.0f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors(); 
    }

    mat4 GetViewMatrix() const {
        return lookAt(Position, Position + Front, Up);
    }

    void SetPose(vec3 position, float yaw, float pitch) {
        Position = position; Yaw = yaw; Pitch = pitch;
        updateCameraVectors();
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD) Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT) Position -= Right * velocity;
        if (direction == RIGHT) Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        Yaw += xoffset * MouseSensitivity;
        Pitch += yoffset * MouseSensitivity;
        if (constrainPitch) Pitch = glm::clamp(Pitch, -89.0f, 89.0f); 
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom = glm::clamp(Zoom - yoffset, 1.0f, 75.0f);
    }

private:
    void updateCameraVectors() {
        vec3 front;
        front.x = cos(radians(Yaw)) * cos(radians(Pitch));
        front.y = sin(radians(Pitch));
        front.z = sin(radians(Yaw)) * cos(radians(Pitch));
        Front = normalize(front);
        Right = normalize(cross(Front, WorldUp)); 
        Up = normalize(cross(Right, Front));      
    }
};

struct MaterialPhong {
    vec3 ka = vec3(0.1f);
    vec3 kd = vec3(0.8f);
    vec3 ks = vec3(0.2f);
    float q = 32.0f;     
    GLuint diffuseMap = 0; 
};

struct MeshPart {
    GLuint VAO = 0; 
    GLuint VBO = 0; 
    int vertexCount = 0;
    string materialName = "default";
};

struct LightSource {
    string name = "light";
    vec3 position = vec3(2.0f, 4.0f, 3.0f);
    vec3 color = vec3(1.0f);
    float intensity = 1.0f;
    bool enabled = true;
};

struct Object3D {
    string name, objPath, mtlPath;
    vec3 position = vec3(0.0f), rotation = vec3(0.0f), scale = vec3(1.0f);
    
    vector<MeshPart> parts;
    map<string, MaterialPhong> materials;

    vector<vec3> pathPoints; 
    float pathSpeed = 0.25f;
    float animationParam = 0.0f; 
    int pathMode = 1; 
    int pathDirection = 1; 
    bool facePathDirection = true; 
    float pathYawOffset = 0.0f; 
    bool animationEnabled = false;

    mat4 getModelMatrix() const {
        mat4 model = mat4(1.0f);
        model = translate(model, position);
        model = rotate(model, rotation.x, vec3(1.0f, 0.0f, 0.0f));
        model = rotate(model, rotation.y, vec3(0.0f, 1.0f, 0.0f));
        model = rotate(model, rotation.z, vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }

    mat4 getNormalMatrix() const {
        return transpose(inverse(getModelMatrix()));
    }
};

struct ObjIndex { int v = 0, vt = 0, vn = 0; };
struct MeshBuildPart { string materialName; vector<GLfloat> buffer; };
struct ObjectSpec {
    string name, objPath, mtlPath;
    vec3 position = vec3(0.0f), rotationDegrees = vec3(0.0f);
    float uniformScale = 1.0f;
};

Camera camera;
float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f; 

vector<Object3D> sceneObjects;
vector<LightSource> sceneLights;
int activeObject = 0; 
vec3 clearColor = vec3(0.12f, 0.12f, 0.14f); 
float ambientStrength = 0.25f;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void printControls();
int setupShader();
GLuint loadTexture(const string& filePath, int &width, int &height);
GLuint getWhiteTexture();
map<string, MaterialPhong> loadMTL(const string& mtlPath);
vector<MeshPart> loadOBJ(const string& path, int* outVertexCount);
string trim(const string& text);
string findSceneConfig();
string directoryName(const string& path);
string resolvePath(const string& path, const string& baseDir);
bool fileExists(const string& path);
bool loadSceneConfig(const string& configPath);
bool loadObjectFromSpec(const ObjectSpec& spec);
void loadFallbackScene();

void updateAnimations(float dt);
void faceObjectToDirection(Object3D& obj, const vec3& direction);
vec3 calculateBezier(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, float t);
vec3 sampleBezier(const vector<vec3>& points, float param);


const GLchar *vertexShaderSource = R"(
#version 450 core
layout (location = 0) in vec3 position; 
layout (location = 1) in vec3 normal;   
layout (location = 2) in vec2 texCoord; 

uniform mat4 model;      
uniform mat4 view;       
uniform mat4 projection; 
uniform mat4 normalMatrix;

out vec3 vNormal;   
out vec3 fragPos;   
out vec2 TexCoord;  

void main()
{
    vec4 worldPos = model * vec4(position, 1.0); 
    gl_Position = projection * view * worldPos;  
    fragPos = worldPos.xyz;
    vNormal = normalize(mat3(normalMatrix) * normal);
    TexCoord = texCoord;
}
)";

const GLchar *fragmentShaderSource = R"(
#version 450 core
const int MAX_LIGHTS = 8;

in vec3 vNormal;
in vec3 fragPos;
in vec2 TexCoord;

uniform vec3 camPos;
uniform vec3 ka; 
uniform vec3 kd; 
uniform vec3 ks; 
uniform float q; 
uniform float ambientStrength;
uniform bool selected; 

uniform int lightCount;
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform bool lightEnabled[MAX_LIGHTS];

uniform sampler2D texture1; 
out vec4 color; 

void main()
{
    vec3 texColor = texture(texture1, TexCoord).rgb; 
    vec3 N = normalize(vNormal); 
    vec3 V = normalize(camPos - fragPos); 

    // 1. Componente Ambiente
    vec3 result = ambientStrength * ka * texColor;

    for (int i = 0; i < lightCount; i++) {
        if (!lightEnabled[i]) continue;

        vec3 L = normalize(lightPos[i] - fragPos); 
        float distance = length(lightPos[i] - fragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // 2. Componente Difusa
        float diff = max(dot(N, L), 0.0); 
        vec3 diffuse = kd * texColor * diff * lightColor[i] * lightIntensity[i];

        // 3. Componente Especular
        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0), q); 
        vec3 specular = ks * spec * lightColor[i] * lightIntensity[i];

        result += attenuation * (diffuse + specular);
    }

    if (selected) result += vec3(0.08, 0.06, 0.0);
    color = vec4(result, 1.0);
}
)";

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Diorama - Cena Final 3D", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

    glEnable(GL_DEPTH_TEST); 
    stbi_set_flip_vertically_on_load(true); 

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

    string configPath = findSceneConfig();
    if (configPath.empty() || !loadSceneConfig(configPath)) {
        cout << "Aviso: cena de configuracao nao encontrada. Carregando cena padrao." << endl;
        loadFallbackScene(); 
    }
    if (sceneObjects.empty()) loadFallbackScene();
    if (sceneLights.empty()) sceneLights.push_back({"key", vec3(2.5f, 5.0f, 3.0f), vec3(1.0f), 1.2f, true});

    printControls();

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateAnimations(deltaTime);

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        mat4 projection = perspective(radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, camera.NearPlane, camera.FarPlane);
        mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));
        glUniform3f(glGetUniformLocation(shaderID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1f(glGetUniformLocation(shaderID, "ambientStrength"), ambientStrength);

        int lightCount = std::min((int)sceneLights.size(), MAX_LIGHTS);
        glUniform1i(glGetUniformLocation(shaderID, "lightCount"), lightCount);
        for (int i = 0; i < lightCount; i++) {
            string index = to_string(i);
            const LightSource& light = sceneLights[i];
            glUniform3f(glGetUniformLocation(shaderID, ("lightPos[" + index + "]").c_str()), light.position.x, light.position.y, light.position.z);
            glUniform3f(glGetUniformLocation(shaderID, ("lightColor[" + index + "]").c_str()), light.color.r, light.color.g, light.color.b);
            glUniform1f(glGetUniformLocation(shaderID, ("lightIntensity[" + index + "]").c_str()), light.intensity);
            glUniform1i(glGetUniformLocation(shaderID, ("lightEnabled[" + index + "]").c_str()), light.enabled);
        }

        for (int i = 0; i < (int)sceneObjects.size(); i++) {
            Object3D& obj = sceneObjects[i];
            
            mat4 model = obj.getModelMatrix();
            mat4 normalMatrix = obj.getNormalMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "normalMatrix"), 1, GL_FALSE, value_ptr(normalMatrix));
            
            // Controle de estado de selecao do objeto
            glUniform1i(glGetUniformLocation(shaderID, "selected"), i == activeObject);

            for (const MeshPart& part : obj.parts) {
                MaterialPhong material;
                auto it = obj.materials.find(part.materialName);
                if (it != obj.materials.end()) material = it->second;
                else if (obj.materials.find("default") != obj.materials.end()) material = obj.materials["default"];

                glUniform3f(glGetUniformLocation(shaderID, "ka"), material.ka.r, material.ka.g, material.ka.b);
                glUniform3f(glGetUniformLocation(shaderID, "kd"), material.kd.r, material.kd.g, material.kd.b);
                glUniform3f(glGetUniformLocation(shaderID, "ks"), material.ks.r, material.ks.g, material.ks.b);
                glUniform1f(glGetUniformLocation(shaderID, "q"), std::max(material.q, 1.0f));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, material.diffuseMap ? material.diffuseMap : getWhiteTexture());

                glBindVertexArray(part.VAO);
                glDrawArrays(GL_TRIANGLES, 0, part.vertexCount);
            }
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window); 
        glfwPollEvents();        
    }

    glfwTerminate();
    return 0;
}



void printControls() {
    cout << "\n=== CONTROLES ===" << endl;
    cout << "[Mouse] olhar | [Scroll] zoom | [W/A/S/D] mover camera" << endl;
    cout << "[TAB] selecionar proximo objeto | [1-5] ligar/desligar luz" << endl;
    cout << "[Setas] mover objeto no plano XZ | [PgUp/PgDn] mover em Y" << endl;
    cout << "[X/Y/Z] rotacionar objeto | [[ / ]] escala uniforme" << endl;
    cout << "[P] salvar ponto para a trajetoria do objeto selecionado" << endl;
    cout << "[Espaco] iniciar/pausar animacao do objeto selecionado | [ESC] sair" << endl;
    if (!sceneObjects.empty()) cout << "Objeto selecionado: " << sceneObjects[activeObject].name << endl;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) { glfwSetWindowShouldClose(window, true); return; }
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8 && action == GLFW_PRESS) {
        int lightIndex = key - GLFW_KEY_1;
        if (lightIndex < (int)sceneLights.size()) {
            sceneLights[lightIndex].enabled = !sceneLights[lightIndex].enabled;
            cout << "Luz " << sceneLights[lightIndex].name << ": " << (sceneLights[lightIndex].enabled ? "ligada" : "desligada") << endl;
        }
        return;
    }

    if (sceneObjects.empty()) return;

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        activeObject = (activeObject + 1) % (int)sceneObjects.size();
        cout << "Objeto selecionado: " << sceneObjects[activeObject].name << endl;
        return;
    }

    Object3D& obj = sceneObjects[activeObject];
    const float moveSpeed = 0.10f;
    const float scaleSpeed = 0.03f;
    const float rotSpeed = radians(3.0f);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if ((obj.pathPoints.size() - 1) / 3 >= 1) { 
            obj.animationEnabled = !obj.animationEnabled;
            cout << "Animacao (Bezier) de " << obj.name << ": " << (obj.animationEnabled ? "ligada" : "pausada") << endl;
        } else {
            cout << "Objeto precisa de blocos de 4 pontos (ex: 4, 7, 10...) para a Bezier." << endl;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        obj.pathPoints.push_back(obj.position);
        obj.pathMode = 2; // Ida e volta
        cout << "Ponto de Controle salvo (" << obj.position.x << ", " << obj.position.y << ", " << obj.position.z << ")" << endl;
        if (obj.pathPoints.size() == 4) {
            cout << "--> 4 Pontos alcancados! Curva Bezier pronta. Pressione ESPACO." << endl;
            obj.animationParam = 0.0f; 
        }
    }

    if (key == GLFW_KEY_LEFT) obj.position.x -= moveSpeed;
    if (key == GLFW_KEY_RIGHT) obj.position.x += moveSpeed;
    if (key == GLFW_KEY_UP) obj.position.z -= moveSpeed;
    if (key == GLFW_KEY_DOWN) obj.position.z += moveSpeed;
    if (key == GLFW_KEY_PAGE_UP) obj.position.y += moveSpeed;
    if (key == GLFW_KEY_PAGE_DOWN) obj.position.y -= moveSpeed;

    if (key == GLFW_KEY_X) obj.rotation.x += rotSpeed;
    if (key == GLFW_KEY_Y) obj.rotation.y += rotSpeed;
    if (key == GLFW_KEY_Z) obj.rotation.z += rotSpeed;

    if (key == GLFW_KEY_LEFT_BRACKET) obj.scale = glm::max(obj.scale - vec3(scaleSpeed), vec3(0.05f));
    if (key == GLFW_KEY_RIGHT_BRACKET) obj.scale += vec3(scaleSpeed);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


void updateAnimations(float dt) {
    for (Object3D& obj : sceneObjects) {
        
        int numSegments = ((int)obj.pathPoints.size() - 1) / 3;
        if (!obj.animationEnabled || numSegments < 1) continue;

        vec3 previousPosition = obj.position;
        float step = obj.pathSpeed * dt;
        float maxParam = (float)numSegments; 

        // Ping-pong (Ida e Volta)
        if (obj.pathMode == 2) {
            obj.animationParam += obj.pathDirection * step;
            if (obj.animationParam >= maxParam) { 
                obj.animationParam = maxParam;
                obj.pathDirection = -1; 
            } else if (obj.animationParam <= 0.0f) { 
                obj.animationParam = 0.0f;
                obj.pathDirection = 1; 
            }
        } else {
            obj.animationParam += step;
            if (obj.animationParam >= maxParam) {
                if (obj.pathMode == 0) {
                    obj.animationParam = maxParam;
                    obj.animationEnabled = false; 
                } else {
                    obj.animationParam = 0.0f; 
                }
            }
        }

        obj.position = sampleBezier(obj.pathPoints, obj.animationParam);
        if (obj.facePathDirection) faceObjectToDirection(obj, obj.position - previousPosition);
    }
}

void faceObjectToDirection(Object3D& obj, const vec3& direction) {
    vec3 flatDirection = vec3(direction.x, 0.0f, direction.z); 
    if (length(flatDirection) < 0.0001f) return;
    flatDirection = normalize(flatDirection);
    obj.rotation.y = std::atan2(flatDirection.x, flatDirection.z) + obj.pathYawOffset;
}

vec3 calculateBezier(const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    
    return (uu * u) * p0 + 
           (3.0f * uu * t) * p1 + 
           (3.0f * u * tt) * p2 + 
           (tt * t) * p3;
}

vec3 sampleBezier(const vector<vec3>& points, float param) {
    int numSegments = ((int)points.size() - 1) / 3;
    if (numSegments < 1) return points[0];

    float clampedParam = glm::clamp(param, 0.0f, (float)numSegments);
    int segment = (int)floor(clampedParam);
    if (segment >= numSegments) {
        segment = numSegments - 1;
        clampedParam = (float)numSegments;
    }
    
    float t = clampedParam - (float)segment;
    int idx = segment * 3; 
    return calculateBezier(points[idx], points[idx+1], points[idx+2], points[idx+3], t);
}

bool loadSceneConfig(const string& configPath) {
    ifstream file(configPath);
    if (!file.is_open()) return false;

    string baseDir = directoryName(configPath);
    map<string, int> objectIndexByName;
    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue; 

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "camera") {
            vec3 position; float yaw, pitch, fov, nearPlane, farPlane;
            if (iss >> position.x >> position.y >> position.z >> yaw >> pitch >> fov >> nearPlane >> farPlane) {
                camera.SetPose(position, yaw, pitch);
                camera.Zoom = fov; camera.NearPlane = nearPlane; camera.FarPlane = farPlane;
            }
        } else if (command == "clearColor") {
            iss >> clearColor.r >> clearColor.g >> clearColor.b;
        } else if (command == "ambient") {
            iss >> ambientStrength;
        } else if (command == "light") {
            LightSource light; int enabled = 1;
            if (iss >> light.name >> light.position.x >> light.position.y >> light.position.z
                    >> light.color.r >> light.color.g >> light.color.b >> light.intensity >> enabled) {
                light.enabled = enabled != 0;
                if ((int)sceneLights.size() < MAX_LIGHTS) sceneLights.push_back(light);
            }
        } else if (command == "object") {
            ObjectSpec spec;
            if (iss >> spec.name >> spec.objPath >> spec.mtlPath >> spec.position.x >> spec.position.y >> spec.position.z
                    >> spec.rotationDegrees.x >> spec.rotationDegrees.y >> spec.rotationDegrees.z >> spec.uniformScale) {
                spec.objPath = resolvePath(spec.objPath, baseDir);
                spec.mtlPath = resolvePath(spec.mtlPath, baseDir);
                if (loadObjectFromSpec(spec)) objectIndexByName[spec.name] = (int)sceneObjects.size() - 1;
            }
        } else if (command == "path") {
            string objectName; float speed = 0.25f; int mode = 1;
            if (!(iss >> objectName >> speed >> mode)) continue;

            auto objectIt = objectIndexByName.find(objectName);
            if (objectIt == objectIndexByName.end()) continue;

            Object3D& obj = sceneObjects[objectIt->second];
            obj.pathPoints.clear();
            obj.pathSpeed = speed;
            obj.pathMode = glm::clamp(mode, 0, 2);
            obj.pathDirection = 1;

            vec3 point;
            while (iss >> point.x >> point.y >> point.z) obj.pathPoints.push_back(point);

            if ((obj.pathPoints.size() - 1) / 3 >= 1) {
                obj.animationEnabled = true;
                obj.position = sampleBezier(obj.pathPoints, 0.0f);
                vec3 nextPosition = sampleBezier(obj.pathPoints, 0.05f);
                faceObjectToDirection(obj, nextPosition - obj.position);
            }
        }
    }
    return true;
}

vector<MeshPart> loadOBJ(const string& path, int* outVertexCount) {
    vector<vec3> vertices, normals;
    vector<vec2> texCoords;
    vector<MeshBuildPart> buildParts;
    buildParts.push_back({"default", {}});
    *outVertexCount = 0;

    ifstream file(path);
    if (!file.is_open()) return {};

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type; iss >> type;

        if (type == "v") { vec3 v; iss >> v.x >> v.y >> v.z; vertices.push_back(v); } 
        else if (type == "vt") { vec2 vt; iss >> vt.x >> vt.y; texCoords.push_back(vt); } 
        else if (type == "vn") { vec3 vn; iss >> vn.x >> vn.y >> vn.z; normals.push_back(vn); } 
        else if (type == "usemtl") { 
            string materialName; iss >> materialName;
            if (materialName.empty()) materialName = "default";
            if (buildParts.back().buffer.empty()) buildParts.back().materialName = materialName;
            else buildParts.push_back({materialName, {}});
        } else if (type == "f") {
            vector<ObjIndex> face; string token;
            while (iss >> token) {
                ObjIndex result;
                size_t firstSlash = token.find('/');
                if (firstSlash == string::npos) { result.v = stoi(token); } 
                else {
                    string vertex = token.substr(0, firstSlash);
                    if (!vertex.empty()) result.v = stoi(vertex);

                    size_t secondSlash = token.find('/', firstSlash + 1);
                    string texCoord = secondSlash == string::npos ? token.substr(firstSlash + 1) : token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
                    if (!texCoord.empty()) result.vt = stoi(texCoord);

                    if (secondSlash != string::npos) {
                        string normal = token.substr(secondSlash + 1);
                        if (!normal.empty()) result.vn = stoi(normal);
                    }
                }
                face.push_back(result);
            }

            if (face.size() < 3) continue;

            auto resolveIndex = [](int index, int size) {
                if (index > 0) return index - 1;
                if (index < 0) return size + index;
                return -1;
            };

            auto appendVertex = [&](const ObjIndex& index) {
                int vi = resolveIndex(index.v, (int)vertices.size());
                int ni = resolveIndex(index.vn, (int)normals.size());
                int ti = resolveIndex(index.vt, (int)texCoords.size());
                vector<GLfloat>& buffer = buildParts.back().buffer;

                if (vi >= 0 && vi < (int)vertices.size()) { buffer.push_back(vertices[vi].x); buffer.push_back(vertices[vi].y); buffer.push_back(vertices[vi].z); } else { buffer.insert(buffer.end(), {0.0f, 0.0f, 0.0f}); }
                if (ni >= 0 && ni < (int)normals.size()) { buffer.push_back(normals[ni].x); buffer.push_back(normals[ni].y); buffer.push_back(normals[ni].z); } else { buffer.insert(buffer.end(), {0.0f, 1.0f, 0.0f}); }
                if (ti >= 0 && ti < (int)texCoords.size()) { buffer.push_back(texCoords[ti].x); buffer.push_back(texCoords[ti].y); } else { buffer.insert(buffer.end(), {0.0f, 0.0f}); }
            };

            for (size_t i = 1; i + 1 < face.size(); i++) {
                appendVertex(face[0]);
                appendVertex(face[i]);
                appendVertex(face[i + 1]);
            }
        }
    }

    vector<MeshPart> parts;
    for (const MeshBuildPart& buildPart : buildParts) {
        if (buildPart.buffer.empty()) continue;

        MeshPart part;
        part.materialName = buildPart.materialName;
        part.vertexCount = (int)buildPart.buffer.size() / 8; 
        *outVertexCount += part.vertexCount;

        glGenVertexArrays(1, &part.VAO); glGenBuffers(1, &part.VBO);
        glBindVertexArray(part.VAO); glBindBuffer(GL_ARRAY_BUFFER, part.VBO);
        glBufferData(GL_ARRAY_BUFFER, buildPart.buffer.size() * sizeof(GLfloat), buildPart.buffer.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
        parts.push_back(part);
    }

    glBindVertexArray(0);
    return parts;
}

map<string, MaterialPhong> loadMTL(const string& mtlPath) {
    map<string, MaterialPhong> materials;
    materials["default"] = MaterialPhong();
    ifstream file(mtlPath);
    if (!file.is_open()) { materials["default"].diffuseMap = getWhiteTexture(); return materials; }

    string basePath = directoryName(mtlPath), line, word;
    MaterialPhong* current = &materials["default"];

    while (getline(file, line)) {
        istringstream iss(line); iss >> word;
        if (word == "newmtl") {
            string materialName; iss >> materialName;
            if (!materialName.empty()) { materials[materialName] = MaterialPhong(); current = &materials[materialName]; }
        } 
        else if (word == "Ka") iss >> current->ka.x >> current->ka.y >> current->ka.z;
        else if (word == "Kd") iss >> current->kd.x >> current->kd.y >> current->kd.z;
        else if (word == "Ks") iss >> current->ks.x >> current->ks.y >> current->ks.z;
        else if (word == "Ns") iss >> current->q;
        else if (word == "map_Kd") {
            size_t keyword = line.find("map_Kd");
            if (keyword != string::npos) {
                string texturePath = trim(line.substr(keyword + 6));
                
                if (!texturePath.empty() && texturePath[0] == '-') {
                    istringstream pIss(texturePath); vector<string> tokens; string token;
                    while (pIss >> token) tokens.push_back(token);
                    for (size_t i = 0; i < tokens.size(); i++) {
                        if (tokens[i].empty()) continue;
                        if (tokens[i][0] != '-') {
                            texturePath = "";
                            for (size_t j = i; j < tokens.size(); j++) { if(j>i) texturePath += " "; texturePath += tokens[j]; }
                            break;
                        }
                        int skip = 1;
                        if (tokens[i] == "-o" || tokens[i] == "-s" || tokens[i] == "-t") skip = 3;
                        else if (tokens[i] == "-mm") skip = 2;
                        while(skip > 0 && i + 1 < tokens.size() && tokens[i+1][0] != '-') { i++; skip--; }
                    }
                }
                
                if (!texturePath.empty()) {
                    int w, h; current->diffuseMap = loadTexture(resolvePath(texturePath, basePath), w, h);
                }
            }
        }
    }
    for (auto& material : materials) { if (!material.second.diffuseMap) material.second.diffuseMap = getWhiteTexture(); }
    return materials;
}

GLuint loadTexture(const string& filePath, int &width, int &height) {
    GLuint texID; glGenTextures(1, &texID); glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrCh; unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrCh, 0);
    if (data) {
        GLenum format = GL_RGB; if (nrCh == 1) format = GL_RED; else if (nrCh == 4) format = GL_RGBA;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glDeleteTextures(1, &texID); texID = getWhiteTexture(); 
    }
    stbi_image_free(data);
    return texID;
}

GLuint getWhiteTexture() {
    static GLuint tex = 0; if (tex) return tex;
    unsigned char whitePixel[] = {255, 255, 255, 255}; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    return tex;
}

bool loadObjectFromSpec(const ObjectSpec& spec) {
    int vCount = 0; vector<MeshPart> parts = loadOBJ(spec.objPath, &vCount);
    if (parts.empty()) return false;
    Object3D obj; obj.name = spec.name; obj.objPath = spec.objPath; obj.mtlPath = spec.mtlPath;
    obj.position = spec.position; obj.rotation = radians(spec.rotationDegrees); obj.scale = vec3(spec.uniformScale);
    obj.pathYawOffset = obj.rotation.y; obj.parts = parts; obj.materials = loadMTL(spec.mtlPath);
    sceneObjects.push_back(obj); return true;
}

void loadFallbackScene() {
    ObjectSpec room; room.name = "sala"; room.objPath = resolvePath("Modelos3D/sala.obj", "../assets");
    room.mtlPath = resolvePath("Modelos3D/sala.mtl", "../assets"); room.uniformScale = 0.5f; loadObjectFromSpec(room);
}

int setupShader() {
    GLint s; GLchar l[512]; GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vertexShaderSource, NULL); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fragmentShaderSource, NULL); glCompileShader(f);
    GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f); return p;
}

string trim(const string& text) { size_t b = text.find_first_not_of(" \t\r\n"); if (b == string::npos) return ""; return text.substr(b, text.find_last_not_of(" \t\r\n") - b + 1); }
bool fileExists(const string& path) { ifstream f(path); return f.good(); }
string directoryName(const string& path) { size_t p = path.find_last_of("/\\"); return p == string::npos ? "" : path.substr(0, p + 1); }
string resolvePath(const string& path, const string& baseDir) { if (path.empty() || fileExists(path)) return path; string fBase = baseDir + (baseDir.empty() || baseDir.back() == '/' ? "" : "/") + path; if (fileExists(fBase)) return fBase; string fAssets = "../assets/" + path; if (fileExists(fAssets)) return fAssets; return "assets/" + path; }
string findSceneConfig() {
    vector<string> filesConfig = {
        "../assets/config.txt",
        "../../assets/config.txt"
    };

    for (const string& config : filesConfig) {
        if (fileExists(config)) return config;
    }
    return "";
}