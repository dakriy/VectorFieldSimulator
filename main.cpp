#include <windows.h>
#include <GL/gl.h>
#include <time.h>
#include <cmath>
#include <cstdio>
#include <complex>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
// Size of the screen in the x direction
const int SCREENX = 1920;
// Size of the screen in the y direction
const int SCREENY = 1200;
//If this is zero, the framerate is not capped. If this is not zero, the frame rate will
//be capped at whatever the value is.
float limitFrameRate = 60.0;

//This key pauses the screen
const int KEY_PAUSE = ' ';
//This key steps one frame
const int KEY_STEP = 'O';
//This key slows down
const int KEY_SLOW = 'S';
//This key speeds up
const int KEY_FAST = 'F';
//This key resets the particles
const int KEY_RESET = 'R';
//This key resets the speed
const int KEY_SPEED_RESET = 'D';

const double PI = 3.14159265358979323846264338327950288;

// Determines the size of the coordinate grids
const int GRID_SIZE = 20;

const bool ACCELERATION = false;

// Maximum number of particles to have on screen
const long MAX_PARTICLES = 50000;

// Determines whether to use fullscreen or not
bool fullscreen = true;

// X Offset for the vector field map
int xOffset = 0;
// Y Offset for the vector field map
int yOffset = 0;

float speedScalar = 1;

const float SCALAR_STEP = 0.03125;
// Window variables
HGLRC hRC = NULL; // Rendering Context
HDC hDC = NULL; // Device Context
HWND hWnd = NULL;
HINSTANCE hInstance;

bool keys[256];
bool active = true;

int selected_particle = 5;

struct vector2f
{
	float x;
	float y;
	float getMagnitude()
	{
		return sqrt((x*x)+ (y*y));
	}
};

// Origin as calcualted by the grid size
vector2f origin;

// Return the x Cartesian Coordinate value
float getCoordXPosition(vector2f p)
{
	return (p.x - origin.x) / GRID_SIZE;
}

// Return the x Cartesian Coordinate value
float getCoordYPosition(vector2f p)
{
	return ((origin.y - p.y) / GRID_SIZE);
}

// Return the x Cartesian Coordinate value
float getCoordXPosition(int x)
{
	return (x - origin.x) / GRID_SIZE;
}

// Return the x Cartesian Coordinate value
float getCoordYPosition(int y)
{
	return ((origin.y - y) / GRID_SIZE);
}



struct particle
{
	// Position
	vector2f p;
	// Velocity
	vector2f v;

	float getSpeed()
	{
		return v.getMagnitude();
	}
	float distanceFromOrigin()
	{
		return sqrt((getCoordXPosition(p) * getCoordXPosition(p)) + (getCoordYPosition(p) * getCoordYPosition(p)));
	}

	// Turns a cartesian coordinate into raw x position.
	void setCoordXPosition(float x)
	{
		p.x = x/GRID_SIZE + origin.x;
	}

	// Turns a cartesian coordinate into raw y position.
	void setCoordYPosition(float y)
	{
		p.y = -(y/GRID_SIZE) + origin.y;
	}

	// Get velocity from position
	void setXVelocity()
	{
		float x = getCoordXPosition(p);
		float y = getCoordYPosition(p);
		v.x = (y)*speedScalar*GRID_SIZE/limitFrameRate;
	}

	// Get velocity from position
	void setYVelocity()
	{
		float x = getCoordXPosition(p);
		float y = getCoordYPosition(p);
		v.y = (-x-y)*speedScalar*GRID_SIZE/limitFrameRate;
	}
};

particle particles[MAX_PARTICLES];

struct // Create A Structure For The Timer Information
{
	__int64 frequency; // Timer Frequency
	float resolution; // Timer Resolution
	unsigned long mm_timer_start;	// Multimedia Timer Start Value
	unsigned long mm_timer_elapsed; // Multimedia Timer Elapsed Time
	bool performance_timer; // Using The Performance Timer?
	__int64 performance_timer_start; // Performance Timer Start Value
	__int64 performance_timer_elapsed; // Performance Timer Elapsed Time
} timer;

// Draw a pixel at (x, y)
void DrawPixel(GLfloat x, GLfloat y)
{
	glLoadIdentity(); //Reset the view

	glTranslatef(x, y, 0.0f);
	glBegin(GL_POINTS);
	glVertex2d(0.0f, 0.0f);
	glEnd();

	return;
}

// Draw a line starting at (x, y) and going up and right the specified distance
// width is how thick the line should be
void DrawLine(GLfloat x, GLfloat y, GLfloat up, GLfloat right, GLfloat width)
{
	glLineWidth(width);
	glLoadIdentity(); //Reset the view
	glTranslatef(x, y, 0.0f);
	glBegin(GL_LINES);
	glVertex2d(0.0f, 0.0f);
	glVertex2d(right, -up);
	glEnd();

	return;
}


bool InitGL(GLvoid) {
	glShadeModel(GL_SMOOTH); //Enable smooth shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	//Black Background
	glClearDepth(1.0f); //Depth Buffer Setup
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); //Set Line antialiasing
	//glEnable(GL_BLEND); //Enable blending
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Type of blending to use

	return true; //Initialization Went OK
}

void drawGrid()
{
	// Draw y axis
	for (int i = 0; i < SCREENX; i += GRID_SIZE)
	{
		if (i == origin.x)
		{
			glColor3f(0, 0, 128);
		} else
		{
			glColor3f(102, 0, 0);
		}
		DrawLine(i, SCREENY, SCREENY, 0, 1);
	}

	// Draw x axis
	for (int i = 0; i < SCREENY; i += GRID_SIZE)
	{
		if (i == origin.y)
		{
			glColor3f(0, 0, 128);
		} else
		{
			glColor3f(102, 0, 0);
		}
		DrawLine(0, i, 0, SCREENX, 1);		
	}
}

// Calcualtes the center of the screen closest to gird axis for grid drawing purposes
void CalcualteCenter()
{
	int midx = SCREENX / 2;
	int midy = SCREENY / 2;
	if (midx % GRID_SIZE >= 5)
	{
		origin.x = (GRID_SIZE - midx % GRID_SIZE) + midx;
	} else
	{
		origin.x = (midx - midx % GRID_SIZE);
	}

	if (midy % GRID_SIZE >= 5)
	{
		origin.y = (GRID_SIZE - midy % GRID_SIZE) + midy;
	}
	else
	{
		origin.y = (midy - midy % GRID_SIZE);
	}
}

void generateRandParticlePosition(particle *p)
{
	(*p).p.x = rand() % SCREENX;
	(*p).p.y = rand() % SCREENY;
}

void DrawGLScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the screen
	drawGrid();

	//Draw particles
	for (long i = 0; i<MAX_PARTICLES; i++) {
		glColor3f(255, 255, 255); //Set the color
		// If particle is at the edge of the screen move it to the other side.
		// FOR A SOURCE
		/*while (particles[i].p.x > SCREENX)
			particles[i].p.x -= SCREENX;

		while (particles[i].p.y > SCREENY)
			particles[i].p.y -= SCREENY;

		while (particles[i].p.x < 0)
			particles[i].p.x += SCREENX;

		while (particles[i].p.y < 0)
			particles[i].p.y += SCREENY;*/

		// FOR A SINK
		if (particles[i].p.x > SCREENX)
			generateRandParticlePosition(&particles[i]);

		if (particles[i].p.y > SCREENY)
			generateRandParticlePosition(&particles[i]);

		if (particles[i].p.x < 0)
			generateRandParticlePosition(&particles[i]);

		if (particles[i].p.y < 0)
			generateRandParticlePosition(&particles[i]);

		if ((int)particles[i].p.y == (int)origin.y && (int)particles[i].p.x == (int)origin.x)
			generateRandParticlePosition(&particles[i]);

		DrawPixel(particles[i].p.x, particles[i].p.y);
	}

	// Pixels off particle
	float pixelsFromParticle = 5;
	// Draw selected particle tracking sights if there is a selected particle
	if (selected_particle >= 0 && selected_particle < MAX_PARTICLES)
	{
		glColor3f(0, 255, 0);
		// Draw the line to the right
		DrawLine(particles[selected_particle].p.x + pixelsFromParticle, particles[selected_particle].p.y, 0, SCREENX - particles[selected_particle].p.x + pixelsFromParticle, 1);
		// Draw the line to the left
		DrawLine(0, particles[selected_particle].p.y, 0, particles[selected_particle].p.x - pixelsFromParticle, 1);
		// Draw the line right above the particle
		DrawLine(particles[selected_particle].p.x, 0, -particles[selected_particle].p.y + pixelsFromParticle, 0, 1);
		// Draw the line below the particle
		DrawLine(particles[selected_particle].p.x, SCREENY, SCREENY - particles[selected_particle].p.y - pixelsFromParticle, 0, 1);
	}
}


// Generate initial state.
void initParticles()
{
	for (long i = 0; i < MAX_PARTICLES; i++)
	{
		generateRandParticlePosition(&particles[i]);
	}
}



void moveParticles()
{
	for (long i = 0; i < MAX_PARTICLES; i++)
	{
		particles[i].setXVelocity();
		particles[i].setYVelocity();
		particles[i].p.x += (particles[i].v.x);
		particles[i].p.y -= (particles[i].v.y);
	}
}

//Set the size of the window
GLvoid ReSizeGLScene(GLsizei width, GLsizei height) {
	glViewport(0, 0, width, height); //Set the size

	glMatrixMode(GL_PROJECTION); //Select the projection matrix
	glLoadIdentity(); //Reset the matrix

	glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f); //Create Ortho View (0,0 at top left)

	glMatrixMode(GL_MODELVIEW); //Select the model view matrix
	glLoadIdentity(); //Reset the model view matrix
}

void TimerInit(void) //Initialize Our Timer (Get It Ready)
{
	memset(&timer, 0, sizeof(timer)); //Clear Our Timer Structure

									  //Check To See If A Performance Counter Is Available
									  //If One Is Available The Timer Frequency Will Be Updated
	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&timer.frequency)) {
		//No Performace Counter Available
		timer.performance_timer = false; //Set Performance Timer To FALSE
		timer.mm_timer_start = clock(); //Use timeGetTime() To Get Current Time
		timer.resolution = 1.0f / CLK_TCK; //Set Our Timer Resolution To .001f
		timer.frequency = CLK_TCK; //Set Our Timer Frequency To 1000
		timer.mm_timer_elapsed = timer.mm_timer_start; //Set The Elapsed Time To The Current Time
	}
	else {
		//Performance Counter Is Available, Use It Instead Of The Multimedia Timer
		//Get The Current Time And Store It In performance_timer_start
		QueryPerformanceCounter((LARGE_INTEGER *)&timer.performance_timer_start);
		timer.performance_timer = TRUE; //Set Performance Timer To TRUE
										//Calculate The Timer Resolution Using The Timer Frequency
		timer.resolution = (float)(((double)1.0f) / ((double)timer.frequency));
		//Set The Elapsed Time To The Current Time
		timer.performance_timer_elapsed = timer.performance_timer_start;
	}
}

float TimerGetTime() { //Get Time In Milliseconds
	__int64 time; //time Will Hold A 64 Bit Integer

	if (timer.performance_timer) { //Are We Using The Performance Timer?
		QueryPerformanceCounter((LARGE_INTEGER *)&time); //Grab The Current Performance Time
														 //Return The Current Time Minus The Start Time Multiplied By The Resolution And 1000 (To Get MS)
		return ((float)(time - timer.performance_timer_start) * timer.resolution)*1000.0f;
	}
	else {
		//Return The Current Time Minus The Start Time Multiplied By The Resolution And 1000 (To Get MS)
		return((float)(clock() - timer.mm_timer_start) * timer.resolution)*1000.0f;
	}
}

// To kill the OpenGL window
GLvoid KillGLWindow(GLvoid) {
	if (fullscreen) {
		ChangeDisplaySettings(NULL, 0); //Switch to window mode so that we can destroy the window
		ShowCursor(true);
	}

	//Does the rendering object exist?
	if (hRC) {
		//Detach the rendering object from the device context
		if (!wglMakeCurrent(NULL, NULL)) {
			MessageBox(NULL, "Release Of DC And RC Failed.", "Shutdown Error", MB_OK | MB_ICONINFORMATION);
		}

		//Try to destroy the rendering object
		if (!wglDeleteContext(hRC)) {
			MessageBox(NULL, "Release Rendering Context Failed.", "Shutdown Error", MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL;
	}

	//Try to destroy the device context
	if (hDC && !ReleaseDC(hWnd, hDC)) {
		MessageBox(NULL, "Release Device Context Failed.", "Shutdown Error", MB_OK | MB_ICONINFORMATION);
		hDC = NULL;
	}

	//Try to destroy the window
	if (hWnd && !DestroyWindow(hWnd)) {
		MessageBox(NULL, "Could Not Release hWnd.", "Shutdown Error", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;
	}

	//Unregister the class
	if (!UnregisterClass("OpenGL", hInstance)) {
		MessageBox(NULL, "Could Not Unregister Class.", "Shutdown Error", MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;
	}
}

//Create the OpenGL window (bits means the bit color (8,16,24,32)
bool CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag) {
	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;
	RECT WindowRect;
	WindowRect.left = (long)0;
	WindowRect.right = (long)width;
	WindowRect.top = (long)0;
	WindowRect.bottom = (long)height;

	fullscreen = fullscreenflag;

	hInstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "OpenGL";

	//Register the window class
	if (!RegisterClass(&wc)) {
		MessageBox(NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (fullscreen) {
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = width;
		dmScreenSettings.dmPelsHeight = height;
		dmScreenSettings.dmBitsPerPel = bits;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		//Try to initialize full screen mode
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
			//Not successful, ask for windowed mode
			if (MessageBox(NULL, "The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?", "NeHe GL", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				fullscreen = false;
			}
			else {
				MessageBox(NULL, "Program Will Now Close.", "ERROR", MB_OK | MB_ICONSTOP);
				return false;
			}
		}
	}

	if (fullscreen) {
		//Extend window, hide cursor
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor(TRUE);
	}
	else {
		//make a more 3D look with WINDOWEDGE
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW; //Makes our window have title, sizing border, window menu, etc.
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	//Try to create the window
	hWnd = CreateWindowEx(dwExStyle, "OpenGL", title, dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		//Not successful, so kill evreything that we have set up
		KillGLWindow();
		MessageBox(NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
		1, // Version Number
		PFD_DRAW_TO_WINDOW | // Format Must Support Window
		PFD_SUPPORT_OPENGL | // Format Must Support OpenGL
		PFD_DOUBLEBUFFER, // Must Support Double Buffering
		PFD_TYPE_RGBA, // Request An RGBA Format
		bits, // Select Our Color Depth
		0, 0, 0, 0, 0, 0, // Color Bits Ignored
		0, // No Alpha Buffer
		0, // Shift Bit Ignored
		0, // No Accumulation Buffer
		0, 0, 0, 0, // Accumulation Bits Ignored
		16, // 16Bit Z-Buffer (Depth Buffer)
		0, // No Stencil Buffer
		0, // No Auxiliary Buffer
		PFD_MAIN_PLANE, // Main Drawing Layer
		0, // Reserved
		0, 0, 0 // Layer Masks Ignored
	};

	if (!(hDC = GetDC(hWnd))) { // getting Device Context
		KillGLWindow();
		MessageBox(NULL, "Can't Create A GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd))) { //find a pixel format
		KillGLWindow();
		MessageBox(NULL, "Can't Find A Suitable PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd)) { //setting the pixel format
		KillGLWindow();
		MessageBox(NULL, "Can't Set The PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(hRC = wglCreateContext(hDC))) { //get a Rendering Context.
		KillGLWindow();
		MessageBox(NULL, "Can't Create A GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	//Make rendering context active
	if (!wglMakeCurrent(hDC, hRC)) {
		KillGLWindow();
		MessageBox(NULL, "Can't Activate The GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	//Show the window, set the focus and foreground, and call the resize function.
	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	ReSizeGLScene(width, height);

	//Initialize OpenGL!
	if (!InitGL()) {
		KillGLWindow();
		MessageBox(NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	return true;
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_ACTIVATE:
		if (!HIWORD(wParam)) {
			active = true;
		}
		else {
			active = false;
		}
		return 0;
	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		keys[wParam] = true;
		return 0;
	case WM_KEYUP:
		keys[wParam] = false;
		return 0;
	case WM_SIZE:
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));
		return 0;		
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	bool done = false;
	long frames = 0;
	float fps;
	clock_t t1, t2;
	char buffer[20];

	if (!CreateGLWindow("Vector Simulator 2016", SCREENX, SCREENY, 32, fullscreen)) {
		return 0;
	}

	TimerInit();
	srand(time(NULL)); //Initialize the random number generator
	initParticles();
	CalcualteCenter();
	t1 = clock();
	while (!done) {
		if (keys[27]) done = true;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				done = true;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			float start = TimerGetTime();
			if (active)
			{
				moveParticles();
				DrawGLScene();
				SwapBuffers(hDC);
			}
			if (keys[KEY_RESET])
			{
				initParticles();
				speedScalar = 1;
			}
			//Step key
			if (keys[KEY_STEP]) {
				active = false;
				moveParticles();
				DrawGLScene();
				SwapBuffers(hDC);
				keys[KEY_STEP] = false;
			}
			if (keys[KEY_SPEED_RESET]) {
				speedScalar = 1;
			}
			//Speeds up the simulation
			if (keys[KEY_FAST]) {
				speedScalar += SCALAR_STEP;
				keys[KEY_FAST] = false;
			}
			//Slows down the simulation
			if (keys[KEY_SLOW]) {
				speedScalar -= SCALAR_STEP;
				keys[KEY_SLOW] = false;
			}
			if (keys[KEY_PAUSE])
			{
				active = !active;
				keys[KEY_PAUSE] = false;
			}
			//Limit the framerate
			if (limitFrameRate != 0) {
				while (TimerGetTime() < start + float(1000.0 / limitFrameRate));
			}
			frames++;
		}
	}
	t2 = clock();
	KillGLWindow();
	fps = frames / (float(t2 - t1) / 1000.0);
	_itoa_s(int(fps), buffer, 10);
	return (msg.wParam);
}
