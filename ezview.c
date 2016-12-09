#define GLFW_DLL 1

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

#include "deps/linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct Pixel {

	unsigned char r, g, b, a;

}   Pixel;

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

// (-1, 1)  (1, 1)
// (-1, -1) (1, -1)

Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0.99999}},
  { { -1, -1 }, { 0, 0.99999 } },
  { { 1, 1 }, { 0.99999, 0 } },
  { { -1, 1 }, { 0, 0} }
  
};

const GLubyte Indices[] = {
	0, 1, 2,
	2, 3, 1
};

mat4x4 m, p, mvp, shear_mat_x, shear_mat_y;
mat4x4 scale_mat;


float current_x = 0;
float current_y = 0;
float current_shear_x = 0;
float current_shear_y = 0;

static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec2 TexCoordIn;\n"
"attribute vec2 vPos;\n"
"varying vec2 TexCoordOut;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}\n";

static const char* fragment_shader_text =
"varying lowp vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}\n";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


//Setup Keyboard shortcuts
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_Q && action != GLFW_RELEASE){
		mat4x4_rotate_Z(m, m, .0628);
	}
	if (key == GLFW_KEY_E && action != GLFW_RELEASE){
		mat4x4_rotate_Z(m, m, -.0628);
	}if (key == GLFW_KEY_W && action != GLFW_RELEASE){
		current_y += .01;
		mat4x4_translate(m, current_x, current_y, 0);
	}
	if (key == GLFW_KEY_S && action != GLFW_RELEASE){
		current_y -= .01;
		mat4x4_translate(m, current_x, current_y, 0);
	}
	if (key == GLFW_KEY_D && action != GLFW_RELEASE){
		current_x += .01;
		mat4x4_translate(m, current_x, current_y, 0);
	}
	if (key == GLFW_KEY_A && action != GLFW_RELEASE){
		current_x -= .01;
		mat4x4_translate(m, current_x, current_y, 0);
	}
	if (key == GLFW_KEY_X && action != GLFW_RELEASE){
		current_shear_x = .25;
		shear_mat_x[1][0] = current_shear_x;
		mat4x4_mul(m, shear_mat_x, m);
	}
	if (key == GLFW_KEY_Z && action != GLFW_RELEASE){
		current_shear_x = -.25;
		shear_mat_x[1][0] = current_shear_x;
		mat4x4_mul(m, shear_mat_x, m);
	}
	if (key == GLFW_KEY_V && action != GLFW_RELEASE){
		current_shear_y = .25;
		shear_mat_y[0][1] = current_shear_y;
		mat4x4_mul(m, shear_mat_y, m);
	}
	if (key == GLFW_KEY_C && action != GLFW_RELEASE){
		current_shear_y = -.25;
		shear_mat_y[0][1] = current_shear_y;
		mat4x4_mul(m, shear_mat_y, m);
	}
	if (key == GLFW_KEY_N && action != GLFW_RELEASE){
		scale_mat[0][0] = 1.25;
		scale_mat[1][1] = 1.25;
		scale_mat[2][2] = 1.25;
		mat4x4_mul(m, scale_mat, m);
	}
	if (key == GLFW_KEY_M && action != GLFW_RELEASE){
		scale_mat[0][0] = .75;
		scale_mat[1][1] = .75;
		scale_mat[2][2] = .75;
		mat4x4_mul(m, scale_mat, m);
	}

		

}

Pixel* readP6(char* fname, int* out_width, int* out_height){
	Pixel* buffer;

	FILE *f;
	f = fopen(fname, "rb");


	if (fgetc(f) != 'P'){
		fprintf(stderr, "Error: This is not a PPM file!");
		exit(1);
	}
	if (fgetc(f) != '6'){
		fprintf(stderr, "Error: This file is not in P6 format!");
		exit(1);
	}

	int specs[3]; //array holding file data parsed from header: specs[0] = height, specs[1] = width, specs[2] = max color value.
	int i = 0;
	while (i < 3){
		unsigned char c = fgetc(f);
		while (c != ' ' && c != '\n'){
			if (c == '#'){ //filter out comments
				while (c != '\n'){
					c = fgetc(f);
				}
			}
			else{
				char *intbuff;
				intbuff = malloc(sizeof(char) * 4);
				int j = 0;
				while (c != ' ' && c != '\n'){
					intbuff[j] = c;
					j++;
					c = fgetc(f);
				}
				specs[i] = atoi(intbuff);
				i++;
			}
		}
	}

	if (specs[2] != 255){
		fprintf(stderr, "Error: The image is not 8-bit per channel.");
		exit(1);
	}

	*out_width = specs[0];
	*out_height = specs[1];
	int width = specs[0];
	int height = specs[1];

	buffer = malloc(sizeof(Pixel)*width*height);

	int j = 0;
	while (j < width*height){
		unsigned char * rgb;
		rgb = malloc(sizeof(unsigned char) * 4);
		fread(rgb, 1, 3, f);
		Pixel p;
		p.r = rgb[0];
		p.g = rgb[1];
		p.b = rgb[2];
		p.a = 255;
		buffer[j] = p;
		j++;
	}

	fclose(f);

	return buffer;





}



void glCompileShaderOrDie(GLuint shader) {
  GLint compiled;
  glCompileShader(shader);
  glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetShaderInfoLog(shader, infoLen, &done, info);
    printf("Unable to compile shader: %s\n", info);
    exit(1);
  }
}
/*
// 4 x 4 image..
unsigned char image[] = {
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,

  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,

  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,

  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255
};
*/
int main(int argc, char *argv[])
{
    GLFWwindow* window;
    GLuint vertex_buffer, index_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;
	

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	
    window = glfwCreateWindow(640, 480, "EZ View", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) (sizeof(float) * 2));
    
    int image_width;
    int image_height;
	char* filename = argv[1];
	Pixel* image_raw;
	unsigned char* image;

	//Load Image Data
	image_raw = readP6(filename, &image_width, &image_height);

	image = malloc(sizeof(unsigned char) * image_width * image_height * 4);

	for (int i = 0; i < image_width * image_height; i++){
		image[i * 4] = image_raw[i].r;
		image[i * 4 + 1] = image_raw[i].g;
		image[i * 4 + 2] = image_raw[i].b;
		image[i * 4 + 3] = image_raw[i].a;
		
	}

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, 
		 GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

	float ratio;
	int width, height;
	mat4x4 mvp, p;

	glfwGetFramebufferSize(window, &width, &height);
	ratio = width / (float)height;

	mat4x4_identity(m);
	mat4x4_identity(shear_mat_x);
	mat4x4_identity(shear_mat_y);
	mat4x4_identity(scale_mat);

	//mat4x4_scale(scale_mat, scale_mat, 2);


	mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
	mat4x4_mul(mvp, p, m);

    while (!glfwWindowShouldClose(window))
    {
        
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;

		mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);

		mat4x4_mul(mvp, p, m);
        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawElements(GL_TRIANGLES,
			sizeof(Indices) / sizeof(GLubyte),
			GL_UNSIGNED_BYTE, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

//! [code]
