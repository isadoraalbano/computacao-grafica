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
    int vertexCount;

    Object3D(glm::vec3 pos, GLuint vao, int vCount) 
        : position(pos), rotation(glm::vec3(0.0f)), scale(glm::vec3(0.30f)), 
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
};

vector<Object3D> sceneObjects;

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
GLuint loadOBJ(const char* path, int* outVertexCount);

// Shaders
const GLchar* vertexShaderSource = "#version 450 core\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main() {\n"
"  gl_Position = model * vec4(position, 1.0);\n"
"  finalColor = vec4(color, 1.0);\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 450 core\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main() {\n"
"  color = finalColor;\n"
"}\n\0";

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial 1 - Isadora Albano", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    // Carregando os dois modelos
    int count1, count2;
    GLuint vao1 = loadOBJ("../assets/Modelos3D/Cube.obj", &count1);
    GLuint vao2 = loadOBJ("../assets/Modelos3D/Suzanne.obj", &count2);

    if (vao1) sceneObjects.push_back(Object3D(glm::vec3(-0.5f, 0.0f, 0.0f), vao1, count1));
    if (vao2) sceneObjects.push_back(Object3D(glm::vec3(0.5f, 0.0f, 0.0f), vao2, count2));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < (int)sceneObjects.size(); i++) {
            Object3D& obj = sceneObjects[i];

            if (i == activeObject) {
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
            }

            glm::mat4 model = obj.getModelMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(obj.VAO);
            glPolygonMode(GL_FRONT_AND_BACK, (i == activeObject) ? GL_LINE : GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
        }
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (sceneObjects.empty()) return;
    Object3D& obj = sceneObjects[activeObject];

    if (action == GLFW_PRESS) {
        // Seleção de objeto (Teclas 1 e 2)
        if (key == GLFW_KEY_1) activeObject = 0;
        if (key == GLFW_KEY_2 && sceneObjects.size() > 1) activeObject = 1;

        if (key == GLFW_KEY_X) {
            rotateX = !rotateX; rotateY = false; rotateZ = false;
            if (rotateX) rotationAngle = obj.rotation.x;
        }
        if (key == GLFW_KEY_Y) {
            rotateX = false; rotateY = !rotateY; rotateZ = false;
            if (rotateY) rotationAngle = obj.rotation.y;
        }
        if (key == GLFW_KEY_Z) {
            rotateX = false; rotateY = false; rotateZ = !rotateZ;
            if (rotateZ) rotationAngle = obj.rotation.z;
        }
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
    vector<glm::vec3> temp_vertices;
    vector<GLfloat> vertex_data;
    ifstream file(path);
    if (!file.is_open()) { cout << "Erro ao abrir OBJ: " << path << endl; return 0; }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type; iss >> type;
        if (type == "v") {
            glm::vec3 v; iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        } else if (type == "f") {
            string s1, s2, s3; iss >> s1 >> s2 >> s3;
            auto getIdx = [](string t) { return stoi(t.substr(0, t.find('/'))) - 1; };
            int indices[3] = { getIdx(s1), getIdx(s2), getIdx(s3) };
            for (int i : indices) {
                vertex_data.push_back(temp_vertices[i].x);
                vertex_data.push_back(temp_vertices[i].y);
                vertex_data.push_back(temp_vertices[i].z);
                vertex_data.push_back(0.8f); vertex_data.push_back(0.7f); vertex_data.push_back(0.2f);
            }
        }
    }
    *outVertexCount = vertex_data.size() / 6;
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(GLfloat), vertex_data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    return VAO;
}

int setupShader() {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vertexShaderSource, NULL); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fragmentShaderSource, NULL); glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}