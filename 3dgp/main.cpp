#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>
GLuint woodTex = 0;
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
#include <cstdio>

GLuint loadBMP(const char* filename)
{
	FILE* file = nullptr;
	fopen_s(&file, filename, "rb");
	if (!file) return 0;

	unsigned char header[54];
	if (fread(header, 1, 54, file) != 54) { fclose(file); return 0; }

	// Must start with BM
	if (header[0] != 'B' || header[1] != 'M') { fclose(file); return 0; }

	unsigned int dataPos = *(unsigned int*)&(header[0x0A]);
	unsigned int width = *(unsigned int*)&(header[0x12]);
	unsigned int height = *(unsigned int*)&(header[0x16]);
	unsigned short bpp = *(unsigned short*)&(header[0x1C]);
	unsigned int compression = *(unsigned int*)&(header[0x1E]);

	// Only supports 24-bit uncompressed BMP
	if (bpp != 24 || compression != 0 || width == 0 || height == 0) { fclose(file); return 0; }

	// Move to pixel data
	fseek(file, dataPos, SEEK_SET);

	// BMP rows are padded to multiples of 4 bytes
	int rowPadded = (width * 3 + 3) & (~3);
	unsigned char* data = new unsigned char[width * height * 3];
	unsigned char* row = new unsigned char[rowPadded];

	for (unsigned int y = 0; y < height; y++)
	{
		if (fread(row, 1, rowPadded, file) != (size_t)rowPadded)
		{
			delete[] row;
			delete[] data;
			fclose(file);
			return 0;
		}

		// BMP is bottom-up, so flip vertically
		unsigned int dstY = (height - 1 - y);
		memcpy(data + dstY * width * 3, row, width * 3);
	}

	delete[] row;
	fclose(file);

	GLuint textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] data;
	return textureID;
}



bool init()
{
	
	glEnable(GL_TEXTURE_2D);

	woodTex = loadBMP("models\\WoodTable.bmp");
	if (!woodTex) return false;

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	// setup lighting
	glEnable(GL_LIGHTING);									// --- DEPRECATED
	glEnable(GL_LIGHT0);									// --- DEPRECATED

	// ---------- POINT LIGHT (LAMP) : GL_LIGHT1 ----------
	glEnable(GL_LIGHT1);

	// Point light colors
	GLfloat pAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };  // usually 0 for point lamps
	GLfloat pDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };  // lamp brightness
	GLfloat pSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };  // shiny highlights

	glLightfv(GL_LIGHT1, GL_AMBIENT, pAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, pDiffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, pSpecular);

	// Attenuation (so it fades with distance)
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.08f);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.02f);

	// ---------- SECOND POINT LIGHT (LAMP 2) : GL_LIGHT2 ----------
	glEnable(GL_LIGHT2);

	glLightfv(GL_LIGHT2, GL_AMBIENT, pAmbient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, pDiffuse);
	glLightfv(GL_LIGHT2, GL_SPECULAR, pSpecular);

	glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
	glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.08f);
	glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.02f);


	// ---------- AMBIENT LIGHT ----------
	GLfloat ambientLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };  // soft ambient
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

	// ---------- DIRECTIONAL DIFFUSE LIGHT ----------
	GLfloat dirDiffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };   // brightness
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dirDiffuse);

	

	

	// load your 3D models here!
	if (!camera.load("models\\camera.3ds")) return false;
	


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
void drawLampPrimitive(const mat4& view, const vec3& basePos, float yawDeg, vec3& outBulbWorldPos)
{
	mat4 m;

	// ---- metal ----
	GLfloat metal[] = { 0.75f, 0.7f, 0.3f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, metal);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, metal);

	// Build a MODEL matrix (world space) so we can extract bulb position correctly
	mat4 baseModel = mat4(1.0f);
	baseModel = translate(baseModel, basePos);
	baseModel = rotate(baseModel, radians(yawDeg), vec3(0, 1, 0));

	// View-space version for drawing (same as your old approach)
	mat4 baseM = view * baseModel;

	// ---- BASE (flat on table) ----
	mat4 baseDrawM = baseM;
	baseDrawM = rotate(baseDrawM, radians(-90.f), vec3(1, 0, 0));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&baseDrawM);
	glutSolidCylinder(0.5, 0.05, 32, 2);

	// ---- GOOSENECK ----
	const int segments = 20;
	const float segLen = 0.09f;
	const float radius = 0.045f;
	const float bend = 4.0f;

	// View-space stem for drawing
	mat4 stem = baseM;
	stem = translate(stem, vec3(0.0f, 0.05f, 0.0f));

	// World-space stem for computing bulb position
	mat4 stemModel = baseModel;
	stemModel = translate(stemModel, vec3(0.0f, 0.05f, 0.0f));

	for (int i = 0; i < segments; i++)
	{
		stem = rotate(stem, radians(-bend), vec3(1, 0, 0));
		stemModel = rotate(stemModel, radians(-bend), vec3(1, 0, 0));

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&stem);
		glutSolidCylinder(radius, segLen, 16, 1);

		stem = translate(stem, vec3(0, segLen, 0));
		stemModel = translate(stemModel, vec3(0, segLen, 0));
	}

	// ---- SOCKET ----
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&stem);
	glutSolidCylinder(0.09, 0.1, 16, 1);

	// ---- BULB ----
	GLfloat bulbOff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bulbOff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bulbOff);

	// View-space bulb draw matrix
	m = translate(stem, vec3(0, 0.14f, 0));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&m);
	glutSolidSphere(0.14, 24, 24);

	// World-space bulb position (THIS is what we need for glLightfv)
	mat4 bulbModel = translate(stemModel, vec3(0, 0.14f, 0));
	outBulbWorldPos = vec3(bulbModel[3]); // translation column
}


vec3 bulbPosSimple(const vec3& lampBase)
{
	// lampBase is the base disk center sitting on the table
	// Adjust these until the glowing sphere sits inside your drawn bulb
	return lampBase + vec3(0.0f, 1.85f, -0.65f);
}


void drawTexturedBox(const mat4& modelView, GLuint tex, float tileU, float tileV)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf((GLfloat*)&modelView);

	glBegin(GL_QUADS);

	// TOP (+Y)
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0);        glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(tileU, 0);    glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(tileU, tileV); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0, tileV);    glVertex3f(-0.5f, 0.5f, 0.5f);

	// BOTTOM (-Y)
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0);        glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(tileU, 0);    glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(tileU, tileV); glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(0, tileV);    glVertex3f(-0.5f, -0.5f, -0.5f);

	// FRONT (+Z)
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);        glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(tileU, 0);    glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(tileU, tileV); glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(0, tileV);    glVertex3f(-0.5f, 0.5f, 0.5f);

	// BACK (-Z)
	glNormal3f(0, 0, -1);
	glTexCoord2f(0, 0);        glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(tileU, 0);    glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(tileU, tileV); glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(0, tileV);    glVertex3f(0.5f, 0.5f, -0.5f);

	// RIGHT (+X)
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 0);        glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(tileU, 0);    glVertex3f(0.5f, -0.5f, -0.5f);
	glTexCoord2f(tileU, tileV); glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0, tileV);    glVertex3f(0.5f, 0.5f, 0.5f);

	// LEFT (-X)
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 0);        glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(tileU, 0);    glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(tileU, tileV); glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(0, tileV);    glVertex3f(-0.5f, 0.5f, -0.5f);

	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}
void drawChair(const mat4& view, const vec3& chairPos, float yawDeg, GLuint tex)
{
	// Chair sizes 
	const float seatW = 2.4f;
	const float seatH = 0.15f;
	const float seatD = 2.4f;

	const float legW = 0.12f;
	const float legH = 2.0f;   // chair height
	const float legD = 0.12f;

	const float backW = seatW;
	const float backH = 3.2f;
	const float backD = 0.15f;

	// Base transform (world -> view)
	mat4 base = view;
	base = glm::translate(base, chairPos);
	base = glm::rotate(base, glm::radians(yawDeg), vec3(0, 1, 0));

	// ---- SEAT ----
	mat4 seatM = base;
	seatM = glm::translate(seatM, vec3(0.0f, legH, 0.0f));
	seatM = glm::scale(seatM, vec3(seatW, seatH, seatD));
	drawTexturedBox(seatM, tex, 2.0f, 2.0f);

	// ---- BACKREST ----
	mat4 backM = base;
	backM = glm::translate(backM, vec3(0.0f, legH + backH * 0.5f, -seatD * 0.5f + backD * 0.5f));
	backM = glm::scale(backM, vec3(backW, backH, backD));
	drawTexturedBox(backM, tex, 2.0f, 2.0f);

	// ---- LEGS (4) ----
	vec3 legOff[4] =
	{
		vec3(seatW * 0.5f - legW * 0.5f, legH * 0.5f,  seatD * 0.5f - legD * 0.5f),
		vec3(-seatW * 0.5f + legW * 0.5f, legH * 0.5f,  seatD * 0.5f - legD * 0.5f),
		vec3(seatW * 0.5f - legW * 0.5f, legH * 0.5f, -seatD * 0.5f + legD * 0.5f),
		vec3(-seatW * 0.5f + legW * 0.5f, legH * 0.5f, -seatD * 0.5f + legD * 0.5f)
	};

	for (int i = 0; i < 4; i++)
	{
		mat4 legM = base;
		legM = glm::translate(legM, vec3(legOff[i].x, legOff[i].y, legOff[i].z));
		legM = glm::scale(legM, vec3(legW, legH, legD));
		drawTexturedBox(legM, tex, 1.0f, 3.0f);
	}
}

void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;

		// ===== TEST MODE: only the lamp point light =====
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);


		vec3 tablePos = vec3(10.0f, 0.0f, 0.0f);
		float tableTopY = 1.0f;

		vec3 lamp1Base = tablePos + vec3(-4.8f, tableTopY + 0.25f, -2.0f);
		vec3 lamp2Base = tablePos + vec3(4.8f, tableTopY + 0.25f, 2.5f);

		vec3 bulb1World = bulbPosSimple(lamp1Base);
		vec3 bulb2World = bulbPosSimple(lamp2Base);

		// Load view into OpenGL BEFORE setting light positions
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&matrixView);

		// Point light at bulb 1
		GLfloat pPos[] = { bulb1World.x, bulb1World.y, bulb1World.z, 1.0f };
		glLightfv(GL_LIGHT1, GL_POSITION, pPos);

		// Point light at bulb 2
		GLfloat pPos2[] = { bulb2World.x, bulb2World.y, bulb2World.z, 1.0f };
		glLightfv(GL_LIGHT2, GL_POSITION, pPos2);

		// Debug: draw glowing markers where bulbs are
		GLfloat emiss[] = { 1,1,1,1 };
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);

		mat4 dbg = matrixView;
		dbg = translate(dbg, bulb1World + vec3(0.6f, -0.5f, -0.5f)); 
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&dbg);
		glutSolidSphere(0.12, 16, 16);

		mat4 dbg2 = matrixView;
		dbg2 = translate(dbg2, bulb2World + vec3(-0.5f, -0.5f, -0.5f));
		glLoadIdentity();
		glMultMatrixf((GLfloat*)&dbg2);
		glutSolidSphere(0.12, 16, 16);

	GLfloat noEmiss[] = { 0,0,0,1 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmiss);

	// ---------- CAMERA MODEL ----------
	GLfloat rgbaGrey[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rgbaGrey);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rgbaGrey);

	m = matrixView;
	m = translate(m, vec3(-3.0f, 0.0f, 0.0f));
	m = rotate(m, radians(180.f), vec3(0.0f, 1.0f, 0.0f));
	m = scale(m, vec3(0.04f));
	camera.render(m);

	// ---------- TABLE (TEXTURED BOX) ----------
	mat4 tableM = matrixView;
	tableM = translate(tableM, tablePos + vec3(0.0f, tableTopY, 0.0f));
	tableM = scale(tableM, vec3(12.0f, 0.4f, 7.0f));

	// Repeat texture across the top
	drawTexturedBox(tableM, woodTex, 4.0f, 2.0f);


	// ---------- TABLE LEGS (TEXTURED BOXES) ----------
	
	vec3 legs[4] = {
	vec3(5.4f, -1.2f,  3.2f),
	vec3(-5.4f, -1.2f,  3.2f),
	vec3(5.4f, -1.2f, -3.2f),
	vec3(-5.4f, -1.2f, -3.2f)
	};

	for (int i = 0; i < 4; i++)
	{
		mat4 legM = matrixView;
		legM = translate(legM, tablePos + vec3(0.0f, tableTopY, 0.0f) + legs[i]);
		legM = scale(legM, vec3(0.3f, 2.4f, 0.3f));

		// More vertical tiling so it doesn't stretch
		drawTexturedBox(legM, woodTex, 1.0f, 3.0f);
	}

	// ---------- CHAIRS ----------
	float chairY = -2.0f; // floor level for chair position (legs go down from seat)
	float chairOffset = 4.8f; // distance from table center

	// Front chair (facing table)
	drawChair(matrixView, tablePos + vec3(0.0f, chairY, chairOffset), 180.0f, woodTex);

	// Back chair
	drawChair(matrixView, tablePos + vec3(0.0f, chairY, -chairOffset), 0.0f, woodTex);

	// Left chair
	drawChair(matrixView, tablePos + vec3(-7.3f, chairY, 0.0f), 90.0f, woodTex);

	// Right chair
	drawChair(matrixView, tablePos + vec3(7.3f, chairY, 0.0f), -90.0f, woodTex);

	// ---------- DRAW THE LAMPS (visual only) ----------
	vec3 dummy1(0), dummy2(0);
	drawLampPrimitive(matrixView, lamp1Base, -25.f, dummy1);
	drawLampPrimitive(matrixView, lamp2Base, 25.f, dummy2);

	// ---------- TEAPOT (make it shiny so specular shows) ----------
	GLfloat rgbaBlue[] = { 0.2f, 0.2f, 0.8f, 1.0f };
	GLfloat specWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rgbaBlue);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rgbaBlue);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specWhite);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);

	m = matrixView;
	m = translate(m, tablePos + vec3(4.0f, tableTopY + 0.6f, 1.0f));
	m = rotate(m, radians(120.f), vec3(0.0f, 1.0f, 0.0f));
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

