LFLAGS := -L$(shell if [ -d /usr/X11R6/lib64 ]; then echo /usr/X11R6/lib64; else echo /usr/X11R6/lib; fi)
$(warning LFLAGS=$(LFLAGS))
LIBS := -lGL -lGLU -lXi -lXmu -lX11 -lm -lnetpbm
SRCS := main.cc newdither.cc
CFLAGS := -Wall -O2 -g
CCLD := $(CC)

OBJS := $(patsubst %.cc, obj/%.o, $(SRCS))

default: stippler stippler_ps stippler_dac

clean:
	rm -rf obj stippler stippler_*
obj/%.d: %.cc 
	mkdir -p $(dir $@)
	$(CC) -MM -MG -MT "$@ $(patsubst obj/%.d,obj/%.o,$@)" \
		$(CFLAGS) $< -o $@.tmp && mv -f $@.tmp $@

obj/%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@.tmp && mv -f $@.tmp $@


stippler: $(OBJS) obj/gcode.o
	$(CC) $(LFLAGS) -o $@ $^ $(LIBS)

stippler_ps: $(OBJS) obj/ps.o
	$(CC) $(LFLAGS) -o $@ $^ $(LIBS)

stippler_dac: $(OBJS) obj/dac.o
	$(CC) $(LFLAGS) -o $@ $^ $(LIBS)

include $(patsubst %.o,%.d,$(OBJS) obj/gcode.o obj/ps.o)
