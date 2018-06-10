#include "MyView.hpp"

#include <sponza/sponza.hpp>
#include <tygra/FileHelper.hpp>
#include <tsl/shapes.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cassert>
#include <vector>

#include <nvToolsExt.h> // For NSight performance 

MyView::MyView()
{
}

MyView::~MyView() 
{
}

void MyView::setScene(const sponza::Context * scene)
{
    scene_ = scene;
}

void MyView::ToggleShowCannyEdgeDetection()
{
	showCannyEdgeDetection = !showCannyEdgeDetection;
}



void MyView::windowViewWillStart(tygra::Window * window)
{
	assert(scene_ != nullptr);

	// Create Light Meshes
	CreateQuadMesh();
	CreateSphereMesh();
	CreateConeMesh();
	CreateSkyboxMesh();

	// Create Programs
	std::vector<std::string> shaderVertexAtrribs;
	shaderVertexAtrribs.push_back("vertex_position");
	shaderVertexAtrribs.push_back("vertex_normal");
	shaderVertexAtrribs.push_back("vertex_texcoord");
	CreateShaderProgram(shader_prog, "global_light_vs", "global_light_fs", shaderVertexAtrribs);

	std::vector<std::string>  lightVertexAtrribs;
	lightVertexAtrribs.push_back("vertex_position");
	CreateShaderProgram(directional_light_prog, "directional_light_vs", "directional_light_fs", lightVertexAtrribs);
	CreateShaderProgram(point_light_prog, "point_light_vs", "point_light_fs", lightVertexAtrribs);
	CreateShaderProgram(spot_light_prog, "spot_light_vs", "spot_light_fs", lightVertexAtrribs);

	std::vector<std::string>  shadowVertexAtrribs;
	shadowVertexAtrribs.push_back("vertex_position");
	CreateShaderProgram(shadow_prog, "shadow_vs", "shadow_fs", shadowVertexAtrribs);
	CreateShaderProgram(shadow_cube_prog, "shadow_cube_vs", "shadow_cube_fs", shadowVertexAtrribs);

	std::vector<std::string>  postVertexAtrribs;
	postVertexAtrribs.push_back("vertex_position");
	CreateShaderProgram(post_processing_prog, "pp_vs", "pp_fs", postVertexAtrribs);
	CreateShaderProgram(pp_gamma_prog, "pp_gamma_vs", "pp_gamma_fs", postVertexAtrribs);

	std::vector<std::string>  skyboxVertexAtrribs;
	skyboxVertexAtrribs.push_back("vertex_position");
	CreateShaderProgram(skybox_prog, "skybox_vs", "skybox_fs", skyboxVertexAtrribs);


	// Create Uniform Buffers
	CreateUniformBuffer(&ubo_per_frame, sizeof(PerFrameUniforms), "PerFrameUniforms", shader_prog);
	CreateUniformBuffer(&ubo_per_model, sizeof(PerModelUniforms), "PerModelUniforms", shader_prog);

	// Create and Fill Mesh data and corresponding VBOs + VAOs 
	CreateVBOs();
	FillMeshData();
	CreateVAOs();



	// Create Gbuffer Objects (including textures, FBOs and RBOs)
	CreateGbufferObjects();
	// Create Lbuffer Objects
	CreateLbufferObjects();

	// Create Shadow depth texture
	glGenFramebuffers(1, &shadow_fbo);
	glGenTextures(1, &shadow_depth_tex_);

	// Create Post Processing 
	glGenFramebuffers(1, &postprocessing_fbo);
	glGenTextures(1, &postprocessing_tex);

	// Create shadow cubemap
	{
		glGenFramebuffers(1, &shadow_cube_fbo);
		glGenTextures(1, &shadowmap_cube_tex);
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_cube_fbo);
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowmap_cube_tex);

		
		//for (int i = 0; i < 6; i++)
		//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, shadowmap_cube_tex, 0);
		for (int i = 0; i < 6; i++)
		{
			//glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, (GLsizei)1024, (GLsizei)1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	

			//glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, shadowmap_cube_tex, 0);
			{
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}

		/*	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, shadowmap_cube_tex, 0);
			{
				GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
				glDrawBuffers(1, DrawBuffers);
			}*/
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
		{
			tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//
	// Create Texture objects
	// Create Sky Box
	glGenFramebuffers(1, &skybox_fbo_);
	glGenTextures(1, &skybox_tex);

	// Sky Box
	glBindFramebuffer(GL_FRAMEBUFFER, skybox_fbo_);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);
	std::string names[6] = { "right.png", "left.png", "bottom.png", "top.png", "back.png", "front.png" };
	for (int i = 0; i < 6; i++)
	{
		TextureObject&  newTextureObject = skyBox[i];

		if(CreateCubeMap(&newTextureObject.object, names[i], GL_TEXTURE_CUBE_MAP_POSITIVE_X + i))
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, skybox_tex, 0);
		else 
			tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "CubeMapTexture not create.....d");

		newTextureObject.offset = i;
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//http://antongerdelan.net/opengl/cubemaps.html
	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
	{
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	
	// do I need?
	    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
}



void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
    glViewport(0, 0, width, height);
	

	// Create a new renderbuffer object using the gbuffer_colour_rbo_id

	// 2.1 Add a new framebuffer object to act as the gbuffer
	//glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_rbo_);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// 2.2 Render buffer for depth, position and normal (per-pixel geometry)? rather than textures
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer_fbo_);
	
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, gbuffer_colour_rbo_);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_rbo_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);//
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_rbo_);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); // <--- fill later (when?), also GL_Texture_2D?
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, gbuffer_position_tex_, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gbuffer_position_tex_, 0);
	

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); // <--- fill later (when?)
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, gbuffer_normal_tex_, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_ambient_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); 
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gbuffer_ambient_tex_, 0);
	
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_specular_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, gbuffer_specular_tex_, 0);

	{
		GLenum DrawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, DrawBuffers);
		
	}

	GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) 
	{
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shadow
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
	glBindTexture(GL_TEXTURE_2D, shadow_depth_tex_);//GL_TEXTURE_RECTANGLE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_depth_tex_, 0);//GL_DEPTH_STENCIL_ATTACHMENT
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth_tex_, 0);
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
	{
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//

	// lbuffer
	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);

	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, lbuffer_colour_rbo_);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_stencil_rbo_);

	glBindTexture(GL_TEXTURE_RECTANGLE, lbuffer_colour_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_SRGB, width, height, 0, GL_RGB, GL_FLOAT, nullptr); // was GL_RGB8 before gamma correction
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, lbuffer_colour_tex_, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lbuffer_colour_tex_, 0);
	{
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
	}

	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
	{
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Post processing
	glBindFramebuffer(GL_FRAMEBUFFER, postprocessing_fbo);

	glBindTexture(GL_TEXTURE_RECTANGLE, postprocessing_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_SRGB, width, height, 0, GL_RGB, GL_FLOAT, nullptr); // was GL_RGB8 before gamma correction (GL_SRGB)
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, postprocessing_tex, 0);

	{
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
	}

	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE)
	{
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//
}



void MyView::windowViewDidStop(tygra::Window * window)
{
	// buffer objects
	glDeleteVertexArrays(1, &vao_sponza);
	glDeleteVertexArrays(1, &vao_friends);
	glDeleteBuffers(1, &vertex_vbo);
	glDeleteBuffers(1, &element_vbo);


	// Delete Programs
	glDeleteProgram(shader_prog);
	glDeleteProgram(directional_light_prog);
	glDeleteProgram(point_light_prog);
	glDeleteProgram(spot_light_prog);
	glDeleteProgram(shadow_prog);
	glDeleteProgram(shadow_cube_prog);
	glDeleteProgram(post_processing_prog);
	glDeleteProgram(pp_gamma_prog);
	glDeleteProgram(skybox_prog);

	// Delete FBOs
	glDeleteFramebuffers(1, &gbuffer_fbo_);
	glDeleteFramebuffers(1, &lbuffer_fbo_);
	glDeleteFramebuffers(1, &shadow_fbo);
	glDeleteFramebuffers(1, &postprocessing_fbo);
	glDeleteFramebuffers(1, &skybox_fbo_);
	glDeleteFramebuffers(1, &shadow_cube_fbo);

	// Delete RBOs
	//glDeleteRenderbuffers(1, &lbuffer_colour_rbo_);
	//glDeleteRenderbuffers(1, &gbuffer_colour_rbo_);
	glDeleteRenderbuffers(1, &depth_stencil_rbo_);

	// Delete UBOs
	glDeleteBuffers(1, &ubo_per_frame);
	glDeleteBuffers(1, &ubo_per_model);

	// Delete Textures
	glDeleteTextures(1, &gbuffer_normal_tex_);
	glDeleteTextures(1, &gbuffer_position_tex_);
	glDeleteTextures(1, &gbuffer_ambient_tex_);
	glDeleteTextures(1, &gbuffer_specular_tex_);
	glDeleteTextures(1, &lbuffer_colour_tex_);
	glDeleteTextures(1, &shadow_depth_tex_);
	glDeleteTextures(1, &postprocessing_tex);
	glDeleteTextures(1, &skybox_tex);
	glDeleteTextures(1, &shadowmap_cube_tex);

	// Light Quad Mesh
	glDeleteBuffers(1, &light_quad_mesh_.vertex_vbo);
	glDeleteBuffers(1, &light_quad_mesh_.element_vbo);
	glDeleteVertexArrays(1, &light_quad_mesh_.vao);

	// Skybox Mesh
	glDeleteBuffers(1, &skybox_mesh.vertex_vbo);
	glDeleteBuffers(1, &skybox_mesh.element_vbo);
	glDeleteVertexArrays(1, &skybox_mesh.vao);

	// Sphere Mesh
	glDeleteBuffers(1, &light_sphere_mesh_.vertex_vbo);
	glDeleteBuffers(1, &light_sphere_mesh_.element_vbo);
	glDeleteVertexArrays(1, &light_sphere_mesh_.vao);

	// Cone Mesh
	glDeleteBuffers(1, &light_cone_mesh.vertex_vbo);
	glDeleteBuffers(1, &light_cone_mesh.element_vbo);
	glDeleteVertexArrays(1, &light_cone_mesh.vao);
}

void MyView::windowViewRender(tygra::Window * window)
{
    assert(scene_ != nullptr);

	// Setup Gbuffer + textures
	#ifdef  TDK_NVTX
	nvtxRangePushA("PhaseOne");
	#endif //  TDK_NVTX

	PhaseOne();

	#ifdef TDK_NVTX
	nvtxRangePop(); // PhaseOne
	#endif // TDK_NVTX



	#ifdef  TDK_NVTX
		nvtxRangePushA("PhaseTwo");
	#endif //  TDK_NVTX

	// Shade to Lbuffer
	PhaseTwo();

	#ifdef TDK_NVTX
		nvtxRangePop(); // PhaseTwo
	#endif // TDK_NVTX


	#ifdef  TDK_NVTX
		nvtxRangePushA("PostProcessingPhase");
	#endif //  TDK_NVTX

	// Apply post processing effects using the LBuffer texture
	PostProcessingPhase();

	#ifdef TDK_NVTX
		nvtxRangePop(); // PostProcessingPhase
	#endif // TDK_NVTX


	#ifdef  TDK_NVTX
		nvtxRangePushA("PhaseThree");
	#endif //  TDK_NVTX

	// Output Lbuffer to screen
	PhaseThree();

	#ifdef TDK_NVTX
		nvtxRangePop(); // PhaseThree
	#endif // TDK_NVTX
}

void MyView::UpdateShadowCubeMap(glm::vec3 light_position, glm::vec3 light_direction)
{
#ifdef  TDK_NVTX
	nvtxRangePushA("Shadow Cube Map");
#endif //  TDK_NVTX

	// |--------------------------|
	// | 1. BIND Shadow FBO       |
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_cube_fbo);


	//??
	glCullFace(GL_FRONT);// WAS GL_BACK
						 //glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	//??

	// |----------------------------------|
	// | 2. Set viewport to texture size  |
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	int width = viewport_size[2];
	int height = viewport_size[3];
	glViewport(0, 0, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT);

	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glUseProgram(shadow_cube_prog);
	
	

	const float aspect_ratio = SHADOW_POINT_WIDTH / SHADOW_POINT_HEIGHT;



	// Define the projection matrix
	glm::mat4 projection_xform = glm::perspective(glm::radians(90.0f),
		aspect_ratio,
		0.01f,
		100.0f);


	glm::vec3 directions[6] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
								glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
	glm::vec3 ups[6] = { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
						glm::vec3(0.0f, 0.0f, 1.0f) , glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
	
	for (int i = 0; i < 6; i++)
	{
		// Define the view matrix
		glm::mat4 view_xform = glm::lookAt(light_position,
			light_position + directions[i],
			ups[i]);
		glm::mat4 light_projection_view_xform = projection_xform * view_xform;

	
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, shadowmap_cube_tex, 0);
		//glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, shadowmap_cube_tex, 0);
		glDrawBuffer(GL_NONE);

	
		glBindVertexArray(vao_sponza);
		for (int i = 0; i < scene_->getAllInstances().size() - 3; i++)
		{
			const sponza::Instance& instance = scene_->getAllInstances()[i];
			const MeshGL& mesh = meshes_[instance.getMeshId()];

			glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());

			glm::mat4 mvp = light_projection_view_xform * model_xform;
			glUniformMatrix4fv(glGetUniformLocation(shadow_cube_prog, "modelViewProjection"), 1, GL_FALSE, glm::value_ptr(mvp));
			glUniformMatrix4fv(glGetUniformLocation(shadow_cube_prog, "model_xform"), 1, GL_FALSE, glm::value_ptr(model_xform));
			glUniform3fv(glGetUniformLocation(shadow_cube_prog, "lightPosition"), 1, glm::value_ptr(light_position));
			
			// Draw
			glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
		}

		glBindVertexArray(vao_friends);
		for (int i = scene_->getAllInstances().size() - 3; i < scene_->getAllInstances().size(); i++)
		{
			const sponza::Instance& instance = scene_->getAllInstances()[i];
			const MeshGL& mesh = meshes_[instance.getMeshId()];

			// Define the model matrix
			glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());

			glm::mat4 mvp = light_projection_view_xform * model_xform;
			glUniformMatrix4fv(glGetUniformLocation(shadow_cube_prog, "modelViewProjection"), 1, GL_FALSE, glm::value_ptr(mvp));
			glUniformMatrix4fv(glGetUniformLocation(shadow_cube_prog, "model_xform"), 1, GL_FALSE, glm::value_ptr(model_xform));
			glUniform3fv(glGetUniformLocation(shadow_cube_prog, "lightPosition"), 1, glm::value_ptr(light_position));

			// Draw
			glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
		}

	}



	// cleanup
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glCullFace(GL_BACK);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


#ifdef TDK_NVTX
	nvtxRangePop(); // Shadow Cube Map
#endif // TDK_NVTX
}

void MyView::UpdateShadowMapPerspective(glm::vec3 light_position, glm::vec3 light_direction, float angleRadians, float near, float far)
{
	return;
	// |--------------------------|
	// | 1. BIND Shadow FBO       |
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);

	//??
	glCullFace(GL_FRONT);// WAS GL_BACK
						 //glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	//??

	// |----------------------------------|
	// | 2. Set viewport to texture size  |
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); // todo, shoyuld be shadowsize


												   // |----------------------------------------------------------------|
												   // | 3. Performance: Turn off writes and read to the colour buffer  |
												   // ~~TODO~~
	//glDrawBuffer(GL_NONE);


	// |----------------------------------|
	// | 4. Clear the depth               |
	//glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// |-----------------------------------------------|
	// | 4. Render as normal using the light's matrix  |

	glUseProgram(shadow_prog);

	
	glm::mat4 depthProjectionMatrix = glm::perspective<float>(glm::radians(angleRadians), 1.0f, 1.0f, 50.0f);
	//glm::mat4 depthViewMatrix = glm::lookAt(light_position, light_position - -(light_direction), glm::vec3(0, 1, 0));
	glm::mat4 depthViewMatrix = glm::lookAt(light_position, light_position + light_direction, glm::vec3(0, 1, 0));
	glm::mat4 lightVP = depthProjectionMatrix * depthViewMatrix;

	
	glBindVertexArray(vao_sponza);
	for (int i = 0; i < scene_->getAllInstances().size() - 3; i++)
	{
		const sponza::Instance& instance = scene_->getAllInstances()[i];
		const MeshGL& mesh = meshes_[instance.getMeshId()];


		glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());

		glm::mat4 depthMVP = lightVP * model_xform; // note that direction will have to do all 6 for point light so loop
		glUniformMatrix4fv(glGetUniformLocation(shadow_prog, "depthMVP"), 1, GL_FALSE, glm::value_ptr(depthMVP));

		// Draw
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
	}

	glBindVertexArray(vao_friends);
	for (int i = scene_->getAllInstances().size() - 3; i < scene_->getAllInstances().size(); i++)
	{
		const sponza::Instance& instance = scene_->getAllInstances()[i];
		const MeshGL& mesh = meshes_[instance.getMeshId()];

		// Define the model matrix
		glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());

		glm::mat4 depthMVP = lightVP * model_xform; // note that direction will have to do all 6 for point light so loop
		glUniformMatrix4fv(glGetUniformLocation(shadow_prog, "depthMVP"), 1, GL_FALSE, glm::value_ptr(depthMVP));

		// Draw
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
	}

	// cleanup
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//	glDisable(GL_POLYGON_OFFSET_FILL);
	int width = viewport_size[2];
	int height = viewport_size[3];
	glViewport(0, 0, width, height);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // added to fix shadowmap size
	glCullFace(GL_BACK);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void MyView::UpdateShadowMap(glm::vec3 light_position, glm::vec3 light_direction, float ortho_size, float near, float far)
{
#ifdef  TDK_NVTX
	nvtxRangePushA("Shadow Map");
#endif //  TDK_NVTX

	// |--------------------------|
	// | 1. BIND Shadow FBO       |
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);

	//??
	glCullFace(GL_FRONT);// WAS GL_BACK
	//glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	//??

	// |----------------------------------|
	// | 2. Set viewport to texture size  |
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT); // todo, shoyuld be shadowsize


	// |----------------------------------------------------------------|
	// | 3. Performance: Turn off writes and read to the colour buffer  |
	 // ~~TODO~~
	glDrawBuffer(GL_NONE);


	 // |----------------------------------|
	 // | 4. Clear the depth               |
	//glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	
	//glEnable(GL_POLYGON_OFFSET_FILL);
	//glPolygonOffset(2.0f, 4.0f);

	// |-----------------------------------------------|
	// | 4. Render as normal using the light's matrix  |

	glUseProgram(shadow_prog);

	glm::mat4 lightVP = CalculateLightVP(light_position, light_direction, ortho_size, near, far);
	
	glBindVertexArray(vao_sponza);
	for (int i = 0; i < scene_->getAllInstances().size() - 3; i++)
	{
		const sponza::Instance& instance = scene_->getAllInstances()[i];
		const MeshGL& mesh = meshes_[instance.getMeshId()];


		glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());
		
		glm::mat4 depthMVP = lightVP * model_xform; // note that direction will have to do all 6 for point light so loop
		glUniformMatrix4fv(glGetUniformLocation(shadow_prog, "depthMVP"), 1, GL_FALSE, glm::value_ptr(depthMVP));

		// Draw
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
	}

	glBindVertexArray(vao_friends);
	for (int i = scene_->getAllInstances().size() - 3; i < scene_->getAllInstances().size(); i++)
	{
		const sponza::Instance& instance = scene_->getAllInstances()[i];
		const MeshGL& mesh = meshes_[instance.getMeshId()];

		// Define the model matrix
		glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());

		glm::mat4 depthMVP = lightVP * model_xform; // note that direction will have to do all 6 for point light so loop
		glUniformMatrix4fv(glGetUniformLocation(shadow_prog, "depthMVP"), 1, GL_FALSE, glm::value_ptr(depthMVP));

		// Draw
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
	}

	// cleanup
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//	glDisable(GL_POLYGON_OFFSET_FILL);
	int width = viewport_size[2];
	int height = viewport_size[3];
	glViewport(0, 0, width, height);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // added to fix shadowmap size
	glCullFace(GL_BACK);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

#ifdef TDK_NVTX
	nvtxRangePop(); // Shadow Map
#endif // TDK_NVTX
}

glm::mat4 MyView::CalculateLightVP(glm::vec3 lightPosition, glm::vec3 light_direction, float orthoSize, float near, float far)
{
	glm::mat4 depthProjectionMatrix = glm::ortho<float>(-orthoSize, orthoSize, -orthoSize, orthoSize, near, far);
	glm::mat4 depthViewMatrix = glm::lookAt(lightPosition, lightPosition + light_direction,  glm::vec3(0,1,0)/*(const glm::vec3 &)scene_->getUpDirection()*/);// lightPosition + light_direction?
	return depthProjectionMatrix * depthViewMatrix;// *glm::mat4(1);
}


void MyView::PostProcessingPhase()
{
	


	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postprocessing_fbo);

	// |-------------------------------------------|
	// | 2. CLEAR COLOUR ATTACHMENTS TO BACKGROUND |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	//glClearColor(0.f, 0.f, 0.25f, 0.f); // Clear colour buffer to Tyrone Blue
	//glClear(GL_COLOR_BUFFER_BIT); // Only clear the COLOR_BUFFER_BIT

	// |-----------------------|
	// | 4. DISABLE DEPTH TEST |
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);

	// |------------------|
	// | 5. DISABLE BLEND |
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);

	{// Gamma correction
		#ifdef  TDK_NVTX
			nvtxRangePushA("Gamma Correction");
		#endif //  TDK_NVTX

		glUseProgram(pp_gamma_prog);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_RECTANGLE, lbuffer_colour_tex_);
		glUniform1i(glGetUniformLocation(pp_gamma_prog, "sampler_world_colour"), 5);

		// | DRAW FULLSCREEN QUAD (for ambient and direcitonal) |
		glBindVertexArray(light_quad_mesh_.vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		#ifdef TDK_NVTX
			nvtxRangePop(); // Gamma Correction
		#endif // TDK_NVTX
	}
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	{//
		#ifdef  TDK_NVTX
				nvtxRangePushA("Anti-Aliasing");
		#endif //  TDK_NVTX

		glUseProgram(post_processing_prog);

		glActiveTexture(GL_TEXTURE5); // 0 or 1??
		glBindTexture(GL_TEXTURE_RECTANGLE, lbuffer_colour_tex_);
		glUniform1i(glGetUniformLocation(post_processing_prog, "sampler_world_colour"), 5);

		glUniform1i(glGetUniformLocation(post_processing_prog, "showEdgeDetection"), (int)showCannyEdgeDetection);

		// |----------------------------------------------------|
		// | DRAW FULLSCREEN QUAD (for ambient and direcitonal) |
		glBindVertexArray(light_quad_mesh_.vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		#ifdef TDK_NVTX
				nvtxRangePop(); // Anti-Aliasing
		#endif // TDK_NVTX
	}
}

void MyView::PhaseOne()
{
	// |--------------|				
	// | DRAW GBUFFER |
	// |--------------|

	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];
	int width = viewport_size[2];
	int height = viewport_size[3];

	// Define the projection matrix
	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()),
		aspect_ratio,
		scene_->getCamera().getNearPlaneDistance(),
		scene_->getCamera().getFarPlaneDistance());

	// Define the view matrix
	const glm::vec3 camera_position = (const glm::vec3 &)scene_->getCamera().getPosition();
	const glm::vec3 camera_direction = (const glm::vec3 &)scene_->getCamera().getDirection();
	glm::mat4 view_xform = glm::lookAt(camera_position,
		camera_position + camera_direction,
		(const glm::vec3 &)scene_->getUpDirection());

	// Define the projectionView matrix
	glm::mat4 projectionView_xform = projection_xform * view_xform;


	// |--------------------------|
	// | 1. BIND GBUFFER          |
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer_fbo_);


	// 
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//

	// |--------------------------------------|
	// | 2. CLEAR DEPTH ATTACHMENT TO 1.0 and |
	// |    STENCIL TO 128 (0x80)             |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	glClearDepth(1.0); // does this do anything by itself?
	glClear(GL_DEPTH_BUFFER_BIT);
	
	glStencilMask(0xFF);
	glClearStencil(0x80);
	glClear(GL_STENCIL_BUFFER_BIT); // should be 128

	// |--------------------------------------|
	// | 3. STENCIL ALWAYS WRITE ZERO (0x00)  |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	// |--------------------------------------|
	// | 4. DEPTH TO TEST LESS THAN AND WRITE |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE); // ~~~~~~~~~~~~~~~~~~~~Is this enabling write?
	glEnable(GL_DEPTH_TEST);


	// |----------------------------|
	// | 5. DISABLE BLEND           |
	glDisable(GL_BLEND);


	// |----------------------------|
	// | 6. BIND 0 TO TEXTURE UNITS |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	////

	glUseProgram(shader_prog);



	




	// |--------------------------|
	// | 7. DRAW SPONZA & FRIENDS |

	PerFrameUniforms per_frame_uniforms;
	per_frame_uniforms.projection_view_xform = projectionView_xform;

	glBindBuffer(GL_UNIFORM_BUFFER, ubo_per_frame);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_frame_uniforms), &per_frame_uniforms);

	#ifdef  TDK_NVTX
		nvtxRangePushA("Generate Sponza Textures");
	#endif //  TDK_NVTX
	glBindVertexArray(vao_sponza);
	for (int i = 0; i < scene_->getAllInstances().size() - 3; i++)
	{
		const sponza::Instance& instance = scene_->getAllInstances()[i];
		const MeshGL& mesh = meshes_[instance.getMeshId()];

		// Define the model matrix
		glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());


		// TODO MATERIAL DATA!!
		glUniform3fv(glGetUniformLocation(shader_prog, "ambient_colour"), 1, glm::value_ptr((const glm::vec3 &)scene_->getAllMaterials()[instance.getMaterialId() - 200].getDiffuseColour()));

		glUniform3fv(glGetUniformLocation(shader_prog, "specular_colour"), 1, glm::value_ptr((const glm::vec3 &)scene_->getAllMaterials()[instance.getMaterialId() - 200].getSpecularColour() * scene_->getAllMaterials()[instance.getMaterialId() - 200].getShininess()));


		// Fill and send the uniform values	
		PerModelUniforms per_model_uniforms;
		per_model_uniforms.model_xform = model_xform;

		glBindBuffer(GL_UNIFORM_BUFFER, ubo_per_model);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_model_uniforms), &per_model_uniforms);

		// Draw
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
	}
	#ifdef TDK_NVTX
		nvtxRangePop(); // Generate Sponza Textures
	#endif // TDK_NVTX

	#ifdef  TDK_NVTX
		nvtxRangePushA("Generate Friends Textures");
	#endif //  TDK_NVTX
	glBindVertexArray(vao_friends);
	for (int i = scene_->getAllInstances().size() - 3; i < scene_->getAllInstances().size(); i++)
	{
		const sponza::Instance& instance = scene_->getAllInstances()[i];
		const MeshGL& mesh = meshes_[instance.getMeshId()];

		// Define the model matrix
		glm::mat4 model_xform = glm::mat4((const glm::mat4x3 &)instance.getTransformationMatrix());

		// MATERIAL DATA!!
		glUniform3fv(glGetUniformLocation(shader_prog, "ambient_colour"), 1, glm::value_ptr((const glm::vec3 &)scene_->getAllMaterials()[instance.getMaterialId() - 200].getDiffuseColour()));
		glUniform3fv(glGetUniformLocation(shader_prog, "specular_colour"), 1, glm::value_ptr((const glm::vec3 &)scene_->getAllMaterials()[instance.getMaterialId() - 200].getSpecularColour() * scene_->getAllMaterials()[instance.getMaterialId() - 200].getShininess()));
		// Pass shininess (float)
		glUniform1f(glGetUniformLocation(shader_prog, "shininess"), scene_->getAllMaterials()[instance.getMaterialId() - 200].getShininess());

		// Fill and send the uniform values	
		PerModelUniforms per_model_uniforms;
		per_model_uniforms.model_xform = model_xform;

		glBindBuffer(GL_UNIFORM_BUFFER, ubo_per_model);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(per_model_uniforms), &per_model_uniforms);

		// Draw
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (const void*)(mesh.first_element_index * sizeof(GLuint)), mesh.first_vertex_index);
	}
	#ifdef TDK_NVTX
		nvtxRangePop(); // Generate Friends Textures
	#endif // TDK_NVTX
}

void MyView::PhaseTwo()
{
	// |--------------------------|
	// | 1. BIND LBUFFER          |
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer_fbo_);

	// |-------------------------------------------|
	// | 2. CLEAR COLOUR ATTACHMENTS TO BACKGROUND |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	//glClearColor(0.f, 0.f, 0.25f, 0.f); // Clear colour buffer to Tyrone Blue
	#ifdef  TDK_NVTX
		nvtxRangePushA("Skybox");
	#endif //  TDK_NVTX

	glUseProgram(skybox_prog);

	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];
	int width = viewport_size[2];
	int height = viewport_size[3];

	// Define the projection matrix
	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()),
		aspect_ratio,
		scene_->getCamera().getNearPlaneDistance(),
		5000.0f);

	// Define the view matrix
	const glm::vec3 camera_position = (const glm::vec3 &)scene_->getCamera().getPosition();
	const glm::vec3 camera_direction = (const glm::vec3 &)scene_->getCamera().getDirection();
	glm::mat4 view_xform = glm::lookAt(camera_position,
		camera_position + camera_direction,
		(const glm::vec3 &)scene_->getUpDirection());
	view_xform[3][0] = 0;
	view_xform[3][1] = 0;
	view_xform[3][2] = 0;

	// send projection matrix
	glUniformMatrix4fv(glGetUniformLocation(skybox_prog, "projection_xform"), 1, GL_FALSE, glm::value_ptr(projection_xform));
	// send view matrix
	glUniformMatrix4fv(glGetUniformLocation(skybox_prog, "view_xform"), 1, GL_FALSE, glm::value_ptr(view_xform));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex);
	glUniform1i(glGetUniformLocation(skybox_prog, "sampler_skybox"), 0);

	// Define the projectionView matrix
	glm::mat4 projectionView_xform = projection_xform * view_xform;


	glBindVertexArray(skybox_mesh.vao);


	glDisable(GL_STENCIL_TEST);
//	glEnable(GL_DEPTH_TEST);
//	glDisable(GL_DEPTH_TEST);
	//glDepthMask(GL_TRUE);
	
	//glDepthMask(GL_FALSE);
	glClear(GL_COLOR_BUFFER_BIT); 	// Only clear the COLOR_BUFFER_BIT

	glDepthFunc(GL_ALWAYS);
	glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.element_count);
	//glDepthMask(GL_TRUE);

	glBindVertexArray(kNullId);
	glUseProgram(kNullId);

	#ifdef TDK_NVTX
		nvtxRangePop(); // Skybox
	#endif // TDK_NVTX
	//




	// |-------------------------------------|
	// | 3. STENCIL EQUAL TO 0 and NOT WRITE |
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~CHECK~
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 0, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// |-----------------------|
	// | 4. DISABLE DEPTH TEST |
	glDisable(GL_DEPTH_TEST);
	//glDepthMask(GL_FALSE);//??

	// |------------------|
	// | 5. DISABLE BLEND |
	glDisable(GL_BLEND);

	#ifdef  TDK_NVTX
		nvtxRangePushA("Directional Lights");
	#endif //  TDK_NVTX

	// 1st draw replace, ambient + directional light
	DrawDirectionalLights();

	#ifdef TDK_NVTX
		nvtxRangePop(); // Directional Lights
	#endif // TDK_NVTX

	// PHASE 2+ - additive drawing for the other lights
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);


	#ifdef  TDK_NVTX
		nvtxRangePushA("Point Lights");
	#endif //  TDK_NVTX

	//glDepthMask(GL_FALSE);
	DrawPointLights();
	//glDepthMask(GL_TRUE);

	#ifdef TDK_NVTX
		nvtxRangePop(); // Point Lights
	#endif // TDK_NVTX
	
	#ifdef  TDK_NVTX
			nvtxRangePushA("Spot Lights");
	#endif //  TDK_NVTX

	// TODO Undo
	DrawSpotLights();

	#ifdef TDK_NVTX
		nvtxRangePop(); // Spot Lights
	#endif // TDK_NVTX

	glDisable(GL_BLEND);
}



void MyView::DrawDirectionalLights()
{
	// |-------------------|
	// | UPDATE SHADOW MAP |
	const glm::vec3 lightPos = glm::vec3(70, 30, 0);//(const glm::vec3 &)scene_->getCamera().getPosition();//glm::vec3(10, 15, -10); //(const glm::vec3 &)scene_->getCamera().getPosition();//glm::vec3(10, 15, -10);
	const glm::vec3 lighDir = (const glm::vec3 &)scene_->getAllDirectionalLights()[0].getDirection();// (const glm::vec3 &)scene_->getCamera().getDirection();//
	const float orthoSize = 35.0f; // ALSO should I let light take control of near and far?
	UpdateShadowMap(lightPos, lighDir, orthoSize, 1.0f, 1000.0f); // note that these are test values
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer_fbo_);
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);


	// |--------------|
	// | BIND PROGRAM |
	glUseProgram(directional_light_prog);


	// |----------------------------------------|
	// | BIND GBUFFER TEXTURES TO TEXTURE UNITS |
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~How do I know if 1 or 2 as these stored in Gbuffer? Maybe make colour attachment 3??
	glActiveTexture(GL_TEXTURE0); // 0 or 1??
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(directional_light_prog, "sampler_world_position"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(directional_light_prog, "sampler_world_normal"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_ambient_tex_);
	glUniform1i(glGetUniformLocation(directional_light_prog, "sampler_world_ambient"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_specular_tex_);
	glUniform1i(glGetUniformLocation(directional_light_prog, "sampler_world_specular"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadow_depth_tex_);
	glUniform1i(glGetUniformLocation(directional_light_prog, "sampler_shadow"), 4);

	// |----------------------------|
	// | SEND LIGHT DATA TO SHADERS |
	// note there is a second directional light
	const glm::vec3 intensity = (const glm::vec3 &)scene_->getAllDirectionalLights()[0].getIntensity();
	glUniform3fv(glGetUniformLocation(directional_light_prog, "light_intensity"), 1, glm::value_ptr(intensity));

	const glm::vec3 direction = (const glm::vec3 &)scene_->getAllDirectionalLights()[0].getDirection();
	glUniform3fv(glGetUniformLocation(directional_light_prog, "light_direction"), 1, glm::value_ptr(direction));

	glm::mat4 light_projection_view_xform = CalculateLightVP(lightPos, lighDir, orthoSize, 1.0f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(directional_light_prog, "light_projection_view_xform"), 1, GL_FALSE, glm::value_ptr(light_projection_view_xform));

	// |----------------------------------------------------|
	// | DRAW FULLSCREEN QUAD (for ambient and direcitonal) |
	glBindVertexArray(light_quad_mesh_.vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void MyView::DrawSpotLights()
{
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];
	int width = viewport_size[2];
	int height = viewport_size[3];


	// Define the projection matrix
	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()),
		aspect_ratio,
		scene_->getCamera().getNearPlaneDistance(),
		scene_->getCamera().getFarPlaneDistance());

	// Define the view matrix
	const glm::vec3 camera_position = (const glm::vec3 &)scene_->getCamera().getPosition();
	const glm::vec3 camera_direction = (const glm::vec3 &)scene_->getCamera().getDirection();
	glm::mat4 view_xform = glm::lookAt(camera_position,
		camera_position + camera_direction,
		(const glm::vec3 &)scene_->getUpDirection());
	glm::mat4 camera_projection_view_xform = projection_xform * view_xform;

	// Use cone light shader program
	glUseProgram(spot_light_prog);

	// |----------------------------------------|
	// | BIND GBUFFER TEXTURES TO TEXTURE UNITS |
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(spot_light_prog, "sampler_world_position"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(spot_light_prog, "sampler_world_normal"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_ambient_tex_);
	glUniform1i(glGetUniformLocation(spot_light_prog, "sampler_world_ambient"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_specular_tex_);
	glUniform1i(glGetUniformLocation(spot_light_prog, "sampler_world_specular"), 3);


	
	glBindVertexArray(light_cone_mesh.vao);

	for (unsigned int i =0; i < scene_->getAllSpotLights().size(); i++)
	{
		const sponza::SpotLight& spot_light = scene_->getAllSpotLights()[i];
	
		glEnable(GL_DEPTH_TEST);
		glCullFace(GL_BACK);
		DrawSpotLight(spot_light, 1.0F, camera_projection_view_xform);

		
		// TAKE OUT, FOR TESTING
		//break;
	}
}

void MyView::DrawPointLights()
{
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];
	int width = viewport_size[2];
	int height = viewport_size[3];


	// Define the projection matrix
	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()),
		aspect_ratio,
		scene_->getCamera().getNearPlaneDistance(),
		scene_->getCamera().getFarPlaneDistance());

	// Define the view matrix
	const glm::vec3 camera_position = (const glm::vec3 &)scene_->getCamera().getPosition();
	const glm::vec3 camera_direction = (const glm::vec3 &)scene_->getCamera().getDirection();
	glm::mat4 view_xform = glm::lookAt(camera_position,
		camera_position + camera_direction,
		(const glm::vec3 &)scene_->getUpDirection());
	glm::mat4 camera_projection_view_xform = projection_xform * view_xform;


	// Use point light shader program
	glUseProgram(point_light_prog);

	// |----------------------------------------|
	// | BIND GBUFFER TEXTURES TO TEXTURE UNITS |
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(point_light_prog, "sampler_world_position"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(point_light_prog, "sampler_world_normal"), 1);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_ambient_tex_);
	glUniform1i(glGetUniformLocation(point_light_prog, "sampler_world_ambient"), 3);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_specular_tex_);
	glUniform1i(glGetUniformLocation(point_light_prog, "sampler_world_specular"), 5);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadow_depth_tex_);
	glUniform1i(glGetUniformLocation(point_light_prog, "sampler_shadow"), 4);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, shadowmap_cube_tex);
	glUniform1i(glGetUniformLocation(point_light_prog, "sampler_shadow_cube"),2);


	glBindVertexArray(light_sphere_mesh_.vao);
	for (unsigned int i = 0; i < scene_->getAllPointLights().size(); i++)
	{
		const sponza::PointLight& point_light = scene_->getAllPointLights()[i];

	
		DrawPointLight(point_light, glm::vec3(1, 0, 0), 1.0F, camera_projection_view_xform);
		////DrawPointLight(point_light, glm::vec3(-1, 0, 0), point_light.getRange() * 2.0f, camera_projection_view_xform);
		//DrawPointLight(point_light, glm::vec3(0, 1, 0), point_light.getRange() * 2.0f, camera_projection_view_xform);
		//DrawPointLight(point_light, glm::vec3(0, -1, 0), point_light.getRange() * 2.0f, camera_projection_view_xform);
		//DrawPointLight(point_light, glm::vec3(0, 0, 1), point_light.getRange() * 2.0f, camera_projection_view_xform);
		//DrawPointLight(point_light, glm::vec3(0, 0, -1), point_light.getRange() * 2.0f, camera_projection_view_xform);

		//break;
	}
}

void MyView::DrawSpotLight(const sponza::SpotLight& spot_light, const float orthoSize, const glm::mat4& camera_projection_view_xform)
{
	// Update Shadow Map
	const glm::vec3 lightPos = (const glm::vec3 &)spot_light.getPosition();
	const glm::vec3 lighDir = (const glm::vec3 &)spot_light.getDirection();
	//UpdateShadowMap(lightPos, lighDir, orthoSize, 1.0f, 1000.0f); // TODO MAKE ANOTHER UNCTION THAT DOESNT USE ORTHOSIZE, SHOULD BE PERSPECTIVE
	UpdateShadowMapPerspective(lightPos, lighDir, spot_light.getConeAngleDegrees(), 1.0f, 1000.0f);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadow_depth_tex_);
	glUniform1i(glGetUniformLocation(spot_light_prog, "sampler_shadow"), 4);
	//

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer_fbo_);
	//glEnable(GL_STENCIL_TEST);
	//glEnable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	//glCullFace(GL_BACK);
	glCullFace(GL_FRONT);
	glEnable(GL_STENCIL_TEST);

	//glDisable(GL_DEPTH_TEST);


	// |--------------|
	// | BIND PROGRAM |
	glUseProgram(spot_light_prog);


	// send projectionView matrix
	glUniformMatrix4fv(glGetUniformLocation(spot_light_prog, "camera_projection_view_xform"), 1, GL_FALSE, glm::value_ptr(camera_projection_view_xform));


	// send position
	glUniform3fv(glGetUniformLocation(spot_light_prog, "light_position"), 1, glm::value_ptr((const glm::vec3 &)spot_light.getPosition()));
	// send range
	glUniform1f(glGetUniformLocation(spot_light_prog, "light_range"), spot_light.getRange());
	//send INTENSITY
	glUniform3fv(glGetUniformLocation(spot_light_prog, "light_colour_intensity"), 1, glm::value_ptr((const glm::vec3 &)spot_light.getIntensity()));
	//send direction
	glUniform3fv(glGetUniformLocation(spot_light_prog, "light_direction"), 1, glm::value_ptr((const glm::vec3 &)spot_light.getDirection()));
	// send angle
	glUniform1f(glGetUniformLocation(spot_light_prog, "cone_angle_degrees"), spot_light.getConeAngleDegrees());
	// send cast_shadow
	glUniform1f(glGetUniformLocation(spot_light_prog, "cast_shadow"), spot_light.getCastShadow());

	//
	glm::mat4 depthProjectionMatrix = glm::perspective<float>(glm::radians(spot_light.getConeAngleDegrees()), 1.0f, 1.0f, 50.0f);
	//glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos - -(lighDir), glm::vec3(0, 1, 0));
	glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos + lighDir, glm::vec3(0, 1, 0));
	glm::mat4 light_projection_view_xform = depthProjectionMatrix * depthViewMatrix ;
	glUniformMatrix4fv(glGetUniformLocation(spot_light_prog, "light_projection_view_xform"), 1, GL_FALSE, glm::value_ptr(light_projection_view_xform));
	//

	//https://stackoverflow.com/questions/41104820/how-to-rotate-an-object-using-glmlookat
	glm::mat4 rotate = glm::inverse(
		glm::lookAt(
			(const glm::vec3 &)spot_light.getPosition(), 
			(const glm::vec3 &)spot_light.getPosition() + (const glm::vec3 &)spot_light.getDirection(), 
			(const glm::vec3 &)scene_->getUpDirection()
					)
										);

	glm::mat4 translate = glm::translate(glm::mat4(), glm::vec3(0, 0, -1)); //glm::mat4()

	float radius = spot_light.getRange() * glm::tan(glm::radians(spot_light.getConeAngleDegrees()) / 2.0f);
	glm::mat4 scaleMat = glm::scale(glm::mat4(), glm::vec3(radius, radius, spot_light.getRange()));

	glm::mat4 light_model_xform = rotate * scaleMat * translate;

	glUniformMatrix4fv(glGetUniformLocation(spot_light_prog, "light_model_xform"), 1, GL_FALSE, glm::value_ptr(light_model_xform));

	// DrawElements
	glDrawElements(GL_TRIANGLES, light_cone_mesh.element_count, GL_UNSIGNED_INT, (void*)0);
}

void MyView::DrawPointLight(const sponza::PointLight& point_light, const glm::vec3 direction, const float orthoSize, const glm::mat4& camera_projection_view_xform)
{
	// |-------------------|
	// | UPDATE SHADOW MAP |
	//UpdateShadowCubeMap((const glm::vec3 &)point_light.getPosition(), direction);
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lbuffer_fbo_);
	glCullFace(GL_FRONT);
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);

	

	// |--------------|
	// | BIND PROGRAM |
	glUseProgram(point_light_prog);


	// send position
	glUniform3fv(glGetUniformLocation(point_light_prog, "light_position"), 1, glm::value_ptr((const glm::vec3 &)point_light.getPosition()));
	// send range
	glUniform1f(glGetUniformLocation(point_light_prog, "light_range"), point_light.getRange());
	//send INTENSITY
	glUniform3fv(glGetUniformLocation(point_light_prog, "light_intensity"), 1, glm::value_ptr((const glm::vec3 &)point_light.getIntensity()));

	// send projectionView matrix
	glUniformMatrix4fv(glGetUniformLocation(point_light_prog, "camera_projection_view_xform"), 1, GL_FALSE, glm::value_ptr(camera_projection_view_xform));

	// send model matrix
	glm::mat4 light_model_xform = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
											0.0f, 1.0f, 0.0f, 0.0f,
											0.0f, 0.0f, 1.0f, 0.0f,
											point_light.getPosition().x, point_light.getPosition().y, point_light.getPosition().z, 1.0f);
	

	float range = point_light.getRange() * 2.0f;
	glm::mat4 scale = glm::mat4(range, 0.0f, 0.0f, 0.0f,
								0.0f, range, 0.0f, 0.0f,
								0.0f, 0.0f, range, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f);

	light_model_xform *= scale;


	glUniformMatrix4fv(glGetUniformLocation(point_light_prog, "light_model_xform"), 1, GL_FALSE, glm::value_ptr(light_model_xform));

	

	glm::mat4 light_projection_view_xform = CalculateLightVP((const glm::vec3 &)point_light.getPosition(), direction, orthoSize, 1.0f, point_light.getRange()); // 1.0 should be range?

	//temp
	/*GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];
	glm::mat4 projection_xform = glm::perspective(glm::radians(90.0f), aspect_ratio, 
		scene_->getCamera().getNearPlaneDistance(), scene_->getCamera().getFarPlaneDistance());
	glm::mat4 view_xform = glm::lookAt((const glm::vec3 &)point_light.getPosition(),  direction, (const glm::vec3 &)scene_->getUpDirection());
	light_projection_view_xform = projection_xform * view_xform;
	*///

	glUniformMatrix4fv(glGetUniformLocation(point_light_prog, "light_projection_view_xform"), 1, GL_FALSE, glm::value_ptr(light_projection_view_xform));

	// DrawElements
	glDrawElements(GL_TRIANGLES, light_sphere_mesh_.element_count, GL_UNSIGNED_INT, (void*)0);
}

void MyView::PhaseThree()
{
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];
	int width = viewport_size[2];
	int height = viewport_size[3];

	// Blit Lbuffer to the screen
	glBindFramebuffer(GL_READ_FRAMEBUFFER, postprocessing_fbo);//lbuffer_fbo_
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(0, 0, width, height,
		0, 0, width, height,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
}

void MyView::CreateShaderProgram(GLuint& shaderProgram, const std::string vsName, const std::string fsName, std::vector<std::string> vertexAtrribs)
{
	GLint compile_status = 0;

	GLuint vertex_shader = CreateShader(GL_VERTEX_SHADER, ("resource:///" + vsName + ".glsl").c_str());
	GLuint fragment_shader = CreateShader(GL_FRAGMENT_SHADER, ("resource:///" + fsName + ".glsl").c_str());

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertex_shader);


	for (int i = 0; i < vertexAtrribs.size(); i++)
	{
		glBindAttribLocation(shaderProgram, i, vertexAtrribs[i].c_str());
	}
	//glBindAttribLocation(shaderProgram, 0, "vertex_position");
	//glBindAttribLocation(shaderProgram, 1, "vertex_normal");//added
	//glBindAttribLocation(shaderProgram, 2, "vertex_texcoord");//added
	glDeleteShader(vertex_shader);

	glAttachShader(shaderProgram, fragment_shader);
	//glBindFragDataLocation(shaderProgram, 0, "reflected_light");
	//glBindFragDataLocation(shaderProgram, 1, "position_out"); // where are these being used?
	//glBindFragDataLocation(shaderProgram, 2, "normal_out");//where are these being used?
	glDeleteShader(fragment_shader);
	glLinkProgram(shaderProgram);

	GLint link_status = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE)
	{
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetProgramInfoLog(shaderProgram, string_length, NULL, log);
		std::cerr << log << std::endl;
	}
}

void MyView::CreateGbufferObjects()
{
	// Gen FBO
	glGenFramebuffers(1, &gbuffer_fbo_);

	// Gen textures
	glGenTextures(1, &gbuffer_position_tex_);
	glGenTextures(1, &gbuffer_normal_tex_);
	glGenTextures(1, &gbuffer_ambient_tex_);
	glGenTextures(1, &gbuffer_specular_tex_);

	// Gen RBOs
	glGenRenderbuffers(1, &depth_stencil_rbo_);
}

void MyView::CreateLbufferObjects()
{
	// Gen FBO
	glGenFramebuffers(1, &lbuffer_fbo_);

	// Gen textures
	glGenTextures(1, &lbuffer_colour_tex_);
}

void MyView::CreateVAOs()
{
	//Sponza VAO
	{
		// Bind a new VAO and the Element VBO
		glGenVertexArrays(1, &vao_sponza);
		glBindVertexArray(vao_sponza);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_vbo);
		glEnableVertexAttribArray(kVertexPosition);
		glVertexAttribPointer(kVertexPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, position));

		glEnableVertexAttribArray(kVertexNormal);
		glVertexAttribPointer(kVertexNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, normal));

		glEnableVertexAttribArray(kVertexTexCoord);
		glVertexAttribPointer(kVertexTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, texcoord));


		glBindVertexArray(kNullId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, kNullId);
		glBindBuffer(GL_ARRAY_BUFFER, kNullId);
	}
	// Friends VAO
	{
		glGenVertexArrays(1, &vao_friends);
		glBindVertexArray(vao_friends);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_vbo);

		glEnableVertexAttribArray(kVertexPosition);
		glVertexAttribPointer(kVertexPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, position));

		glEnableVertexAttribArray(kVertexNormal);
		glVertexAttribPointer(kVertexNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, normal));


		glBindVertexArray(kNullId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, kNullId);
		glBindBuffer(GL_ARRAY_BUFFER, kNullId);
	}

}

void MyView::CreateVBOs()
{
	sponza::GeometryBuilder builder;
	const auto& scene_meshes = builder.getAllMeshes();

	GLsizeiptr verticesBufferSize = 0;
	GLsizeiptr elementsBufferSize = 0;
	for (const auto& scene_mesh : scene_meshes)
	{
		verticesBufferSize += scene_mesh.getPositionArray().size() * sizeof(Vertex);
		elementsBufferSize += scene_mesh.getElementArray().size() * sizeof(sponza::InstanceId);
	}

	// Create Vertex VBO
	CreateBuffer(&vertex_vbo, GL_ARRAY_BUFFER, verticesBufferSize, nullptr);
	// Create Element VBO
	CreateBuffer(&element_vbo, GL_ELEMENT_ARRAY_BUFFER, elementsBufferSize, nullptr);
}

void MyView::FillMeshData()
{
	sponza::GeometryBuilder builder;
	const auto& scene_meshes = builder.getAllMeshes();

	GLuint firstVertexIndex = 0;
	GLuint firstElementIndex = 0;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	// Loop through each mesh in the scene and assign the data to the map
	for (int i = 0; i < scene_meshes.size(); i++)
	{
		const auto& scene_mesh = scene_meshes[i];
		MeshGL& newMesh = meshes_[scene_mesh.getId()];

		// Get the data
		const auto& positions = scene_mesh.getPositionArray();
		const auto& elements = scene_mesh.getElementArray();
		const auto& normals = scene_mesh.getNormalArray();
		const auto& texcoords = scene_mesh.getTextureCoordinateArray();

		// Initialise the vertices vector with the data
		std::vector<Vertex> _vertices;
		_vertices.resize(scene_mesh.getPositionArray().size());

		for (int i = 0; i < _vertices.size(); i++)
		{
			_vertices[i].position = (const glm::vec3 &)positions[i];
			_vertices[i].normal = (const glm::vec3 &)normals[i];

			if (i < texcoords.size())
				_vertices[i].texcoord = (const glm::vec2 &)texcoords[i];
			else
				_vertices[i].texcoord = glm::vec2(0, 0);
		}

		// -----------------------------------------------
		newMesh.first_vertex_index = firstVertexIndex;
		newMesh.first_element_index = firstElementIndex;
		newMesh.element_count = elements.size();


		//--------------------------------------------------------------------------------------------	
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, newMesh.first_element_index * sizeof(sponza::InstanceId), elements.size() * sizeof(sponza::InstanceId), elements.data());
		glBufferSubData(GL_ARRAY_BUFFER, newMesh.first_vertex_index * sizeof(Vertex), _vertices.size() * sizeof(Vertex), _vertices.data());

		firstVertexIndex += _vertices.size();
		firstElementIndex += elements.size();
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, kNullId);
	glBindBuffer(GL_ARRAY_BUFFER, kNullId);

}

void MyView::CreateSkyboxMesh()
{
	std::vector<glm::vec3> vertices(36);
	
	const  float SIZE = 1000.0f;// 1024.0f;
	vertices[0] = glm::vec3(-SIZE, SIZE, -SIZE );
	 vertices[1] = glm::vec3(-SIZE, -SIZE, -SIZE );
	 vertices[2] = glm::vec3(SIZE, -SIZE, -SIZE );
	 vertices[3] = glm::vec3(SIZE, -SIZE, -SIZE );
	 vertices[4] = glm::vec3(SIZE, SIZE, -SIZE );
	 vertices[5] = glm::vec3(-SIZE, SIZE, -SIZE );

	 vertices[6] = glm::vec3(-SIZE, -SIZE, SIZE );
	 vertices[7] = glm::vec3(-SIZE, -SIZE, -SIZE );
	 vertices[8] = glm::vec3(-SIZE, SIZE, -SIZE );
	 vertices[9] = glm::vec3(-SIZE, SIZE, -SIZE );
	 vertices[10] = glm::vec3(-SIZE, SIZE, SIZE );
	 vertices[11] = glm::vec3(-SIZE, -SIZE, SIZE );

	 vertices[12] = glm::vec3(SIZE, -SIZE, -SIZE );
	 vertices[13] = glm::vec3(SIZE, -SIZE, SIZE );
	 vertices[14] = glm::vec3(SIZE, SIZE, SIZE );
	 vertices[15] = glm::vec3(SIZE, SIZE, SIZE );
	 vertices[16] = glm::vec3(SIZE, SIZE, -SIZE );
	 vertices[17] = glm::vec3(SIZE, -SIZE, -SIZE );

	 vertices[18] = glm::vec3(-SIZE, -SIZE, SIZE );
	 vertices[19] = glm::vec3(-SIZE, SIZE, SIZE );
	 vertices[20] = glm::vec3(SIZE, SIZE, SIZE );
	 vertices[21] = glm::vec3(SIZE, SIZE, SIZE );
	 vertices[22] = glm::vec3(SIZE, -SIZE, SIZE );
	 vertices[23] = glm::vec3(-SIZE, -SIZE, SIZE );

	 vertices[24] = glm::vec3(-SIZE, SIZE, -SIZE);
	 vertices[25] = glm::vec3(SIZE, SIZE, -SIZE );
	 vertices[26] = glm::vec3(SIZE, SIZE, SIZE );
	 vertices[27] = glm::vec3(SIZE, SIZE, SIZE);
	 vertices[28] = glm::vec3(-SIZE, SIZE, SIZE );
	 vertices[29] = glm::vec3(-SIZE, SIZE, -SIZE );

	 vertices[30] = glm::vec3(-SIZE, -SIZE, -SIZE );
	 vertices[31] = glm::vec3(-SIZE, -SIZE, SIZE );
	 vertices[32] = glm::vec3(SIZE, -SIZE, -SIZE );
	 vertices[33] = glm::vec3(SIZE, -SIZE, -SIZE );
	 vertices[34] = glm::vec3(-SIZE, -SIZE, SIZE );
	 vertices[35] = glm::vec3(SIZE, -SIZE, SIZE);

	skybox_mesh.element_count = vertices.size();

	glGenBuffers(1, &skybox_mesh.vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, skybox_mesh.vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &skybox_mesh.vao);
	glBindVertexArray(skybox_mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, skybox_mesh.vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void MyView::CreateQuadMesh()
{
	/*
	* Tutorial: this section of code creates a fullscreen quad to be used
	*           when computing global illumination effects (e.g. ambient)
	*/

	std::vector<glm::vec2> vertices(4);
	vertices[0] = glm::vec2(-1, -1);
	vertices[1] = glm::vec2(1, -1);
	vertices[2] = glm::vec2(1, 1);
	vertices[3] = glm::vec2(-1, 1);

	glGenBuffers(1, &light_quad_mesh_.vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_quad_mesh_.vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(glm::vec2),
		vertices.data(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &light_quad_mesh_.vao);
	glBindVertexArray(light_quad_mesh_.vao);
	glBindBuffer(GL_ARRAY_BUFFER, light_quad_mesh_.vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
		sizeof(glm::vec2), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}

void MyView::CreateConeMesh()
{
	tsl::IndexedMeshPtr mesh = tsl::createConePtr(1.0f, 1.0f, 12);
	mesh = tsl::cloneIndexedMeshAsTriangleListPtr(mesh.get());
	light_cone_mesh.element_count = mesh->indexCount();

	glGenBuffers(1, &light_cone_mesh.vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_cone_mesh.vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		mesh->vertexCount() * sizeof(glm::vec3),
		mesh->positionArray(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &light_cone_mesh.element_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_cone_mesh.element_vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		mesh->indexCount() * sizeof(unsigned int),
		mesh->indexArray(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &light_cone_mesh.vao);
	glBindVertexArray(light_cone_mesh.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_cone_mesh.element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_cone_mesh.vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		sizeof(glm::vec3), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void MyView::CreateSphereMesh()
{
	/*
	* Tutorial: this code creates a sphere to use when deferred shading
	*           with a point light source.
	*/
	
	tsl::IndexedMeshPtr mesh = tsl::createSpherePtr(1.f, 12);
	mesh = tsl::cloneIndexedMeshAsTriangleListPtr(mesh.get());
	light_sphere_mesh_.element_count = mesh->indexCount();

	glGenBuffers(1, &light_sphere_mesh_.vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_sphere_mesh_.vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		mesh->vertexCount() * sizeof(glm::vec3),
		mesh->positionArray(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &light_sphere_mesh_.element_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_sphere_mesh_.element_vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		mesh->indexCount() * sizeof(unsigned int),
		mesh->indexArray(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &light_sphere_mesh_.vao);
	glBindVertexArray(light_sphere_mesh_.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_sphere_mesh_.element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_sphere_mesh_.vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		sizeof(glm::vec3), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
}


// Helper functions
GLuint MyView::CreateShader(GLenum type, const char* uri)
{
	GLint compile_status = GL_FALSE;

	GLuint shader = glCreateShader(type);
	std::string shader_string = tygra::createStringFromFile(uri);
	const char* vertex_shader_code = shader_string.c_str();

	glShaderSource(shader, 1, (const GLchar **)&vertex_shader_code, NULL);
	glCompileShader(shader);



	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetShaderInfoLog(shader, string_length, NULL, log);
		std::cerr << log << std::endl;
	}
	return shader;
}

void MyView::CreateBuffer(GLuint* buffers, GLenum target, GLsizeiptr size, const void* data)
{
	glGenBuffers(1, buffers);
	glBindBuffer(target, *buffers);

	glBufferData(target,
		size, // size of data in bytes
		data, // pointer to the data 
		GL_STATIC_DRAW);

	glBindBuffer(target, kNullId);
}

void MyView::CreateUniformBuffer(GLuint* buffer, GLsizeiptr size, const GLchar* name, GLuint& program)
{
	// Gen Uniform Buffer
	glGenBuffers(1, buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, *buffer);
	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STREAM_DRAW);


	// Bind Uniform Buffer
	glBindBufferBase(GL_UNIFORM_BUFFER, current_index, *buffer);
	glUniformBlockBinding(program, glGetUniformBlockIndex(program, name), current_index);

	// Update index ready for next creation
	current_index++;
}

bool MyView::CreateCubeMap(GLuint* texture, std::string name, GLenum type)
{
	tygra::Image texture_image = tygra::createImageFromPngFile("content:///" + name);
	if (texture_image.doesContainData())
	{
		//glGenTextures(1, texture);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, *texture);
		//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
		glTexImage2D(type,
			0,
			GL_RGBA,
			(GLsizei)texture_image.width(),
			(GLsizei)texture_image.height(),
			0,
			pixel_formats[texture_image.componentsPerPixel()],
			texture_image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
			texture_image.pixelData());

		//glBindTexture(GL_TEXTURE_CUBE_MAP, kNullId);

		return true;
	}

	return false;

}

void MyView::CreateTextureImage(GLuint* texture, std::string name)
{
	tygra::Image texture_image = tygra::createImageFromPngFile("content:///" + name);
	if (texture_image.doesContainData())
	{
		glGenTextures(1, texture);
		glBindTexture(GL_TEXTURE_2D, *texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };

		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			(GLsizei)texture_image.width(),
			(GLsizei)texture_image.height(),
			0,
			pixel_formats[texture_image.componentsPerPixel()],
			texture_image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
			texture_image.pixelData());

		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, kNullId);
	}

}