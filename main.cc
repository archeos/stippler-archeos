//    This is a component of stippler, an image processing tool
//    Copyright 2004, 2005 Jeff Epler <jepler@unpythonic.net>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <sys/time.h>
#include <signal.h>

extern "C" {
#include <pgm.h>
}

extern void output(char *name, bool reverse_video, bool variable_size, double maxden, 
        double *den, int ndots, struct point *G);

sig_atomic_t user_quit = 0;

void handler(int ignore) { user_quit = 1; }

class UniqueColor {
public:
    GLint bits[3];
    GLubyte color[3];
    int masks[3];
    int shifts[3];
    int idx;

    static void setup_opengl_state();
    void _init_common();
    UniqueColor(int rb, int gb, int bb);
    UniqueColor();
    int see(int new_idx);
    int identify(GLubyte r, GLubyte g, GLubyte b);
    UniqueColor operator++();
};

void UniqueColor::setup_opengl_state() {
    glDisable (GL_BLEND);
    glDisable (GL_DITHER);
    glDisable (GL_FOG);
    glDisable (GL_LIGHTING);
    glDisable (GL_TEXTURE_1D);
    glDisable (GL_TEXTURE_2D);
    glDisable (GL_TEXTURE_3D);
    // glEnable (GL_DEPTH_TEST);
    glShadeModel (GL_FLAT);
}

void UniqueColor::_init_common(void) {
    shifts[0] = 8-bits[0];
    shifts[1] = 8-bits[1];
    shifts[2] = 8-bits[2];
    masks[0] = (1<<bits[0])-1;
    masks[1] = ((1<<bits[1])-1);
    masks[2] = ((1<<bits[2])-1);
    color[0] = color[1] = color[2] = 0;
    idx = 0;
}

UniqueColor::UniqueColor(int rb, int gb, int bb) {
    bits[0] = rb; bits[1] = gb; bits[2] = bb;
    _init_common();
}

UniqueColor::UniqueColor() {
    glGetIntegerv(GL_RED_BITS, &bits[0]);
    glGetIntegerv(GL_GREEN_BITS, &bits[1]);
    glGetIntegerv(GL_BLUE_BITS, &bits[2]);
    _init_common();
}

int UniqueColor::see(int new_idx) {
    idx = new_idx;
    int j = new_idx + 1;
    color[0] = (j & masks[0]) << shifts[0]; j >>= bits[0];
    color[1] = (j & masks[1]) << shifts[1]; j >>= bits[1];
    color[2] = (j & masks[2]) << shifts[2];
    return idx;
}

int UniqueColor::identify(GLubyte r, GLubyte g, GLubyte b) {
    int i;
    i = (b >> shifts[2]);
    i = (i << bits[1]) | (g >> shifts[1]);
    i = (i << bits[0]) | (r >> shifts[0]);
    return i-1;
}

UniqueColor UniqueColor::operator++() {
    see(idx + 1);
    return *this;
}

GLuint cone;

void error_check(char *where) {
    GLenum e = glGetError();
    if(e == GL_NO_ERROR) return;
    fprintf(stderr, "%s: %s (%d)\n", where, gluErrorString(e), e);
}

#define VISIBLE

Display *d;
Window p;
void wait_for_map() {
    while(1) {
        XEvent e;
        XNextEvent(d, &e);
        if(e.type == X_ConfigureWindow) break;
    }
}

void make_context(int w, int h) {
    d = XOpenDisplay(NULL);
#ifdef VISIBLE
    int attr[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 5,
                   GLX_DEPTH_SIZE, 16, None };
    XVisualInfo *vi = glXChooseVisual(d, DefaultScreen(d), attr);
    XSetWindowAttributes xswa;

    xswa.event_mask = ExposureMask|VisibilityChangeMask|StructureNotifyMask|ResizeRedirectMask;
    xswa.colormap = \
        XCreateColormap(d, DefaultRootWindow(d), vi->visual, AllocNone);
    // the XCreateWindow call is wrong for some reason I don't understand
    Window p = XCreateWindow(d, DefaultRootWindow(d), 0, 0,
            w, h, 0, vi->depth, InputOutput, vi->visual,
            CWColormap|CWEventMask, &xswa);
    XMapWindow(d, p);
    GLXContext ctx = glXCreateContext(d, vi, NULL, True);

    wait_for_map();
#else
    int attr[] = { GLX_RGBA, GLX_RED_SIZE, 5,
                   GLX_DEPTH_SIZE, 16, None };
    XVisualInfo *vi = glXChooseVisual(d, DefaultScreen(d), attr);
    Pixmap px = XCreatePixmap(d, DefaultRootWindow(d), w, h, vi->depth);
    GLXPixmap p = glXCreateGLXPixmap(d, vi, px);
    GLXContext ctx = glXCreateContext(d, vi, NULL, True);
#endif
    glXMakeCurrent(d, p, ctx);
}

void make_cone(int sz) {
    cone = glGenLists(1);
    glNewList(cone, GL_COMPILE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,0,-1);
    for(int i=0; i<=16; i++) {
        double theta = i * M_PI / 8;
        glVertex3f(cos(theta) * sz * M_SQRT2, sin(theta) * sz, 0);
    }
    glEnd();
    glEndList();
}

int rows, cols;
gray maxval;

gray **load_image(char *image, bool reverse_video) {
    FILE *fp = fopen(image, "rb");
    gray **res = pgm_readpgm(fp, &cols, &rows, &maxval);
    fclose(fp);
    if(reverse_video) {
        for(int y=0; y<rows; y++) {
            for(int x=0; x<cols; x++) {
                res[y][x] = maxval - res[y][x];
            }
        }
    }
    return res;
}    

UniqueColor setup_opengl_state() {
    make_context(cols, rows);
    UniqueColor::setup_opengl_state();
    make_cone((int)hypot(rows, cols) / 10+1);

    glClearColor(0,0,0,0);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(0, cols, 0, rows, 0, -1);
    glMatrixMode( GL_MODELVIEW );
    UniqueColor u;
    error_check("setup");
    return u;
}

struct point { double x, y; };

void voronoi(int scale, UniqueColor &u, int ndots, struct point points[], GLvoid *buf) {
    error_check("before cone loop");
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    for(int i=1; i<ndots; i++) {
        u.see(i);
        glColor3ubv(u.color);
        glPushMatrix();
        glTranslated(scale*points[i].x, scale*points[i].y, 1);
        glCallList(cone);
        glPopMatrix();
    }
    glFlush();
    error_check("cone loop");
    glReadPixels(0, 0, cols, rows, GL_RGBA,
                 GL_UNSIGNED_INT_8_8_8_8, (GLvoid*)buf);
    error_check("glReadPixels");
}

extern void dither(gray **density, int ndots, struct point *G);

int main(int argc, char **argv) {
    double heat = 2.1;
    bool variable_size = getenv("VARIABLE_SIZE") != NULL;
    bool reverse_video = getenv("REVERSE_VIDEO") != NULL;
    fprintf(stderr, "variable_size=%d\n", variable_size);
    fprintf(stderr, "reverse_video=%d\n", reverse_video);
    unsetenv("__GL_FSAA_MODE");
    if(argc < 2 || argc > 4) { 
        fprintf(stderr, "usage: %s pgmfile [scale [dotcount]]\n", argv[0]);
        return 1;
    }
    char *image = argv[1];

    gray **density = load_image(image, reverse_video);

    int scale = argc >= 3 ? atoi(argv[2]) : 1;
    int ndots = argc >= 4 ? atoi(argv[3]) : rows * cols * scale * scale / 500;
    if(scale > 16) {
        ndots = scale;
        scale = (int)ceil(sqrt(ndots * 500. / rows / cols));
    }
    fprintf(stderr, "scale=%d ndots=%d\n", scale, ndots);
    srand48(time(NULL));

    UniqueColor u = setup_opengl_state();
    int color_mask = (u.masks[0] << u.shifts[0] << 24)
        | (u.masks[1] << u.shifts[1] << 16)
        | (u.masks[2] << u.shifts[2] << 8);

    struct point G[ndots];
    dither(density, ndots, G);

    unsigned char buf[rows * cols * 4];
    unsigned int *ibuf = (unsigned int *)buf;
    signal(SIGINT, handler);

    double xnum[ndots], ynum[ndots], den[ndots];
    for(int i=0; !user_quit; i++) {

        memset(xnum, 0, sizeof(double) * ndots); 
        memset(ynum, 0, sizeof(double) * ndots); 
        memset(den, 0, sizeof(double) * ndots); 

        for(int rr = 0; rr<scale; rr++) {
        for(int cc = 0; cc<scale; cc++) {
            glPushMatrix();
            glTranslated(-cc * cols, -rr * rows, 0);
            glDrawBuffer(GL_BACK);
            voronoi(scale, u, ndots, G, (GLvoid*)buf);
            for(int y=0; y < rows; y++)
            for(int x=0; x < cols; ) {
                int x1;

                unsigned int c0 = ibuf[x + cols * y] & color_mask;
                int j = u.identify((c0 >> 24) & 0xff, (c0 >> 16) & 0xff,
                        (c0 >> 8) & 0xff);
                if(j >= ndots || j < 0) {
                    if(j != -1) fprintf(stderr, "identify(%x) -> %d [%d]\n", c0, j, ndots); 
                    x++; continue;
                }
                for(x1 = x; x1 < cols; x1++) {
                    if((ibuf[x1 + cols * y] & color_mask) != c0) {
                        break;
                    }
                    int ix = (x1 + cols * cc)/scale;
                    int iy = rows - (y + rows * rr)/scale - 1;
                    double dd = density[iy][ix];
                    den[j] +=  dd;
                    xnum[j] += (.5+x1 + cols*cc)/scale * dd;
                    ynum[j] += (.5+y  + rows*rr)/scale * dd;
                }
                x = x1;
            }
            glPopMatrix();
        } }
        glDrawBuffer(GL_FRONT);
        if(reverse_video) {
            glClearColor(1,1,1,1);
            glColor3f(0,0,0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glColor3f(1,1,1);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glAlphaFunc(GL_GREATER, 0.5);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POINT_SMOOTH);

        glPointSize(2.5);
        double moved = 0;
        double maxdist = 0;
        double maxden = 0;
        for(int j = 0; j < ndots; j++) {
            if(den[j] > maxden) maxden = den[j];
        }
        for(int j = 0; j < ndots; j++) {
            if (den[j] != 0) {
                    double ix = xnum[j] / den[j] - G[j].x;
                    double iy = ynum[j] / den[j] - G[j].y;
		    double nx = G[j].x + heat * ix;
		    double ny = G[j].y + heat * iy;
		    double dist = hypot(nx - G[j].x, ny - G[j].y);
		    if(dist > 1000) {
			fprintf(stderr, "%4d: (%7.4f %7.4f)->(%7.4f %7.4f)  %7.4f\n",
			    j, G[j].x, G[j].y, nx, ny, dist);
			fprintf(stderr, "    : %7.4f %7.4f %7.4f\n",
			    xnum[j], ynum[j], den[j]);
            } else {
		G[j].x = drand48() * cols;
		G[j].y = drand48() * rows;
	    }
            moved += dist;
            if(dist > maxdist) maxdist = dist;
            G[j].x = nx; G[j].y = ny;
	    }
            if(variable_size) glPointSize(4.0 * sqrt(den[j] / maxden));
            error_check("setting point size");
            glBegin(GL_POINTS);
            glVertex2f(G[j].x, G[j].y);
            glEnd();
        }
        error_check("drawing points");
        if(reverse_video)
            glClearColor(0,0,0,0);
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        fprintf(stderr, "moved %.3f %7.0f %5.3f\n", heat, moved, maxdist);
        heat = 1 + (heat - 1) * .99;
        if(maxdist < .1 / scale) break;
        glFinish();
    }

    double maxden = 0;
    for(int j = 0; j < ndots; j++) {
        if(den[j] > maxden) maxden = den[j];
    }
 
    output(argv[1], reverse_video, variable_size, maxden, den, ndots, G);

    glPointSize(2.5);
    glEnable(GL_BLEND);
    if(reverse_video) {
        glClearColor(1,1,1,1);
        glColor3f(0,0,0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glColor3f(1,1,1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glAlphaFunc(GL_GREATER, 0.5);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    while(!user_quit) {
        glDrawBuffer(GL_FRONT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for(int j=0; j<ndots; j++) {
            if(variable_size) glPointSize(1 + den[j] * 4.0 / maxden);
            glBegin(GL_POINTS);
            glVertex2f(G[j].x, G[j].y);
            glEnd();
        }
        glFinish();
        sleep(1);
    }

    return 0;
}
