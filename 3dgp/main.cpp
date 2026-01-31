#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// 3D models
C3dglModel camera;
C3dglModel lamp;
// The View Matrix
mat4 matrixView;

// Camera & navigation
float maxspeed = 4.f;	// camera max speed
float accel = 4.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)

bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// setup lighting
	glEnable(GL_LIGHTING);									// --- DEPRECATED
	glEnable(GL_LIGHT0);									// --- DEPRECATED

	
	// ---------- AMBIENT LIGHT ----------
	GLfloat ambientLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };  // soft ambient
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);



	// load your 3D models here!
	if (!camera.load("models\\camera.3ds")) return false;
	if (!lamp.load("models\\Lamp.3ds")) return false;


	// Initialise the View Matrix (initial position of the camera)
	matrixView = rotate(mat4(1), radians(12.f), vec3(1, 0, 0));
	matrixView *= lookAt(
    vec3(0.0f, 6.0f, 18.0f),   // BACK + UP
    vec3(0.0f, 3.0f, 0.0f),    // look at table
    vec3(0.0f, 1.0f, 0.0f));


	// setup the screen background colour
	glClearColor(0.18f, 0.25f, 0.22f, 1.0f);   // deep grey background

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Shift to speed up your movement" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;

	return true;
}
// Returns bulb world position (in scene coords, NOT view space) in outBulbPos
void drawLampPrimitive(const mat4& view, const vec3& basePos, float yawDeg)
{
	mat4 m;

	// ---- metal ----
	GLfloat metal[] = { 0.75f, 0.7f, 0.3f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, metal);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, metal);

	// ---- BASE (flat on table) ----
	mat4 baseM = view;
	baseM = translate(baseM, basePos);
	baseM = rotate(baseM, radians(yawDeg), vec3(0, 1, 0));

	// ONLY rotate for drawing the base
	mat4 baseDrawM = rotate(baseM, radians(-90.f), vec3(1, 0, 0));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&baseDrawM);
	glutSolidCylinder(0.5, 0.05, 32, 2);


	mat4 stem = baseM;
	stem = translate(stem, vec3(0.0f, 0.05f, 0.0f)); // start just above base

	// ---- GOOSENECK (smooth curve) ----
	const int segments = 20;
	const float segLen = 0.09f;
	const float radius = 0.045f;
	const float bend = 4.0f;

	
	stem = translate(stem, vec3(0, 0.05f, 0));

	for (int i = 0; i < segments; i++)
	{
		stem = rotate(stem, radians(-bend), vec3(1, 0, 0));

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&stem);
		glutSolidCylinder(radius, segLen, 16, 1);

		stem = translate(stem, vec3(0, segLen, 0));
	}

	// ---- SOCKET ----
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&stem);
	glutSolidCylinder(0.09, 0.1, 16, 1);

	// ---- BULB (IMPORTANT FOR LIGHT) ----
	GLfloat bulbOff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bulbOff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bulbOff);

	m = translate(stem, vec3(0, 0.14f, 0));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&m);
	glutSolidSphere(0.14, 24, 24);
}



void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;
	vec3 tablePos = vec3(10.0f, 0.0f, 0.0f);
	float tableTopY = 1.0f;

	// ---------- MATERIAL: GREY ----------
	GLfloat rgbaGrey[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rgbaGrey);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rgbaGrey);

	// ---------- CAMERA MODEL ----------
	m = matrixView;
	m = translate(m, vec3(-3.0f, 0.0f, 0.0f));
	m = rotate(m, radians(180.f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(0.04f, 0.04f, 0.04f));
	camera.render(m);

	// ---------- TABLE MATERIAL (WOOD) ----------
	GLfloat woodCol[] = { 0.55f, 0.35f, 0.18f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, woodCol);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, woodCol);

	// ---------- TABLE TOP ----------
	m = matrixView;
	m = translate(m, tablePos + vec3(0.0f, tableTopY, 0.0f));
	m = scale(m, vec3(12.0f, 0.4f, 7.0f));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&m);
	glutSolidCube(1.0);



	// ---------- TABLE LEGS ----------
	vec3 legs[4] = {
	vec3(5.4f, -1.2f,  3.2f),
	vec3(-5.4f, -1.2f,  3.2f),
	vec3(5.4f, -1.2f, -3.2f),
	vec3(-5.4f, -1.2f, -3.2f)
	};

	for (int i = 0; i < 4; i++)
	{
		m = matrixView;
		m = translate(m, tablePos + vec3(0.0f, tableTopY, 0.0f) + legs[i]);
		m = scale(m, vec3(0.3f, 2.4f, 0.3f));

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&m);
		glutSolidCube(1.0);

		vec3 tablePos = vec3(15.0f, 0.0f, 9.0f);
		float tableTopY = 1.0f;
	}

	// ---------- LAMPS (ON TABLE) ----------
	vec3 lamp1Base = tablePos + vec3(-4.8f, tableTopY + 0.25f, -2.0f);
	vec3 lamp2Base = tablePos + vec3(4.8f, tableTopY + 0.25f, 2.5f);

	drawLampPrimitive(matrixView, lamp1Base, -25.f);
	drawLampPrimitive(matrixView, lamp2Base, 25.f);



	// ---------- MATERIAL: BLUE ----------
	GLfloat rgbaBlue[] = { 0.2f, 0.2f, 0.8f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rgbaBlue);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rgbaBlue);

	
	// ---------- TEAPOT (ON THE TABLE) ----------
	m = matrixView;
	m = translate(m, tablePos + vec3(4.0f, tableTopY + 0.6f, 1.0f));

	m = rotate(m, radians(120.f), vec3(0.0f, 1.0f, 0.0f));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&m);
	glutSolidTeapot(0.6);

}


void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in seconds
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// setup the View Matrix (camera)
	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;

	// render the scene objects
	renderScene(matrixView, time, deltaTime);

	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	mat4 matrixProjection = perspective(radians(_fov), ratio, 0.02f, 1000.f);

	// Setup the Projection Matrix
	glMatrixMode(GL_PROJECTION);							// --- DEPRECATED
	glLoadIdentity();										// --- DEPRECATED
	glMultMatrixf((GLfloat*)&matrixProjection);				// --- DEPRECATED
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = glm::clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("3DGL Scene: First Example");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	C3dglLogger::log("Vendor: {}", (const char *)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char *)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	return 1;
}

