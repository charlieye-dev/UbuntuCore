#include <sys/time.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "utils.h"

int windowWidth = -1;
int windowHeight = -1;

GLint iLocPosition;
GLint iLocColor;
GLint TiLocPosition;
GLint TiLocColor;
GLint iLocMVP;

EGLDisplay eglDisplay;
EGLContext eglContext;
EGLSurface eglSurface;
static GLuint program;

struct wl_display  *display;
struct wl_registry *registry;
struct wl_compositor *compositor;
struct wl_shell *shell;
struct wl_surface *surface;
struct wl_shell_surface *shell_surface;
struct wl_egl_window *egl_window;

static void init_shader()
{
	GLuint vertex_shader, fragment_shader;
	GLint ret;
	char *log;

	static const char *VS_SOURCE =
		"precision mediump float;\n"
		"attribute vec4 av4position;\n"
		"attribute vec3 av3colour;\n"
		"uniform mat4 mvp;\n"
		"varying vec3 vv3colour;\n"
		"void main()\n"
		"{\n"
		"	vv3colour = av3colour;\n"
		"	gl_Position = mvp * av4position;\n"
		"}\n";

	static const char *FS_SOURCE =
		"precision mediump float;\n"
		"varying vec3 vv3colour;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = vec4(vv3colour, 1.0);\n"
		"}\n";

	//Pass Program Shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &VS_SOURCE, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
	if(!ret) {
		fprintf(stderr, "Error: vertex shader compilation failed!:\n");

		glGetShaderiv (vertex_shader, GL_INFO_LOG_LENGTH, &ret);
		if(ret > 1) {
			log = (char *)malloc(ret);
			glGetShaderInfoLog(vertex_shader, ret, NULL, log);
			printf("%s", log);
		}
	}

	//Pass Fragment Shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &FS_SOURCE, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
	if(!ret) {
		fprintf(stderr, "Error: fragment shader compilation failed!:\n");

		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);
		if(ret > 1) {
			log = (char *)malloc(ret);
			glGetShaderInfoLog(fragment_shader, ret, NULL, log);
			printf ("%s", log);
		}
	}

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &ret);
	if(!ret) {
		fprintf (stderr, "Error: program linking failed!:\n");

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);
		if(ret > 1) {
			log = (char *) malloc (ret);
			glGetProgramInfoLog (program, ret, NULL, log);
			printf ("%s", log);
		}
	}

	glUseProgram (program);
}


void draw()
{
	static float angleX = 0, angleY = 0, angleZ = 0;

	GL_CHECK(glUseProgram(program));

	/* Enable attributes for position, color and texture coordinates etc. */
	GL_CHECK(glEnableVertexAttribArray(iLocPosition));
	GL_CHECK(glEnableVertexAttribArray(iLocColor));

	/* Populate attributes for position, color and texture coordinates etc. */
	GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices));
	GL_CHECK(glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, colors));

	/*
	 * Do some rotation with Euler angles. It is not a fixed axis as
	 * quaternions would be, but the effect is nice.
	 */
	Matrix modelView = Matrix::createRotationX(angleX);
	Matrix rotation = Matrix::createRotationY(angleY);

	modelView = rotation * modelView;

	rotation = Matrix::createRotationZ(angleZ);
	Matrix translate1 = Matrix::createTranslation(0.0, 0.0, 0.0);

	modelView = translate1 *rotation * modelView;

	/* Pull the camera back from the cube */
	modelView[14] -= 3.5;

	Matrix perspective = Matrix::matrixPerspective(45.0f, windowWidth/(float)windowHeight, 0.01f, 100.0f);
	Matrix modelViewPerspective = perspective * modelView;

	GL_CHECK(glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, modelViewPerspective.getAsArray()));

	/* Update cube's rotation angles for animating. */
	angleX += 2.0;
	angleY += 2.0;
	angleZ += 0.0;

	if(angleX >= 360) angleX -= 360;
	if(angleX < 0) angleX += 360;
	if(angleY >= 360) angleY -= 360;
	if(angleY < 0) angleY += 360;
	if(angleZ >= 360) angleZ -= 360;
	if(angleZ < 0) angleZ += 360;

	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 36));

	Matrix translate = Matrix::createTranslation(1.5,0.0,0.0);
	modelView = translate * rotation * modelView;
	modelViewPerspective = perspective * modelView;

	GL_CHECK(glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, modelViewPerspective.getAsArray()));

	GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
}

void init_gl()
{
	/* Get attribute locations of non-fixed attributes like colour and texture coordinates. */
	iLocPosition = GL_CHECK(glGetAttribLocation(program, "av4position"));
	iLocColor = GL_CHECK(glGetAttribLocation(program, "av3colour"));

	TiLocPosition = GL_CHECK(glGetAttribLocation(program, "av4position"));
	TiLocColor = GL_CHECK(glGetAttribLocation(program, "av3colour"));

	/* Get uniform locations */
	iLocMVP = GL_CHECK(glGetUniformLocation(program, "mvp"));

	GL_CHECK(glEnable(GL_CULL_FACE));
	GL_CHECK(glEnable(GL_DEPTH_TEST));

	/* Set clear screen color. */
	GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
}

static void destroy_egl()
{
	eglDestroySurface(eglDisplay, eglSurface);
	eglDestroyContext(eglDisplay, eglContext);
	eglTerminate(eglDisplay);
}

// listeners
static void registry_add_object (void *data, struct wl_registry *registry,
                    uint32_t name, const char *interface,  uint32_t version) {
	if (!strcmp(interface,"wl_compositor")) {
		compositor = wl_registry_bind (registry, name, &wl_compositor_interface, 1);
	} else if (!strcmp(interface,"wl_shell")) {
		shell = wl_registry_bind (registry, name, &wl_shell_interface, 1);
	}
}

static void registry_remove_object (void *data, struct wl_registry *registry, uint32_t name) {

}

static struct wl_registry_listener registry_listener = {
	&registry_add_object,
	&registry_remove_object
};

static int init_wayland() {
	display = wl_display_connect (NULL);
	registry = wl_display_get_registry(display);
	wl_registry_add_listener (registry, &registry_listener, NULL);
	wl_display_roundtrip (display);

	surface = wl_compositor_create_surface (compositor);
	shell_surface = wl_shell_get_shell_surface (shell, surface);
	//  wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, window);
	wl_shell_surface_set_toplevel(shell_surface);
	egl_window = wl_egl_window_create(surface, WIDTH, HEIGHT);
}

static void destroy_wayland() {
	eglDestroySurface(eglDisplay, eglSurface);
	wl_egl_window_destroy(egl_window);
	wl_shell_surface_destroy(shell_surface);
	wl_surface_destroy(surface);
	eglTerminate(eglDisplay);
	wl_display_disconnect(display);
}

static int init_egl() {
	EGLint major, minor, n;
	EGLint visual_id;
	EGLConfig eglConfig;
	EGLint eglConfigCount;

	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	static const EGLint config_attribs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_DEPTH_SIZE, 1,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	eglDisplay = eglGetDisplay((void *)display);
	if(!eglDisplay) {
		fprintf(stderr, "Error: eglGetDisplay() failed\n");
		return -1;
	}

	if(!eglInitialize(eglDisplay, &major, &minor)) {
		fprintf(stderr, "Error: eglGetDisplay() failed\n");
		eglTerminate(eglDisplay);
		return -1;
	}

	if(!eglBindAPI(EGL_OPENGL_ES_API)) {
		fprintf(stderr, "Error: eglBindAPI() failed\n");
		eglTerminate(eglDisplay);
		return -1;
	}

	if(!eglChooseConfig(eglDisplay, config_attribs, &eglConfig, 1, &eglConfigCount)) {
		fprintf(stderr, "Error: eglChooseConfig() failed\n");
		eglTerminate(eglDisplay);
		return -1;
	}

	if(!eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &visual_id)) {
		fprintf(stderr, "Error: eglGetConfigAttrib() failed\n");
		eglTerminate(eglDisplay);
		return -1;
	}

	eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, context_attribs);
	if(eglContext == EGL_NO_CONTEXT) {
		fprintf(stderr, "Error: eglCreateContext() failed\n");
		eglTerminate(eglDisplay);
		return -1;
	}

	eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, egl_window, NULL);
	if(eglSurface == EGL_NO_SURFACE) {
		fprintf(stderr, "Error: eglCreateWindowSurface() failed\n");
		eglDestroyContext(eglDisplay, eglContext);
		eglTerminate(eglDisplay);
		return -1;
	}

	eglMakeCurrent (eglDisplay, eglSurface, eglSurface, eglContext);
}

/* Calculate fps. */
static void calculate_fps(int *frames, int *benchmark_time) {
	uint32_t time;
	struct timeval tv;

	const uint32_t benchmark_interval = 5;

	//Get current ms.
	gettimeofday(&tv, NULL);
	time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	if (*frames == 0)
		*benchmark_time = time;
	if(time - *benchmark_time > (benchmark_interval * 1000)) {
		printf("%d frames in %d seconds: %f fps\n",
				*frames, benchmark_interval,
				(float) *frames / benchmark_interval);
		*benchmark_time = time;
		*frames = 0;
	}

	(*frames)++;
}

static void es_info() {
	const char *s;

	s = eglQueryString(eglDisplay, EGL_VERSION);
	printf("EGL_VERSION = %s\n", s);

	s = eglQueryString(eglDisplay, EGL_VENDOR);
	printf("EGL_VENDOR = %s\n", s);

	s = eglQueryString(eglDisplay, EGL_CLIENT_APIS);
	printf("EGL_CLIENT_APIS = %s\n", s);

	s = eglQueryString(eglDisplay, EGL_EXTENSIONS);
	printf("EGL_EXTENSIONS = %s\n", s);
}

static void usage() {
	printf("Usage:\n");
	printf("  -info         show OpenGL ES info\n");
	printf("  -fps          show fps\n");
}

int main(int argc, char **argv)
{
	int frames = 0;
	int benchmark_time;
	bool running = true;
	bool printInfo = false;
	bool printFps = false;

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-info") == 0) {
			printInfo = true;
		} else if(strcmp(argv[i], "-fps") == 0) {
			printFps = true;
		} else {
			usage();
			return -1;
		}
	}

	if(init_wayland() < 0) {
		return -1;
	}

	if(init_egl() < 0) {
		destroy_wayland();
		return -1;
	}

	if(printInfo) {
		es_info();
	}

	init_shader();
	init_gl();

	while(running) {
        wl_display_dispatch_pending(display);
		draw();
		eglSwapBuffers(eglDisplay, eglSurface);

		// calculate fps
		if(printFps)
			calculate_fps(&frames, &benchmark_time);
	}

	//Clean
	destroy_egl();
	destroy_wayland();
}
