#pragma once

#include <raptr/common/filesystem.hpp>
#include <cstdint>

namespace raptr
{

class Shader
{
public:
  Shader(const FileInfo& shader_path)
    : path(shader_path) {  }
  virtual ~Shader() = default;
  virtual int32_t type() = 0;

public:
  FileInfo path;
};

class FragmentShader : public Shader
{
  int32_t type() { return GL_FRAGMENT_SHADER; }
};

class VertexShader : public Shader
{
  int32_t type() { return GL_VERTEX_SHADER; }
};

class ShaderProgram
{
public:
  ShaderProgram(const FileInfo& shader_toml);
  FragmentShader frag;
  VertexShader vert;
};

}