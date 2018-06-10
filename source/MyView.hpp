#pragma once

#include <sponza/sponza_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_map>

class MyView : public tygra::WindowViewDelegate
{
public:

	MyView();

	~MyView();

	void setScene(const sponza::Context * scene);

public:

	void ToggleShowCannyEdgeDetection();

private:

	void windowViewWillStart(tygra::Window * window) override;

	void windowViewDidReset(tygra::Window * window,
		int width,
		int height) override;

	void windowViewDidStop(tygra::Window * window) override;

	void windowViewRender(tygra::Window * window) override;

	const sponza::Context * scene_;

private:

	/* Helper function to Create a buffer
	*	buffer - the address of the buffer to be written to.
	*	target - which type of buffer (e.g. GL_ARRAY_BUFFER)
	*	size - size of the buffer(in bytes)
	*	data - pointer to the data itself
	* @return NA
	*/
	void CreateBuffer(GLuint* buffer, GLenum target, GLsizeiptr size, const void* data);

	/* Helper function to Create a shader
	*	type - type of shader file (e.g. Fragment or Vertex)
	*	uri - location of the shader file
	* @return GLuint
	*/
	GLuint CreateShader(GLenum type, const char* uri);

	/* Helper function to Create a Uniform Buffer
	*	buffer - reference to the uniform buffer
	*	size - size of the buffer in bytes
	*	name - corresponding name to be located in the glsl shader code
	*	program - program to attach uniform buffer to
	* @return GLuint
	*/
	void CreateUniformBuffer(GLuint* buffer, GLsizeiptr size, const GLchar* name, GLuint& program);


	/* Helper function to Create a texture
	*	texture - a GLuint type representing a single texture
	*	name - name of the texture which will be searched for in the content folder
	* @return NA
	*/
	void CreateTextureImage(GLuint* texture, std::string name);

	bool CreateCubeMap(GLuint* texture, std::string name, GLenum type);

	void CreateVBOs();
	void FillMeshData();
	void CreateVAOs();
	void CreateGbufferObjects();
	void CreateLbufferObjects();
	void CreateShaderProgram(GLuint& shaderProgram, const std::string vsName, const std::string fsName, std::vector<std::string> vertexAtrribs);
	void CreateQuadMesh();
	void CreateSphereMesh();
	void CreateConeMesh();
	void CreateSkyboxMesh();
	void PhaseOne();
	void PhaseTwo();
	void PhaseThree();
	void UpdateShadowMap(const glm::vec3 light_position, const glm::vec3 light_direction, const float ortho_size, float near, float far);
	glm::mat4 CalculateLightVP(glm::vec3 lightPosition, glm::vec3 light_direction, float orthoSize, float near, float far);
	void DrawPointLights();
	void DrawPointLight(const sponza::PointLight& point_light, const glm::vec3 direction, const float orthoSize, const glm::mat4& camera_projection_view_xform);
	void DrawSpotLight(const sponza::SpotLight& spot_light, const float orthoSize, const glm::mat4& camera_projection_view_xform);
	void DrawDirectionalLights();
	void DrawSpotLights();
	void PostProcessingPhase();
	void UpdateShadowCubeMap(glm::vec3 light_position, glm::vec3 light_direction);
	void UpdateShadowMapPerspective(glm::vec3 light_position, glm::vec3 light_direction, float angleRadians, float near, float far);

private:

	bool showCannyEdgeDetection = false;

	const float SHADOW_WIDTH{ 1024 }; //		1280, 1024, 4096
	const float SHADOW_HEIGHT{ 1024 };//1920	720
	const float SHADOW_POINT_WIDTH{ 256 }; //		1280, 1024
	const float SHADOW_POINT_HEIGHT{ 256 };

	 /* Data representing a texture object
	(all the data I need when rendering and binding textures)*/
	struct TextureObject
	{
		GLuint object;
		int offset;
		//	glm::vec2 dimensions;
	};
	TextureObject skyBox[6];

	// Texture Buffer Objects
	GLuint gbuffer_position_tex_{ 0 };
	GLuint gbuffer_normal_tex_{ 0 };
	GLuint gbuffer_ambient_tex_{ 0 };
	GLuint gbuffer_specular_tex_{ 0 };
	GLuint lbuffer_colour_tex_{ 0 };
	GLuint shadow_depth_tex_{ 0 };
	GLuint postprocessing_tex{ 0 };
	GLuint skybox_tex{ 0 };
	GLuint shadowmap_cube_tex{ 0 };

	// Frame Buffer Objects
	GLuint gbuffer_fbo_{ 0 };
	GLuint lbuffer_fbo_{ 0 };
	GLuint shadow_fbo{ 0 };
	GLuint shadow_cube_fbo{ 0 };
	GLuint postprocessing_fbo{ 0 };
	GLuint skybox_fbo_{ 0 };

//	GLuint lbuffer_multisampled_fbo{ 0 }; // note, do I multisample the gbuffer? I dont think so
	// remember to glEnable(GL_MULTISAMPLE); at some point
	
	// Render Buffer Objects
	GLuint depth_stencil_rbo_{ 0 };

	GLuint depth_stencil_multisampled_rbo_{ 0 };
	

	// Vertex Buffer Objects
	GLuint vertex_vbo{ 0 };
	GLuint element_vbo{ 0 };

	// Vertex Array Object
	GLuint vao_sponza{ 0 };
	GLuint vao_friends{ 0 };

	// Uniform Buffer Object (UBO)
	GLuint ubo_per_frame{ 0 }; // Called less often
	GLuint ubo_per_model{ 0 }; // (a.k.a Perdraw) Called frequently, holds model data
	GLuint current_index{ 0 };	// to keep track of the current number of UBOs

	 // Uniform Buffer Structs (padding is used here but not in glsl)
	struct PerFrameUniforms
	{
		glm::mat4 projection_view_xform;
	};
	struct PerModelUniforms // (a.k.a PerDraw)
	{
		glm::mat4 model_xform;
	};

	// Programs
	GLuint shader_prog{ 0 };
	GLuint directional_light_prog{ 0 };
	GLuint point_light_prog{ 0 };
	GLuint spot_light_prog{ 0 };
	GLuint shadow_prog{ 0 };
	GLuint post_processing_prog{ 0 };
	GLuint pp_gamma_prog{ 0 };
	GLuint skybox_prog{ 0 };
	GLuint shadow_cube_prog{ 0 };
	// Per-Vertex data
	// Used so that the data is interleaved improving efficency
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texcoord;
	};
	// Represents a mesh
	struct MeshGL
	{
		// Vertex data
		Vertex vertex;

		GLuint first_element_index;
		GLsizei element_count;
		GLuint first_vertex_index;
	};


	// A collection of every indivual mesh, identified by a 'MeshID'
	/* providing an easy to use container as every mesh has a MeshID which
	can be accessed. This data is created in the start and therefore is efficent as
	no unecessary operations are required in the render loop.*/
	std::unordered_map<sponza::MeshId, MeshGL> meshes_;

	// (to avoid magic numbers)
	const static GLuint kNullId = 0;
	// Indexes
	enum VertexAttribIndexes
	{
		kVertexPosition = 0,
		kVertexNormal = 1,
		kVertexTexCoord = 2
	};
	enum FragmentDataIndexes
	{
		kFragmentColour = 0
	};

	//
	struct LightMesh
	{
		GLuint vertex_vbo{ 0 };
		GLuint element_vbo{ 0 };
		GLuint vao{ 0 };
		int element_count{ 0 };
	};
	LightMesh light_quad_mesh_; // vertex array of vec2 position
	LightMesh light_sphere_mesh_; // element array into vec3 position
	LightMesh light_cone_mesh;
	LightMesh skybox_mesh;
};
