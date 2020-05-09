#include <iostream>
#include <fstream>
#include <algorithm>
#include <math.h>
#include <vector>
#include <gl/glut.h>
#include <gl/gl.h>
#include <gl/glu.h>
#define KEY_ESC 27
#define KEY_SPACE 32

using namespace std;

#pragma region GLOBAL
const double PI = 3.14159265358979323846;
const double HALF_PI = PI * 0.5;
const double ONEHALF_PI = PI * 1.5;
const double TWO_PI = PI * 2.0;

const GLdouble ORTHO = 1.0;
const double SPH_R = 0.05;
const double SPH_R_SMALL = 0.03;
const double SPH_DIST = 0.12;
const double SPH_DIST_HALF = 0.06;
const GLint SLICE_N = 25;
const GLint SLICE_LESS_N = 10;
const double CUBE_SIZE = 0.1;
const double CLR_WHITE[] = { 0.9, 0.9, 0.9, 1.0 };
const double CLR_RED[] = { 0.9, 0.5, 0.5, 1.0 };
const double CLR_LIGHTRED[] = { 1.0, 0.7, 0.7, 1.0 };
const double CLR_YELLOW[] = { 0.8, 0.8, 0.5, 1.0 };
const double CLR_BLUE[] = { 0.5, 0.5, 0.9, 1.0 };
static GLUquadric * quadric = gluNewQuadric();

enum E_TURN { TURN_RED, TURN_BLUE };
enum E_JUDGE { J_NONE, J_GREAT, J_GOOD, J_BAD };
enum E_DIREC { D_UP, D_DOWN, D_LEFT, D_RIGHT };
enum E_TYPE { T_NORMAL, T_REVERSE, T_FAST, T_SLOW };

// �ð�
int lastElapsedTime = 0;
int deltaTime = 0;
const int COUNTER_LIMIT = 16;  // ��ǥ FPS = 1000 �� 16 �� 60
int counter = 0;
int zoomFXTimer = 0;
const int ZOOM_FX_TIME = 1000;  // ms
int colorFXTimer = 0;
const int COLOR_FX_TIME = 1000;  // ms

// ī�޶�
double camAngle = 1.0;  // rad

// ����
int stage = 1;
E_TURN turn = TURN_RED;
bool hasSwapped = false;
const int DEFAULT_LIFE = 3;
int life = DEFAULT_LIFE;
bool hasReached = false;
bool hasDamaged = false;
bool isGameOver = false;
bool isGameClear = false;
int score = 0;
int highScore[4] = { 0, 0, 0, 0 };

// ��ġ �� ����
double posAX = 0.0, posAY = 0.0, posAZ = 0.0;
double posBX = SPH_DIST, posBY = 0.0, posBZ = 0.0;
double defaultAlpha;
double alphaDelta;
double sphereAngle = 0.0;
double sphereAlpha;
double sphereRotateDirection = -1.0;
double angleSinceSwap = 0.0;

// ����
double greatRange = 0.25;
double goodRange = 0.5;
E_JUDGE judge = J_NONE;

// ��
vector<E_DIREC> cells;
vector<E_TYPE> cellTypes;
int cellIdx = 0;
#pragma endregion GLOBAL

// �Լ� ����
void resetStage();
void startStage();
void returnToLobby();

// Reshape �ݹ�
void reshape(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	w < h ? glViewport(0, 0, w, w) : glViewport(0, 0, h, h);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

#pragma region DISPLAY
// ī�޶� ��ġ ����
void updateCam() {
	const double EYE_Y = -0.7;
	const double CAM_DIST = 0.3;
	double eyeX = CAM_DIST * cos(camAngle);
	double eyeZ = CAM_DIST * sin(camAngle);

	gluLookAt(eyeX, EYE_Y, eyeZ, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}
// ���� ����Ʈ ó��
void updateBG() {
	if (colorFXTimer > 0) {
		judge == J_BAD ?
			glClearColor(0.1f + 0.0001f * colorFXTimer, 0.1f, 0.1f, 1.0f)
			: glClearColor(0.1f, 0.1f + 0.0001f * colorFXTimer, 0.1f, 1.0f);
		colorFXTimer -= deltaTime;
	}
	else {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	}
}

// 3D
void drawItem(E_TYPE type, double a) {
	if (type == T_NORMAL) return;

	glPushMatrix(); {
		switch (type) {
		case T_REVERSE:
			glColor4d(0.5, 0.8, 0.5, a);
			break;
		case T_FAST:
			glColor4d(0.8, 0.5, 0.5, a);
			break;
		case T_SLOW:
			glColor4d(0.5, 0.5, 0.8, a);
			break;
		}
		gluSphere(quadric, SPH_R_SMALL, SLICE_LESS_N, SLICE_LESS_N);
	} glPopMatrix();
}
void drawArrow(E_DIREC direc, double a) {
	glPushMatrix(); {
		gluQuadricDrawStyle(quadric, GLU_LINE);
		switch (direc) {
		case D_UP:
			glTranslated(0.0, 0.0, 0.03);
			break;
		case D_RIGHT:
			glTranslated(0.03, 0.0, 0.0);
			glRotated(90.0, 0.0, 1.0, 0.0);
			break;
		case D_DOWN:
			glTranslated(0.0, 0.0, -0.03);
			glRotated(180.0, 0.0, 1.0, 0.0);
			break;
		case D_LEFT:
			glTranslated(-0.03, 0.0, 0.0);
			glRotated(270.0, 0.0, 1.0, 0.0);
			break;
		}
		gluCylinder(quadric, 0.03, 0.0, SPH_DIST_HALF, 4, SLICE_LESS_N);
	} glPopMatrix();
}
void drawWay() {
	static const int RENDER_MAX = 10;
	double brightness, red, green, blue, dist;
	glPushMatrix(); {
		int size = cells.size();
		int from = (cellIdx > RENDER_MAX) ? cellIdx - RENDER_MAX : 0;
		int to = (cellIdx + RENDER_MAX < size) ? cellIdx + RENDER_MAX : size;
		glTranslated(posAX, posAY, posAZ);

		for (int i = cellIdx; i < to; i++) {
			glColor4d(0.9, 0.9, 0.9, 0.5);
			glutWireCube(CUBE_SIZE);

			dist = i - cellIdx;
			red = 1.0 - dist * 0.05;
			green = 1.0 - abs(dist - RENDER_MAX / 2) * 0.1;
			blue = min(0.3 + dist * 0.1, 1.0);
			brightness = 0.9 - abs(dist) * 0.05;
			glColor4d(red, green, blue, brightness);
			drawArrow(cells[i], brightness);

			switch (cells[i]) {
			case D_RIGHT:
				glTranslated(SPH_DIST, 0.0, 0.0);
				break;
			case D_LEFT:
				glTranslated(-SPH_DIST, 0.0, 0.0);
				break;
			case D_UP:
				glTranslated(0.0, 0.0, SPH_DIST);
				break;
			case D_DOWN:
				glTranslated(0.0, 0.0, -SPH_DIST);
				break;
			}
			glColor4d(red, green, blue, brightness);
			glutWireCube(CUBE_SIZE);
			if (i >= cellIdx) drawItem(cellTypes[i], brightness);
		}
	} glPopMatrix();
}
void drawPlayer() {
	glPushMatrix(); {
		gluQuadricDrawStyle(quadric, GLU_FILL);
		glTranslated(posAX, posAY, posAZ);

		glColor3dv(turn == TURN_RED ? CLR_RED : CLR_BLUE);
		gluSphere(quadric, SPH_R, SLICE_N, SLICE_N);

		glPushMatrix(); {

			glTranslated(posBX, posBY, posBZ);

			glColor3dv(turn == TURN_RED ? CLR_BLUE : CLR_RED);
			gluSphere(quadric, SPH_R, SLICE_N, SLICE_N);
		} glPopMatrix();

	} glPopMatrix();
}
// UI
void drawCircle(double radius, int angle) {
	glPushMatrix(); {
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i <= angle; i++)  // �� �׸��� ���� ����
		{
			float angle = PI / 180.0f * i;
			//glVertex2f(0.1f * cos(angle), 0.1f * sin(angle));
			glVertex3d(0.0, 0.0, 0.0);
			glVertex3d(radius * cos(angle), 0.0, radius * sin(angle));
		}
		glEnd();
	} glPopMatrix();
}
void drawHeart() {
	glPushMatrix(); {
		glTranslated(0.0225, 0.0, 0.0);
		drawCircle(0.025, 360);
		glTranslated(-0.045, 0.0, 0.0);
		drawCircle(0.025, 360);
		glTranslated(0.0225, 0.0, -0.02);
		glBegin(GL_POLYGON);
		glVertex3d(0.0, 0.0, 0.02);
		glVertex3d(-0.039, 0.0, 0.0);
		glVertex3d(0.0, 0.0, -0.05);
		glVertex3d(0.039, 0.0, 0.0);
		glEnd();
	} glPopMatrix();
}
void drawJudgeText() {
	const unsigned char greatText[] = "GREAT!";
	const unsigned char goodText[] = "Good!";
	const unsigned char missText[] = "MISS!";
	if (colorFXTimer > 0) {
		glPushMatrix(); {
			glTranslated(0.0, 0.0, 0.3 - 0.0001*colorFXTimer);
			if (judge == J_GOOD) glColor3dv(CLR_YELLOW);
			else if (judge == J_BAD) glColor3dv(CLR_LIGHTRED);
			glRasterPos2d(0.0, 0.0);
			int len = 0;
			switch (judge) {
			case J_GREAT:
				len = strlen((const char *)greatText);
				for (int i = 0; i < len; i++) {
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, greatText[i]);
				}
				break;
			case J_GOOD:
				len = strlen((const char *)goodText);
				for (int i = 0; i < len; i++) {
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, goodText[i]);
				}
				break;
			case J_BAD:
				len = strlen((const char *)missText);
				for (int i = 0; i < len; i++) {
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, missText[i]);
				}
				break;
			}

		} glPopMatrix();
	}
}
void drawScoreText() {
	const unsigned char scoreText[] = "Score:";
	char buffer[33];
	_itoa_s(score, buffer, 10);
	glPushMatrix(); {
		glTranslated(-0.9, 0.0, 0.8);
		glColor3dv(CLR_WHITE);
		glRasterPos2d(0.0, 0.0);
		int len = 0;
		len = strlen((const char *)scoreText);
		for (int i = 0; i < len; i++) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, scoreText[i]);
		}
		glTranslated(0.15, 0.0, 0.0);
		glRasterPos2d(0.0, 0.0);
		len = strlen((const char *)buffer);
		for (int i = 0; i < len; i++) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (char)buffer[i]);
		}
	} glPopMatrix();
}
void drawGameOverText() {
	const unsigned char gameClearText[] = "GAME CLEAR !  (press R to retry)";
	const unsigned char gameOverText[] = "Game Over !  (press R to retry)";
	glPushMatrix(); {
		glTranslated(-0.3, 0.0, 0.8);
		glColor3dv(isGameClear ? CLR_WHITE : CLR_LIGHTRED);
		glRasterPos2d(0.0, 0.0);
		int len = strlen((const char *)(isGameClear ? gameClearText : gameOverText));
		for (int i = 0; i < len; i++) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, isGameClear ? gameClearText[i] : gameOverText[i]);
		}
	} glPopMatrix();
}
void drawUI() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluLookAt(0.0, -0.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushMatrix(); {
		glTranslated(-0.9, 0.0, 0.95);
		double scale = 1.0 + 0.0003 * zoomFXTimer;
		glScaled(scale, 0.0, scale);

		for (int i = 0; i < life; i++) {
			drawHeart();
			glTranslated(0.11, 0.0, 0.0);
		}
	} glPopMatrix();

	drawJudgeText();  // ���� �ؽ�Ʈ ǥ��
	drawScoreText();  // ���� �ؽ�Ʈ ǥ��
	if (isGameOver) drawGameOverText();  // ���� ���� �ؽ�Ʈ ǥ��
}

// Display �ݹ�
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	updateCam();  // ī�޶� ��ġ ����

	glPushMatrix(); {
		const double DEFAULT_ZOOM = 1.4;
		glScaled(DEFAULT_ZOOM, DEFAULT_ZOOM, DEFAULT_ZOOM);
		double scale = 1.0 + 0.0003 * zoomFXTimer;
		glScaled(scale, scale, scale);
		if (zoomFXTimer > 0) zoomFXTimer -= deltaTime;

		posBX = cos(sphereAngle) * SPH_DIST;
		posBZ = sin(sphereAngle) * SPH_DIST;
		double posX = posAX + posBX / 2.0;
		double posZ = posAZ + posBZ / 2.0;
		glTranslated(-posX, 0.0, -posZ);

		updateBG();  // ���� ����Ʈ ó��
		drawWay();
		drawPlayer();
	} glPopMatrix();

	glColor3dv(turn == TURN_RED ? CLR_BLUE : CLR_RED);
	drawUI();

	glutSwapBuffers();
}
#pragma endregion DISPLAY

// ���� Ŭ���� �� ���� ���� ó��
void gameClear() {
	isGameOver = true;
	isGameClear = true;
	zoomFXTimer = ZOOM_FX_TIME * 3;
	colorFXTimer = COLOR_FX_TIME * 3;
}
void gameOver() {
	isGameOver = true;
	highScore[stage] = score > highScore[stage] ? score : highScore[stage];
	cout << "����� ���� : " << score << '\n';
	cout << "�ְ����� : " << highScore[stage] << '\n';
}

// ��� ����
void damage() {
	if (hasDamaged) return;

	judge = J_BAD;
	life--;
	if (life == 0) gameOver();
	colorFXTimer = 1000;
	hasDamaged = true;
}
// Ÿ�̹� ��ħ ����
bool checkFail(E_DIREC direction) {
	switch (direction) {
	case D_RIGHT:
		if (sphereAngle >= TWO_PI - goodRange || sphereAngle <= goodRange) {
			hasReached = true;
			return false;
		}
		break;
	case D_UP:
		if (sphereAngle >= HALF_PI - goodRange && sphereAngle <= HALF_PI + goodRange) {
			hasReached = true;
			return false;
		}
		break;
	case D_LEFT:
		if (sphereAngle >= PI - goodRange && sphereAngle <= PI + goodRange) {
			hasReached = true;
			return false;
		}
		break;
	case D_DOWN:
		if (sphereAngle >= ONEHALF_PI - goodRange && sphereAngle <= ONEHALF_PI + goodRange) {
			hasReached = true;
			return false;
		}
		break;
	}
	if (angleSinceSwap < HALF_PI) hasReached = false;
	return hasReached;
}

// Idle �ݹ�
void idle() {
	int elapsedTime = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = elapsedTime - lastElapsedTime;
	lastElapsedTime = elapsedTime;
	counter += deltaTime;

	// UPDATE (�� 60 FPS)
	if (counter >= COUNTER_LIMIT) {
		counter %= COUNTER_LIMIT;

		glutPostRedisplay();
	}

	if (isGameOver) return;

	// ����
	double delta = sphereAlpha * deltaTime;
	sphereAngle += sphereRotateDirection * delta;
	angleSinceSwap += delta;
	// 0.0 <= sphereAngle < 2��
	if (sphereAngle >= TWO_PI) sphereAngle -= TWO_PI;
	else if (sphereAngle < 0.0) sphereAngle += TWO_PI;

	if (!hasDamaged && checkFail(cells[cellIdx]) && cellIdx != 0) damage();  // Ÿ�̹� ��ħ ����
}

// �����ϴ� �� ��ȯ
void swapSphere(E_DIREC direction) {
	if (hasSwapped) return;

	switch (direction) {
	case D_RIGHT:
		posAX += SPH_DIST;
		sphereAngle = PI;
		break;
	case D_UP:
		posAZ += SPH_DIST;
		sphereAngle = ONEHALF_PI;
		break;
	case D_LEFT:
		posAX -= SPH_DIST;
		sphereAngle = 0.0;
		break;
	case D_DOWN:
		posAZ -= SPH_DIST;
		sphereAngle = HALF_PI;
		break;
	}
	turn = (turn == TURN_RED) ? TURN_BLUE : TURN_RED;

	angleSinceSwap = 0.0;
	hasSwapped = true;
	hasReached = false;
	hasDamaged = false;
}
// Ÿ�̹� ����
E_JUDGE judgeTiming(E_DIREC direction) {
	switch (direction) {
	case D_RIGHT:
		if (sphereAngle >= TWO_PI - greatRange || sphereAngle <= greatRange) return J_GREAT;
		else if (sphereAngle >= TWO_PI - goodRange || sphereAngle <= goodRange) return J_GOOD;
		break;
	case D_UP:
		if (sphereAngle >= HALF_PI - greatRange && sphereAngle <= HALF_PI + greatRange) return J_GREAT;
		else if (sphereAngle >= HALF_PI - goodRange && sphereAngle <= HALF_PI + goodRange) return J_GOOD;
		break;
	case D_LEFT:
		if (sphereAngle >= PI - greatRange && sphereAngle <= PI + greatRange) return J_GREAT;
		else if (sphereAngle >= PI - goodRange && sphereAngle <= PI + goodRange) return J_GOOD;
		break;
	case D_DOWN:
		if (sphereAngle >= ONEHALF_PI - greatRange && sphereAngle <= ONEHALF_PI + greatRange) return J_GREAT;
		else if (sphereAngle >= ONEHALF_PI - goodRange && sphereAngle <= ONEHALF_PI + goodRange) return J_GOOD;
		break;
	}

	return J_BAD;
}
// Ư��(������) ĭ ó��
void procSpecial() {
	switch (cellTypes[cellIdx]) {
	case T_REVERSE:  // ȸ�� ���� �ݴ�� (�ʷ�)
		sphereRotateDirection *= -1.0;
		break;
	case T_FAST:  // ������ (����)
		sphereAlpha += alphaDelta;
		greatRange *= 2.0;
		goodRange *= 2.0;
		break;
	case T_SLOW:  // ������ (�Ķ�)
		sphereAlpha -= alphaDelta;
		greatRange *= 0.5;
		goodRange *= 0.5;
		break;
	}
}
void addScore() {
	score += judge == J_GREAT ? 700 : judge == J_GOOD ? 300 : 0;
}
// �� ��ȯ �õ�
void tryToSwap() {
	E_DIREC direction = cells[cellIdx];
	judge = judgeTiming(direction);
	if (judge != J_BAD) {
		swapSphere(direction);
		procSpecial();

		cellIdx++;
		if (cellIdx == cells.size()) {
			gameClear();
			return;
		}

		// ����Ʈ Ÿ�̸� ����
		zoomFXTimer = ZOOM_FX_TIME;
		colorFXTimer = COLOR_FX_TIME;
		addScore();  // ���� �ݿ�
	}
	else {
		damage();
	}
}

// Keyboard �ݹ�
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case KEY_SPACE:
		if (isGameOver) break;
		tryToSwap();
		break;
	case 'r': case 'R':
		resetStage();
		break;
	case KEY_ESC:
		returnToLobby();
	}
}
void keyboardUp(unsigned char key, int x, int y) {
	switch (key) {
	case KEY_SPACE:
		hasSwapped = false;
		break;
	}
}

// �������� ���� ȭ�� �ݹ�
void displayLobby() {
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	gluLookAt(0.0, -0.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushMatrix(); {
		const unsigned char selectStageText[] = "Choose Stage";
		const unsigned char easyText[] = "Easy";
		const unsigned char normalText[] = "Normal";
		const unsigned char hardText[] = "Hard";
		glPushMatrix(); {
			glTranslated(-0.2, 0.0, 0.5);
			glColor3dv(CLR_WHITE);
			glRasterPos2d(0.0, 0.0);
			int len = strlen((const char *)selectStageText);
			for (int i = 0; i < len; i++) {
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, selectStageText[i]);
			}
			glTranslated(-0.3, 0.0, -0.2);
			glColor3dv(CLR_WHITE);
			glRasterPos2d(0.0, 0.0);
			len = strlen((const char *)easyText);
			for (int i = 0; i < len; i++) {
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, easyText[i]);
			}
			if (stage == 1) {
				glTranslated(0.05, 0.0, -0.15);
				drawArrow(D_UP, 1.0);
				glTranslated(0.05, 0.0, 0.15);
			}
			glTranslated(0.3, 0.0, 0.0);
			glColor3dv(CLR_WHITE);
			glRasterPos2d(0.0, 0.0);
			len = strlen((const char *)normalText);
			for (int i = 0; i < len; i++) {
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, normalText[i]);
			}
			if (stage == 2) {
				glTranslated(0.05, 0.0, -0.15);
				drawArrow(D_UP, 1.0);
				glTranslated(0.05, 0.0, 0.15);
			}
			glTranslated(0.3, 0.0, 0.0);
			glColor3dv(CLR_WHITE);
			glRasterPos2d(0.0, 0.0);
			len = strlen((const char *)hardText);
			for (int i = 0; i < len; i++) {
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, hardText[i]);
			}
			if (stage == 3) {
				glTranslated(0.05, 0.0, -0.15);
				drawArrow(D_UP, 1.0);
				glTranslated(0.05, 0.0, 0.15);
			}
		} glPopMatrix();
	} glPopMatrix();

	glutSwapBuffers();
}
void keyboardLobby(unsigned char key, int x, int y) {
	switch (key) {
	case 'a': case 'A':
		stage--;
		if (stage == 0) stage = 3;
		break;
	case 'd': case 'D':
		stage++;
		if (stage > 3) stage = 1;
		break;
	case KEY_SPACE:
		startStage();
		break;
	case KEY_ESC:
		exit(0);
	}

	glutPostRedisplay();
}

// �� ���� �ҷ�����
void readMapFile() {
	char str[128];
	char c;
	ifstream inFile(stage == 1 ? "map01.txt" : stage == 2 ? "map02.txt" : "map03.txt");

	inFile.getline(str, 128);
	defaultAlpha = atof(str);
	alphaDelta = defaultAlpha * 0.5;

	cells.clear();
	cellTypes.clear();

	while (!inFile.eof()) {
		c = inFile.get();
		switch (c) {
		case 'U':
			cells.push_back(D_UP);
			break;
		case 'D':
			cells.push_back(D_DOWN);
			break;
		case 'L':
			cells.push_back(D_LEFT);
			break;
		case 'R':
			cells.push_back(D_RIGHT);
			break;
		}
		c = inFile.get();
		switch (c) {
		case 'N':
			cellTypes.push_back(T_NORMAL);
			break;
		case 'R':
			cellTypes.push_back(T_REVERSE);
			break;
		case 'F':
			cellTypes.push_back(T_FAST);
			break;
		case 'S':
			cellTypes.push_back(T_SLOW);
			break;
		}
		c = inFile.get();
	}

	inFile.close();
}
// �������� �ʱ�ȭ
void resetStage() {
	posAX = 0.0;
	posAZ = 0.0;
	posBX = SPH_DIST;
	posBZ = 0.0;
	sphereAngle = 0.0;
	sphereRotateDirection = -1.0;
	angleSinceSwap = 0.0;
	sphereAlpha = defaultAlpha;
	turn = TURN_RED;
	judge = J_NONE;
	life = DEFAULT_LIFE;
	hasReached = false;
	hasDamaged = false;
	cellIdx = 0;
	score = 0;
	isGameClear = false;
	zoomFXTimer = 0;
	colorFXTimer = 0;

	isGameOver = false;
}
// �������� ���� �� �ݹ� ��ȯ
void startStage() {
	readMapFile();
	resetStage();
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	updateCam();  // ī�޶� ��ġ ����
}
// �������� ���� ȭ������ ����
void returnToLobby() {
	resetStage();
	glutDisplayFunc(displayLobby);
	glutIdleFunc(NULL);
	glutKeyboardFunc(keyboardLobby);
	glutKeyboardUpFunc(NULL);

	glutPostRedisplay();
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(900, 10);
	glutInitWindowSize(1000, 1000);
	glutCreateWindow("Duetrip");

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);  // ���� ��� ����
	glFrontFace(GL_CCW);
	gluQuadricDrawStyle(quadric, GLU_LINE);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClearDepth(1.0f);
	gluPerspective(60, 1.0, 1.0, 30.0);

	glutReshapeFunc(reshape);
	glutDisplayFunc(displayLobby);
	glutKeyboardFunc(keyboardLobby);

	resetStage();
	glutMainLoop();

	return(0);
}
