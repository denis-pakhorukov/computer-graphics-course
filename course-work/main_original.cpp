#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <GL/glut.h>


#ifdef _WIN32
#define drand48() ((float)rand()/RAND_MAX)
#endif


/* #define SCREEN_SAVER_MODE */

enum SprayType {
    PS_WATERFALL,
    PS_FOUNTAIN
};

typedef struct {
    double x, y, z;
    double radius;
} PSsphere;

typedef struct {
    double position[3];           /* current position */
    double previous[3];           /* previous position */
    double velocity[3];           /* velocity (magnitude & direction) */
    double dampening;             /* % of energy lost on collision */
    bool alive;                   /* is this particle alive? */
} Particle;

const double GRAVITY = -9.8;

const int NUM_SPHERES = 3;
PSsphere spheres[NUM_SPHERES] = {    /* position of spheres */
        {-0.1, 0.6, 0,   0.4},
        {-0.7, 1.4, 0,   0.25},
        {0.1,  1.5, 0.1, 0.3},
};
Particle *particles = nullptr;
int num_particles = 10000;
SprayType type = SprayType::PS_WATERFALL;
int points = 1;
int draw_spheres = 3;
int frame_rate = 1;
float frame_time = 0;
float flow = 500;
float slow_down = 1;

int spin_x = 0;
int spin_y = 0;
int particle_radius = 4;


/* timedelta: returns the number of seconds that have elapsed since
   the previous call to the function. */
#if defined(_WIN32)
#include <sys/timeb.h>
#else

#include <climits>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <cstring>

#endif
#ifndef CLK_TCK
#define CLK_TCK 1000
#endif

float timedelta() {
    static long begin = 0;
    static long finish, difference;

#if defined(_WIN32)
    static struct timeb tb;
    ftime(&tb);
    finish = tb.time*1000+tb.millitm;
#else
    static struct tms tb;
    finish = times(&tb);
#endif

    difference = finish - begin;
    begin = finish;

    return (float) difference / (float) CLK_TCK;
}


/* text: draws a string of text with an 18 point helvetica bitmap font
   at position (x,y) in window space (bottom left corner is (0,0). */
void text(int x, int y, char *s) {
    int lines;
    char *p;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH),
            0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3ub(0, 0, 0);
    glRasterPos2i(x + 1, y - 1);
    for (p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x + 1, y - 1 - (lines * 18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3ub(128, 0, 255);
    glRasterPos2i(x, y);
    for (p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x, y - (lines * 18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_FOG);
    glEnable(GL_DEPTH_TEST);
}


bool fequal(float a, float b) {
    float epsilon = 0.1;
    float f = a - b;
    return f < epsilon && f > -epsilon;
}


void psTimeStep(Particle *p, double dt) {
    if (!p->alive)
        return;

    p->velocity[0] += 0;
    p->velocity[1] += GRAVITY * dt;
    p->velocity[2] += 0;

    p->previous[0] = p->position[0];
    p->previous[1] = p->position[1];
    p->previous[2] = p->position[2];

    p->position[0] += p->velocity[0] * dt;
    p->position[1] += p->velocity[1] * dt;
    p->position[2] += p->velocity[2] * dt;
}


void psNewParticle(Particle *p, float dt) {
    if (type == SprayType::PS_WATERFALL) {
        p->velocity[0] = -2 * (drand48() - 0.0);
        p->velocity[1] = 0;
        p->velocity[2] = 0 * (drand48() - 0.0);
        p->position[0] = 0;
        p->position[1] = 2;
        p->position[2] = 0;
        p->previous[0] = p->position[0];
        p->previous[1] = p->position[1];
        p->previous[2] = p->position[2];
        p->dampening = 0.45 * drand48();
        p->alive = true;
    } else if (type == SprayType::PS_FOUNTAIN) {
        p->velocity[0] = 2 * (drand48() - 0.5);
        p->velocity[1] = 5;
        p->velocity[2] = 2 * (drand48() - 0.5);
        p->position[0] = -0.1;
        p->position[1] = 0.9;
        p->position[2] = 0;
        p->previous[0] = p->position[0];
        p->previous[1] = p->position[1];
        p->previous[2] = p->position[2];
        p->dampening = 0.35 * drand48();
        p->alive = true;
    }

    psTimeStep(p, 2 * dt * drand48());
}


/* psBounce: the particle has gone past (or exactly hit) the ground
   plane, so calculate the time at which the particle actually
   intersected the ground plane (s).  essentially, this just rolls
   back time to when the particle hit the ground plane, then starts
   time again from then.

   -  -   o A  (previous position)
   |  |    \
   |  s     \   o  (position it _should_ be at) -
   t  |      \ /                                | t - s
   |  - ------X--------                         -
   |           \
   -            o B  (new position)

   A + V*s = 0 or s = -A/V

   to calculate where the particle should be:

   A + V*t + V*(t-s)*d

   where d is a damping factor which accounts for the loss
   of energy due to the bounce. */
void psBounce(Particle *p, float dt) {
    if (!p->alive)
        return;

    /* since we know it is the ground plane, we only need to
       calculate s for a single dimension. */
    double s = -p->previous[1] / p->velocity[1];

    p->position[0] = (p->previous[0] + p->velocity[0] * s +
                      p->velocity[0] * (dt - s) * p->dampening);
    p->position[1] = -p->velocity[1] * (dt - s) * p->dampening; /* reflect */
    p->position[2] = (p->previous[2] + p->velocity[2] * s +
                      p->velocity[2] * (dt - s) * p->dampening);

    /* damp the reflected velocity (since the particle hit something,
       it lost some energy) */
    p->velocity[0] *= p->dampening;
    p->velocity[1] *= -p->dampening;        /* reflect */
    p->velocity[2] *= p->dampening;
}

void psCollideSphere(Particle *p, PSsphere *s) {
    if (!p->alive) {
        return;
    }

    double vx = p->position[0] - s->x;
    double vy = p->position[1] - s->y;
    double vz = p->position[2] - s->z;
    double distance = std::sqrt(vx * vx + vy * vy + vz * vz);

    if (distance < s->radius) {
        p->position[0] = s->x + (vx / distance) * s->radius;
        p->position[1] = s->y + (vy / distance) * s->radius;
        p->position[2] = s->z + (vz / distance) * s->radius;
        p->previous[0] = p->position[0];
        p->previous[1] = p->position[1];
        p->previous[2] = p->position[2];
        p->velocity[0] = vx / distance;
        p->velocity[1] = vy / distance;
        p->velocity[2] = vz / distance;
    }
}


void reshape(int width, int height) {
    float black[] = {0, 0, 0, 0};

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (double) width / height, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 1, 3, 0, 1, 0, 0, 1, 0);
    glFogfv(GL_FOG_COLOR, black);
    glFogf(GL_FOG_START, 2.5);
    glFogf(GL_FOG_END, 4);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glPointSize(particle_radius);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);

    timedelta();
}


void display() {
    static int i;
    static double c;
    static int j = 0;
    static char s[32];
    static int frames = 0;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    glRotatef(spin_y, 1, 0, 0);
    glRotatef(spin_x, 0, 1, 0);

    glEnable(GL_LIGHTING);
    if (draw_spheres) {
        for (i = 0; i < draw_spheres; i++) {
            glPushMatrix();
            glTranslatef(spheres[i].x, spheres[i].y, spheres[i].z);
            glColor3ub(0, 255, 128);
            glutSolidSphere(spheres[i].radius, 16, 16);
            glPopMatrix();
        }
    }
    glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    glColor3ub(128, 128, 255);
    glVertex3f(-2, 0, -2);
    glVertex3f(-2, 0, 2);
    glVertex3f(2, 0, 2);
    glVertex3f(2, 0, -2);
    glEnd();

    if (points) {
        glBegin(GL_POINTS);

        for (i = 0; i < num_particles; i++) {
            if (particles[i].alive == 0)
                continue;
            c = particles[i].position[1] / 2.1 * 255;
            glColor3ub(c, 128 + c * 0.5, 255);
            glVertex3dv(particles[i].position);
        }
        glEnd();
    } else {
        glBegin(GL_LINES);
        for (i = 0; i < num_particles; i++) {
            if (particles[i].alive == 0)
                continue;
            c = particles[i].previous[1] / 2.1 * 255;
            glColor4ub(c, 128 + c * 0.5, 255, 32);
            glVertex3dv(particles[i].previous);
            c = particles[i].position[1] / 2.1 * 255;
            glColor4ub(c, 128 + c * 0.5, 255, 196);
            glVertex3dv(particles[i].position);
        }
        glEnd();
    }

    /* spit out frame rate. */
    if (frame_rate) {
        frames++;
        if (frames > 7) {
            sprintf(s, "%g fps", (float) 7 / frame_time);
            frame_time = 0;
            frames = 0;
        }
        text(5, 5, s);
    }

    glPopMatrix();
    glutSwapBuffers();
}

void idle() {
    static int i, j;
    static int living = 0;        /* index to end of live particles */
    static float dt;
    static float last = 0;

    dt = timedelta();
    frame_time += dt;

#if 1
    /* slow the simulation if we can't keep the frame rate up around
       10 fps */
    if (dt > 0.1) {
        slow_down = 1.0 / (100 * dt);
    } else if (dt < 0.1) {
        slow_down = 1;
    }
#endif

    dt *= slow_down;

    /* resurrect a few particles */
    for (i = 0; i < flow * dt; i++) {
        psNewParticle(&particles[living], dt);
        living++;
        if (living >= num_particles)
            living = 0;
    }

    for (i = 0; i < num_particles; i++) {
        psTimeStep(&particles[i], dt);

        /* collision with sphere? */
        if (draw_spheres) {
            for (j = 0; j < draw_spheres; j++) {
                psCollideSphere(&particles[i], &spheres[j]);
            }
        }

        /* collision with ground? */
        if (particles[i].position[1] <= 0) {
            psBounce(&particles[i], dt);
        }

        /* dead particle? */
        if (particles[i].position[1] < 0.1 &&
            fequal(particles[i].velocity[1], 0)) {
            particles[i].alive = 0;
        }
    }

    glutPostRedisplay();
}

void visible(int state) {
    if (state == GLUT_VISIBLE)
        glutIdleFunc(idle);
    else
        glutIdleFunc(nullptr);
}

void bail(int code) {
    free(particles);
    exit(code);
}

#ifdef SCREEN_SAVER_MODE
void
ss_keyboard(char key, int x, int y)
{
    bail(0);
}

void
ss_mouse(int button, int state, int x, int y)
{
    bail(0);
}

void
ss_passive(int x, int y)
{
    static int been_here = 0;

    /* for some reason, GLUT sends an initial passive motion callback
       when a window is initialized, so this would immediately
       terminate the program.  to get around this, see if we've been
       here before. (actually if we've been here twice.) */

    if (been_here > 1)
    bail(0);
    been_here++;
}

#else

void keyboard(unsigned char key, int x, int y) {
    static int fullscreen = 0;
    static int old_x = 50;
    static int old_y = 50;
    static int old_width = 512;
    static int old_height = 512;
    static int s = 0;

    switch (key) {
        case 27:
            bail(0);
            break;

        case 't':
            if (type == PS_WATERFALL)
                type = PS_FOUNTAIN;
            else if (type == PS_FOUNTAIN)
                type = PS_WATERFALL;
            break;

        case 's':
            draw_spheres++;
            if (draw_spheres > NUM_SPHERES)
                draw_spheres = 0;
            break;

        case 'S':
            printf("PSsphere spheres[NUM_SPHERES] = {/* position of spheres */\n");
            for (s = 0; s < NUM_SPHERES; s++) {
                printf("  { %g, %g, %g, %g },\n",
                       spheres[s].x, spheres[s].y,
                       spheres[s].z, spheres[s].radius);
            }
            printf("};\n");
            break;

        case 'l':
            points = !points;
            break;

        case 'P':
            particle_radius++;
            glPointSize(particle_radius);
            break;

        case 'p':
            particle_radius--;
            if (particle_radius < 1)
                particle_radius = 1;
            glPointSize(particle_radius);
            break;

        case '+':
            flow += 100;
            if (flow > num_particles)
                flow = num_particles;
            printf("%g particles/second\n", flow);
            break;

        case '-':
            flow -= 100;
            if (flow < 0)
                flow = 0;
            printf("%g particles/second\n", flow);
            break;

        case '#':
            frame_rate = !frame_rate;
            break;

        case '~':
            fullscreen = !fullscreen;
            if (fullscreen) {
                old_x = glutGet(GLUT_WINDOW_X);
                old_y = glutGet(GLUT_WINDOW_Y);
                old_width = glutGet(GLUT_WINDOW_WIDTH);
                old_height = glutGet(GLUT_WINDOW_HEIGHT);
                glutFullScreen();
            } else {
                glutReshapeWindow(old_width, old_height);
                glutPositionWindow(old_x, old_y);
            }
            break;

        case '!':
            s++;
            if (s >= NUM_SPHERES)
                s = 0;
            break;

        case '4':
            spheres[s].x -= 0.05;
            break;

        case '6':
            spheres[s].x += 0.05;
            break;

        case '2':
            spheres[s].y -= 0.05;
            break;

        case '8':
            spheres[s].y += 0.05;
            break;

        case '7':
            spheres[s].z -= 0.05;
            break;

        case '3':
            spheres[s].z += 0.05;
            break;

        case '9':
            spheres[s].radius += 0.05;
            break;

        case '1':
            spheres[s].radius -= 0.05;
            break;

    }
}

void menu(int item) {
    keyboard((unsigned char) item, 0, 0);
}

void menustate(int state) {
    /* hook up a fake time delta to avoid jumping when menu comes up */
    if (state == GLUT_MENU_NOT_IN_USE)
        timedelta();
}

#endif

int old_x, old_y;

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

int main(int argc, char **argv) {
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(640, 480);
    glutInit(&argc, argv);

    glutCreateWindow("Particles");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
#ifdef SCREEN_SAVER_MODE
    glutPassiveMotionFunc(ss_passive);
    glutKeyboardFunc(ss_keyboard);
    glutMouseFunc(ss_mouse);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutFullScreen();
#else
    glutMotionFunc(motion);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
#endif

    glutMenuStateFunc(menustate);
    glutCreateMenu(menu);
    glutAddMenuEntry("Particle", 0);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[f]   Fog on/off", 'f');
    glutAddMenuEntry("[t]   Spray type", 't');
    glutAddMenuEntry("[s]   Collision spheres", 's');
    glutAddMenuEntry("[-]   Less flow", '-');
    glutAddMenuEntry("[+]   More flow", '+');
    glutAddMenuEntry("[p]   Smaller points", 'p');
    glutAddMenuEntry("[P]   Larger points", 'P');
    glutAddMenuEntry("[l]   Toggle points/lines", 'l');
    glutAddMenuEntry("[#]   Toggle framerate on/off", '#');
    glutAddMenuEntry("[~]   Toggle fullscreen on/off", '~');
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("Use the numeric keypad to move the spheres", 0);
    glutAddMenuEntry("[!]   Change active sphere", 0);
    glutAddMenuEntry("", 0);
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            fprintf(stderr, "%s [particles] [flow] [speed%]\n", argv[0]);
            exit(0);
        }
        sscanf(argv[1], "%d", &num_particles);
        if (argc > 2)
            sscanf(argv[2], "%f", &flow);
        if (argc > 3)
            sscanf(argv[3], "%f", &slow_down);
    }

    particles = (Particle *) malloc(sizeof(Particle) * num_particles);

    glutVisibilityFunc(visible);
    glutMainLoop();
    return 0;
}
