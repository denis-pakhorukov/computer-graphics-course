#include <GL/glut.h>

std::size_t current_iteration = 1;
std::size_t max_iteration = 50;

int spin_x = 50, spin_y = 50;
int old_x = 0, old_y = 0;

float ambient[] = {1.0, 1.0, 1.0, 1};
float lightpos[] = {0.6, 0.5, -1.0};

const std::size_t X_DIM = 7;
const std::size_t Y_DIM = 3;
const std::size_t Z_DIM = 3;

float circle_p[X_DIM][Y_DIM][Z_DIM] = {
        {{0,     0,   0.4},  {0,     0.4, 0.4},  {0,     0.8, 0.4}},
        {{-0.15, 0.0, 0.4},  {-0.15, 0.4, 0.4},  {-0.15, 0.8, 0.4}},
        {{-0.4,  0.0, 0.2},  {-0.4,  0.4, 0.2},  {-0.4,  0.8, 0.2}},
        {{0,     0,   -0.3}, {0,     0.4, -0.3}, {0,     0.8, -0.3}},
        {{0.4,   0,   0.2},  {0.4,   0.4, 0.2},  {0.4,   0.8, 0.2}},
        {{0.15,  0.0, 0.4},  {0.15,  0.4, 0.4},  {0.15,  0.8, 0.4}},
        {{0,     0,   0.4},  {0,     0.4, 0.4},  {0,     0.8, 0.4}}
};
float surface_p[X_DIM][Y_DIM][Z_DIM] = {
        {{0, 0,   0.4},  {0, 0.1, 0.4},  {0, 0.2, 0.4}},
        {{0, 0.0, 0.4},  {0, 0.1, 0.4},  {0, 0.2, 0.4}},
        {{0, 0.0, 0.2},  {0, 0.1, 0.2},  {0, 0.2, 0.2}},
        {{0, 0,   -0.3}, {0, 0.1, -0.3}, {0, 0.2, -0.3}},
        {{0, 0,   -0.2}, {0, 0.1, -0.2}, {0, 0.2, -0.2}},
        {{0, 0.0, -0.4}, {0, 0.1, -0.4}, {0, 0.2, -0.4}},
        {{0, 0,   -0.4}, {0, 0.1, -0.4}, {0, 0.2, -0.4}}
};

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glRotated(spin_x, 1, 0, 0);
    glRotated(spin_y, 0, 1, 0);

    float progress = (float) current_iteration / max_iteration;
    float points[X_DIM][Y_DIM][Z_DIM];
    for (int i = 0; i < X_DIM; i++) {
        for (int j = 0; j < Y_DIM; j++) {
            for (int k = 0; k < Z_DIM; k++) {
                points[i][j][k] = surface_p[i][j][k] + progress * (circle_p[i][j][k] - surface_p[i][j][k]);
            }
        }
    }

    glMap2f(GL_MAP2_VERTEX_3, 0, 1, Y_DIM * Z_DIM, X_DIM, 0, 1, Z_DIM, Y_DIM, &points[0][0][0]);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_COLOR_MATERIAL);
    glPointSize(3);
    glColor3d(1, 1, 1);
    glBegin(GL_POINTS);
    for (auto &line : points) {
        for (auto &point : line) {
            glVertex3fv(point);
        }
    }
    glEnd();
    glDisable(GL_COLOR_MATERIAL);

    float a[] = {0.7, 0.7, 1.0, 0.7};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, a);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glMapGrid2f(100, 0, 1, 1, 0, 1);
    glEvalMesh2(GL_FILL, 0, 100, 0, 1);
    glEnable(GL_AUTO_NORMAL);

    glDisable(GL_BLEND);

    float b[] = {0.2, 0.2, 0.2, 1};
    float c[] = {0, 0.5, 0, 0};
    glMaterialfv(GL_FRONT, GL_AMBIENT, b);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

    glutSwapBuffers();
}

void idle() {
    static int count = 0;
    static int step = +1;

    if (count != 10000) { // slow animation down
        count++;
        return;
    }

    count = 0;

    if ((current_iteration > max_iteration && step > 0) || (current_iteration <= 0 && step < 0)) {
        step = -step;
    }

    current_iteration += step;

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    old_x = x;
    old_y = y;

    glutPostRedisplay();
}

void motion(int x, int y) {
    spin_x = x - old_x;
    spin_y = y - old_y;

    glutPostRedisplay();
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);

    glutInitWindowSize(500, 500);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    glutCreateWindow("Lab 3");
    glutDisplayFunc(display);
    glutIdleFunc(idle);

    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

    glEnable(GL_MAP2_VERTEX_3);

    glutMainLoop();
}