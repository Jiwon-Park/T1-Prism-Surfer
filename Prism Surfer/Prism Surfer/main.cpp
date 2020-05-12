#include "cgmath.h"		// slee's simple math library
#include "cgut.h"		// slee's OpenGL utility
#include "trackball.h"	// virtual trackball
#include "shaders.h"
//*************************************
// global constants
static const char* window_name = "2018312734 Jiwon Park A3";
const uint	NUM_DIVIDERS = 50;

// 0 = Sun, 8 = Uranus
const uint	NUM_RECT = 9;

//*************************************
// common structures
struct camera
{
	vec3	eye = vec3(0, 30, 300);
	vec3	at = vec3(0, 0, 0);
	vec3	up = vec3(0, 1, 0);
	mat4	view_matrix = mat4::look_at(eye, at, up);

	float	fovy = PI / 4.0f; // must be in radian
	float	aspect;
	float	dnear = 1.0f;
	float	dfar = 1000.0f;
	mat4	projection_matrix;
};

//*************************************
// window objects
GLFWwindow* window = nullptr;
ivec2		window_size = cg_default_window_size(); // initial window size

//*************************************
// OpenGL objects
GLuint	program = 0;	// ID holder for GPU program

//*************************************
// global variables
int		frame = 0;				// index of rendering frames
int		zoom_trigger = 0;
int		pan_trigger = 0;

//*************************************
// scene objects
mesh* pMesh = nullptr;
camera		cam;
trackball	tb;

//*************************************

mesh* create_rectangle_mesh(float width, float height)
{
	mesh* msh = new mesh();

	// TODO: Creating vertex buffer and index buffer of a rectangle

	return msh;
}



void update()
{
	// update projection matrix
	cam.aspect = window_size.x / float(window_size.y);
	cam.projection_matrix = mat4::perspective(cam.fovy, cam.aspect, cam.dnear, cam.dfar);

	// build the model matrix for oscillating scale
	float t = float(glfwGetTime());
	//float scale	= 1.0f+float(cos(t*1.5f))*0.05f;
	//mat4 model_matrix = mat4::scale( scale, scale, scale );

	// update uniform variables in vertex/fragment shaders
	GLint uloc;

	// TODO: updating view matrix per frame


	uloc = glGetUniformLocation(program, "view_matrix");			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.view_matrix);
	uloc = glGetUniformLocation(program, "projection_matrix");	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.projection_matrix);
}

void render()
{
	GLint uloc;
	mat4 model_matrix;
	// clear screen (with background color) and clear depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// notify GL that we use our own program
	glUseProgram(program);

	for (int i = 0; i < NUM_RECT; i++)
	{
		float t = float(glfwGetTime()) / 10.0f;
		/************
		model_matrix = mat4::rotate(vec3(0, 0, 1), t * av[i] * angular_velocity_multiplier)
			* mat4::translate(vec3(radii[i] * orbit_radius_multiplier, 0, 0))
			* mat4::rotate(vec3(0, 0, 1), t * self_av[i] * self_rotation_multiplier);
		************/
		// TODO: generate model matrix
		uloc = glGetUniformLocation(program, "model_matrix");
		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

		// bind vertex array object
		if (pMesh && pMesh->vertex_array) glBindVertexArray(pMesh->vertex_array);

		// render vertices: trigger shader programs to process vertex data
		glDrawElements(GL_TRIANGLES, pMesh->index_list.size(), GL_UNSIGNED_INT, nullptr);
	}
	// swap front and back buffers, and display to screen
	glfwSwapBuffers(window);
}

void reshape(GLFWwindow* window, int width, int height)
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width, height);
	glViewport(0, 0, width, height);
}

void print_help()
{
	printf("[help]\n");
	printf("- press ESC or 'q' to terminate the program\n");
	printf("- press F1 or 'h' to see help\n");
	printf("- press Home to reset camera\n");
	printf("\n");
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	// TODO: Implementing keyboard action

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)	glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_H || key == GLFW_KEY_F1)	print_help();
		else if (key == GLFW_KEY_HOME)						cam = camera();
		else if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT) zoom_trigger = 1;
		else if (key == GLFW_KEY_RIGHT_CONTROL || key == GLFW_KEY_LEFT_CONTROL) pan_trigger = 1;
	}
	if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT) {
			zoom_trigger = 0;
			tb.endzoom();
		}
		else if (key == GLFW_KEY_RIGHT_CONTROL || key == GLFW_KEY_LEFT_CONTROL) {
			pan_trigger = 0;
			tb.endpan();
		}
	}
}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
		vec2 npos = cursor_to_ndc(pos, window_size);
		if (action == GLFW_PRESS && zoom_trigger) tb.beginzoom(cam.view_matrix, npos.y);
		else if (action == GLFW_PRESS && pan_trigger) tb.beginpan(cam.view_matrix, npos);
		else if (action == GLFW_PRESS)			tb.begin(cam.view_matrix, npos);
		else if (action == GLFW_RELEASE) {
			tb.end();
			tb.endzoom();
			tb.endpan();
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
		vec2 npos = cursor_to_ndc(pos, window_size);
		if (action == GLFW_PRESS)			tb.beginzoom(cam.view_matrix, npos.y);
		else if (action == GLFW_RELEASE)	tb.endzoom();
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		dvec2 pos; glfwGetCursorPos(window, &pos.x, &pos.y);
		vec2 npos = cursor_to_ndc(pos, window_size);
		if (action == GLFW_PRESS)			tb.beginpan(cam.view_matrix, npos);
		else if (action == GLFW_RELEASE)	tb.endpan();
	}
}

void motion(GLFWwindow* window, double x, double y)
{
	//if(!tb.is_tracking() && !tb.) return;
	vec2 npos = cursor_to_ndc(dvec2(x, y), window_size);
	if (tb.is_tracking())		cam.view_matrix = tb.update(npos);
	if (tb.is_trackingzoom())
	{
		cam.view_matrix = tb.zoom(npos.y);
	}
	if (tb.is_trackingpan())
	{
		cam.view_matrix = tb.pan(npos);
	}
}

bool user_init()
{
	// log hotkeys
	print_help();

	// init GL states
	glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);	// set clear color
	glEnable(GL_CULL_FACE);								// turn on backface culling
	glEnable(GL_DEPTH_TEST);								// turn on depth tests

	// load the mesh
	pMesh = create_rectangle_mesh(1.0f, 1.0f);
	return true;
}

void user_finalize()
{
}

int main(int argc, char* argv[])
{
	// create window and initialize OpenGL extensions
	if (!(window = cg_create_window(window_name, window_size.x, window_size.y))) { glfwTerminate(); return 1; }
	if (!cg_init_extensions(window)) { glfwTerminate(); return 1; }	// version and extensions

	// initializations and validations
	if (!(program = cg_create_program_from_string(vert_shader, frag_shader))) { glfwTerminate(); return 1; }	// create and compile shaders/program
	if (!user_init()) { printf("Failed to user_init()\n"); glfwTerminate(); return 1; }					// user initialization

	// register event callbacks
	glfwSetWindowSizeCallback(window, reshape);	// callback for window resizing events
	glfwSetKeyCallback(window, keyboard);			// callback for keyboard events
	glfwSetMouseButtonCallback(window, mouse);	// callback for mouse click inputs
	glfwSetCursorPosCallback(window, motion);		// callback for mouse movement

	// enters rendering/event loop
	for (frame = 0; !glfwWindowShouldClose(window); frame++)
	{
		glfwPollEvents();	// polling and processing of events
		update();			// per-frame update
		render();			// per-frame render
	}

	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}
