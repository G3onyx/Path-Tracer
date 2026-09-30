#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader(std::string vertexPath, std::string fragmentPath);
    Shader(std::string computePath);
    ~Shader();

    void use() const;

    // Uniform setters
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;

    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setIVec2(const std::string& name, const glm::ivec2& value) const;

    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    unsigned int ID;
    mutable std::unordered_map<std::string, int> locationMap;

    int getLocation(const std::string& name) const;

    std::string readFile(const std::string& filename);

    void checkCompileErrors(unsigned int shader, std::string type);
};