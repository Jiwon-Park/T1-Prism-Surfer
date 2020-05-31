#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "cgmath.h"		// slee's simple math library
#include "cgut.h"		// slee's OpenGL utility
#include "trackball.h"	// virtual trackball
#include "shaders.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define MAX_MAP_V 0.008f*PI
#define MIN_MAP_V 0.004f*PI
#define MIN_MAP_C 3
#define MAX_MAP_C 5
#define OBS_CREATE_TIME 1.5
#define OBS_CREATE_DIST 70
#define FRONT_SPEED 0.3f
#define SIDE_SPEED 0.2f
#define CAM_PLAYER_DISTANCE 15.0f

using namespace std;
//*************************************
// global constants
static const char*	window_name = "prismsurfer";
static const uint	texture_num = 4;
static const char*	texture_path[texture_num] = { "background.jpg", "tiles.png", "obstacle.png", "player.png" };
static const bool	texture_alpha[texture_num] = { false, true, true, true };

const uint	NUM_RECT = 6;
const float radius = 5.0f;
const float width = 2 * radius * tanf(PI / 6);
const float height = 5.0f;

const float backwidth = width * 14;
const float backheight = backwidth / 1440 * 960;

const uint dist_view = 12;

//*************************************
// common structures
struct camera
{
	vec3	eye = vec3(0, 0, 0);
	vec3	at = vec3(0, 0, 1);
	vec3	up = vec3(0, 1, 0);
	mat4	view_matrix = mat4::look_at(eye, at, up);

	float	fovy = PI / 4.0f; // must be in radian
	float	aspect;
	float	dnear = 1.0f;
	float	dfar = 1000.0f;
	mat4	projection_matrix;
};

typedef struct Obstacle{
	int wall_num;
	float position;
}obstacle;

//*************************************
// window objects
GLFWwindow* window = nullptr;
ivec2		window_size = cg_default_window_size(); // initial window size

//*************************************
// OpenGL objects
GLuint	program = 0;	// ID holder for GPU program
GLuint  texture[texture_num];		// bg, tile, obstacle, player
//*************************************
// global variables
int		frame = 0;				// index of rendering frames
int		zoom_trigger = 0;
int		pan_trigger = 0;

int state_right = 0;
int state_left = 0;

float map_v, map_c, player_position = 3.5 * width, ob_time, map_angle = 0;
vector<obstacle> obstacles;

//*************************************
// scene objects
mesh* mMesh = nullptr, * oMesh = nullptr, * pMesh = nullptr, * bMesh = nullptr;
camera		cam;
trackball	tb;

//*************************************

mesh* create_rectangle_mesh(float width, float height)
{
	mesh* msh = new mesh();

	// TODO: Creating vertex buffer and index buffer of a rectangle
	vector<vertex> vlist;
	vector<uint> ilist = { 0,2,1,2,0,3 };
	vertex v;
	v.pos = vec3(-width / 2, radius, 0);				//left two vertex
	v.norm = vec3(0, -1, 0);
	v.tex = vec2(.0f, .0f);

	vlist.push_back(v);

	v.pos = vec3(-width / 2, radius, height);
	v.tex = vec2(.0f, 1.0f);
	vlist.push_back(v);

	v.pos = vec3(width / 2, radius, height);			//right two vertex
	//v.norm = vec3(-sinf(PI / 6), -cosf(PI / 6), 0);
	v.tex = vec2(1.0f, 1.0f);
	vlist.push_back(v);

	v.pos = vec3(width / 2, radius, 0);
	v.tex = vec2(1.0f, .0f);
	vlist.push_back(v);
	
	msh->vertex_list.resize(vlist.size());
	msh->index_list.resize(ilist.size());
	copy(vlist.begin(), vlist.end(), msh->vertex_list.begin());
	copy(ilist.begin(), ilist.end(), msh->index_list.begin());

	glGenBuffers(1, &msh->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, msh->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * msh->vertex_list.size(), &msh->vertex_list[0], GL_STATIC_DRAW);

	glGenBuffers(1, &msh->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, msh->index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * msh->index_list.size(), &msh->index_list[0], GL_STATIC_DRAW);

	msh->vertex_array = cg_create_vertex_array(msh->vertex_buffer, msh->index_buffer);
	if(!msh->vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return nullptr; }

	return msh;
}

mesh* create_obstacle_mesh(float width, float height)
{
	mesh* msh = new mesh();

	// TODO: Creating vertex buffer and index buffer of a rectangle
	vector<vertex> vlist;
	vector<uint> ilist = { 0,1,2,2,3,0 };
	vertex v;

	v.pos = vec3(-width / 2, radius, 0);				//up two vertex
	v.norm = vec3(0, 0, -1);
	v.tex = vec2(.0f, .0f);

	vlist.push_back(v);

	v.pos = vec3(width / 2, radius, 0);
	v.tex = vec2(.0f, 1.0f);
	vlist.push_back(v);

	v.pos = vec3(width * (radius - height) / (2 * radius), radius - height, 0);			//down two vertex
	v.tex = vec2(1.0f, 1.0f);
	vlist.push_back(v);

	v.pos = vec3(-width * (radius - height) / (2 * radius), radius - height, 0);
	v.tex = vec2(1.0f, .0f);
	vlist.push_back(v);

	msh->vertex_list.resize(vlist.size());
	msh->index_list.resize(ilist.size());
	copy(vlist.begin(), vlist.end(), msh->vertex_list.begin());
	copy(ilist.begin(), ilist.end(), msh->index_list.begin());

	glGenBuffers(1, &msh->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, msh->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * msh->vertex_list.size(), &msh->vertex_list[0], GL_STATIC_DRAW);

	glGenBuffers(1, &msh->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, msh->index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * msh->index_list.size(), &msh->index_list[0], GL_STATIC_DRAW);

	msh->vertex_array = cg_create_vertex_array(msh->vertex_buffer, msh->index_buffer);
	if (!msh->vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return nullptr; }

	return msh;
}

mesh* create_player_mesh(float width, float height)
{
	mesh* msh = new mesh();

	// TODO: Creating vertex buffer and index buffer of a rectangle
	vector<vertex> vlist;
	vector<uint> ilist = { 0,2,1,2,0,3 };
	vertex v;
	v.pos = vec3(-width / 2, 0, 0);				//left two vertex
	v.norm = vec3(0, -1, 0);
	v.tex = vec2(.0f, .0f);

	vlist.push_back(v);

	v.pos = vec3(-width / 2, -height, 0);
	v.tex = vec2(.0f, 1.0f);
	vlist.push_back(v);

	v.pos = vec3(width / 2, -height, 0);			//right two vertex
	//v.norm = vec3(-sinf(PI / 6), -cosf(PI / 6), 0);
	v.tex = vec2(1.0f, 1.0f);
	vlist.push_back(v);

	v.pos = vec3(width / 2, 0, 0);
	v.tex = vec2(1.0f, .0f);
	vlist.push_back(v);

	msh->vertex_list.resize(vlist.size());
	msh->index_list.resize(ilist.size());
	copy(vlist.begin(), vlist.end(), msh->vertex_list.begin());
	copy(ilist.begin(), ilist.end(), msh->index_list.begin());

	glGenBuffers(1, &msh->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, msh->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * msh->vertex_list.size(), &msh->vertex_list[0], GL_STATIC_DRAW);

	glGenBuffers(1, &msh->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, msh->index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * msh->index_list.size(), &msh->index_list[0], GL_STATIC_DRAW);

	msh->vertex_array = cg_create_vertex_array(msh->vertex_buffer, msh->index_buffer);
	if (!msh->vertex_array) { printf("%s(): failed to create vertex aray\n", __func__); return nullptr; }

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
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// Draw background
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	model_matrix = mat4::rotate(vec3(0,0,1), -map_angle) * mat4::translate(0, -backheight / 2, height * dist_view + cam.eye.z) * mat4::rotate(vec3(0, 0, 1), PI);

	uloc = glGetUniformLocation(program, "model_matrix");
	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

	if (bMesh && bMesh->vertex_array) glBindVertexArray(bMesh->vertex_array);
	glDrawElements(GL_TRIANGLES, bMesh->index_list.size(), GL_UNSIGNED_INT, nullptr);

	// Draw field
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	int st = int(cam.eye.z / height);
	for (int s = st; s < st + int(dist_view); s++)
	{
		for (int i = 0; i < NUM_RECT; i++)
		{
			//float t = float(glfwGetTime()) / 10.0f;
			// TODO: generate model matrix

			model_matrix = mat4::translate(vec3(0, 0, height * s)) * mat4::rotate(vec3(0, 0, 1), PI * i / 3);
			uloc = glGetUniformLocation(program, "model_matrix");
			if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

			// bind vertex array object
			if (mMesh && mMesh->vertex_array) glBindVertexArray(mMesh->vertex_array);



			// render vertices: trigger shader programs to process vertex data
			glDrawElements(GL_TRIANGLES, mMesh->index_list.size(), GL_UNSIGNED_INT, nullptr);
		}
	}

	// Draw obstacle
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	for (auto it = obstacles.rbegin(); it != obstacles.rend(); ++it)
	{
		model_matrix = mat4::translate(vec3(0, 0, it->position)) * mat4::rotate(vec3(0, 0, 1), PI * it->wall_num / 3);
		uloc = glGetUniformLocation(program, "model_matrix");
		if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);
		else printf("model matrix find error\n");

		if (oMesh && oMesh->vertex_array)glBindVertexArray(oMesh->vertex_array);


		glDrawElements(GL_TRIANGLES, oMesh->index_list.size(), GL_UNSIGNED_INT, nullptr);
	}

	glBindTexture(GL_TEXTURE_2D, texture[3]);
	int player_loc = int(player_position / width);
	float player_off = player_position - float(player_loc * width) - width / 2;
	model_matrix = mat4::translate(vec3(0, 0, cam.eye.z + CAM_PLAYER_DISTANCE)) * mat4::rotate(vec3(0, 0, 1), PI * player_loc / 3)
		* mat4::translate(vec3(-player_off, 0, 0)) * mat4::translate(vec3(0, radius, 0));
	uloc = glGetUniformLocation(program, "model_matrix");
	if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

	if (pMesh && pMesh->vertex_array)glBindVertexArray(pMesh->vertex_array);
	glDrawElements(GL_TRIANGLES, pMesh->index_list.size(), GL_UNSIGNED_INT, nullptr);

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
		else if (key == GLFW_KEY_LEFT) {
			state_left = 1;
		}
		else if (key == GLFW_KEY_RIGHT) {
			state_right = 1;
		}
	}
	if(action==GLFW_RELEASE){
		if (key == GLFW_KEY_LEFT) {
			state_left = 0;
		}
		else if (key == GLFW_KEY_RIGHT) {
			state_right = 0;
		}
	}
}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
	//delete when release
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
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	mMesh = create_rectangle_mesh(width, height);
	oMesh = create_obstacle_mesh(width, radius / 2);
	pMesh = create_player_mesh(width / 5, width / 5);
	bMesh = create_player_mesh(backwidth, backheight);


	glGenTextures(texture_num, texture);
	image *data;
	for (uint i = 0; i < texture_num; i++)
	{
		data = cg_load_image(texture_path[i], texture_alpha[i]);
		if (data == NULL)
		{
			printf("Texture file load failed %d\n", i);
			return false;
		}
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB + int(texture_alpha[i]), data->width, data->height, 0, GL_RGB + int(texture_alpha[i]), GL_UNSIGNED_BYTE, data->ptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		free(data);
	}

	return true;
}

void user_finalize()
{
}

float rand_range(float min, float max) {
	return (float)(rand() % 10000) / 10000 * (max - min) + min;
}

void create_obstacle() {
	int flag[6] = { 0, };
	int num = rand() % 3 + 3;
	for (int i = 0; i < num; i++) {
		int t = rand() % 6;
		if (flag[t] == 1) {
			i--;
			continue;
		}
		flag[t] = 1;
		obstacle ob;
		ob.wall_num = t;
		ob.position = cam.eye.z + OBS_CREATE_DIST;
		obstacles.push_back(ob);
	}
}

void game_initialize() {
	srand((int)time(NULL));
	int dif;
	printf("Select difficulty\n1:easy\n2:hard\n>>");
	scanf("%d", &dif);
	if (dif == 2) {
		map_v = rand_range(MIN_MAP_V, MAX_MAP_V);
		map_c = rand_range(MIN_MAP_C, MAX_MAP_C);
	}
	else {
		map_v = 0;
		map_c = 999999999.0f;
	}
	float t = float(glfwGetTime());
	create_obstacle();
	ob_time =t+ OBS_CREATE_TIME;
}

int game_update() {
	float t = float(glfwGetTime());
	if (t >= ob_time) {
		create_obstacle();
		ob_time += OBS_CREATE_TIME;
	}
	if (t >= map_c) {
		map_v = rand_range(MIN_MAP_V, MAX_MAP_V);
		if (rand() % 2 == 0) {
			map_v *= -1;
		}
		map_c += rand_range(MIN_MAP_C, MAX_MAP_C);
	}
	//player update
	if(state_right ==1 && state_left==0){
		player_position -= SIDE_SPEED;
		if (t < 60) cam.eye.z -= t/60.0f * SIDE_SPEED;
		else cam.eye.z -= SIDE_SPEED;
		player_position = player_position <0  ? player_position + width * 6 : player_position;
	}
	else if (state_left == 1 && state_right == 0) {
		player_position += SIDE_SPEED;
		if (t < 60) cam.eye.z += t/60.0f * SIDE_SPEED;
		else cam.eye.z += SIDE_SPEED;
		player_position = player_position > width * 6 ? player_position - width * 6 : player_position;
	}
	//obstacle crush check, remove
	vector<obstacle>::iterator it = obstacles.begin();
	while (it != obstacles.end()) {
		if (it->position < cam.eye.z + CAM_PLAYER_DISTANCE) {
			if (it->wall_num * width <= player_position + width / 10 && (it->wall_num + 1) * width >= player_position - width / 10) {
				return -1;
			}

			it = obstacles.erase(it);
		}
		else {
			it++;
		}
	}
	//cam update
	cam.eye.z += FRONT_SPEED;
	if (t < 60) cam.eye.z += t/60.0f * FRONT_SPEED;
	else cam.eye.z += FRONT_SPEED;
	cam.at.z = cam.eye.z + 1;
	map_angle += map_v;
	map_angle = map_angle > 2 * PI ? map_angle - 2 * PI : map_angle;
	map_angle = map_angle < 0 ? map_angle + 2 * PI : map_angle;

	cam.up.x = sinf(map_angle);
	cam.up.y = cosf(map_angle);
	cam.view_matrix = mat4::look_at(cam.eye, cam.at, cam.up);
	return 0;
}

int main(int argc, char* argv[])
{
	// create window and initialize OpenGL extensions
	game_initialize();
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
	int time_count = 0;
	float ctime;
	for (frame = 0; !glfwWindowShouldClose(window);)
	{
		if(time_count ==0){
			ctime =float( glfwGetTime());
			time_count++;
		}
		if (ctime + 1.0f / 60 <= glfwGetTime()) {
			frame++;
			glfwPollEvents();	// polling and processing of events
			if (game_update()) break;
			update();			// per-frame update
			render();			// per-frame render
			time_count = 0;
		}
		// FPS
	}
	// todo:print score, game over
	// normal termination
	user_finalize();

	return 0;
}
