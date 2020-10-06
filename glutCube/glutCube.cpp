// The OpenGL sample is from https://www.opengl.org/archives/resources/code/samples/glut_examples/examples/cube.c

#include "OpenXRGL.h"

// OpenGL related Headers
#include <GL/glew.h>
#include <GL/glut.h>

// link lib
#pragma comment(lib,"freeglut.lib")
#pragma comment(lib,"glew32.lib")

#pragma region OpenGL render related data
GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
	{-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
	{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
	{0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
	{4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */
#pragma endregion

COpenXRGL gXRGL;


void drawBox(void)
{
	for (int i = 0; i < 6; i++) {
		glBegin(GL_QUADS);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

void display(void)
{
	gXRGL.processEvent();
	gXRGL.draw([](const COpenXRGL::TMatrix& matProj, const COpenXRGL::TMatrix& matModelView) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Setup the view of the cube. */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(matProj.data());

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(matModelView.data());

		//glTranslatef(0.0, 0.0, -1.0);
		//glRotatef(60, 1.0, 0.0, 0.0);
		//glRotatef(-20, 0.0, 0.0, 1.0);

		drawBox();
	});
	glutSwapBuffers();
}

void initGL(void)
{
	/* Setup cube vertex data. */
	v[0][0] = v[1][0] = v[2][0] = v[3][0] = -0.1;
	v[4][0] = v[5][0] = v[6][0] = v[7][0] = 0.1;
	v[0][1] = v[1][1] = v[4][1] = v[5][1] = -0.1;
	v[2][1] = v[3][1] = v[6][1] = v[7][1] = 0.1;
	v[0][2] = v[3][2] = v[4][2] = v[7][2] = 0.1;
	v[1][2] = v[2][2] = v[5][2] = v[6][2] = -0.1;

	/* Enable a single OpenGL light. */
	{
		GLfloat light_diffuse[] = { 1.0, 0.0, 0.0, 1.0 };  /* Red diffuse light. */
		GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };  /* Infinite light location. */
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	}
	{
		GLfloat light_diffuse[] = { 0.0, 1.0, 0.0, 1.0 };  /* Red diffuse light. */
		GLfloat light_position[] = { -1.0, 1.0, -1.0, 0.0 };  /* Infinite light location. */
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, light_position);
	}
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);

	/* Use depth buffering for hidden surface elimination. */
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}

int main(int argc, char** argv)
{
	#pragma region Initialize OpenGL
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("OpenXR + glut Cube");
	glewInit();
	glutDisplayFunc(display);
	glutIdleFunc([]() {glutPostRedisplay(); });
	initGL();
	#pragma endregion

	gXRGL.init();

	glutMainLoop();
	return 0;
}
