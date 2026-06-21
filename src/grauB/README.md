# Visualizador de Cenas 3D com OpenGL

Projeto final da disciplina Computação Gráfica. Consiste em um visualizador de cenas (Diorama) capaz de carregar e renderizar modelos 3D complexos através de um arquivo de configuração personalizado.

## 🚀 Funcionalidades
- **Parser de Cena Customizado:** Leitura de arquivos de configuração em formato de texto para inserção de instâncias de luzes, câmeras e objetos (`.obj` e `.mtl`).
- **Iluminação Local de Phong:** Suporte a múltiplas fontes de luz pontuais e direcionais, aplicando coeficientes extraídos automaticamente dos materiais ($Ka, Kd, Ks$).
- **Câmera Sintética (FPS):** Navegação pelo ambiente utilizando teclado e mouse.
- **Interação e Transformação Geométrica:** Seleção de instâncias em tempo real via teclado, aplicando operações de Escala, Translação e Rotação usando a matriz Model.
- **Animação Paramétrica:** Criação dinâmica de trajetórias suaves em tempo real utilizando Curvas Cúbicas de Bézier.
OBS: MAIS INFORMAÇÕES DE CONTROLE DA CENA NO TERMINAL AO EXECUTAR

## 🛠️ Setup e Execução

### Dependências
Este projeto requer a configuração das seguintes bibliotecas em seu ambiente:
- **GLFW** (Gerenciamento de janelas e inputs)
- **GLAD** (Carregamento de ponteiros OpenGL)
- **GLM** (Biblioteca matemática para transformações)
- **stb_image** (Carregamento de texturas via cabeçalho)

### Como Compilar
1. Clone o repositório em sua máquina local.
2. Certifique-se de que o CMake ou seu sistema de build (ex: Makefile/Visual Studio) está apontando corretamente para os diretórios `include` e `lib` das dependências.
3. Coloque os arquivos `.obj`, `.mtl`, as imagens de textura e o `config.txt` na pasta `assets` na raiz do executável.
4. Compile o arquivo principal `Diorama.cpp`.

## 📦 Assets e Procedência
Os modelos 3D e texturas utilizados no projeto foram obtidos de plataformas gratuitas e processados antes de serem exportados:
- **Processamento:** O software [Blender](https://www.blender.org/) foi utilizado para realizar a quebra de faces complexas (triangularização) e adequação dos mapeamentos UV para exportação dos `.obj` e `.mtl`.
- **Modelo "Sala" / Diorama:** Obtido de https://sketchfab.com/features/free-3d-models
- **Modelo "Gato":** Obtido de https://sketchfab.com/features/free-3d-models
- **Texturas Adicionais:** Retiradas de https://ambientcg.com/ e https://polyhaven.com/textures.

## 📚 Referências e Bibliografia
Para a elaboração desta arquitetura, utilizei os seguintes materiais como referência:
- Material de Aula da Disciplina de Fundamentos de Computação Gráfica - UNISINOS (Prof. Rossana e Guilherme).
- [LearnOpenGL](https://learnopengl.com/) - Joey de Vries (Guias de Lighting e Shaders).
- [Anton's OpenGL 4 Tutorials](http://antongerdelan.net/opengl/) - Anton Gerdelan.
- Documentação oficial da biblioteca matemática [GLM](https://glm.g-truc.net/0.9.8/index.html).

## LINK GRAVAÇÃO:
https://asavbrm-my.sharepoint.com/:v:/g/personal/isadoraalbano_edu_unisinos_br/IQD62TkqOklDRLzlnNUcHQyTAdTG9SRW0s4fLqasLO365G8?e=YFk6cN&nav=eyJyZWZlcnJhbEluZm8iOnsicmVmZXJyYWxBcHAiOiJTdHJlYW1XZWJBcHAiLCJyZWZlcnJhbFZpZXciOiJTaGFyZURpYWxvZy1MaW5rIiwicmVmZXJyYWxBcHBQbGF0Zm9ybSI6IldlYiIsInJlZmVycmFsTW9kZSI6InZpZXcifX0%3D