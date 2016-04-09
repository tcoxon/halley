#pragma once

namespace Halley
{
	class ResourceLoader;
	class MaterialParameterBinding;

	class Shader
	{
	public:
		explicit Shader(String name);
		virtual ~Shader() {}

		virtual void bind() = 0;
		virtual void compile() = 0;

		virtual void addVertexSource(String src) = 0;
		virtual void addGeometrySource(String src) = 0;
		virtual void addPixelSource(String src) = 0;

		virtual unsigned int getUniformLocation(String name) = 0;
		virtual unsigned int getAttributeLocation(String name) = 0;

	protected:
		String name;
	};
}