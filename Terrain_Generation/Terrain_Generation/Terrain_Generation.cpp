#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma comment(lib, "glew32.lib") 

using namespace std;
using namespace glm;

//////////////////////////////
//							//
// Constant Initialisers	//
//							//
//////////////////////////////

//////////////////////////////////////////////////////////
//														//
// Terrain Size											//
//														//
// WARNING - System breaks when MAP_SIZE >= 31			//
//														//
//////////////////////////////////////////////////////////

const int MAP_SIZE = 29;

// Window Size Parameters
const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 1024;

// Used in Diamond-Square
short int step_size = MAP_SIZE - 1;

// Initialise terrain - set values in the height map to 0
float terrain[MAP_SIZE][MAP_SIZE] = {};	

struct Vertex {
	float colors[4];

	vec4 coords;
	vec3 normals;
};

struct Matrix4x4 {
	float entries[16];
};

static mat4 projMat = mat4(1.0);

mat4 modelViewMat = mat4(1.0);

static const Matrix4x4 IDENTITY_MATRIX4x4 =	{
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	}
};

static enum buffer { TERRAIN_VERTICES };
static enum object { TERRAIN };

// Globals
static Vertex terrainVertices[MAP_SIZE*MAP_SIZE] = {};

const int numStripsRequired = MAP_SIZE - 1;
const int verticesPerStrip = 2 * MAP_SIZE;

unsigned int terrainIndexData[numStripsRequired][verticesPerStrip];

// Ambient Value 
static const vec4 globAmb = vec4(0.2, 0.2, 0.2, 1.0);

// Material Struct 
struct Material {
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

// Light Struct 
struct Light {
	vec4 ambCols;
	vec4 difCols;
	vec4 specCols;
	vec4 coords;
};

// Normal Calc Struct
struct TriNorm {
	vec3 PointOne;
	vec3 PointTwo;
	vec3 PointThree;
};

// Front and back material Properties 
static const Material terrainFandB = {
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.0, 0.0, 1.0),
	50.0f
};

// Light constructor 
static const Light light0 = {
	vec4(0.0, 0.0, 0.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 0.0, 0.0)
};

float strafe = 0.25f;

//////////////////////////////
//							//
// Camera Initialisation	//
//							//
//////////////////////////////

// Camera vector definers - Same as (eye, centre, up)
vec3 camera(-3.0f, 0.0f, 33.0f);	// Variable camera 
vec3 centre(0, 0, 0);				// Point for Camera to look at 
vec3 up(0, 180, 0);					// Camera rotation vector

float speed = 0.2;

float cameraTheta = 0;			// Camera Angle Rotation
float cameraPhi = 0;			// 3D Camera Angle Rotation 

vec3 los(0.0, 0.0, -1.0);		// Camera Control

static unsigned int
programId,
vertexShaderId,
fragmentShaderId,
modelViewMatLoc,
projMatLoc,
normalMatLoc,
buffer[1],
vao[1],
texture[1],
grassTexLoc;

// Function to read text file, used to read shader files
char* readTextFile(char* aTextFile) {
	FILE* filePointer = fopen(aTextFile, "rb");
	char* content = NULL;
	long numVal = 0;

	fseek(filePointer, 0L, SEEK_END);
	numVal = ftell(filePointer);
	fseek(filePointer, 0L, SEEK_SET);
	content = (char*)malloc((numVal + 1) * sizeof(char));
	fread(content, 1, numVal, filePointer);
	content[numVal] = '\0';
	fclose(filePointer);
	return content;
}

//////////////////////////////////////////////////////////
//														//
// Diamond Square Algorithm								//
//														//
// Average height of 4 points + random added height		//
//														//
//////////////////////////////////////////////////////////

void diamondStep(int x, int y) {
	terrain[x + (step_size / 2)][y + (step_size / 2)] = ((terrain[x][y] + terrain[x][y + step_size] + terrain[x + step_size][y] + terrain[x + step_size][y + step_size]) / 4) + rand() % 3 - 1;
}

void squareStep(int x, int y) {
	if (step_size == (MAP_SIZE - 1)) {
		terrain[x + (step_size / 2)][y] = ((terrain[x][y] + terrain[step_size][y] + terrain[step_size / 2][step_size / 2] + terrain[step_size / 2][step_size / 2]) / 4) + rand() % 3 - 1;	// Top 
		terrain[x][y + (step_size / 2)] = ((terrain[x][y] + terrain[x][step_size] + terrain[step_size / 2][step_size / 2] + terrain[step_size / 2][step_size / 2]) / 4) + rand() % 3 - 1;	// Left 
		terrain[x + step_size][y + (step_size / 2)] = ((terrain[step_size][y] + terrain[step_size][step_size] + terrain[step_size / 2][step_size / 2] + terrain[step_size / 2][step_size / 2]) / 4) + rand() % 3 - 1;	// Right
		terrain[x + (step_size / 2)][y + step_size] = ((terrain[x][step_size] + terrain[step_size / 2][step_size / 2] + terrain[step_size / 2][step_size / 2] + terrain[step_size][step_size]) / 4) + rand() % 3 - 1;	// Bottom
	}
	else {
		terrain[x + (step_size / 2)][y] = ((terrain[x][y] + terrain[x][step_size] + terrain[step_size][y] + terrain[step_size][step_size]) / 4) + rand() % 3 - 1;
		terrain[x][y + (step_size / 2)] = ((terrain[x][y] + terrain[x][step_size] + terrain[step_size][y] + terrain[step_size][step_size]) / 4) + rand() % 3 - 1;
		terrain[x + (step_size)][y + (step_size / 2)] = ((terrain[x][y] + terrain[x][step_size] + terrain[step_size][y] + terrain[step_size][step_size]) / 4) + rand() % 3 - 1;
		terrain[x + (step_size / 2)][y + (step_size / 2)] = ((terrain[x][y] + terrain[x][step_size] + terrain[step_size][y] + terrain[step_size][step_size]) / 4) + rand() % 3 - 1;
	}
}

//////////////////////////////////
//								//
// Built-in shader test			//
//								//
//////////////////////////////////

void shaderCompileTest(GLuint shader)
{
	GLint result = GL_FALSE;
	int logLength;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
	glGetShaderInfoLog(shader, logLength, NULL, &vertShaderError[0]);
	cout << &vertShaderError[0] << std::endl;
}

//////////////////////////////////
//								//
// Initialisation routine		//
//								//
//////////////////////////////////

void setup(void) {
	vec4 colorsExport;		// Ambient Light Value

	float rand_max = 1.0;	// Maximum random value
	float H = 1.0;			// Roughness co-efficient

	for (int x = 0; x < MAP_SIZE; x++) {
		for (int z = 0; z < MAP_SIZE; z++) {
			terrain[x][z] = 0;
		}
	}

	srand(time(NULL));	// Seed generator
	terrain[0][0] = rand() % 3 - 1;
	terrain[0][MAP_SIZE - 1] = rand() % 3 - 1;
	terrain[MAP_SIZE - 1][0] = rand() % 3 - 1;
	terrain[MAP_SIZE - 1][MAP_SIZE - 1] = rand() % 3 - 1;

	//////////////////////////////////
	//								//
	// Diamond Square Algorithm		//
	//								//
	//////////////////////////////////
	while (step_size > 1) {
		for (int x(0); x < MAP_SIZE - 1; x += step_size)
			for (int y(0); y < MAP_SIZE - 1; y += step_size) {
			diamondStep(x, y);
			}

		for (int x(0); x < MAP_SIZE - 1; x += step_size)
			for (int y(0); y < MAP_SIZE - 1; y += step_size) {
			squareStep(x, y);
			}

		rand_max = rand_max * pow(2, -H);
		step_size = step_size / 2;
	}

	//////////////////////////////////////
	//									//
	// Vertex Normal Setup				//
	//									//
	//////////////////////////////////////
	
	TriNorm triangle;

	vec3 edgeOne, edgeTwo, normOne, normTwo;

	//////////////////////////////////////////////////////////////////////////
	//																		//
	// Triangle Norms														//
	// Use MAP_SIZE - 2 so that it never reaches the ends of the terrain	//
	//																		//
	//////////////////////////////////////////////////////////////////////////

	for (int x(0); x < MAP_SIZE - 2; x++) {
		for (int y(0); y < MAP_SIZE - 2; y++) {
			// Triangle 1 Setup

			// Left Side
			triangle.PointOne.x = x;
			triangle.PointOne.y = y;
			triangle.PointOne.z = terrain[x][y];

			// Right Side
			triangle.PointTwo.x = x + 1;
			triangle.PointTwo.y = y;
			triangle.PointTwo.z = terrain[x + 1][y];

			// Bottom Side
			triangle.PointThree.x = x;
			triangle.PointThree.y = y + 1;
			triangle.PointThree.z = terrain[x][y + 1];

			edgeOne = triangle.PointTwo - triangle.PointOne;
			edgeTwo = triangle.PointThree - triangle.PointOne;

			normOne = cross(edgeOne, edgeTwo);

			terrainVertices[x + MAP_SIZE + 1].normals += normOne;

			// Triangle 2 Setup
			
			// Left Side
			triangle.PointOne.x = x;
			triangle.PointOne.y = y + 1;
			triangle.PointOne.z = terrain[x][y + 1];

			// Right Side
			triangle.PointTwo.x = x + 1;
			triangle.PointTwo.y = y;
			triangle.PointTwo.z = terrain[x + 1][y];

			// Bottom Side
			triangle.PointThree.x = x + 1;
			triangle.PointThree.y = y + 1;
			triangle.PointThree.z = terrain[x + 1][y + 1];

			edgeOne = triangle.PointOne - triangle.PointTwo;
			edgeTwo = triangle.PointOne - triangle.PointThree;

			normTwo = cross(edgeOne, edgeTwo);

			terrainVertices[x + MAP_SIZE + 1].normals += normTwo;
		}
	}

	//////////////////////////
	//						//
	// Vertex Norms			//
	//						//
	//////////////////////////

	for (int x(0); x < MAP_SIZE - 2; x++)
		for (int y(0); y < MAP_SIZE - 2; y++) {
		// Top Left
		if (x == 0 && y == 0) {
			// Only do [x + 1][y + 1]
			terrainVertices[1].normals = normalize(terrainVertices[1 + (1 + (MAP_SIZE))].normals);
		}

		// Top Right
		if (x == 0 && y == MAP_SIZE - 1) {
			// Only do [x - 1][y + 1]
			terrainVertices[MAP_SIZE].normals = normalize(terrainVertices[(MAP_SIZE - 1) + MAP_SIZE].normals);
		}

		// Bottom Left
		if (x == 0 && y == MAP_SIZE - 1) {
			// Only do [x + 1][y - 1]
			terrainVertices[(MAP_SIZE * MAP_SIZE) - (MAP_SIZE + 1)].normals = normalize(terrainVertices[((MAP_SIZE * MAP_SIZE) - (MAP_SIZE + 1)) - (MAP_SIZE + 1)].normals);
		}

		// Bottom Right
		if (x == MAP_SIZE - 1 && y == MAP_SIZE - 1) {
			// Only do [x - 1][y - 1]
			terrainVertices[(MAP_SIZE * MAP_SIZE)].normals = normalize(terrainVertices[(MAP_SIZE * MAP_SIZE) - (MAP_SIZE + 1)].normals);
		}

		// Left Column
		if (x == 0 && (y > 0 || y < MAP_SIZE - 2)) {
			// Only do [x + 1][y - 1] and [x + 1][y + 1]
			terrainVertices[(MAP_SIZE * y)].normals = normalize(terrainVertices[(1 + (MAP_SIZE * y)) + (MAP_SIZE + 1)].normals + terrainVertices[(1 + (MAP_SIZE * y)) + (MAP_SIZE - 1)].normals);
		}

		// Right Column
		if (x == MAP_SIZE - 1 && (y > 0 || y < MAP_SIZE - 2)) {
			// Only do [x - 1][y - 1] and [x - 1][y + 1]
			terrainVertices[(MAP_SIZE * y)].normals = normalize(terrainVertices[(1 + (MAP_SIZE * y)) - (MAP_SIZE + 1)].normals + terrainVertices[(1 + (MAP_SIZE * y)) + (MAP_SIZE - 1)].normals);
		}

		// Top Row
		if (y == 0 && (x > 0 || x < MAP_SIZE - 2)) {
			// Only do [x - 1][y + 1] and [x + 1] [y + 1]
			terrainVertices[x].normals = normalize(terrainVertices[(1 + x) + (MAP_SIZE - 1)].normals + terrainVertices[(1 + x) + (MAP_SIZE + 1)].normals);
		}

		// Bottom Row
		if (y == MAP_SIZE - 1 && (x > 0 || x < MAP_SIZE - 1)) {
			// Only do [x - 1][y - 1] and [x + 1] [y - 1]
			terrainVertices[x].normals = normalize(terrainVertices[(1 + (MAP_SIZE * y)) - (MAP_SIZE + 1)].normals + terrainVertices[(1 + x) - (MAP_SIZE + 1)].normals);
		}

		else {
			// Do [x - 1][y - 1], [x + 1][y - 1], [x - 1][y + 1], [x + 1][y + 1]
			terrainVertices[x + (MAP_SIZE * y)].normals = normalize(terrainVertices[x + ((MAP_SIZE * y) - 1)].normals + terrainVertices[x - ((MAP_SIZE * y) - 1)].normals
				+ terrainVertices[x - ((MAP_SIZE * y) + 1)].normals + terrainVertices[x + ((MAP_SIZE * y) + 1)].normals);
		}
		}

	///////////////////////////////////////

	// Intialise vertex array
	int i = 0;

	for (int z = 0; z < MAP_SIZE; z++) {
		for (int x = 0; x < MAP_SIZE; x++) {
			// Set the coords (1st 4 elements) and a default colour of black (2nd 4 elements) 
			terrainVertices[i] = { { (float)x, terrain[x][z], (float)z, 1.0 }, { 0.0, 0.0, 0.0, 1.0 } };
			i++;
		}
	}

	// Builds index data 
	for (int z = 0; z < MAP_SIZE - 1; z++) {
		i = z * MAP_SIZE;
		for (int x = 0; x < MAP_SIZE * 2; x += 2) {
			terrainIndexData[z][x] = i;
			i++;
		}
		for (int x = 1; x < MAP_SIZE * 2 + 1; x += 2) {
			terrainIndexData[z][x] = i;
			i++;
		}
	}

	glClearColor(1.0, 1.0, 1.0, 0.0);
	glEnable(GL_DEPTH_TEST);

	// Create shader program executable - read, compile and link shaders 
	char* vertexShader = readTextFile("vertexShader.glsl");
	vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, (const char**)&vertexShader, NULL);
	glCompileShader(vertexShaderId);

	char* fragmentShader = readTextFile("fragmentShader.glsl");
	fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, (const char**)&fragmentShader, NULL);
	glCompileShader(fragmentShaderId);

	programId = glCreateProgram();
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);
	glLinkProgram(programId);
	glUseProgram(programId);

	///////////////////////////////////////

	// Create vertex array object (VAO) and vertex buffer object (VBO) and associate data with vertex shader.
	glGenVertexArrays(1, vao);
	glGenBuffers(1, buffer);
	glBindVertexArray(vao[TERRAIN]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[TERRAIN_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(terrainVertices), terrainVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)sizeof(terrainVertices[0].coords));	
	glEnableVertexAttribArray(1);
	///////////////////////////////////////

	// Obtain projection matrix uniform location and set value.
	projMatLoc = glGetUniformLocation(programId, "projMat");
	//projMat = ortho(0.0, 100.0, 0.0, 100.0, 1.0, 1.0);
	projMat = perspective(radians(60.0), (double)SCREEN_WIDTH / (double)SCREEN_HEIGHT, 0.1, 200.0);
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, value_ptr(projMat));

	///////////////////////////////////////

	// Obtain modelview matrix uniform location and set value.
	// Move terrain into view - glm::translate replaces glTranslatef
	modelViewMat = translate(modelViewMat, camera); // 5x5 grid
	modelViewMatLoc = glGetUniformLocation(programId, "modelViewMat");
	glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, value_ptr(modelViewMat));

	///////////////////////////////////////

	// Obtain normal matrix uniform location and set value - Correct at Step 10 of Workshop 7
	static mat3 normalMat = mat3(1.0);
	normalMatLoc = glGetUniformLocation(programId, "normalMat");
	normalMat = transpose(inverse(mat3(modelViewMatLoc)));
	glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, value_ptr(normalMat));

	///////////////////////////////////////

	// BUFFER ASSIGNMENTS

	// Lighting assignment for buffer 
	glUniform4fv(glGetUniformLocation(programId, "globAmb"), 1, &globAmb[0]);

	// Material assignment for buffer - Material Uniform Locations		
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.ambRefl"), 1, &terrainFandB.ambRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.difRefl"), 1, &terrainFandB.difRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.specRefl"), 1, &terrainFandB.specRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.shininess"), 1, &terrainFandB.shininess);

	// Material assignment for buffer - Light
	glUniform4fv(glGetUniformLocation(programId, "light0.ambCols"), 1, &light0.ambCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.difCols"), 1, &light0.difCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.specCols"), 1, &light0.specCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.coords"), 1, &light0.coords[0]);

}

// Drawing routine.
void drawScene(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 2D Camera View
	modelViewMat = lookAt(camera, camera + los, up);
	glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, value_ptr(modelViewMat));

	// For each row - draw the triangle strip
	for (int i = 0; i < MAP_SIZE - 1; i++) {
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, terrainIndexData[i]);
	}

	glFlush();
}

// OpenGL window reshape routine.
void resize(int w, int h) {
	glViewport(0, 0, w, h);
}

// Keyboard input processing routine.
void keyInput(unsigned char key, int x, int y) {
	switch (key) {
		// Strafes Right
	case 'a':
		camera -= cross(los, up) * (speed / 10);
		drawScene();
		break;
		// Strafes Left
	case 'd':
		camera += cross(los, up) * (speed / 10);
		drawScene();
		break;
		// Moves Forward
	case'w':
		camera.x = camera.x + los.x * speed;
		camera.z = camera.z + los.z * speed;
		drawScene();
		break;
		// Moves Back
	case's':
		camera.x = camera.x - los.x * speed;
		camera.z = camera.z - los.z * speed;
		drawScene();
		break;
		// Rotates Left
	case'q': 
		cameraTheta -= 3;
		los.x = sin(radians(cameraTheta));
		los.z = -cos(radians(cameraTheta));
		drawScene();
		break;
		// Rotates Right
	case'e':
		cameraTheta += 3;
		los.x = sin(radians(cameraTheta));
		los.z = -cos(radians(cameraTheta));
		drawScene();
		break;
		// Moves camera statically up
	case 'r':
		camera.y = camera.y + 1;
		drawScene();
		break;
		// Moves camera statically down
	case 't':
		camera.y = camera.y - 1;
		drawScene();
		break;
		// Rotates upwards
	case'x':
		cameraPhi += 3;
		los.x = (cos(radians(cameraPhi)) * sin(radians(cameraTheta)));
		los.y = sin(radians(cameraPhi));
		los.z = (cos(radians(cameraPhi)) * -cos(radians(cameraTheta)));
		drawScene();
		break;
		// Rotates downwards
	case'z':
		cameraPhi -= 3;
		los.x = (cos(radians(cameraPhi)) * sin(radians(cameraTheta)));
		los.y = sin(radians(cameraPhi));
		los.z = (cos(radians(cameraPhi)) * -cos(radians(cameraTheta)));
		drawScene();
		break;
	case 27:
		exit(0);
		break;
	default:
		break;
	}
}

// Main routine.
int main(int argc, char* argv[]) {
	glutInit(&argc, argv);

	// Set the version of OpenGL (4.2)
	glutInitContextVersion(4, 2);
	// The core profile excludes all discarded features
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// Forward compatibility excludes features marked for deprecation ensuring compatability with future versions
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("TerrainGeneration");

	// Set OpenGL to render in wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyInput);

	glewExperimental = GL_TRUE;
	glewInit();

	setup();

	glutMainLoop();
}
