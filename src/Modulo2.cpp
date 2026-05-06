#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL)
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

// Código fonte do Fragment Shader (em GLSL)
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

// Estrutura para representar um cubo
struct Cube {
    glm::vec3 position;
    glm::vec3 rotation;
    float scale;
    
    Cube(glm::vec3 pos = glm::vec3(0.0f), float s = 1.0f) 
        : position(pos), rotation(glm::vec3(0.0f)), scale(s) {}
    
    glm::mat4 getModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(scale));
        return model;
    }
};

// Vector para armazenar múltiplos cubos
vector<Cube> cubes;
int activeCube = 0; // Índice do cubo ativo

bool rotateX = false, rotateY = false, rotateZ = false;
float rotationAngle = 0.0f;

// Função main
int main()
{
    // Inicialização da GLFW
    glfwInit();

    // Criação da janela GLFW
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Cubo 3D -- Isadora!", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Erro ao inicializar GLAD" << endl;
        return -1;
    }

    // Obtendo as informações de versão
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;
    cout << "Controles:" << endl;
    cout << "X, Y, Z - Rotacionar nos respectivos eixos" << endl;
    cout << "W, A, S, D - Mover nos eixos X e Z" << endl;
    cout << "I, J - Mover nos eixo Y" << endl;
    cout << "[ ] - Diminuir/Aumentar escala" << endl;
    cout << "1, 2 - Selecionar cubo" << endl;

    // Definindo as dimensões da viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Gerando a geometria do cubo
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    glEnable(GL_DEPTH_TEST);

    // Criar dois cubos iniciais
    cubes.push_back(Cube(glm::vec3(-0.4f, 0.0f, 0.0f), 1.0f));
    cubes.push_back(Cube(glm::vec3(0.4f, 0.0f, 0.0f), 0.8f));

    // Loop da aplicação
    while (!glfwWindowShouldClose(window))
    {
        // Checa eventos de input
        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
		glPointSize(20);

        // Renderiza cada cubo
        for (int i = 0; i < cubes.size(); i++)
        {
            Cube& cube = cubes[i];

            if (i == activeCube)
            {
                if (rotateX)
                {
                    rotationAngle += 0.01f;
                    cube.rotation.x = rotationAngle;
                }
                else if (rotateY)
                {
                    rotationAngle += 0.01f;
                    cube.rotation.y = rotationAngle;
                }
                else if (rotateZ)
                {
                    rotationAngle += 0.01f;
                    cube.rotation.z = rotationAngle;
                }
            }

            glm::mat4 model = cube.getModelMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Chamada de desenho - Polígonos preenchidos
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Pede pra OpenGL desalocar os buffers
    glDeleteVertexArrays(1, &VAO);
    // Finaliza a execução da GLFW
    glfwTerminate();
    return 0;
}

// Função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (activeCube < cubes.size())
        {
            Cube& activeCubeRef = cubes[activeCube];

            // Rotação
            if (action == GLFW_PRESS)
            {
                if (key == GLFW_KEY_X)
                {
                    rotateX = !rotateX;
                    rotateY = false;
                    rotateZ = false;
                    if (rotateX)
                        rotationAngle = activeCubeRef.rotation.x;
                }

                if (key == GLFW_KEY_Y)
                {
                    rotateX = false;
                    rotateY = !rotateY;
                    rotateZ = false;
                    if (rotateY)
                        rotationAngle = activeCubeRef.rotation.y;
                }

                if (key == GLFW_KEY_Z)
                {
                    rotateX = false;
                    rotateY = false;
                    rotateZ = !rotateZ;
                    if (rotateZ)
                        rotationAngle = activeCubeRef.rotation.z;
                }

                // Selecionar cubo (teclas 1 e 2)
                if (key == GLFW_KEY_1 || key == GLFW_KEY_2)
                {
                    int cubeIndex = (key == GLFW_KEY_1) ? 0 : 1;
                    activeCube = cubeIndex;
                }
            }

            // Translação - Eixo X
            if (key == GLFW_KEY_A)
            {
                activeCubeRef.position.x -= 0.05f;
            }
            else if (key == GLFW_KEY_D)
            {
                activeCubeRef.position.x += 0.05f;
            }

            // Translação - Eixo Z
            if (key == GLFW_KEY_W)
            {
                activeCubeRef.position.z += 0.05f;
            }
            else if (key == GLFW_KEY_S)
            {
                activeCubeRef.position.z -= 0.05f;
            }

            // Translação - Eixo Y
            if (key == GLFW_KEY_I)
            {
                activeCubeRef.position.y += 0.1f;
            }

            if (key == GLFW_KEY_J)
            {
                activeCubeRef.position.y -= 0.1f;
            }

            // Escala
            if (key == GLFW_KEY_LEFT_BRACKET)
            {
                activeCubeRef.scale -= 0.05f;
                if (activeCubeRef.scale < 0.1f)
                    activeCubeRef.scale = 0.1f;
            }

            if (key == GLFW_KEY_RIGHT_BRACKET)
            {
                activeCubeRef.scale += 0.05f;
                if (activeCubeRef.scale > 5.0f)
                    activeCubeRef.scale = 5.0f;
            }
        }
    }
}

// Função que compila e linka os shaders
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Linkando os shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria a geometria do cubo com 6 faces coloridas
int setupGeometry()
{
    GLfloat vertices[] = {
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,

         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,

         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 1.0f,

        -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
    };

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Atributo posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}