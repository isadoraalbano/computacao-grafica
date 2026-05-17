#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD e GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const GLuint WIDTH = 1000, HEIGHT = 1000;

bool rotateX = false, rotateY = false, rotateZ = false;
float rotationAngle = 0.0f;
int activeObject = 0;

// Estrutura para os Objetos 3D
struct Object3D {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    GLuint VAO;
    GLuint textureID;
    int vertexCount;

    Object3D(glm::vec3 pos, GLuint vao, GLuint texID, int vCount)
        : position(pos), rotation(glm::vec3(0.0f)), scale(glm::vec3(0.50f)),
          VAO(vao), textureID(texID), vertexCount(vCount) {}

    glm::mat4 getModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

vector<Object3D> sceneObjects;

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
GLuint loadOBJ(const char* path, int* outVertexCount);
GLuint loadTexture(const char* texturePath);
string loadMTL(const char* mtlPath);

// SHADERS
const GLchar* vertexShaderSource = "#version 450 core\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"layout (location = 2) in vec2 texCoord;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"out vec2 TexCoord;\n"
"void main() {\n"
"  gl_Position = model * vec4(position, 1.0);\n"
"  finalColor = vec4(color, 1.0);\n"
"  TexCoord = texCoord;\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 450 core\n"
"in vec4 finalColor;\n"
"in vec2 TexCoord;\n"
"out vec4 color;\n"
"uniform sampler2D tex_buffer;\n"
"void main() {\n"
"  color = texture(tex_buffer, TexCoord);\n"
"}\n\0";

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Modulo 3 - Isadora Albano", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    // Carregar textura
    GLuint texture = loadTexture("../assets/Modelos3D/Suzanne.png");

    // Carregando o modelo OBJ
    int vertexCount;
    GLuint vao = loadOBJ("../assets/Modelos3D/Suzanne.obj", &vertexCount);

    if (vao) {
        Object3D obj(glm::vec3(0.0f, 0.0f, 0.0f), vao, texture, vertexCount);
        obj.rotation.y = glm::radians(180.0f);  // Virar de frente
        sceneObjects.push_back(obj);
    }

    cout << "\n--- CONTROLES ---" << endl;
    cout << "[X/Y/Z] - Rotacionar nos eixos X/Y/Z" << endl;
    cout << "[W/A/S/D] - Mover em X/Z" << endl;
    cout << "[I/J] - Mover em Y" << endl;
    cout << "[ / ] - Escala" << endl;
    cout << "[ESC] - Sair" << endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        // Renderizar o objeto Suzanne
        Object3D& obj = sceneObjects[0];

        if (rotateX) {
            rotationAngle += 0.01f;
            obj.rotation.x = rotationAngle;
        } else if (rotateY) {
            rotationAngle += 0.01f;
            obj.rotation.y = rotationAngle;
        } else if (rotateZ) {
            rotationAngle += 0.01f;
            obj.rotation.z = rotationAngle;
        }

        glm::mat4 model = obj.getModelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Ativar e vincular a textura
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.textureID);
        glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

        glBindVertexArray(obj.VAO);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (sceneObjects.empty()) return;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_X) {
            rotateX = !rotateX; rotateY = false; rotateZ = false;
            if (rotateX) rotationAngle = sceneObjects[activeObject].rotation.x;
        }
        if (key == GLFW_KEY_Y) {
            rotateX = false; rotateY = !rotateY; rotateZ = false;
            if (rotateY) rotationAngle = sceneObjects[activeObject].rotation.y;
        }
        if (key == GLFW_KEY_Z) {
            rotateX = false; rotateY = false; rotateZ = !rotateZ;
            if (rotateZ) rotationAngle = sceneObjects[activeObject].rotation.z;
        }
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        Object3D& obj = sceneObjects[activeObject];
        float speed = 0.01f;
        if (key == GLFW_KEY_W) obj.position.y += speed;
        if (key == GLFW_KEY_S) obj.position.y -= speed;
        if (key == GLFW_KEY_A) obj.position.x -= speed;
        if (key == GLFW_KEY_D) obj.position.x += speed;
        if (key == GLFW_KEY_I) obj.position.z -= speed;
        if (key == GLFW_KEY_J) obj.position.z += speed;
        if (key == GLFW_KEY_RIGHT_BRACKET) obj.scale += glm::vec3(0.02f);
        if (key == GLFW_KEY_LEFT_BRACKET) obj.scale -= glm::vec3(0.02f);
    }
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

GLuint loadOBJ(const char* path, int* outVertexCount) {
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;

    ifstream file(path);
    if (!file.is_open()) {
        cout << "ERRO: Nao foi possivel encontrar o arquivo em: " << path << endl;
        return 0;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (type == "vt") {
            glm::vec2 vt;
            iss >> vt.x >> vt.y;
            vt.y = 1.0f - vt.y; 
            texCoords.push_back(vt);
        }
        else if (type == "vn") {
            glm::vec3 vn;
            iss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (type == "f") {
            string v1, v2, v3;
            iss >> v1 >> v2 >> v3;

            auto parseVertex = [&](const string& vertexStr) {
                size_t pos1 = vertexStr.find('/');
                size_t pos2 = vertexStr.find('/', pos1 + 1);

                int vi = stoi(vertexStr.substr(0, pos1)) - 1;
                int ti = (pos1 != string::npos && pos2 != pos1 + 1) ? stoi(vertexStr.substr(pos1 + 1, pos2 - pos1 - 1)) - 1 : 0;
                int ni = (pos2 != string::npos) ? stoi(vertexStr.substr(pos2 + 1)) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                vBuffer.push_back(1.0f);
                vBuffer.push_back(1.0f);
                vBuffer.push_back(1.0f);

                if (ti >= 0 && ti < (int)texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].x);
                    vBuffer.push_back(texCoords[ti].y);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            };

            parseVertex(v1);
            parseVertex(v2);
            parseVertex(v3);
        }
    }

    file.close();

    *outVertexCount = vBuffer.size() / 8;

    if (*outVertexCount == 0) return 0;

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

GLuint loadTexture(const char* texturePath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cout << "ERRO: Falha ao carregar textura: " << texturePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}