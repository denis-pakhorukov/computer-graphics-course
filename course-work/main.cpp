#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <GL/glut.h>


#ifdef _WIN32
#define drand48() ((double)rand()/RAND_MAX)
#endif


/* #define SCREEN_SAVER_MODE */

typedef struct {
    double position[3];           /* current position */
    double previous[3];           /* previous position */
    double velocity[3];           /* velocity (magnitude & direction) */
    double dampening;             /* % of energy lost on collision */
    bool alive;                   /* is this particle alive? */
} Particle;

const double GRAVITY = -9.8;

Particle *particles = nullptr;
int num_particles = 100;
double particle_radius = 0.2;
int points = 1;
int draw_spheres = 3;
int frame_rate = 1;
double frame_time = 0;
double flow = 100;
double slow_down = 1;
double time_factor = 5; // how fast time goes

float light_position[] = {-1.0, 2.0, 0.0, 1.0};
float lightSpecular = 0.5;
float light_attenuation = 0.5;
unsigned int texture[1];         // Space For One Texture

int spin_x = 0;
int spin_y = 0;


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
#include <stdexcept>

#endif
#ifndef CLK_TCK
#define CLK_TCK 1000
#endif

double timedelta() {
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

    return (double) difference / (double) CLK_TCK;
}

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


void timeStep(Particle *p, double dt) {
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


void initializeParticle(Particle *p, double dt) {
    p->velocity[0] = 3 * (drand48() - 0.0);
    p->velocity[1] = 5 * (1);
    p->velocity[2] = 1 * (drand48() - 0.5);
    p->position[0] = -2;
    p->position[1] = 0.3;
    p->position[2] = 0;
    p->previous[0] = p->position[0];
    p->previous[1] = p->position[1];
    p->previous[2] = p->position[2];
    p->dampening = 0.35 + drand48();
    p->alive = true;
    timeStep(p, 2 * dt * drand48());
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
void bounce(Particle *p, double dt) {
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

void collideSphere(Particle *p, Particle *s) {
    if (!p->alive) {
        return;
    }

    double vx = p->position[0] - s->position[0];
    double vy = p->position[1] - s->position[1];
    double vz = p->position[2] - s->position[2];
    double distance = std::sqrt(vx * vx + vy * vy + vz * vz);

    if (distance < particle_radius) {
        p->position[0] += (vx / distance) * particle_radius;
        p->position[1] += (vy / distance) * particle_radius;
        p->position[2] += (vz / distance) * particle_radius;
        p->previous[0] = p->position[0];
        p->previous[1] = p->position[1];
        p->previous[2] = p->position[2];
        p->velocity[0] = vx / distance + 1;
        p->velocity[1] = vy / distance + 1;
        p->velocity[2] = vz / distance + 1;
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
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_TEXTURE_2D);

    timedelta();
}


void display() {
    static int i;
    static int frames = 0;
    static char framerate[32];

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    glRotatef(spin_y, 1, 0, 0);
    glRotatef(spin_x, 0, 1, 0);

    glDisable(GL_LIGHTING);

    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glBegin(GL_QUADS);
    glColor3ub(255, 255, 255);

    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-3, -particle_radius, -3);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-3, -particle_radius, 3);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(3, -particle_radius, 3);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(3, -particle_radius, -3);

    glEnd();
    glPopMatrix();

    glEnable(GL_LIGHTING);

    GLfloat specular[] = {lightSpecular, lightSpecular, lightSpecular, 1.0};
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, light_attenuation);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    glBegin(GL_POINTS);
    double c = 0;
    for (i = 0; i < num_particles; i++) {
        if (!particles[i].alive)
            continue;
        c = std::abs(particles[i].position[1]) / 2.1 * 255;
        c = c <= 255 ? c : 255;
        glColor3ub(100 + 0.5 * c, 0.7 * c, 0);
        // glVertex3dv(particles[i].position);
        glPushMatrix();
        glRotatef(spin_y, 1, 0, 0);
        glRotatef(spin_x, 0, 1, 0);
        glTranslatef(particles[i].position[0], particles[i].position[1], particles[i].position[2]);
        glutSolidSphere(particle_radius, 16, 16);
        glPopMatrix();
    }
    glEnd();

    /* spit out frame rate. */
    if (frame_rate) {
        frames++;
        if (frames > 7) {
            sprintf(framerate, "%g fps", 7.0 / frame_time);
            frame_time = 0;
            frames = 0;
        }
        text(5, 5, framerate);
    }

    glPopMatrix();
    glutSwapBuffers();
}

void idle() {
    static int living = 0;        /* index to end of live particles */

    double dt = timedelta();
    frame_time += dt;

    /* slow the simulation if we can't keep the frame rate up around
       10 fps */
    if (dt > 0.1) {
        slow_down = 1.0 / (100 * dt);
    } else if (dt < 0.1) {
        slow_down = 1;
    }

    dt *= slow_down;

    // init new particles
    static int idle_frames = 0;
    idle_frames++;
    if (idle_frames >= flow) {
        idle_frames = 0;
        initializeParticle(&particles[living], dt);
        living++;
        if (living >= num_particles)
            living = 0;
    }

    for (int i = 0; i < num_particles; i++) {
        timeStep(&particles[i], time_factor * dt);

        /* collision with ground? */
        if (particles[i].position[1] <= 0) {
            bounce(&particles[i], dt);
        }

        /* collision with sphere? */
        if (draw_spheres) {
            for (int j = 0; j < num_particles; j++) {
                if (i == j) {
                    continue;
                }
                collideSphere(&particles[i], &particles[j]);
            }
        }

        /* dead particle? */
//        if (particles[i].position[1] < 0.1 &&
//            fequal(particles[i].velocity[1], 0)) {
//            particles[i].alive = false;
//        }
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

    switch (key) {
        case 27:
            bail(0);
            break;

        case 'p':
            particle_radius += 0.01;
            glPointSize(particle_radius);
            break;

        case 'o':
            particle_radius -= 0.01;
            if (particle_radius < 0) {
                particle_radius = 0.01;
            }
            glPointSize(particle_radius);
            break;

        case '+':
            flow += 10;
            if (flow > num_particles)
                flow = num_particles;
            printf("%g particles/second\n", flow);
            break;

        case '-':
            flow -= 10;
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

        default:
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
    glutVisibilityFunc(visible);
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
    glutAddMenuEntry("[-]   Less flow", '-');
    glutAddMenuEntry("[+]   More flow", '+');
    glutAddMenuEntry("[o]   Smaller points", 'o');
    glutAddMenuEntry("[p]   Larger points", 'p');
    glutAddMenuEntry("[#]   Toggle framerate", '#');
    glutAddMenuEntry("[~]   Toggle fullscreen", '~');
    glutAddMenuEntry("[Esc] Quit", 27);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    texture[0] = loadBmpTexture("ground.bmp");

    particles = (Particle *) malloc(sizeof(Particle) * num_particles);

    glutMainLoop();
    return 0;
}
