bin_PROGRAMS = gles2-wayland

noinst_HEADERS = utils.h

gles2_wayland_SOURCES = gles2.cpp \
                        utils.cpp

gles2_wayland_CPPFLAGS = -fPIC \
                         -fpermissive

gles2_wayland_LDADD = -lm \
                      -lEGL \
                      -lGLESv2 \
                      -lwayland-egl \
                      -lwayland-client
