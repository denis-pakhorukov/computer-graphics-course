#include <GL/gl.h>
#include <GL/glut.h>
#include <stdexcept>

GLfloat lightSpecular = 1.0,
        lightAttenuation = 0.2,

        lightColor[] = {1.0, 1.0, 0.0},
        lightColorIncrement[] = {0.0, 0.01, 0.0},

        lightAngle = 0.0,
        lightAngleIncrement = -0.8,

        lightPosition[] = {2.0, 3.0, 2.0, 1.0},

        objectAlpha = 0.4;

GLuint texture[1];         // Space For One Texture

static void reverseBytes(void *x, const int n) {
    char *ch = (char *) x;
    for (int k = 0; k < n / 2; k++) {
        char tmp = ch[k];
        ch[k] = ch[n - 1 - k];
        ch[n - 1 - k] = tmp;
    }
}

unsigned int loadBmpTexture(const std::string &filename) {
    unsigned int texture; /* Texture name */
    unsigned short magic; /* Image magic */
    /* Image dimensions */
    unsigned int dx;
    unsigned int dy;
    unsigned short nbp;
    unsigned short bpp; /* Planes and bits per pixel */
    unsigned int k;      /* Counter */

    /*  Open file */
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f)
        throw std::logic_error("Cannot open file");

    /*  Check image magic */
    if (fread(&magic, 2, 1, f) != 1)
        throw std::logic_error("Cannot read magic");

    if (magic != 0x4D42 && magic != 0x424D)
        throw std::logic_error("Image magic not BMP");

    //  Seek to and read header
    if (fseek(f, 16, SEEK_CUR) || fread(&dx, 4, 1, f) != 1 || fread(&dy, 4, 1, f) != 1 ||
        fread(&nbp, 2, 1, f) != 1 || fread(&bpp, 2, 1, f) != 1 || fread(&k, 4, 1, f) != 1)
        throw std::logic_error("Cannot read header");

    //  Reverse bytes on big endian hardware (detected by backwards magic)
    if (magic == 0x424D) {
        reverseBytes(&dx, 4);
        reverseBytes(&dy, 4);
        reverseBytes(&nbp, 2);
        reverseBytes(&bpp, 2);
        reverseBytes(&k, 4);
    }

    if (dx < 1 || dx > 65536)
        throw std::logic_error("image width out of range\n");

    if (dy < 1 || dy > 65536)
        throw std::logic_error("image width out of range\n");

    if (nbp != 1)
        throw std::logic_error("bit planes is not 1\n");

    if (bpp != 24)
        throw std::logic_error("bits per pixel is not 24\n");

    if (k != 0)
        throw std::logic_error("compressed files not supported\n");

    /*  Allocate image memory */
    unsigned int size = 3 * dx * dy;
    unsigned char *image = (unsigned char *) malloc(size);
    if (!image)
        throw std::logic_error("Cannot allocate %d bytes of memory for image\n");

    /*  Seek to and read image */
    if (fseek(f, 20, SEEK_CUR) || fread(image, size, 1, f) != 1)
        throw std::logic_error("Error reading data from image\n");

    fclose(f);

    /*  Reverse colors (BGR -> RGB) */
    for (k = 0; k < size; k += 3) {
        unsigned char temp = image[k];
        image[k] = image[k + 2];
        image[k + 2] = temp;
    }

    /*  Generate 2D texture */
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    /*  Scale linearly when image size doesn't match */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    /*  Copy image */
    glTexImage2D(GL_TEXTURE_2D, 0, 3, dx, dy, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    if (glGetError())
        throw std::logic_error("Error in glTexImage2D\n");

    /*  Free image memory */
    free(image);
    /*  Return texture name */
    return texture;
}

void drawAxes() {
    glPushMatrix();
    GLfloat axis_length = 3.0;
    glColor3d(1.0, 1.0, 1.0);

    //............ X axis ..............
    glBegin(GL_LINES);
    glVertex3d(0., 0., 0.);
    glVertex3d(axis_length, 0., 0.);
    glEnd();

    glRasterPos3d(axis_length, 0.0, 0.0);

    // ........... Y axis ..............
    glBegin(GL_LINES);
    glVertex3d(0., 0., 0.);
    glVertex3d(0., axis_length, 0.);
    glEnd();

    glRasterPos3d(0.0, axis_length, 0.0);

    // ........... Z axis .............
    glBegin(GL_LINES);
    glVertex3d(0., 0., 0.);
    glVertex3d(0., 0., axis_length);
    glEnd();

    glRasterPos3d(0.0, 0.0, axis_length);

    glPopMatrix();
}

void drawGlowworm() {
    glPushMatrix();
    glutSolidSphere(0.2, 128, 128);
    glPopMatrix();
}

void drawCube() {
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, texture[0]);

    glBegin(GL_QUADS);
    // Front Face
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    // Back Face
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    // Top Face
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    // Bottom Face
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    // Right face
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    // Left Face
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glEnd();
    glPopMatrix();
}

void drawCone() {
    glPushMatrix();
    GLfloat color[] = {1.0, 0.5, 0.5, objectAlpha};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    glColor4f(1.0, 1.0, 1.0, objectAlpha);
    glRotated(-90, 1.0, 0.0, 0.0);
    glutSolidCone(1.5, 1, 128, 128);
    glPopMatrix();
}

void drawTorus() {
    glPushMatrix();
    glScalef(2, 2, 2);

    GLfloat mat_diffuse[] = { 0.2, 0.5, 0.8, 1.0 };
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat high_shininess[] = { 100.0 };

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, high_shininess);
    glutSolidTorus(0.1, 0.2, 32, 32);
    glPopMatrix();
}

void drawLight() {
    GLfloat specular[] = {lightSpecular, lightSpecular, lightSpecular, 1.0};
    GLfloat diffuse[] = {lightColor[0], lightColor[1], lightColor[2]};

    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, lightAttenuation);
    //glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.1);
    //glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0);

    // двухсторонний расчет освещения
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40, 1, 4, 20);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(5.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

//    glTranslatef(0.0, 0.0, 0.0);
//    drawAxes();

    glPushMatrix();
    glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
    glRotatef(lightAngle, 0, 1, 0);
    glTranslatef(1.5f, 0.0f, 0.0f);
    glColor3fv(lightColor);
    drawGlowworm();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
    glRotatef(lightAngle, 0, 1, 0);
    glTranslatef(1.5f, 0.0f, 0.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    drawLight();
    glPopMatrix();

    glColor3f(1.0, 1.0, 1.0);

    glTranslatef(0.0, 0.0, 0.0);
    drawCube();

    glTranslatef(0, 1.1, 0);
    drawCone();

    glTranslatef(1.25, 0.0, 2.75);
    drawTorus();

    //GLfloat mat_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
    GLfloat mat_ambient_color[] = {0.8, 0.8, 0.2, 1.0};
    GLfloat mat_diffuse[] = {0.2, 0.5, 0.8, 1.0};
    GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat low_shininess[] = {5.0};
    GLfloat high_shininess[] = {100.0};

    glPopMatrix();
    glFlush();
}

void idle() {
    for (int i = 0; i < 3; ++i) {
        if ((lightColor[i] > 1 && lightColorIncrement[i] > 0) || (lightColor[i] <= 0.0 && lightColorIncrement[i] < 0)) {
            lightColorIncrement[i] = -lightColorIncrement[i];
        }
        lightColor[i] += lightColorIncrement[i];
    }

    lightAngle += lightAngleIncrement;

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

    glutCreateWindow("Window Title!");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_NORMALIZE);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);

    texture[0] = loadBmpTexture("bricks.bmp");
    glutMainLoop();
}
