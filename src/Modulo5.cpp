#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;
using namespace glm;

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
public:
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;

    // angulos de Euler
    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(vec3 position = vec3(0.0f, 0.0f, 3.0f), vec3 up = vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f) 
      : Front(vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.05f), Zoom(45.0f) 
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    mat4 GetViewMatrix() {
        return lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    // Scroll do mouse
    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
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
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float q;
    GLuint diffuseMap;
};

struct Object3D {
    glm::vec3 position;
    glm::vec3 rotation; 
    glm::vec3 scale;    
    bool rotX, rotY, rotZ; 
    GLuint VAO;
    int vertexCount;

    Object3D(glm::vec3 pos, GLuint vao, int vCount) 
        : position(pos), rotation(glm::vec3(0.0f)), scale(glm::vec3(0.5f)), 
          rotX(false), rotY(false), rotZ(false), VAO(vao), vertexCount(vCount) {}
    
    glm::mat4 getModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }

    glm::mat4 getNormalMatrix() {
        return transpose(inverse(getModelMatrix()));
    }
};

Camera camera(vec3(0.0f, 0.0f, 4.0f));
float lastX = 1000.0f / 2.0;
float lastY = 1000.0f / 2.0;
bool firstMouse = true;

// Temporizaçao
float deltaTime = 0.0f;
float lastFrame = 0.0f;

vector<Object3D> sceneObjects;
int activeObject = 0;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window); 
int setupShader();
GLuint loadOBJ(const char* path, int* outVertexCount);
GLuint loadTexture(string filePath, int &width, int &height);
MaterialPhong loadMTL(const char* mtlPath);

const GLuint WIDTH = 1000, HEIGHT = 1000;

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
out vec4 fragPos;
out vec2 TexCoord;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;
    fragPos = worldPos;
    
    vNormal = normalize(mat3(normalMatrix) * normal);
    TexCoord = texCoord;
}
)";

const GLchar *fragmentShaderSource = R"(
#version 450 core
in vec3 vNormal;
in vec4 fragPos;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 camPos;

uniform vec3 ka; 
uniform vec3 kd;
uniform vec3 ks;
uniform float q;

uniform sampler2D texture1;

out vec4 color;

void main()
{
    vec3 objectColor = texture(texture1, TexCoord).rgb;
    vec3 ambient = ka * 0.3;

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vec3(fragPos));
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = kd * diff;

    vec3 R = normalize(reflect(-L, N));
    vec3 V = normalize(camPos - vec3(fragPos));
    float spec = max(dot(R, V), 0.0);
    vec3 specular = ks * pow(spec, q);

    vec3 result = (ambient + diffuse + specular) * objectColor;
    color = vec4(result, 1.0);
}
)";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Modulo 5 Camera - Isadora Albano", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Trava o mouse para permitir movimentação 360
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);
    stbi_set_flip_vertically_on_load(true);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

    int nVerts;
    GLuint VAO = loadOBJ("../assets/Modelos3D/Suzanne.obj", &nVerts);
    if (VAO) {
        sceneObjects.push_back(Object3D(glm::vec3(0.0f, 0.0f, 0.0f), VAO, nVerts));
    }
    MaterialPhong material = loadMTL("../assets/Modelos3D/Suzanne.mtl");

    vec3 lightPos = vec3(2.0f, 3.0f, 2.0f);

    cout << "\n____ CONTROLES DA CAMERA ____" << endl;
    cout << "[Mouse] - Olhar ao redor" << endl;
    cout << "[Scroll] - Zoom" << endl;
    cout << "[W/A/S/D] - Movimentar a camera" << endl;
    cout << "[X/Y/Z] - Girar" << endl;
    cout << "[ESC] - Sair" << endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        mat4 projection = perspective(radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

        mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));

        glUniform3f(glGetUniformLocation(shaderID, "ka"), material.ka.r, material.ka.g, material.ka.b);
        glUniform3f(glGetUniformLocation(shaderID, "kd"), material.kd.r, material.kd.g, material.kd.b);
        glUniform3f(glGetUniformLocation(shaderID, "ks"), material.ks.r, material.ks.g, material.ks.b);
        glUniform1f(glGetUniformLocation(shaderID, "q"), material.q);
        glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shaderID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.diffuseMap);

        for (int i = 0; i < (int)sceneObjects.size(); i++) {
            Object3D& obj = sceneObjects[i];

            if (obj.rotX) obj.rotation.x += 0.002f;
            if (obj.rotY) obj.rotation.y += 0.002f;
            if (obj.rotZ) obj.rotation.z += 0.002f;

            mat4 model = obj.getModelMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "normalMatrix"), 1, GL_FALSE, value_ptr(obj.getNormalMatrix()));

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (sceneObjects.empty()) return;
    Object3D& obj = sceneObjects[activeObject];

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_X) { obj.rotX = !obj.rotX; obj.rotY = false; obj.rotZ = false; }
        if (key == GLFW_KEY_Y) { obj.rotX = false; obj.rotY = !obj.rotY; obj.rotZ = false; }
        if (key == GLFW_KEY_Z) { obj.rotX = false; obj.rotY = false; obj.rotZ = !obj.rotZ; }
    }
}

// olhar ao redor
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX; 
    float yoffset = lastY - ypos; 
    lastX = xpos; 
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

GLuint loadOBJ(const char* path, int* outVertexCount) {
    vector<glm::vec3> vertices, normals;
    vector<glm::vec2> texCoords;
    vector<GLfloat> vBuffer;

    ifstream file(path);
    if (!file.is_open()) return 0;

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type; iss >> type;
        if (type == "v") { glm::vec3 v; iss >> v.x >> v.y >> v.z; vertices.push_back(v); }
        else if (type == "vt") { glm::vec2 vt; iss >> vt.x >> vt.y; texCoords.push_back(vt); }
        else if (type == "vn") { glm::vec3 vn; iss >> vn.x >> vn.y >> vn.z; normals.push_back(vn); }
        else if (type == "f") {
            string s;
            for(int i=0; i<3; i++) {
                iss >> s;
                size_t p1 = s.find('/'), p2 = s.find('/', p1+1);
                int vi = stoi(s.substr(0, p1)) - 1;
                int ti = (p1 != string::npos && p2 != p1+1) ? stoi(s.substr(p1+1, p2-p1-1)) - 1 : -1;
                int ni = (p2 != string::npos) ? stoi(s.substr(p2+1)) - 1 : -1;

                if (vi >= 0) { vBuffer.push_back(vertices[vi].x); vBuffer.push_back(vertices[vi].y); vBuffer.push_back(vertices[vi].z); } 
                else { vBuffer.insert(vBuffer.end(), {0.0f, 0.0f, 0.0f}); }

                if (ni >= 0 && ni < normals.size()) { vBuffer.push_back(normals[ni].x); vBuffer.push_back(normals[ni].y); vBuffer.push_back(normals[ni].z); } 
                else { vBuffer.insert(vBuffer.end(), {0.0f, 1.0f, 0.0f}); }

                if (ti >= 0 && ti < texCoords.size()) { vBuffer.push_back(texCoords[ti].x); vBuffer.push_back(texCoords[ti].y); } 
                else { vBuffer.insert(vBuffer.end(), {0.0f, 0.0f}); }
            }
        }
    }

    *outVertexCount = vBuffer.size() / 8;
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
    
    return VAO;
}

MaterialPhong loadMTL(const char* mtlPath) {
    MaterialPhong mat = {vec3(0.1f), vec3(0.8f), vec3(1.0f), 32.0f, 0};
    ifstream file(mtlPath);
    if (!file.is_open()) return mat;
    string basePath = string(mtlPath);
    size_t pos = basePath.find_last_of('/');
    basePath = (pos != string::npos) ? basePath.substr(0, pos + 1) : "";
    string line, word;
    while (getline(file, line)) {
        istringstream iss(line); iss >> word;
        if (word == "Ka") iss >> mat.ka.x >> mat.ka.y >> mat.ka.z;
        else if (word == "Kd") iss >> mat.kd.x >> mat.kd.y >> mat.kd.z;
        else if (word == "Ks") iss >> mat.ks.x >> mat.ks.y >> mat.ks.z;
        else if (word == "Ns") iss >> mat.q;
        else if (word == "map_Kd") {
            string tex; iss >> tex; int w, h; mat.diffuseMap = loadTexture(basePath + tex, w, h);
        }
    }
    return mat;
}

GLuint loadTexture(string filePath, int &width, int &height) {
    GLuint texID; glGenTextures(1, &texID); glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int nrCh; unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrCh, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, (nrCh == 3 ? GL_RGB : GL_RGBA), width, height, 0, (nrCh == 3 ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    return texID;
}

int setupShader() {
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERRO: Compilacao Vertex Shader falhou\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERRO: Compilacao Fragment Shader falhou\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERRO: Linkagem do Shader Program falhou\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}