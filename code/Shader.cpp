#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

static const std::string SHADER_PATH = "shaders\\";


Shader::Shader(std::string vertexPath, std::string fragmentPath) {
    std::string vStr = readFile(SHADER_PATH + vertexPath);
    std::string fStr = readFile(SHADER_PATH + fragmentPath);
    const char* vCode = vStr.c_str();
    const char* fCode = fStr.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::Shader(std::string computePath) {
    std::string cStr = readFile(SHADER_PATH + computePath);
    const char* cCode = cStr.c_str();

    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cCode, NULL);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(compute);

}

Shader::~Shader() { glDeleteProgram(ID); }

void Shader::use() const { glUseProgram(ID); }

std::string Shader::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int Shader::getLocation(const std::string& name) const {
    auto it = locationMap.find(name);
    if (it != locationMap.end()) return it->second;

    int location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) 
        std::cerr << "Warning: Uniform '" << name << "' not found!" << std::endl;
    
    locationMap[name] = location;
    return location;
}

// Uniform setters
void Shader::setBool(const std::string& name, bool value) const { glUniform1i(getLocation(name), (int)value); }
void Shader::setInt(const std::string& name, int value) const { glUniform1i(getLocation(name), value); }
void Shader::setFloat(const std::string& name, float value) const { glUniform1f(getLocation(name), value); }

void Shader::setVec2(const std::string& name, const glm::vec2& value) const { glUniform2fv(getLocation(name), 1, glm::value_ptr(value)); }
void Shader::setIVec2(const std::string& name, const glm::ivec2& value) const { glUniform2iv(getLocation(name), 1, glm::value_ptr(value)); };

void Shader::setVec3(const std::string& name, const glm::vec3& value) const { glUniform3fv(getLocation(name), 1, glm::value_ptr(value)); }
void Shader::setVec4(const std::string& name, const glm::vec4& value) const { glUniform4fv(getLocation(name), 1, glm::value_ptr(value)); }
void Shader::setMat2(const std::string& name, const glm::mat2& mat) const { glUniformMatrix2fv(getLocation(name), 1, GL_FALSE, glm::value_ptr(mat)); }
void Shader::setMat3(const std::string& name, const glm::mat3& mat) const { glUniformMatrix3fv(getLocation(name), 1, GL_FALSE, glm::value_ptr(mat)); }
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const { glUniformMatrix4fv(getLocation(name), 1, GL_FALSE, glm::value_ptr(mat)); }

void Shader::checkCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << type << "_SHADER_COMPILATION_ERROR\n" << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
        }
    }
}