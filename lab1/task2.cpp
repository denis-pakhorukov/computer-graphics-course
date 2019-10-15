#include <GL/glut.h>

GLfloat torusDY = 0.0,
        torusDYIncrement = 0.01,
        sphereScale = 0.0,
        sphereScaleIncrement = 0.01;


void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.3, 0.8, 0.3);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(-1.5, 0.0 + torusDY, -4.5);
    glutWireTorus(0.3, 1.0, 32, 32);

    glLoadIdentity();
    glTranslatef(1.5, 0.0, -4.5);
    glScalef(sphereScale, sphereScale, sphereScale);
    glutWireSphere(0.6, 32, 32);

    glFlush();
    glutSwapBuffers();
}

void reshapeFunc(int x, int y) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, (GLdouble) x / y, 0.5, 20.0);
    glViewport(0, 0, x, y);
}

void idleFunc() {
    if ((torusDY > 1 && torusDYIncrement > 0) || (torusDY < -1 && torusDYIncrement < 0)) {
        torusDYIncrement = -torusDYIncrement;
    }
    if ((sphereScale > 2 && sphereScaleIncrement > 0) || (sphereScale < 0 && sphereScaleIncrement < 0)) {
        sphereScaleIncrement = -sphereScaleIncrement;
    }
    torusDY += torusDYIncrement;
    sphereScale += sphereScaleIncrement;
    glutPostRedisplay();
}


int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 400);
    glutCreateWindow("Torus rotation");
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glutDisplayFunc(display);
    glutReshapeFunc(reshapeFunc);
    glutIdleFunc(idleFunc);

    glutMainLoop();
    return 0;
}
