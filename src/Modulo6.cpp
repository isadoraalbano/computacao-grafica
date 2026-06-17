#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

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

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f)) 
      : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f) {
        Position = position;
        WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        Yaw = -90.0f;
        Pitch = 0.0f;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(int direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == 0) Position += Front * velocity; 
        if (direction == 1) Position -= Front * velocity; 
        if (direction == 2) Position -= Right * velocity; 
        if (direction == 3) Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        updateCameraVectors();
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

struct MaterialPhong {
    glm::vec3 ka, kd, ks;
    float q;
    GLuint diffuseMap;
};

struct Object3D {
    GLuint VAO;
    int nVertices;
    GLuint textureID;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    // Propriedades da Trajetória
    std::vector<glm::vec3> pathPoints; 
    int currentTarget = 0;             
    bool isMoving = false;
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
        return glm::transpose(glm::inverse(getModelMatrix()));
    }
};

const GLuint WIDTH = 1000, HEIGHT = 1000;
Object3D object;          
Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));

// Timers
float deltaTime = 0.0f; 
float lastFrame = 0.0f; 

// Mouse
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;

// Luzes
bool keyLightOn = true;
bool fillLightOn = true;
bool backLightOn = true;

// Velocidades do objeto
float moveSpeed = 0.1f;
float scaleSpeed = 0.02f;
float rotSpeed = 0.05f;

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int setupShader();
GLuint loadOBJ(const char* path, int* outVertexCount);
GLuint loadTexture(string filePath, int &width, int &height);
MaterialPhong loadMTL(const char* mtlPath);

const GLchar *vertexShaderSource = R"(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
uniform mat4 normalMatrix;

out vec2 TexCoord;
out vec3 vNormal;
out vec4 fragPos; 

void main()
{
    fragPos = model * vec4(position, 1.0);
    gl_Position = projection * view * fragPos;
    TexCoord = texCoord;
    vNormal = normalize(mat3(normalMatrix) * normal);
}
)";

const GLchar *fragmentShaderSource = R"(
#version 450 core
in vec2 TexCoord;
in vec4 fragPos;
in vec3 vNormal;

uniform sampler2D texBuff;
uniform vec3 camPos;
uniform vec3 ka, kd, ks;
uniform float q;

uniform vec3 lightPos[3];
uniform bool lightOn[3];
uniform float kl[3];
uniform float kq[3];

out vec4 color;

void main()
{
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 objectColor = texture(texBuff, TexCoord).rgb;

    vec3 ambient = ka * 0.2;
    vec3 diffuseTotal = vec3(0.0);
    vec3 specularTotal = vec3(0.0);

    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vec3(fragPos));

    for(int i = 0; i < 3; i++) {
        if(lightOn[i]) {
            float d = length(lightPos[i] - vec3(fragPos));
            float att = 1.0 / (1.0 + kl[i] * d + kq[i] * (d * d));

            vec3 L = normalize(lightPos[i] - vec3(fragPos));
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = kd * diff * lightColor * att; 
            diffuseTotal += diffuse;

            vec3 R = normalize(reflect(-L, N));
            float spec = max(dot(R, V), 0.0);
            spec = pow(spec, q);
            vec3 specular = ks * spec * lightColor * att;
            specularTotal += specular;
        }
    }

    vec3 result = (ambient + diffuseTotal + specularTotal) * objectColor;
    color = vec4(result, 1.0);
}
)";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Modulo 6 - Trajetorias - Isadora Albano", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    stbi_set_flip_vertically_on_load(true);

    GLuint shaderID = setupShader();

    int nVerts;
    GLuint vaoOBJ = loadOBJ("../assets/Modelos3D/Suzanne.obj", &nVerts);
    MaterialPhong mat = loadMTL("../assets/Modelos3D/Suzanne.mtl");

    if (vaoOBJ) {
        object.VAO = vaoOBJ;
        object.nVertices = nVerts;
        object.textureID = mat.diffuseMap;
        object.position = glm::vec3(0.0f, 0.0f, 0.0f);
        object.scale = glm::vec3(0.5f);
        object.rotation = glm::vec3(0.0f);
    } else {
        cout << "Falha ao carregar OBJ." << endl;
    }

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    glUniform3f(glGetUniformLocation(shaderID, "ka"), mat.ka.r, mat.ka.g, mat.ka.b);
    glUniform3f(glGetUniformLocation(shaderID, "kd"), mat.kd.r, mat.kd.g, mat.kd.b);
    glUniform3f(glGetUniformLocation(shaderID, "ks"), mat.ks.r, mat.ks.g, mat.ks.b);
    glUniform1f(glGetUniformLocation(shaderID, "q"), mat.q);

    cout << "\n=== CONTROLES ===" << endl;
    cout << "[CÂMERA] Mouse: Olhar ao redor | WASD: Andar" << endl;
    cout << "[OBJETO] Setas: Mover objeto | X,Y,Z: Rotacionar | [ ]: Escala" << endl;
    cout << "[TRAJETORIA] P: Salvar Ponto Atual | ESPACO: Iniciar/Pausar Animacao" << endl;
    cout << "[LUZES] 1: Key Light | 2: Fill Light | 3: Back Light" << endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(0, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(3, deltaTime);

        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        
        glUniform3f(glGetUniformLocation(shaderID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);

        glm::vec3 objPos = object.position;
        
        glm::vec3 keyPos = objPos + glm::vec3(2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderID, "lightPos[0]"), keyPos.x, keyPos.y, keyPos.z);
        glUniform1i(glGetUniformLocation(shaderID, "lightOn[0]"), keyLightOn);
        glUniform1f(glGetUniformLocation(shaderID, "kl[0]"), 0.09f); 
        glUniform1f(glGetUniformLocation(shaderID, "kq[0]"), 0.032f);

        glm::vec3 fillPos = objPos + glm::vec3(-2.0f, 1.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderID, "lightPos[1]"), fillPos.x, fillPos.y, fillPos.z);
        glUniform1i(glGetUniformLocation(shaderID, "lightOn[1]"), fillLightOn);
        glUniform1f(glGetUniformLocation(shaderID, "kl[1]"), 0.35f); 
        glUniform1f(glGetUniformLocation(shaderID, "kq[1]"), 0.44f);

        glm::vec3 backPos = objPos + glm::vec3(0.0f, 2.0f, -3.0f);
        glUniform3f(glGetUniformLocation(shaderID, "lightPos[2]"), backPos.x, backPos.y, backPos.z);
        glUniform1i(glGetUniformLocation(shaderID, "lightOn[2]"), backLightOn);
        glUniform1f(glGetUniformLocation(shaderID, "kl[2]"), 0.14f); 
        glUniform1f(glGetUniformLocation(shaderID, "kq[2]"), 0.07f);

        if (object.isMoving && object.pathPoints.size() >= 2) {
            glm::vec3 target = object.pathPoints[object.currentTarget];
            glm::vec3 direction = target - object.position;
            float distance = glm::length(direction);
            float step = 2.0f * deltaTime; 

            if (distance <= step) {
                object.position = target;
                object.currentTarget = (object.currentTarget + 1) % object.pathPoints.size();
            } else {
                object.position += glm::normalize(direction) * step;
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, object.textureID);
        glBindVertexArray(object.VAO);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glm::mat4 model = object.getModelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(object.getNormalMatrix()));

        glDrawArrays(GL_TRIANGLES, 0, object.nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }
    
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLightOn = !keyLightOn;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLightOn = !fillLightOn;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLightOn = !backLightOn;

    if (key == GLFW_KEY_X && (action == GLFW_PRESS || action == GLFW_REPEAT)) 
        object.rotation.x += rotSpeed;
    if (key == GLFW_KEY_Y && (action == GLFW_PRESS || action == GLFW_REPEAT)) 
        object.rotation.y += rotSpeed;
    if (key == GLFW_KEY_Z && (action == GLFW_PRESS || action == GLFW_REPEAT)) 
        object.rotation.z += rotSpeed;

    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        object.scale -= glm::vec3(scaleSpeed);
        if (object.scale.x < 0.1f) object.scale = glm::vec3(0.1f);
    }
    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        object.scale += glm::vec3(scaleSpeed);
    }

    // Controle de Trajetórias
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        object.pathPoints.push_back(object.position);
        cout << "Ponto Adicionado (" << object.position.x << ", " << object.position.y << ", " << object.position.z << ")" << endl;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        object.isMoving = !object.isMoving;
        cout << "Animacao " << (object.isMoving ? "INICIADA" : "PAUSADA") << endl;
    }

    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        object.position.y += moveSpeed;
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        object.position.y -= moveSpeed;
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        object.position.x -= moveSpeed;
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        object.position.x += moveSpeed; 
    if (key == GLFW_KEY_PAGE_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        object.position.y += moveSpeed;
    if (key == GLFW_KEY_PAGE_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        object.position.y -= moveSpeed;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(firstMouse) {
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

int setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERRO::VERTEX_SHADER\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERRO::FRAGMENT_SHADER\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERRO::SHADER_PROGRAM_LINKING\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
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
    MaterialPhong mat = {glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(1.0f), 32.0f, 0};
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
        GLenum format = (nrCh == 4) ? GL_RGBA : GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    return texID;
}