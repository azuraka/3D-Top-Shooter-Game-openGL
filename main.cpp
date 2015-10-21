#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "imageloader.h"
#include "vec3f.h"
#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)
using namespace std;

float angle=0.0;
float angle_axis=0.0;
float angular_vel=20.0;
float angular_vel_axis=0.0;
float angle_bent = 0.0;
float reduce = 0.06;
float x = 0.0;
float y = 0.0;
float a = 0.0;
float b = 0.0;
float target_x = 0.0;
float target_y = 0.0;
float power_box_height = 5.0;
float arrow_angle = 90.0;
float cam = 1;
float flag = 0;
float launched = 0;
float target = 0;
float collision = 0;
int score = 10;


//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) {
			w = w2;
			l = l2;
			
			hs = new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
			}
			
			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}
			
			computedNormals = false;
		}
		
		~Terrain() {
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
			}
			delete[] hs;
			
			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}
		
		int width() {
			return w;
		}
		
		int length() {
			return l;
		}
		
		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];
					
					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
	};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
	Terrain* loadTerrain(const char* filename, float height) {
		Image* image = loadBMP(filename);
		Terrain* t = new Terrain(image->width, image->height);
		for(int y = 0; y < image->height; y++) {
			for(int x = 0; x < image->width; x++) {
				unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
				float h = height * ((color / 255.0f) - 0.5f);
				t->setHeight(x, y, h);
			}
		}

		delete image;
		t->computeNormals();
		return t;
	}

	float _angle = 225.0f;
	Terrain* _terrain;

	void cleanup() {
		delete _terrain;
	}

	void handleKeypress1(unsigned char key, int x, int y) {
		switch (key) {
		case 27: //Escape key
		cleanup();
		exit(0);

		case '1':{
			cam = 1;
			break;
		}

		case '2':{
			cam = 2;
			break;
		}

		case '3':{
			cam = 3;
			break;
		}

		case '4':{
			cam = 4;
			break;
		}

		case '5':{
			cam = 5;
			break;
		}
		case 'a':{
			_angle -= 0.9f;
			break;
		}
		case 'd':{
			_angle += 0.9f;
			break;
		}
		case 32:{
			launched=1;
			break;
		}
	}
}

void handleKeypress2(int key, int x, int y) {

	if (key == GLUT_KEY_UP && launched==0){
		if(power_box_height<=9.5)
			power_box_height+=0.5;
		else
			power_box_height=10.0;
	}
	if (key == GLUT_KEY_DOWN && launched==0){
		if(power_box_height>=0.5)
			power_box_height-=0.5;
		else
			power_box_height=0.0;
	}
	if (key == GLUT_KEY_LEFT && launched==0 && arrow_angle>=90.0){
		arrow_angle -=10.0;
	}
	if (key == GLUT_KEY_RIGHT && launched==0 && arrow_angle<=180.0){
		arrow_angle +=10.0;
	}
}

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}

void drawText(char text[], int length, int x, int y){
	glMatrixMode(GL_PROJECTION);
	double *matrix = new double[16];
	glGetDoublev(GL_PROJECTION_MATRIX, matrix);
	glLoadIdentity();
	glOrtho(0,800,0,600,-5,5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glLoadIdentity();
	glRasterPos2i(x,y);
	for(int i=0;i<length;i++){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)text[i]);
	}
	if (!strcmp(text, "Score: ")){
		if(score>0){
			int temp=score;
			int factor=10;
			while(temp/factor>10)
				factor=factor*10;
			while(factor!=0){
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, temp/factor+48);
				temp=temp%factor;
				factor=factor/10;	
			}
		}
	else
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 0+48);
	}
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(matrix);
	glMatrixMode(GL_MODELVIEW);
}


void drawScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(cam==1){                 // Player view
		glTranslatef(0.0f, 0.0f, -10.0f);
		glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(-_angle, 0.0f, 1.0f, 0.0f);
	}


	if(cam==2){                // top view
		gluLookAt(
			-3.0+a*0.08,  1.0+_terrain->getHeight(a, b)*0.08,  -3.0+b*0.08,
			target_x, -1.0, target_y,     // Direction of camera
			0.0,    10.0,    0.0);   // perpendicular direction
	}

	else if(cam==3){				   // overhead view
		gluLookAt(
			0.0,  12.0,  0.0,
			0.0, -15.0, 0.0,     // Direction of camera
			10.0,    0.0,    10.0);   // perpendicular direction
	}

	else if(cam==4){					// helicopter cam
		gluLookAt(
			-4.0+a*0.08,  1.5+_terrain->getHeight(a, b)*0.08,  -4.0+b*0.08,
			10.0, -2.0, 10.0,     // Direction of camera
			0.0,    10.0,    0.0);   // perpendicular direction
	}

	else if(cam==5){	              // Follow cam
		gluLookAt(
			-4.0+a*0.08,  1.5+_terrain->getHeight(a, b)*0.08,  -4.0+b*0.08,
			10.0, -2.0, 10.0,     // Direction of camera
			0.0,    10.0,    0.0);   // perpendicular direction
	}

	GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
	
	GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
	GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	
	float scale = 5.0f / max(_terrain->width() - 1, _terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float)(_terrain->width() - 1) / 2,
		0.0f,
		-(float)(_terrain->length() - 1) / 2);
	
	glColor3f(1.3f, 0.9f, 0.0f);
	for(int z = 0; z < _terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for(int x = 0; x < _terrain->width(); x++) {
			Vec3f normal = _terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z), z);
			normal = _terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, _terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

	
	// Draw Top
	glColor3f(1.0, 0.5, 0.5); 
	glPushMatrix();
	glTranslatef(a, _terrain->getHeight(a, b),b);
	glRotatef(angle_axis, 0.0, 1.0, 0.0);
	glRotatef(-angle_bent, 1.0, 0.0, 0.0);
	glTranslatef(x,3.5,y);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glRotatef(angle, 0.0, 0.0, 1.0);
	glutSolidCone(0.7, 3.5, 150, 30);
	
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glutSolidCone(0.5, 2.5, 150, 30);
	glRotatef(-90.0, 0.0, 1.0, 0.0);
	glutSolidCone(0.5, 2.5, 150, 30);
	glRotatef(-90.0, 0.0, 1.0, 0.0);
	glutSolidCone(0.5, 2.5, 150, 30);
	glRotatef(-90.0, 0.0, 1.0, 0.0);
	glutSolidCone(0.5, 2.5, 150, 30);
	glPopMatrix();

	//Draw Target
	
	glPushMatrix();
	if(target==0){
		target = 1;
		target_x = rand()% _terrain->length();
		target_y = rand()% _terrain->width();
	}
	glTranslatef(target_x,_terrain->getHeight(target_x,target_y)+5.0,target_y);
	glRotatef(45,0,1,0);
	glColor3f(1.0, 0.0, 1.0);
	glutSolidTorus( 0.2, 4.5, 25, 15);
	//glColor3f(0.0, 0.7, 1.0);
	//glutWireTorus( 5.0, 5.0, 20, 20);
	glPopMatrix();

	// Draw Power Box
	glPushMatrix();
	glColor3f(0.5, 0.5, 0.5);
	glTranslatef(50.0,5.0,-10.0);
	glRotatef(10.0,1.0,0.0,0.0);
	glScalef(0.5, 10.0 , 0.5);
	glutWireCube(3.0);
	glPopMatrix();

	// Draw Power
	glPushMatrix();
	glColor3f(0.8, 1.0, 0.5);
	glTranslatef(50.0,5.0,-10.0);
	glRotatef(10.0,1.0,0.0,0.0);
	glScalef(0.5, power_box_height , 0.5);
	glutSolidCube(3.0);
	glPopMatrix();

	// Draw Arrow
	if(launched==0){
		glPushMatrix();
		glColor3f(0.8, 0.8, 0.8);
		//glTranslatef(0.3,-4.3,3.2);
		glRotatef(90.0, 1.0, 0.0, 0.0);
		glRotatef(arrow_angle, 0.0, 0.0, 1.0);
		glTranslatef(0.3,-3.0,1.0);
		glScalef(0.1, 2.0 , 0.1);
		glutSolidCube(3.0);
		glPopMatrix();		
	}

	char text2[50]="Cam1 (Press 1): User View";
	int len = strlen(text2);
	drawText(text2,len,50,500);

	char text3[50]="Cam2 (Press 2): Top View";
	len = strlen(text3);
	drawText(text3,len,50,480);

	char text4[50]="Cam3 (Press 3): Overhead View";
	len = strlen(text4);
	drawText(text4,len,50,460);

	char text5[50]="Cam4 (Press 4): Helicopter Cam";
	len = strlen(text5);
	drawText(text5,len,50,440);

	char text6[50]="Cam5 (Press 5): Follow Cam";
	len = strlen(text6);
	drawText(text6,len,50,420);


	char text7[50]="Score: ";
	len = strlen(text7);
	drawText(text7,len,50,50);

	glutSwapBuffers();
}

void update(int value) {
	//_angle += 0.3f;
	if(launched==1){
		angle += angular_vel;
		if(flag==0)
			angle_axis += angular_vel_axis;
		else if(angular_vel_axis>0){
			angle_axis -= angular_vel_axis;
			angular_vel_axis -= 1.8*reduce;
		}
		angular_vel -= reduce/1.5;
		if(angular_vel_axis<5)
			angular_vel_axis +=reduce;
		if(angle_bent<65)
			angle_bent += reduce*5;
		else if (flag!=1){
			flag=1;
			angular_vel = 0;
		}
		if(a<_terrain->length()-1 && b<_terrain->width()-1 && flag==0){
			arrow_angle = arrow_angle;
			a= a + power_box_height*0.08*fabs(sin(DEG2RAD(arrow_angle)));
			b= b + power_box_height*0.08*fabs(cos(DEG2RAD(arrow_angle)));
			if(sqrt((target_x-a)*(target_x-a)+(target_y-b)*(target_y-b))<3.0){
				collision = 1;
				flag=0;
				target=0;
				launched=0;
				score+=10;
				a=0.0;
				b=0.0;
				angle=0.0;
				angle_axis=0.0;
				angular_vel=25.0;
				angular_vel_axis=0.0;
				angle_bent = 0.0;
			}
		}
		else if(flag==1 && fabs(angular_vel_axis)<0.001){
			flag=0;
			launched=0;
			target=0;
			score-=1;
			a=0.0;
			b=0.0;
			angle=0.0;
			angle_axis=0.0;
			angular_vel=25.0;
			angular_vel_axis=0.0;
			angle_bent = 0.0;
		}
		if (_angle > 360) {
			_angle -= 360;
		}
		if (angular_vel > 360) {
			angular_vel -= 360;
		}
		if (angular_vel < 0) {
			angular_vel = 0;
		}
	}
	glutPostRedisplay();
	glutTimerFunc(25, update, 0);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1200, 1200);
	
	glutCreateWindow("Top Shooter");
	initRendering();
	
	_terrain = loadTerrain("heightmap.bmp", 20);
	
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress1);
	glutSpecialFunc(handleKeypress2);
	glutReshapeFunc(handleResize);
	glutTimerFunc(25, update, 0);
	
	glutMainLoop();
	return 0;
}
