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

#include <algorithm>
#include <cstdio>

struct point { double x, y; };
extern int cols, rows;

void output(char *name, bool reverse_video, bool variable_size, double maxden,
            double *den, int ndots, struct point *G) {
    printf("// ndots=%d\n", ndots);
    printf("prog_uint16_t data[] = {\n");
    int div = std::max(rows, cols);
    for(int i=0; i<ndots; i++) {
        int x = (int)(G[i].x * 1023 / div);
        int y = (int)(G[i].y * 1023 / div);
        printf("%5d, %5d, ", (x << 1) | 0x1000, (y << 1)| 0x5000);
        if(i % 4 == 3) printf("\n");
    }
    printf("};\n");
}
