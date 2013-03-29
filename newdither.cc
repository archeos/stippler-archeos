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

#include <pgm.h>
#include <cmath>
#include <algorithm>
#include <stdlib.h>

struct point { double x, y; };

extern int rows, cols;
extern gray maxval;
static double threshhold, err;
static struct point *G;
static int n, ndots;
static gray **density; 

typedef void(*callback)(int x, int y);

static void hilbert(int x, int y, int xi, int xj, int yi, int yj,
                    int n, int d, callback c) {
    if(n >= d) {
        c(x, y);
        return;
    }
    xi /= 2;
    xj /= 2;
    yi /= 2;
    yj /= 2;
    n += 1;
    hilbert(x,          y,           yi,  yj,  xi,  xj, n, d, c);
    hilbert(x+xi,       y+xj,        xi,  xj,  yi,  yj, n, d, c);
    hilbert(x+xi+yi,    y+xj+yj,     xi,  xj,  yi,  yj, n, d, c);
    hilbert(x+xi+yi+yi, y+xj+yj+yj, -yi, -yj, -xi, -xj, n, d, c);
}

static void dither_pixel(int x, int y) {
    if(x < 0 || y < 0 || x >= cols || y >= rows) return;
    err += density[y][x];
    if(err > threshhold / 2) {
        if(n < ndots) {
            // fprintf(stderr, "%4d %4d %4d %8.1f\n", x, y, density[y][x], err);
            G[n].x = x + drand48(); G[n].y = rows-y-1 + drand48();
        }
        err -= threshhold;
        n++;
    }
}

double log2(int value)
{
    return log(value) / log(2);
}


void dither(gray **density_, int ndots_, struct point *G_) {
    density = density_; ndots = ndots_; G = G_;
    double total = 0;
    for(int py=0; py<rows; py++)
        for(int px = 0; px<cols; px++) 
            total += density[py][px];
    threshhold = total / ndots; 
    fprintf(stderr, "total=%f threshhold=%f\n", total, threshhold);

    int size = std::max(rows, cols);
    int level = (int)ceil(log2(size));
    fprintf(stderr, "level=%d\n", level);
    err = 0;
    hilbert(0, 0, 1<<level, 0, 0, 1<<level, 0, level, dither_pixel);
    fprintf(stderr, "ndots=%d n=%d\n", ndots, n);
    for(;n < ndots; n++) {
	do {
		G[n].x = drand48() * cols;
		G[n].y = drand48() * rows;
	} while (drand48() * maxval > density[rows-(int)G[n].y-1][(int)G[n].x]);
    }
}
