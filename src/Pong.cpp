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

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

// Estrutura de Material atualizada para suportar Textura
struct MaterialPhong {
    vec3 ka;  // Ambiente
    vec3 kd;  // Difusa
    vec3 ks;  // Especular
    float q;  // Brilho (Shininess)
    GLuint diffuseMap; // ID da Textura
};

// Estrutura do Objeto
struct Object3D {
    glm::vec3 position;
    glm::vec3 rotation; 
    glm::vec3 scale;    
    bool rotX, rotY, rotZ; 

    GLuint VAO;
    int vertexCount;

    Object3D(glm::vec3 pos, GLuint vao, int vCount) 
        : position(pos), rotation(glm::vec3(0.0f)), scale(glm::vec3(1.0f)), 
          rotX(false), rotY(false), rotZ(false), 
          VAO(vao), vertexCount(vCount) {}
    
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

vector<Object3D> sceneObjects;
int activeObject = 0;

// Protótipos
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
GLuint loadOBJ(const char* path, int* outVertexCount);
GLuint loadTexture(string filePath, int &width, int &height);
MaterialPhong loadMTL(const char* mtlPath);

const GLuint WIDTH = 800, HEIGHT = 800;

// ========== VERTEX SHADER ==========
const GLchar *vertexShaderSource = R"(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;

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

// ========== FRAGMENT SHADER COM TEXTURA ==========
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

uniform sampler2D texture1; // Recebe a imagem da textura

out vec4 color;

void main()
{
    // A cor base agora vem da Textura em vez da cor do vértice
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

    // Multiplica a iluminação final pela cor da textura
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

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Iluminacao - Phong e Textura", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // Dizemos ao OpenGL para associar a variavel 'texture1' ao slot de textura 0
    glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

    // Carregando Modelos e Materiais
    int nVerts;
    GLuint VAO = loadOBJ("assets/Modelos3D/Suzanne.obj", &nVerts);
    if (VAO) {
        sceneObjects.push_back(Object3D(glm::vec3(0.0f, 0.0f, 0.0f), VAO, nVerts));
    }

    MaterialPhong material = loadMTL("assets/Modelos3D/Suzanne.mtl");

    vec3 lightPos = vec3(2.0f, 3.0f, 2.0f);
    vec3 camPos = vec3(0.0f, 0.0f, 3.0f);

    glUniform3f(glGetUniformLocation(shaderID, "ka"), material.ka.r, material.ka.g, material.ka.b);
    glUniform3f(glGetUniformLocation(shaderID, "kd"), material.kd.r, material.kd.g, material.kd.b);
    glUniform3f(glGetUniformLocation(shaderID, "ks"), material.ks.r, material.ks.g, material.ks.b);
    glUniform1f(glGetUniformLocation(shaderID, "q"), material.q);
    glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(shaderID, "camPos"), camPos.x, camPos.y, camPos.z);

    mat4 view = lookAt(camPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));

    mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        // Ativa a textura do material lido no MTL
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.diffuseMap);

        for (int i = 0; i < (int)sceneObjects.size(); i++) {
            Object3D& obj = sceneObjects[i];

            if (obj.rotX) obj.rotation.x += 0.01f;
            if (obj.rotY) obj.rotation.y += 0.01f;
            if (obj.rotZ) obj.rotation.z += 0.01f;

            mat4 model = obj.getModelMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

            mat4 normalMatrix = obj.getNormalMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "normalMatrix"), 1, GL_FALSE, value_ptr(normalMatrix));

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
    if (sceneObjects.empty()) return;
    Object3D& obj = sceneObjects[activeObject];

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_TAB) activeObject = (activeObject + 1) % sceneObjects.size();
        if (key == GLFW_KEY_X) { obj.rotX = !obj.rotX; obj.rotY = false; obj.rotZ = false; }
        if (key == GLFW_KEY_Y) { obj.rotX = false; obj.rotY = !obj.rotY; obj.rotZ = false; }
        if (key == GLFW_KEY_Z) { obj.rotX = false; obj.rotY = false; obj.rotZ = !obj.rotZ; }
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        float speed = 0.05f;
        if (key == GLFW_KEY_A) obj.position.x -= speed;
        if (key == GLFW_KEY_D) obj.position.x += speed;
        if (key == GLFW_KEY_W) obj.position.y += speed;
        if (key == GLFW_KEY_S) obj.position.y -= speed;
        if (key == GLFW_KEY_RIGHT_BRACKET) obj.scale += glm::vec3(0.02f);
        if (key == GLFW_KEY_LEFT_BRACKET) obj.scale -= glm::vec3(0.02f);
    }
}

GLuint loadOBJ(const char* path, int* outVertexCount) {
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;

    ifstream file(path);
    if (!file.is_open()) {
        cout << "ERRO ao abrir OBJ: " << path << endl;
        return 0;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 v; iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (type == "vt") {
            glm::vec2 vt; iss >> vt.x >> vt.y;
            vt.y = 1.0f - vt.y;
            texCoords.push_back(vt);
        }
        else if (type == "vn") {
            glm::vec3 vn; iss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (type == "f") {
            string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            auto parseVertex = [&](const string& vertexStr) {
                size_t pos1 = vertexStr.find('/');
                size_t pos2 = vertexStr.find('/', pos1 + 1);

                int vi = stoi(vertexStr.substr(0, pos1)) - 1;
                int ti = (pos1 != string::npos && pos2 != pos1 + 1) ? stoi(vertexStr.substr(pos1 + 1, pos2 - pos1 - 1)) - 1 : -1;
                int ni = (pos2 != string::npos) ? stoi(vertexStr.substr(pos2 + 1)) - 1 : -1;

                if (vi >= 0) { vBuffer.push_back(vertices[vi].x); vBuffer.push_back(vertices[vi].y); vBuffer.push_back(vertices[vi].z); } 
                else { vBuffer.insert(vBuffer.end(), {0.0f, 0.0f, 0.0f}); }

                vBuffer.insert(vBuffer.end(), {1.0f, 1.0f, 1.0f});

                if (ni >= 0 && ni < normals.size()) { vBuffer.push_back(normals[ni].x); vBuffer.push_back(normals[ni].y); vBuffer.push_back(normals[ni].z); } 
                else { vBuffer.insert(vBuffer.end(), {0.0f, 1.0f, 0.0f}); }

                if (ti >= 0 && ti < texCoords.size()) { vBuffer.push_back(texCoords[ti].x); vBuffer.push_back(texCoords[ti].y); } 
                else { vBuffer.insert(vBuffer.end(), {0.0f, 0.0f}); }
            };

            parseVertex(v1); parseVertex(v2); parseVertex(v3);
        }
    }

    *outVertexCount = vBuffer.size() / 11;

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (void*)(9 * sizeof(GLfloat))); glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    return VAO;
}

MaterialPhong loadMTL(const char* mtlPath) {
    MaterialPhong mat;
    mat.ka = vec3(0.1f, 0.2f, 0.3f); 
    mat.kd = vec3(0.4f, 0.6f, 0.8f);
    mat.ks = vec3(1.0f); mat.q = 32.0f; 
    mat.diffuseMap = 0; // 0 significa nenhuma textura por padrão

    ifstream file(mtlPath);
    if (!file.is_open()) {
        cout << "Aviso: Arquivo MTL nao encontrado." << endl;
        return mat;
    }

    // Calcula o diretorio base para carregar a imagem da textura a partir do caminho do MTL
    string basePath = string(mtlPath);
    size_t pos = basePath.find_last_of('/');
    if (pos != string::npos) basePath = basePath.substr(0, pos + 1);
    else basePath = "";

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string word; iss >> word;
        if (word == "Ka") iss >> mat.ka.x >> mat.ka.y >> mat.ka.z;
        else if (word == "Kd") iss >> mat.kd.x >> mat.kd.y >> mat.kd.z;
        else if (word == "Ks") iss >> mat.ks.x >> mat.ks.y >> mat.ks.z;
        else if (word == "Ns") iss >> mat.q;
        else if (word == "map_Kd") {
            string texName;
            iss >> texName;
            int w, h;
            // Carrega a textura combinando o diretorio base com o nome do arquivo
            mat.diffuseMap = loadTexture(basePath + texName, w, h);
        }
    }
    return mat;
}

GLuint loadTexture(string filePath, int &width, int &height) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        if (nrChannels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cout << "ERRO: Falha ao carregar textura em: " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

int setupShader() {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vertexShaderSource, NULL); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fragmentShaderSource, NULL); glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    
    GLint success;
    glGetProgramiv(p, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(p, 512, NULL, infoLog);
        cout << "ERRO NA COMPILACAO DOS SHADERS:\n" << infoLog << endl;
    }

    glDeleteShader(v); glDeleteShader(f);
    return p;
}