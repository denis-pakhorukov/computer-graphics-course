#include <GL/glut.h>

GLfloat position = 0.0,
        positionIncrement = 0.01,
        lightR = 0.0,
        lightRIncrement = 0.001,
        lightG = 0.0,
        lightGIncrement = 0.005,
        lightB = 0.0,
        lightBIncrement = 0.002;


void setMaterial(GLfloat ambientR, GLfloat ambientG, GLfloat ambientB,
                 GLfloat diffuseR, GLfloat diffuseG, GLfloat diffuseB,
                 GLfloat specularR, GLfloat specularG, GLfloat specularB,
                 GLfloat shininess) {

    GLfloat ambient[] = {ambientR, ambientG, ambientB, 1.0};
    GLfloat diffuse[] = {diffuseR, diffuseG, diffuseB, 1.0};
    GLfloat specular[] = {specularR, specularG, specularB, 1.0};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat ambcolor[] = {0.0, 0.0, 1.0};
    GLfloat lightpos[] = {0.0, position, position, 1.0};
    GLfloat lightcolor[] = {lightR, lightG, lightB};

    /* future matrix manipulations should affect the modelview matrix */
    glMatrixMode(GL_MODELVIEW);

    /* draw scene */
    glPushMatrix();
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambcolor);     // ambient light

    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);          // config light source
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightcolor);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightcolor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightcolor);

    // cube
    glPushMatrix();
    glutSolidCube(2);
    glPopMatrix();

    glTranslatef(0, 1, 0);
    // cone
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    glColor4f(1.0, 1.0, 1.0, 0.5);
    glutSolidCone(1.5, 1, 16, 16);
    glColor4f(0.0, 0.0, 0.0, 0.0);
    glPopMatrix();

    glTranslatef(1.25, 0.0, 2.75);
    // torus
    glPushMatrix();
    glScalef(3, 3, 2);
    setMaterial(1.0, 1.0, 1.0,
                1.0, 1.0, 1.0,
                1.0, 1.0, 1.0,
                128.0);
    glutSolidTorus(0.1, 0.2, 32, 32);
    glPopMatrix();

    setMaterial(0.2, 0.2, 0.2, 0.8, 0.8, 0.8, 0.0, 0.0, 0.0, 0.0);
    glPopMatrix();
    glFlush();
}

void idle() {
    if ((position > 3 && positionIncrement > 0) || (position <= 0 && positionIncrement < 0)) {
        positionIncrement = -positionIncrement;
    }
    if ((lightR > 1 && lightRIncrement > 0) || (lightR <= 0 && lightRIncrement < 0)) {
        lightRIncrement = -lightRIncrement;
    }
    if ((lightG > 1 && lightGIncrement > 0) || (lightG <= 0 && lightGIncrement < 0)) {
        lightGIncrement = -lightGIncrement;
    }
    if ((lightB > 1 && lightBIncrement > 0) || (lightB <= 0 && lightBIncrement < 0)) {
        lightBIncrement = -lightBIncrement;
    }
    position += positionIncrement;
    lightR += lightRIncrement;
    lightG += lightGIncrement;
    lightB += lightBIncrement;

    glutPostRedisplay();
}

void reshape(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, (GLdouble) width / height, 0.5, 20.0);
    glViewport(0, 0, width, height);
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);

    glutInitWindowSize(500, 500);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);

    glutCreateWindow("hello, teapot!");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40, 1, 4, 20);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(5.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glutMainLoop();
}
