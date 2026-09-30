/* =====================================================================
   DHAKA CITY 3D  -  Computer Graphics & Animation Course Project
   C++ / OpenGL (GLUT / freeglut) - Code::Blocks + MinGW ready
   =====================================================================
   Features implemented (mapped to course requirements):
   2. Full 3D scene (buildings, road, park, sky, water)
   3. Keyboard + mouse controlled Rotation / Translation / Scaling
   4. Complex compound objects (buildings, car, windmill, traffic light,
      trees, framed window)
   5. Continuously rotating object -> windmill blades
   6. Framed window on the Mall wall reveals the outside park/sky (glass,
      alpha-blended) -> external environment visible within composition
   7. Two light sources (Sun/Moon directional + Street Lamp point light)
      with full ambient/diffuse/specular material setup on every object
   8. Separate OBJECT transform (Mall building - move/rotate/scale) and
      VIEW transform (free-fly camera via gluLookAt)
   9. Procedurally generated textures: brick, wood, road asphalt, grass,
      glass, water  (no external image files needed -> portable build)
   Extra: day / night cycle, car driving and obeying a traffic signal,
   footpath, crosswalk, on-screen HUD.
   =====================================================================
   OBJECT INDEX  (each major scene object has its own function, tagged
   with an OID comment right above its definition, e.g. ///OID-1)
   ---------------------------------------------------------------------
    OID-1  : sky()            - sky dome + sun/moon
    OID-2  : cloud()          - puffy drifting clouds
    OID-3  : road()           - road, footpath, lane markings, crosswalk
    OID-4  : building()       - generic textured building + windows
    OID-5  : framedWindow()   - wood-framed glass window (view-through)
    OID-6  : tree()           - park tree
    OID-7  : windmill()       - continuously rotating windmill
    OID-8  : car()            - car (drives, obeys traffic signal)
    OID-9  : trafficLight()   - 3-state traffic signal
    OID-10 : streetLamp()     - street lamp (visual source of Light1)
    OID-11 : slide()          - playground slide
    OID-12 : swing()          - playground swing (pendulum animation)
    OID-13 : park()           - assembles trees/pond/bench/slide/swing
    OID-14 : metroViaduct()   - elevated MRT piers, deck and rails
    OID-15 : metroTrain()     - moving metro train on the viaduct
    OID-16 : metroStation()   - elevated station platform with roof
    OID-17 : citySkyline()    - extra procedural buildings filling the block
    OID-18 : cityTraffic()    - the full multi-car fleet (was single car)
    OID-19 : pedestrians()    - people walking back and forth on footpaths
   ===================================================================== */

#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// ---------------------------------------------------------------------
//  GLOBAL STATE
// ---------------------------------------------------------------------
int   winW = 1280, winH = 720;

// ----- VIEW (viewing-coordinate) transform : free-fly camera ---------
float camX = 0.0f, camY = 9.0f, camZ = 28.0f;
float camYaw = -100.0f;     // degrees, rotation around Y
float camPitch = -16.0f;    // degrees, up/down look
bool  mouseDown = false;
int   lastMouseX = 0, lastMouseY = 0;

// ----- OBJECT (object-coordinate) transform : applied to the Mall ----
float objX = 13.0f, objY = 0.0f, objZ = -6.0f;
float objRotY = 0.0f;
float objScale = 1.0f;

// ----- Day / Night cycle ----------------------------------------------
float dayFactor = 1.0f;      // 1 = full day, 0 = full night
bool  autoCycle = true;
float cycleClock = 0.0f;     // seconds
const float CYCLE_LENGTH = 40.0f;   // seconds per full day/night loop

// ----- Continuously rotating object : windmill ------------------------
float windmillAngle = 0.0f;

// ----- Cars & traffic signal --------------------------------------------
// Multiple cars now share the road: three drive on the near lane
// (x=-4.4, +Z direction) and two on the far lane (x=+4.4, -Z direction),
// each with its own colour and slight speed variation, all obeying the
// same shared traffic signal at the stop line.
const int   NUM_CARS = 5;
float       carZ[NUM_CARS]         = { -55.0f, -30.0f, -10.0f,  50.0f,  25.0f };
const float carLaneX[NUM_CARS]     = {  -4.4f,   -4.4f,   -4.4f,   4.4f,   4.4f };
const float carDir[NUM_CARS]       = {   1.0f,    1.0f,    1.0f,  -1.0f,  -1.0f };
const float carSpeedMul[NUM_CARS]  = {   1.0f,   0.85f,    1.1f,   0.9f,   1.0f };
const float carColor[NUM_CARS][3]  = { {0.75f,0.05f,0.05f}, {0.05f,0.15f,0.55f},
                                        {0.85f,0.75f,0.05f}, {0.10f,0.40f,0.15f},
                                        {0.50f,0.50f,0.55f} };
bool        carStopped[NUM_CARS]   = { false, false, false, false, false };
const float CAR_SPEED   = 9.0f;
const float STOP_LINE_Z = -3.0f;

int   trafficState = 1;      // 0 = RED, 1 = GREEN, 2 = YELLOW
float trafficClock = 0.0f;
const float GREEN_TIME = 6.0f, YELLOW_TIME = 1.5f, RED_TIME = 5.0f;

// ----- Metro rail (elevated MRT-6 style line above the road median) ----
float metroZ = -75.0f;             // train position along the viaduct
const float METRO_SPEED = 17.0f;   // faster than road traffic
const float METRO_DECK_Y = 9.5f;   // viaduct deck height above ground

// ----- Sun / Moon position (simple arc) --------------------------------
float sunAngle = 200.0f;
const float CEL_RADIUS = 55.0f;   // orbit radius of sun/moon

// ----- General animation clock (always runs, even if day/night frozen) --
float animClock = 0.0f;
float swingAngle = 0.0f;   // pendulum angle for the park swing (degrees)

// ----- "Viewing scenario" : fixed camera inside the Apartments, looking
// out through its window - demonstrates a second, distinct viewing
// transformation separate from the free-fly outdoor camera. -----------
bool  insideView = false;
const float APT_X = 11.0f, APT_Z = -14.0f, APT_EYE_Y = 4.0f;
const float APT_EYE_Z = -12.0f;  // just inside the front (+z) wall
float savedCamX, savedCamY, savedCamZ, savedYaw, savedPitch;

// Camera preset mode: 0=free, 1=overview, 2=inside apartment,
// 3=street level, 4=metro view, 5=high city view.
int cameraPreset = 0;

void setCameraPreset(int preset)
{
    cameraPreset = preset;
    insideView = false;

    switch (preset){
        case 1: // Wide city overview
            camX = 0.0f; camY = 22.0f; camZ = 42.0f;
            camYaw = -90.0f; camPitch = -18.0f;
            break;

        case 2: // Inside Apartments, looking through the window
            insideView = true;
            camX = APT_X; camY = APT_EYE_Y; camZ = APT_EYE_Z;
            camYaw = 90.0f; camPitch = 0.0f;
            break;

        case 3: // Human/street-level view
            camX = 0.0f; camY = 2.2f; camZ = 20.0f;
            camYaw = -90.0f; camPitch = -3.0f;
            break;

        case 4: // Elevated metro view
            camX = 0.0f; camY = 14.0f; camZ = 28.0f;
            camYaw = -90.0f; camPitch = -12.0f;
            break;

        case 5: // High panoramic city view
            camX = 26.0f; camY = 25.0f; camZ = 28.0f;
            camYaw = -135.0f; camPitch = -24.0f;
            break;

        default: // Original/free camera
            camX = 0.0f; camY = 9.0f; camZ = 28.0f;
            camYaw = -100.0f; camPitch = -16.0f;
            break;
    }
}

// ----- Textures ----------------------------------------------------------
GLuint texBrick, texWood, texRoad, texGrass, texGlass, texWater, texRoof, texConcrete;

int lastTimeMs = 0;

// forward declaration (defined near the lighting section below)
void computeCelestial(float &sunX,float &sunY,float &moonX,float &moonY);

// =======================================================================
//  PROCEDURAL TEXTURE GENERATION  (no external image files required)
// =======================================================================
GLuint uploadTexture(unsigned char *data, int w, int h, bool alpha=false)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLenum fmt = alpha ? GL_RGBA : GL_RGB;
    gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, w, h, fmt, GL_UNSIGNED_BYTE, data);
    return id;
}

GLuint genBrickTexture()
{
    const int N = 64;
    unsigned char img[N*N*3];
    for (int y=0; y<N; y++){
        for (int x=0; x<N; x++){
            int by = y % 8;
            int rowOffset = ((y/8) % 2) * 8;
            int bxo = (x + rowOffset) % 16;
            bool mortar = (bxo < 1) || (by < 1);
            int idx = (y*N+x)*3;
            if (mortar){ img[idx]=200; img[idx+1]=195; img[idx+2]=185; }
            else {
                int shade = 150 + (rand()%25) - 12;
                img[idx]=(unsigned char)(shade+30);
                img[idx+1]=(unsigned char)(shade-40);
                img[idx+2]=(unsigned char)(shade-55);
            }
        }
    }
    return uploadTexture(img, N, N);
}

GLuint genWoodTexture()
{
    const int N = 64;
    unsigned char img[N*N*3];
    for (int y=0; y<N; y++){
        for (int x=0; x<N; x++){
            float stripe = 0.5f + 0.5f*sinf(x*0.9f + sinf(y*0.15f)*2.0f);
            int base = 110 + (int)(stripe*60);
            int noise = rand()%10;
            int idx=(y*N+x)*3;
            img[idx]   = (unsigned char)(base+40+noise);
            img[idx+1] = (unsigned char)(base-10+noise);
            img[idx+2] = (unsigned char)(base-55+noise);
        }
    }
    return uploadTexture(img, N, N);
}

GLuint genRoadTexture()
{
    const int N = 64;
    unsigned char img[N*N*3];
    for (int y=0; y<N; y++){
        for (int x=0; x<N; x++){
            int shade = 100 + (rand()%18);
            int idx=(y*N+x)*3;
            img[idx]=img[idx+1]=img[idx+2]=(unsigned char)shade;
        }
    }
    return uploadTexture(img, N, N);
}

GLuint genGrassTexture()
{
    const int N = 64;
    unsigned char img[N*N*3];
    for (int y=0; y<N; y++){
        for (int x=0; x<N; x++){
            int g = 155 + (rand()%55);
            int idx=(y*N+x)*3;
            img[idx]=(unsigned char)(g*0.35f);
            img[idx+1]=(unsigned char)g;
            img[idx+2]=(unsigned char)(g*0.3f);
        }
    }
    return uploadTexture(img, N, N);
}

GLuint genWaterTexture()
{
    const int N = 64;
    unsigned char img[N*N*3];
    for (int y=0; y<N; y++){
        for (int x=0; x<N; x++){
            float wave = 0.5f+0.5f*sinf(x*0.5f+y*0.3f);
            int idx=(y*N+x)*3;
            img[idx]=(unsigned char)(30+wave*20);
            img[idx+1]=(unsigned char)(110+wave*40);
            img[idx+2]=(unsigned char)(180+wave*50);
        }
    }
    return uploadTexture(img, N, N);
}

GLuint genGlassTexture()
{
    const int N = 32;
    unsigned char img[N*N*4];
    for (int y=0; y<N; y++){
        for (int x=0; x<N; x++){
            int idx=(y*N+x)*4;
            img[idx]=180; img[idx+1]=225; img[idx+2]=240; img[idx+3]=120;
        }
    }
    return uploadTexture(img, N, N, true);
}

GLuint genRoofTexture()
{
    const int N=32;
    unsigned char img[N*N*3];
    for(int y=0;y<N;y++) for(int x=0;x<N;x++){
        int idx=(y*N+x)*3;
        int shade=90+(rand()%20);
        img[idx]=(unsigned char)shade; img[idx+1]=(unsigned char)(shade*0.55f); img[idx+2]=(unsigned char)(shade*0.45f);
    }
    return uploadTexture(img,N,N);
}

GLuint genConcreteTexture()
{
    const int N=32;
    unsigned char img[N*N*3];
    for(int y=0;y<N;y++) for(int x=0;x<N;x++){
        int idx=(y*N+x)*3;
        int shade=150+(rand()%25);
        img[idx]=img[idx+1]=img[idx+2]=(unsigned char)shade;
    }
    return uploadTexture(img,N,N);
}

// =======================================================================
//  MATERIAL HELPER
// =======================================================================
void setMaterial(float ar,float ag,float ab,
                  float dr,float dg,float db,
                  float sr,float sg,float sb,
                  float shininess, float alpha=1.0f)
{
    GLfloat amb[]={ar,ag,ab,alpha};
    GLfloat dif[]={dr,dg,db,alpha};
    GLfloat spec[]={sr,sg,sb,alpha};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void setEmissive(float r,float g,float b)
{
    GLfloat em[]={r,g,b,1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
}
void clearEmissive(){ GLfloat z[]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z); }

// =======================================================================
//  PRIMITIVE DRAWING
// =======================================================================
// Axis-aligned box centred at origin, base sits on local y=0 plane is NOT
// assumed here (call sites translate as needed). w,h,d = full size.
void drawBox(float w, float h, float d, GLuint tex=0, float ru=1.0f, float rv=1.0f, bool skipFront=false)
{
    float x=w*0.5f, y=h*0.5f, z=d*0.5f;
    if (tex){ glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);}
    else glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
      // front (+z) - skipped when this box needs a real window opening
      // cut into it (see drawFrontWallWithHole below), so the wall isn't
      // solid geometry blocking the view through that opening
      if (!skipFront){
      glNormal3f(0,0,1);
      glTexCoord2f(0,0);   glVertex3f(-x,-y, z);
      glTexCoord2f(ru,0);  glVertex3f( x,-y, z);
      glTexCoord2f(ru,rv); glVertex3f( x, y, z);
      glTexCoord2f(0,rv);  glVertex3f(-x, y, z);
      }
      // back (-z)
      glNormal3f(0,0,-1);
      glTexCoord2f(0,0);   glVertex3f( x,-y,-z);
      glTexCoord2f(ru,0);  glVertex3f(-x,-y,-z);
      glTexCoord2f(ru,rv); glVertex3f(-x, y,-z);
      glTexCoord2f(0,rv);  glVertex3f( x, y,-z);
      // left (-x)
      glNormal3f(-1,0,0);
      glTexCoord2f(0,0);   glVertex3f(-x,-y,-z);
      glTexCoord2f(ru,0);  glVertex3f(-x,-y, z);
      glTexCoord2f(ru,rv); glVertex3f(-x, y, z);
      glTexCoord2f(0,rv);  glVertex3f(-x, y,-z);
      // right (+x)
      glNormal3f(1,0,0);
      glTexCoord2f(0,0);   glVertex3f( x,-y, z);
      glTexCoord2f(ru,0);  glVertex3f( x,-y,-z);
      glTexCoord2f(ru,rv); glVertex3f( x, y,-z);
      glTexCoord2f(0,rv);  glVertex3f( x, y, z);
      // top (+y)
      glNormal3f(0,1,0);
      glTexCoord2f(0,0);   glVertex3f(-x, y, z);
      glTexCoord2f(ru,0);  glVertex3f( x, y, z);
      glTexCoord2f(ru,rv); glVertex3f( x, y,-z);
      glTexCoord2f(0,rv);  glVertex3f(-x, y,-z);
      // bottom (-y)
      glNormal3f(0,-1,0);
      glTexCoord2f(0,0);   glVertex3f(-x,-y,-z);
      glTexCoord2f(ru,0);  glVertex3f( x,-y,-z);
      glTexCoord2f(ru,rv); glVertex3f( x,-y, z);
      glTexCoord2f(0,rv);  glVertex3f(-x,-y, z);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// Draws only the front (+z) face of a w x h box as a picture-frame-shaped
// wall (4 strips) with a genuine rectangular hole left empty in the
// middle - unlike a flat glass panel glued onto a solid wall, nothing is
// drawn in the hole itself, so camera rays (and the eye, in the "inside
// the apartment" viewing scenario) can pass straight through it.
void drawFrontWallWithHole(float w, float h, float holeW, float holeH, float holeCenterY,
                            GLuint tex, float ru, float rv, float z)
{
    float X=w*0.5f, Y=h*0.5f;
    float hw=holeW*0.5f;
    float hy0=holeCenterY-holeH*0.5f, hy1=holeCenterY+holeH*0.5f;
    if (hy0 < -Y) hy0=-Y;
    if (hy1 > Y) hy1=Y;
    if (tex){ glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);}
    else glDisable(GL_TEXTURE_2D);

    auto tv = [&](float ylocal){ return (ylocal+Y)/h*rv; };
    auto tu = [&](float xlocal){ return (xlocal+X)/w*ru; };

    glBegin(GL_QUADS);
      glNormal3f(0,0,1);
      // top strip (hy1 .. Y), full width
      glTexCoord2f(0,tv(hy1));  glVertex3f(-X,hy1,z);
      glTexCoord2f(ru,tv(hy1)); glVertex3f( X,hy1,z);
      glTexCoord2f(ru,rv);      glVertex3f( X, Y,z);
      glTexCoord2f(0,rv);       glVertex3f(-X, Y,z);
      // bottom strip (-Y .. hy0), full width
      glTexCoord2f(0,0);        glVertex3f(-X,-Y,z);
      glTexCoord2f(ru,0);       glVertex3f( X,-Y,z);
      glTexCoord2f(ru,tv(hy0)); glVertex3f( X,hy0,z);
      glTexCoord2f(0,tv(hy0));  glVertex3f(-X,hy0,z);
      // left strip (-X .. -hw), between hy0..hy1
      glTexCoord2f(0,tv(hy0));       glVertex3f(-X,hy0,z);
      glTexCoord2f(tu(-hw),tv(hy0)); glVertex3f(-hw,hy0,z);
      glTexCoord2f(tu(-hw),tv(hy1)); glVertex3f(-hw,hy1,z);
      glTexCoord2f(0,tv(hy1));       glVertex3f(-X,hy1,z);
      // right strip (hw .. X), between hy0..hy1
      glTexCoord2f(tu(hw),tv(hy0)); glVertex3f( hw,hy0,z);
      glTexCoord2f(ru,tv(hy0));     glVertex3f( X,hy0,z);
      glTexCoord2f(ru,tv(hy1));     glVertex3f( X,hy1,z);
      glTexCoord2f(tu(hw),tv(hy1)); glVertex3f( hw,hy1,z);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void drawCylinderY(float radius, float height, GLuint tex=0)
{
    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    if (tex){ gluQuadricTexture(q, GL_TRUE); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,tex);}
    else glDisable(GL_TEXTURE_2D);
    glPushMatrix();
      glRotatef(-90,1,0,0);
      gluCylinder(q, radius, radius, height, 18, 1);
      glTranslatef(0,0,height);
      gluDisk(q, 0, radius, 18, 1);
      glTranslatef(0,0,-height);
      glRotatef(180,0,1,0);
      gluDisk(q, 0, radius, 18, 1);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    gluDeleteQuadric(q);
}

void drawConeY(float radius, float height)
{
    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
      glRotatef(-90,1,0,0);
      gluCylinder(q, radius, 0.0, height, 16, 1);
      gluDisk(q, 0, radius, 16, 1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawText3D(float x,float y,float z, const char *s, void*font=GLUT_BITMAP_HELVETICA_18)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1,1,1);
    glRasterPos3f(x,y,z);
    for (const char *c=s; *c; c++) glutBitmapCharacter(font, *c);
    glEnable(GL_LIGHTING);
}

// =======================================================================
//  COMPLEX OBJECTS
// =======================================================================

// ---- a building with textured walls, a roof, a text label and a grid
//      of glow-able windows (day = pale glass, night = warm glow) -------
///OID-4 : building - generic textured building with roof, label and
//          a window grid that lights up at night
void drawBuilding(float w, float h, float d, GLuint wallTex, const char* label)
{
    glPushMatrix();
      glTranslatef(0, h*0.5f, 0);
      setMaterial(0.35f,0.35f,0.35f, 0.75f,0.75f,0.75f, 0.15f,0.15f,0.15f, 12);
      drawBox(w, h, d, wallTex, w/3.0f, h/3.0f);
    glPopMatrix();

    // roof
    glPushMatrix();
      glTranslatef(0, h+0.15f, 0);
      setMaterial(0.25f,0.15f,0.12f, 0.55f,0.3f,0.25f, 0.1f,0.1f,0.1f, 8);
      drawBox(w+0.4f, 0.3f, d+0.4f, texRoof, 2,2);
    glPopMatrix();

    // window grid on front face (+z)
    int cols = (int)(w/1.4f); if (cols<1) cols=1;
    int rows = (int)(h/1.6f); if (rows<1) rows=1;
    float winW_ = w/(cols*1.6f), winH_ = h/(rows*1.8f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int r=0;r<rows;r++){
        for (int c=0;c<cols;c++){
            float wx = -w/2.0f + (c+0.5f)*(w/cols);
            float wy = (r+0.6f)*(h/rows);
            bool lit = (dayFactor < 0.55f) && (((r*7+c*3) % 3) != 0); // some windows glow at night
            if (lit) { setEmissive(0.95f,0.85f,0.35f); setMaterial(0.4f,0.35f,0.1f,0.9f,0.8f,0.3f,0.8f,0.8f,0.6f,60,0.95f); }
            else     { clearEmissive(); setMaterial(0.25f,0.35f,0.4f,0.45f,0.65f,0.75f,0.9f,0.9f,0.9f,90,0.55f); }
            glPushMatrix();
              glTranslatef(wx, wy, d*0.5f+0.02f);
              glBegin(GL_QUADS);
                glNormal3f(0,0,1);
                glVertex3f(-winW_/2,-winH_/2,0);
                glVertex3f( winW_/2,-winH_/2,0);
                glVertex3f( winW_/2, winH_/2,0);
                glVertex3f(-winW_/2, winH_/2,0);
              glEnd();
            glPopMatrix();
        }
    }
    clearEmissive();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (label && strlen(label)>0)
        drawText3D(-w*0.3f, h+0.9f, d*0.5f+0.3f, label);
}

///OID-4b : buildingWithOpenWindow - same as building(), but with a real
//          rectangular hole cut through the front wall (instead of a
//          solid wall + decorative panel) so the "inside" viewing
//          scenario can genuinely see out through it. Used for the
//          Apartments building.
void drawBuildingWithOpenWindow(float w, float h, float d, GLuint wallTex,
                                 const char* label, float holeW, float holeH, float holeCenterY)
{
    glPushMatrix();
      glTranslatef(0, h*0.5f, 0);
      setMaterial(0.35f,0.35f,0.35f, 0.75f,0.75f,0.75f, 0.15f,0.15f,0.15f, 12);
      drawBox(w, h, d, wallTex, w/3.0f, h/3.0f, /*skipFront=*/true);
      drawFrontWallWithHole(w, h, holeW, holeH, holeCenterY - h*0.5f, wallTex, w/3.0f, h/3.0f, d*0.5f);
    glPopMatrix();

    // roof
    glPushMatrix();
      glTranslatef(0, h+0.15f, 0);
      setMaterial(0.25f,0.15f,0.12f, 0.55f,0.3f,0.25f, 0.1f,0.1f,0.1f, 8);
      drawBox(w+0.4f, 0.3f, d+0.4f, texRoof, 2,2);
    glPopMatrix();

    // small glowing side windows only (front face already has the real
    // opening, so it's excluded from the usual window grid)
    int rows = (int)(h/1.6f); if (rows<1) rows=1;
    float winW_ = 0.5f, winH_ = h/(rows*1.8f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int r=0;r<rows;r++){
        float wy = (r+0.6f)*(h/rows);
        bool lit = (dayFactor < 0.55f) && ((r%2)==0);
        if (lit) { setEmissive(0.95f,0.85f,0.35f); setMaterial(0.4f,0.35f,0.1f,0.9f,0.8f,0.3f,0.8f,0.8f,0.6f,60,0.95f); }
        else     { clearEmissive(); setMaterial(0.25f,0.35f,0.4f,0.45f,0.65f,0.75f,0.9f,0.9f,0.9f,90,0.55f); }
        glPushMatrix();
          glTranslatef(d*0.5f+0.02f, wy, 0);
          glRotatef(90,0,1,0);
          glBegin(GL_QUADS);
            glNormal3f(0,0,1);
            glVertex3f(-winW_/2,-winH_/2,0);
            glVertex3f( winW_/2,-winH_/2,0);
            glVertex3f( winW_/2, winH_/2,0);
            glVertex3f(-winW_/2, winH_/2,0);
          glEnd();
        glPopMatrix();
    }
    clearEmissive();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (label && strlen(label)>0)
        drawText3D(-w*0.3f, h+0.9f, d*0.5f+0.3f, label);
}

// ---- Framed picture/observation window mounted on a wall -------------
//      Satisfies requirement 6: a glazed opening that reveals the
//      external environment (park & sky) behind the wall it sits on.
///OID-5 : framedWindow - wood-framed glass window (view-through, REQ 6)
void drawFramedWindow(float w, float h)
{
    // wooden frame (4 thin bars)
    setMaterial(0.25f,0.15f,0.08f, 0.5f,0.32f,0.15f, 0.25f,0.2f,0.15f, 20);
    float t = 0.12f; // frame thickness
    glPushMatrix(); glTranslatef(0, h/2-t/2, 0); drawBox(w, t, 0.15f, texWood,3,1); glPopMatrix();
    glPushMatrix(); glTranslatef(0,-h/2+t/2, 0); drawBox(w, t, 0.15f, texWood,3,1); glPopMatrix();
    glPushMatrix(); glTranslatef(-w/2+t/2, 0, 0); drawBox(t, h, 0.15f, texWood,1,3); glPopMatrix();
    glPushMatrix(); glTranslatef( w/2-t/2, 0, 0); drawBox(t, h, 0.15f, texWood,1,3); glPopMatrix();
    // cross bars
    glPushMatrix(); drawBox(w-2*t, 0.06f, 0.1f, texWood,3,1); glPopMatrix();
    glPushMatrix(); drawBox(0.06f, h-2*t, 0.1f, texWood,1,3); glPopMatrix();

    // glass pane - transparent, lets the outside scene show through
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setMaterial(0.3f,0.4f,0.45f, 0.4f,0.6f,0.65f, 0.9f,0.9f,0.9f, 90, 0.35f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGlass);
    glBegin(GL_QUADS);
      glNormal3f(0,0,1);
      glTexCoord2f(0,0); glVertex3f(-(w/2-t), -(h/2-t), 0.02f);
      glTexCoord2f(1,0); glVertex3f( (w/2-t), -(h/2-t), 0.02f);
      glTexCoord2f(1,1); glVertex3f( (w/2-t),  (h/2-t), 0.02f);
      glTexCoord2f(0,1); glVertex3f(-(w/2-t),  (h/2-t), 0.02f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ---- Tree ---------------------------------------------------------------
///OID-6 : tree - park tree (trunk + two-tier crown)
void drawTree(float trunkH, float crownR)
{
    setMaterial(0.2f,0.12f,0.06f, 0.4f,0.25f,0.12f, 0.05f,0.05f,0.05f, 6);
    drawCylinderY(0.18f, trunkH, texWood);
    glPushMatrix();
      glTranslatef(0, trunkH, 0);
      setMaterial(0.15f,0.35f,0.08f, 0.4f,0.78f,0.25f, 0.08f,0.15f,0.06f, 10);
      drawConeY(crownR, crownR*2.1f);
      glTranslatef(0, crownR*1.0f, 0);
      drawConeY(crownR*0.75f, crownR*1.7f);
    glPopMatrix();
}

// ---- Windmill : REQUIREMENT 5 - continuously rotating object -----------
///OID-7 : windmill - continuously rotating object (REQ 5)
void drawWindmill()
{
    setMaterial(0.3f,0.3f,0.3f,0.6f,0.6f,0.6f,0.3f,0.3f,0.3f,20);
    drawCylinderY(0.25f, 7.0f, texConcrete);
    glPushMatrix();
      glTranslatef(0, 7.0f, 0);
      setMaterial(0.15f,0.15f,0.15f,0.35f,0.35f,0.35f,0.2f,0.2f,0.2f,20);
      glRotatef(90,0,1,0);
      drawCylinderY(0.35f, 0.6f);
      glRotatef(windmillAngle, 0,0,1);
      for (int i=0;i<4;i++){
          glPushMatrix();
            glRotatef(90.0f*i, 0,0,1);
            glTranslatef(0, 1.6f, 0);
            setMaterial(0.5f,0.05f,0.05f,0.85f,0.15f,0.15f,0.3f,0.3f,0.3f,30);
            drawBox(0.5f, 3.2f, 0.08f);
          glPopMatrix();
      }
    glPopMatrix();
}

// ---- Car : REQUIREMENT 4 complex object, drives & obeys signal ---------
///OID-8 : car - complex object, drives and obeys the traffic signal.
//          Parameterized by lane position/direction/colour so the same
//          drawer can be reused for every car in the fleet (see OID-18).
void drawCar(float laneX, float z, float dirSign, float r, float g, float b)
{
    glPushMatrix();
      // driving lane kept clear of the metro viaduct's footprint
      // (deck spans roughly x -2.6..2.6) so the car is never hidden
      // underneath it when viewed from above.
      glTranslatef(laneX, 0.55f, z);
      // face the direction this car is actually driving: the near lane
      // (dirSign>0) faces one way, the far lane (dirSign<0) faces the
      // opposite way, so headlights/traffic always point forward
      glRotatef(dirSign>0 ? -90.0f : 90.0f, 0,1,0);
      setMaterial(r*0.45f,g*0.45f,b*0.45f, r,g,b, 0.9f,0.9f,0.9f, 70);
      drawBox(2.0f, 0.55f, 1.0f);
      glPushMatrix();
        glTranslatef(-0.15f, 0.5f, 0);
        setMaterial(r*0.45f,g*0.45f,b*0.45f, r,g,b, 0.9f,0.9f,0.9f, 70);
        drawBox(1.1f, 0.42f, 0.92f);
      glPopMatrix();
      // headlights (emissive)
      setEmissive((dayFactor<0.5f)?1.0f:0.6f, (dayFactor<0.5f)?1.0f:0.6f, 0.6f);
      setMaterial(0.6f,0.6f,0.4f,0.9f,0.9f,0.6f,0.9f,0.9f,0.9f,60);
      glPushMatrix(); glTranslatef(1.0f,0.05f,0.35f); drawBox(0.06f,0.15f,0.15f); glPopMatrix();
      glPushMatrix(); glTranslatef(1.0f,0.05f,-0.35f); drawBox(0.06f,0.15f,0.15f); glPopMatrix();
      clearEmissive();
      // wheels
      setMaterial(0.02f,0.02f,0.02f,0.08f,0.08f,0.08f,0.15f,0.15f,0.15f,10);
      float wx[4]={0.65f,0.65f,-0.65f,-0.65f};
      float wz[4]={0.55f,-0.55f,0.55f,-0.55f};
      for (int i=0;i<4;i++){
          glPushMatrix();
            glTranslatef(wx[i], -0.28f, wz[i]);
            glRotatef(90,1,0,0);
            drawCylinderY(0.26f, 0.18f);
          glPopMatrix();
      }
    glPopMatrix();
}

///OID-18 : cityTraffic - draws every car in the NUM_CARS fleet at its
//           current position/lane/colour (positions updated in idle())
void drawTraffic()
{
    for (int i=0;i<NUM_CARS;i++){
        drawCar(carLaneX[i], carZ[i], carDir[i],
                carColor[i][0], carColor[i][1], carColor[i][2]);
    }
}

// ---- Traffic light -------------------------------------------------------
///OID-9 : trafficLight - 3-state signal (red/yellow/green)
void drawTrafficLight(float x, float z)
{
    glPushMatrix();
      glTranslatef(x, 0, z);
      setMaterial(0.15f,0.15f,0.15f,0.3f,0.3f,0.3f,0.2f,0.2f,0.2f,15);
      drawCylinderY(0.1f, 3.2f);
      glTranslatef(0, 3.2f, 0);
      drawBox(0.5f, 1.3f, 0.4f);

      float ry = 0.9f;
      for (int i=0;i<3;i++){
          bool on = (i==0 && trafficState==0) || (i==1 && trafficState==2) || (i==2 && trafficState==1);
          float r=0.15f,g=0.15f,b=0.15f;
          if (i==0){ r = on?1.0f:0.3f; }
          if (i==1){ g = on?1.0f:0.3f; r = on?0.8f:0.2f; }
          if (i==2){ g = on?1.0f:0.3f; }
          if (on) setEmissive(r*0.9f,g*0.9f,b*0.2f);
          setMaterial(r*0.4f,g*0.4f,b*0.2f,r,g,b,0.7f,0.7f,0.5f,70);
          glPushMatrix();
            glTranslatef(0, ry, 0.25f);
            glutSolidSphere(0.16f, 14, 14);
          glPopMatrix();
          clearEmissive();
          ry -= 0.45f;
      }
    glPopMatrix();
}

// =======================================================================
//  ROAD / FOOTPATH / GROUND / SKY
// =======================================================================
void drawGroundPlane(float x, float z, float w, float d, GLuint tex, float ru, float rv, float y=0.0f)
{
    setMaterial(0.3f,0.3f,0.3f, 0.8f,0.8f,0.8f, 0.05f,0.05f,0.05f, 4);
    glPushMatrix();
      glTranslatef(x, y, z);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, tex);
      glBegin(GL_QUADS);
        glNormal3f(0,1,0);
        glTexCoord2f(0,0);   glVertex3f(-w/2,0,-d/2);
        glTexCoord2f(ru,0);  glVertex3f( w/2,0,-d/2);
        glTexCoord2f(ru,rv); glVertex3f( w/2,0, d/2);
        glTexCoord2f(0,rv);  glVertex3f(-w/2,0, d/2);
      glEnd();
      glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

///OID-3 : road - road surface, lane markings, footpaths, crosswalk
void drawRoadSystem()
{
    // main road surface
    drawGroundPlane(0,0, 12, 140, texRoad, 3, 40, 0.01f);
    // lane markings (dashed)
    setMaterial(0.4f,0.4f,0.1f,0.9f,0.85f,0.2f,0.3f,0.3f,0.1f,20);
    for (float z=-68; z<70; z+=6){
        glPushMatrix();
          glTranslatef(0, 0.03f, z);
          drawBox(0.25f, 0.02f, 3.0f);
        glPopMatrix();
    }
    // footpaths both sides
    drawGroundPlane(-8.0f, 0, 4, 140, texConcrete, 2, 40, 0.015f);
    drawGroundPlane( 8.0f, 0, 4, 140, texConcrete, 2, 40, 0.015f);
    // crosswalk near the traffic light
    setMaterial(0.5f,0.5f,0.5f,0.92f,0.92f,0.92f,0.2f,0.2f,0.2f,10);
    for (float x=-5.5f; x<5.5f; x+=1.3f){
        glPushMatrix();
          glTranslatef(x, 0.02f, -1.0f);
          drawBox(0.7f, 0.02f, 3.2f);
        glPopMatrix();
    }
}

///OID-2 : cloud - a single puffy cloud made of overlapping soft spheres
//          (caller sets glColor/alpha before calling, so it can fade
//          the cloud in/out with the day-night cycle)
void drawCloud(float x, float y, float z, float s)
{
    glPushMatrix();
      glTranslatef(x, y, z);
      glScalef(s, s*0.6f, s);
      glutSolidSphere(1.0f, 12, 12);
      glPushMatrix(); glTranslatef(-1.1f, -0.1f, 0.2f); glutSolidSphere(0.75f,12,12); glPopMatrix();
      glPushMatrix(); glTranslatef( 1.1f, -0.1f,-0.2f); glutSolidSphere(0.75f,12,12); glPopMatrix();
      glPushMatrix(); glTranslatef( 0.4f,  0.35f,0.3f); glutSolidSphere(0.65f,12,12); glPopMatrix();
      glPushMatrix(); glTranslatef(-0.5f,  0.3f,-0.3f); glutSolidSphere(0.6f,12,12); glPopMatrix();
    glPopMatrix();
}

///OID-1 : sky - sky dome with day/night gradient + sun/moon
void drawSky()
{
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    float topR = 0.10f+0.35f*dayFactor, topG = 0.10f+0.55f*dayFactor, topB = 0.25f+0.55f*dayFactor;
    float horR = 0.55f+0.35f*dayFactor, horG = 0.6f+0.35f*dayFactor, horB = 0.65f+0.30f*dayFactor;
    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    glFrontFace(GL_CW);
    int slices=24, stacks=12;
    for (int i=0;i<stacks;i++){
        float lat0 = M_PI*(-0.02 + (double)i/stacks*0.5);
        float lat1 = M_PI*(-0.02 + (double)(i+1)/stacks*0.5);
        float t0 = (float)i/stacks, t1=(float)(i+1)/stacks;
        float r0=horR+(topR-horR)*t0, g0=horG+(topG-horG)*t0, b0=horB+(topB-horB)*t0;
        float r1=horR+(topR-horR)*t1, g1=horG+(topG-horG)*t1, b1=horB+(topB-horB)*t1;
        glBegin(GL_QUAD_STRIP);
        for (int j=0;j<=slices;j++){
            float lng = 2*M_PI*(double)j/slices;
            float x0=cosf(lng)*cosf(lat0), y0=sinf(lat0), z0=sinf(lng)*cosf(lat0);
            float x1=cosf(lng)*cosf(lat1), y1=sinf(lat1), z1=sinf(lng)*cosf(lat1);
            glColor3f(r0,g0,b0); glVertex3f(x0*95,y0*95,z0*95);
            glColor3f(r1,g1,b1); glVertex3f(x1*95,y1*95,z1*95);
        }
        glEnd();
    }
    glFrontFace(GL_CCW);
    gluDeleteQuadric(q);

    // sun and moon - two bodies on opposite sides of one orbit; each is
    // only drawn while it is above the horizon, so the sun is clearly
    // visible during the day and the moon takes over at night.
    float sunX,sunY,moonX,moonY; computeCelestial(sunX,sunY,moonX,moonY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive glow
    if (sunY > -8.0f){
        glPushMatrix();
          glTranslatef(sunX, sunY+4.0f, -25.0f);
          // soft glow halo
          glColor4f(1.0f, 0.9f, 0.5f, 0.35f);
          glutSolidSphere(6.5f, 18, 18);
          // bright core
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glColor4f(1.0f, 0.95f, 0.55f, 1.0f);
          glutSolidSphere(3.2f, 20, 20);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPopMatrix();
    }
    if (moonY > -8.0f){
        glPushMatrix();
          glTranslatef(moonX, moonY+4.0f, -25.0f);
          glColor4f(0.75f, 0.78f, 0.9f, 0.25f);
          glutSolidSphere(4.5f, 18, 18);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glColor4f(0.88f, 0.9f, 0.95f, 1.0f);
          glutSolidSphere(2.4f, 20, 20);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPopMatrix();
    }
    glDisable(GL_BLEND);

    // drifting clouds - visible mainly in daylight, fade out at night
    if (dayFactor > 0.15f){
        glDisable(GL_LIGHTING);
        float alpha = dayFactor;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float drift = fmodf(animClock*1.2f, 160.0f);
        glColor4f(1,1,1,alpha);
        drawCloud(-60.0f+drift, 30.0f, -40.0f, 6.0f);
        drawCloud(-20.0f+drift, 22.0f, -55.0f, 4.5f);
        drawCloud( 30.0f-drift*0.7f, 34.0f, -30.0f, 7.0f);
        glDisable(GL_BLEND);
    }

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Shared sun/moon orbit calculation - sun and moon sit on opposite sides
// of the same circle, so whichever one is above the horizon is the body
// that should be visible and doing the lighting.
void computeCelestial(float &sunX,float &sunY,float &moonX,float &moonY)
{
    float sunRad  = sunAngle*M_PI/180.0f;
    float moonRad = (sunAngle+180.0f)*M_PI/180.0f;
    sunX  = cosf(sunRad)*CEL_RADIUS;
    sunY  = sinf(sunRad)*CEL_RADIUS;
    moonX = cosf(moonRad)*CEL_RADIUS;
    moonY = sinf(moonRad)*CEL_RADIUS;
}

// =======================================================================
//  LIGHTING  -  REQUIREMENT 7 : two lights, full material properties
// =======================================================================
void setupLights()
{
    // LIGHT0 : Sun / Moon - directional-ish, intensity & colour follow
    // the day/night cycle. Follows whichever body (sun or moon) is
    // currently above the horizon.
    float sx,sy,mx,my; computeCelestial(sx,sy,mx,my);
    bool sunUp = sy > my;
    float lx = sunUp ? sx : mx;
    float ly = fmaxf(sunUp ? sy : my, 3.0f);
    GLfloat pos0[] = { lx, ly+8.0f, -20.0f, 1.0f };
    GLfloat amb0[] = { 0.4f+0.3f*dayFactor, 0.4f+0.3f*dayFactor, 0.42f+0.28f*dayFactor, 1};
    GLfloat dif0[] = { 0.4f+0.55f*dayFactor, 0.4f+0.5f*dayFactor, 0.4f+0.4f*dayFactor, 1};
    GLfloat spc0[] = { 0.5f*dayFactor, 0.5f*dayFactor, 0.5f*dayFactor, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spc0);

    // LIGHT1 : Street lamp - point light near the traffic-light corner,
    // stronger at night.
    GLfloat pos1[] = { 4.0f, 6.5f, -2.0f, 1.0f };
    float night = 1.0f - dayFactor;
    GLfloat amb1[] = { 0.05f, 0.05f, 0.03f, 1};
    GLfloat dif1[] = { 0.9f*night, 0.75f*night, 0.35f*night, 1};
    GLfloat spc1[] = { 0.7f*night, 0.7f*night, 0.5f*night, 1};
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT1, GL_AMBIENT, amb1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, dif1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spc1);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.01f);

    GLfloat globalAmb[] = {0.32f+0.2f*dayFactor,0.32f+0.2f*dayFactor,0.34f+0.2f*dayFactor,1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
}

// street-lamp pole (visual source for LIGHT1)
///OID-10 : streetLamp - street lamp, visual source of Light1
void drawStreetLamp(float x,float z)
{
    glPushMatrix();
      glTranslatef(x,0,z);
      setMaterial(0.15f,0.15f,0.15f,0.3f,0.3f,0.3f,0.2f,0.2f,0.2f,15);
      drawCylinderY(0.09f, 6.5f);
      glTranslatef(0,6.5f,0);
      glRotatef(90,1,0,0);
      drawCylinderY(0.09f, 0.9f);
      glRotatef(-90,1,0,0);
      glTranslatef(0,-0.05f,0.9f);
      if (dayFactor<0.5f) setEmissive(0.95f,0.85f,0.4f);
      setMaterial(0.5f,0.45f,0.2f,0.9f,0.8f,0.4f,0.6f,0.6f,0.5f,50);
      glutSolidSphere(0.22f,12,12);
      clearEmissive();
    glPopMatrix();
}

// ---- Playground slide -----------------------------------------------
///OID-11 : slide - playground slide
void drawSlide()
{
    float platH = 2.0f;
    // support legs - thicker, bright red poles
    setMaterial(0.4f,0.03f,0.03f, 0.85f,0.1f,0.08f, 0.5f,0.5f,0.5f, 45);
    glPushMatrix(); glTranslatef(-0.6f,platH/2,-0.55f); drawBox(0.2f,platH,0.2f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.6f,platH/2,-0.55f); drawBox(0.2f,platH,0.2f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.6f,platH/2, 0.55f); drawBox(0.2f,platH,0.2f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.6f,platH/2, 0.55f); drawBox(0.2f,platH,0.2f); glPopMatrix();

    // ladder on the back side - bright yellow rungs
    setMaterial(0.4f,0.35f,0.02f, 0.85f,0.75f,0.08f, 0.4f,0.4f,0.3f, 30);
    for (float ry=0.3f; ry<platH; ry+=0.38f){
        glPushMatrix(); glTranslatef(0, ry, 0.6f); drawBox(1.2f,0.09f,0.09f); glPopMatrix();
    }

    // top platform - blue
    setMaterial(0.03f,0.15f,0.4f, 0.1f,0.4f,0.85f, 0.4f,0.4f,0.5f, 55);
    glPushMatrix(); glTranslatef(0, platH+0.08f, 0); drawBox(1.3f,0.16f,1.25f); glPopMatrix();
    // little rail on platform
    setMaterial(0.4f,0.03f,0.03f, 0.85f,0.1f,0.08f, 0.5f,0.5f,0.5f, 45);
    glPushMatrix(); glTranslatef(0, platH+0.45f, 0.55f); drawBox(1.2f,0.6f,0.1f); glPopMatrix();

    // inclined slide surface - bright, high-contrast yellow-orange
    setMaterial(0.4f,0.18f,0.0f, 0.95f,0.55f,0.05f, 0.7f,0.7f,0.5f, 75);
    glPushMatrix();
      glTranslatef(0, platH*0.55f, -1.1f);
      glRotatef(38, 1,0,0);
      drawBox(1.1f, 0.08f, 2.4f);
      // side guards - bright red
      setMaterial(0.4f,0.03f,0.03f, 0.85f,0.1f,0.08f, 0.5f,0.5f,0.5f, 45);
      glPushMatrix(); glTranslatef(-0.55f,0.16f,0); drawBox(0.08f,0.32f,2.4f); glPopMatrix();
      glPushMatrix(); glTranslatef( 0.55f,0.16f,0); drawBox(0.08f,0.32f,2.4f); glPopMatrix();
    glPopMatrix();
}

// ---- Swing set (দোলনা) - gently pendulums via swingAngle -------------
///OID-12 : swing - playground swing, pendulum animation via swingAngle
void drawSwing()
{
    float postH = 2.4f, spanZ = 2.8f;
    // bright green A-frame poles
    setMaterial(0.03f,0.3f,0.05f, 0.1f,0.7f,0.15f, 0.4f,0.4f,0.3f, 40);
    for (float z = -spanZ/2; z <= spanZ/2; z += spanZ){
        glPushMatrix();
          glTranslatef(-1.0f, postH/2, z);
          glRotatef(18, 0,0,1);
          drawBox(0.16f, postH, 0.16f);
        glPopMatrix();
        glPushMatrix();
          glTranslatef(1.0f, postH/2, z);
          glRotatef(-18, 0,0,1);
          drawBox(0.16f, postH, 0.16f);
        glPopMatrix();
    }
    // top horizontal bar - yellow, thick so it reads clearly
    setMaterial(0.4f,0.35f,0.02f, 0.85f,0.75f,0.08f, 0.4f,0.4f,0.3f, 30);
    glPushMatrix();
      glTranslatef(0, postH, 0);
      glRotatef(90,0,1,0);
      drawCylinderY(0.14f, spanZ);
    glPopMatrix();

    // two hanging seats, swaying with swingAngle (pendulum about the bar)
    for (float z = -0.8f; z <= 0.8f; z += 1.6f){
        glPushMatrix();
          glTranslatef(0, postH, z);
          glRotatef(swingAngle, 1, 0, 0);      // pendulum motion
          setMaterial(0.15f,0.15f,0.15f,0.35f,0.35f,0.35f,0.2f,0.2f,0.2f,15);
          glPushMatrix(); glTranslatef(-0.32f,-0.6f,0); drawCylinderY(0.035f,0.6f); glPopMatrix();
          glPushMatrix(); glTranslatef( 0.32f,-0.6f,0); drawCylinderY(0.035f,0.6f); glPopMatrix();
          glPushMatrix();
            glTranslatef(0,-1.2f,0);
            setMaterial(0.4f,0.03f,0.03f, 0.85f,0.1f,0.08f, 0.4f,0.4f,0.3f, 35);
            drawBox(0.7f,0.08f,0.32f, texWood, 1,1);
          glPopMatrix();
        glPopMatrix();
    }
}

// =======================================================================
//  PARK
// =======================================================================
///OID-14 : metroViaduct - elevated MRT piers, deck girder and rails
//          Runs the length of the road along the median (x = 0).
void drawMetroViaduct()
{
    // support piers every 14 units along the line
    for (float z = -70.0f; z <= 70.0f; z += 14.0f){
        glPushMatrix();
          glTranslatef(0, 0, z);
          // tapered concrete pier
          setMaterial(0.32f,0.32f,0.3f, 0.72f,0.72f,0.68f, 0.15f,0.15f,0.15f, 18);
          glPushMatrix();
            glTranslatef(0, (METRO_DECK_Y-1.2f)*0.5f, 0);
            drawBox(1.5f, METRO_DECK_Y-1.2f, 1.5f, texConcrete, 1, 4);
          glPopMatrix();
          // pier cap (wider head the girder rests on)
          glPushMatrix();
            glTranslatef(0, METRO_DECK_Y-1.0f, 0);
            drawBox(3.2f, 0.7f, 2.2f, texConcrete, 2, 1);
          glPopMatrix();
        glPopMatrix();
    }

    // continuous box girder deck
    setMaterial(0.34f,0.34f,0.33f, 0.75f,0.75f,0.72f, 0.18f,0.18f,0.18f, 22);
    glPushMatrix();
      glTranslatef(0, METRO_DECK_Y-0.3f, 0);
      drawBox(5.2f, 1.0f, 150.0f, texConcrete, 3, 60);
    glPopMatrix();

    // parapet walls on both edges of the deck
    setMaterial(0.3f,0.3f,0.3f, 0.7f,0.7f,0.68f, 0.15f,0.15f,0.15f, 18);
    glPushMatrix(); glTranslatef(-2.45f, METRO_DECK_Y+0.55f, 0); drawBox(0.3f,0.8f,150.0f, texConcrete,1,60); glPopMatrix();
    glPushMatrix(); glTranslatef( 2.45f, METRO_DECK_Y+0.55f, 0); drawBox(0.3f,0.8f,150.0f, texConcrete,1,60); glPopMatrix();

    // two steel rails running the length of the deck
    setMaterial(0.25f,0.25f,0.28f, 0.5f,0.5f,0.55f, 0.85f,0.85f,0.9f, 85);
    glPushMatrix(); glTranslatef(-0.75f, METRO_DECK_Y+0.3f, 0); drawBox(0.14f,0.18f,150.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.75f, METRO_DECK_Y+0.3f, 0); drawBox(0.14f,0.18f,150.0f); glPopMatrix();

    // sleepers under the rails
    setMaterial(0.2f,0.14f,0.08f, 0.42f,0.3f,0.16f, 0.08f,0.08f,0.08f, 8);
    for (float z = -72.0f; z <= 72.0f; z += 2.0f){
        glPushMatrix();
          glTranslatef(0, METRO_DECK_Y+0.16f, z);
          drawBox(2.0f, 0.1f, 0.35f, texWood, 1, 1);
        glPopMatrix();
    }
}

///OID-15 : metroTrain - moving metro train (multiple coaches) on the line
void drawMetroTrain()
{
    const int COACHES = 3;
    const float CO_LEN = 7.5f, GAP = 0.6f;

    for (int i = 0; i < COACHES; i++){
        float z = metroZ - i*(CO_LEN+GAP);
        glPushMatrix();
          glTranslatef(0, METRO_DECK_Y+1.35f, z);

          // main body - MRT green/white livery
          setMaterial(0.25f,0.3f,0.25f, 0.85f,0.88f,0.86f, 0.6f,0.6f,0.6f, 75);
          drawBox(2.5f, 2.1f, CO_LEN);

          // green stripe band along the sides
          setMaterial(0.02f,0.25f,0.12f, 0.05f,0.6f,0.28f, 0.4f,0.5f,0.45f, 60);
          glPushMatrix(); glTranslatef(0, 0.35f, 0); drawBox(2.56f, 0.45f, CO_LEN-0.1f); glPopMatrix();

          // roof (slightly rounded look via a thin wide slab)
          setMaterial(0.3f,0.3f,0.32f, 0.6f,0.6f,0.65f, 0.4f,0.4f,0.4f, 40);
          glPushMatrix(); glTranslatef(0, 1.1f, 0); drawBox(2.3f, 0.22f, CO_LEN-0.2f); glPopMatrix();

          // windows along both sides - glow at night
          bool lit = (dayFactor < 0.55f);
          if (lit) setEmissive(0.9f,0.88f,0.6f);
          setMaterial(0.2f,0.3f,0.35f, 0.4f,0.6f,0.7f, 0.9f,0.9f,0.9f, 90);
          for (float wz = -CO_LEN/2+1.2f; wz < CO_LEN/2-0.6f; wz += 1.5f){
              glPushMatrix(); glTranslatef( 1.28f, 0.55f, wz); drawBox(0.05f,0.7f,1.0f); glPopMatrix();
              glPushMatrix(); glTranslatef(-1.28f, 0.55f, wz); drawBox(0.05f,0.7f,1.0f); glPopMatrix();
          }
          clearEmissive();

          // front headlights only on the leading coach
          if (i == 0){
              setEmissive(1.0f,1.0f,0.85f);
              setMaterial(0.6f,0.6f,0.5f, 0.95f,0.95f,0.8f, 0.9f,0.9f,0.9f, 80);
              glPushMatrix(); glTranslatef(-0.7f,-0.5f, CO_LEN/2+0.03f); drawBox(0.35f,0.22f,0.06f); glPopMatrix();
              glPushMatrix(); glTranslatef( 0.7f,-0.5f, CO_LEN/2+0.03f); drawBox(0.35f,0.22f,0.06f); glPopMatrix();
              clearEmissive();
              // windscreen
              setMaterial(0.15f,0.2f,0.25f, 0.25f,0.35f,0.45f, 0.9f,0.9f,0.9f, 95);
              glPushMatrix(); glTranslatef(0, 0.5f, CO_LEN/2+0.03f); drawBox(1.9f,0.85f,0.06f); glPopMatrix();
          }

          // bogies / wheel housings underneath
          setMaterial(0.08f,0.08f,0.08f, 0.18f,0.18f,0.18f, 0.2f,0.2f,0.2f, 15);
          glPushMatrix(); glTranslatef(0,-1.2f, CO_LEN*0.28f); drawBox(1.8f,0.4f,1.6f); glPopMatrix();
          glPushMatrix(); glTranslatef(0,-1.2f,-CO_LEN*0.28f); drawBox(1.8f,0.4f,1.6f); glPopMatrix();
        glPopMatrix();
    }
}

///OID-16 : metroStation - elevated station platform beside the viaduct
void drawMetroStation(float z)
{
    glPushMatrix();
      glTranslatef(0, 0, z);

      // platform slabs on both sides of the track
      setMaterial(0.35f,0.35f,0.33f, 0.78f,0.78f,0.74f, 0.15f,0.15f,0.15f, 20);
      glPushMatrix(); glTranslatef(-4.0f, METRO_DECK_Y+0.4f, 0); drawBox(3.0f,0.4f,18.0f, texConcrete,2,10); glPopMatrix();
      glPushMatrix(); glTranslatef( 4.0f, METRO_DECK_Y+0.4f, 0); drawBox(3.0f,0.4f,18.0f, texConcrete,2,10); glPopMatrix();

      // roof columns
      setMaterial(0.3f,0.3f,0.32f, 0.6f,0.6f,0.65f, 0.4f,0.4f,0.4f, 35);
      for (float cz = -7.5f; cz <= 7.5f; cz += 5.0f){
          glPushMatrix(); glTranslatef(-5.2f, METRO_DECK_Y+2.1f, cz); drawBox(0.25f,3.0f,0.25f); glPopMatrix();
          glPushMatrix(); glTranslatef( 5.2f, METRO_DECK_Y+2.1f, cz); drawBox(0.25f,3.0f,0.25f); glPopMatrix();
      }

      // station canopy roof
      setMaterial(0.25f,0.28f,0.3f, 0.45f,0.55f,0.6f, 0.5f,0.5f,0.5f, 50);
      glPushMatrix(); glTranslatef(0, METRO_DECK_Y+3.8f, 0); drawBox(12.0f,0.3f,19.0f, texRoof,4,6); glPopMatrix();

      // station name board
      drawText3D(-2.0f, METRO_DECK_Y+4.4f, 9.6f, "Metro Station");

      // access staircase down to the footpath
      setMaterial(0.32f,0.32f,0.3f, 0.7f,0.7f,0.66f, 0.15f,0.15f,0.15f, 18);
      for (int s = 0; s < 14; s++){
          float sy = 0.4f + s*(METRO_DECK_Y/14.0f);
          glPushMatrix();
            glTranslatef(6.6f + s*0.32f, sy, -10.5f);
            drawBox(0.5f, 0.28f, 2.6f, texConcrete, 1, 1);
          glPopMatrix();
      }
    glPopMatrix();
}

///OID-13 : park - assembles trees, pond, benches, slide and swing
void drawPark()
{
    drawGroundPlane(-24, -18, 20, 26, texGrass, 4, 5, 0.005f);
    float tx[]={-30,-24,-18,-30,-18,-27};
    float tz[]={-24,-28,-24,-12,-12,-18};
    for (int i=0;i<6;i++){
        glPushMatrix();
          glTranslatef(tx[i],0,tz[i]);
          drawTree(1.6f+0.3f*(i%3), 1.1f+0.15f*(i%2));
        glPopMatrix();
    }
    // small pond
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setMaterial(0.05f,0.1f,0.2f,0.15f,0.35f,0.55f,0.8f,0.8f,0.9f,90,0.85f);
    drawGroundPlane(-22,-19, 7, 5, texWater, 2,2, 0.02f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    // benches
    for (int i=0;i<2;i++){
        glPushMatrix();
          glTranslatef(-27, 0.35f, -14.0f+i*5.0f);
          setMaterial(0.2f,0.12f,0.06f,0.45f,0.28f,0.14f,0.1f,0.1f,0.1f,10);
          drawBox(1.6f,0.1f,0.6f, texWood,2,1);
          glTranslatef(0,-0.3f,0);
          drawBox(1.6f,0.5f,0.08f, texWood,2,1);
        glPopMatrix();
    }

    // playground - slide and swing, on their own sandy pad so they stand
    // out clearly against the grass, placed in open ground (no trees
    // nearby) and scaled up so they read clearly at city scale.
    setMaterial(0.35f,0.28f,0.15f, 0.7f,0.58f,0.32f, 0.05f,0.05f,0.05f, 4);
    drawGroundPlane(-18, -8, 11, 8, texConcrete, 3, 2, 0.008f);

    glPushMatrix();
      glTranslatef(-21.5f, 0, -9.5f);
      glRotatef(35, 0,1,0);
      glScalef(1.8f, 1.8f, 1.8f);
      drawSlide();
    glPopMatrix();

    glPushMatrix();
      glTranslatef(-15.0f, 0, -7.0f);
      glRotatef(-15, 0,1,0);
      glScalef(1.7f, 1.7f, 1.7f);
      drawSwing();
    glPopMatrix();
}

// =======================================================================
//  CITY SKYLINE  -  extra procedural buildings so the scene reads as a
//  full city block instead of just 2-3 isolated landmark buildings.
//  Uses the same drawBuilding() (OID-4) already used for Mall/Apartments/
//  Coffee Shop, just called in a loop with varying size/texture/label -
//  same pattern already used for the park's trees (parallel arrays).
// =======================================================================
///OID-17 : citySkyline - extra procedural buildings filling the block
void drawCitySkyline()
{
    const int N = 8;
    // position (world x,z) of each building's base
    static const float bx[N] = { 16.0f, 23.0f, 16.0f, 23.0f, 16.0f, 23.0f, 16.0f, 23.0f };
    static const float bz[N] = { 18.0f, 10.0f,  0.0f, -8.0f,-16.0f,-24.0f,-32.0f,-40.0f };
    // footprint (width, depth) and height - varied so the skyline has a
    // natural uneven silhouette rather than a row of identical boxes
    static const float bw[N] = { 4.5f, 5.5f, 4.0f, 5.0f, 4.5f, 6.0f, 4.0f, 5.0f };
    static const float bd[N] = { 4.0f, 5.0f, 4.0f, 4.5f, 4.0f, 5.0f, 4.0f, 4.5f };
    static const float bh[N] = { 7.0f,12.0f, 6.0f, 9.5f, 8.0f,14.0f, 5.5f,10.5f };
    static const char* blabel[N] = {
        "Bank", "Grand Hotel", "Pharmacy", "Trade Center",
        "Residency", "City Tower", "Bookstore", "Business Plaza"
    };

    for (int i = 0; i < N; i++){
        // alternate wall texture (brick / concrete) for visual variety
        GLuint tex = (i % 2 == 0) ? texBrick : texConcrete;
        glPushMatrix();
          glTranslatef(bx[i], 0, bz[i]);
          drawBuilding(bw[i], bh[i], bd[i], tex, blabel[i]);
        glPopMatrix();
    }
}

// =======================================================================
//  PEDESTRIANS  -  simple people walking back and forth on the footpaths,
//  so the street feels lived-in rather than just cars + empty pavement.
// =======================================================================
// One low-poly person: torso, head, and swinging arms/legs for a simple
// walk-cycle. Colour is passed in so each pedestrian looks different.
void drawPerson(float r, float g, float b)
{
    float legSwing = sinf(animClock*6.0f) * 22.0f;   // walking motion

    // legs (dark trousers/shoes), swinging opposite to each other
    setMaterial(0.05f,0.05f,0.05f, 0.15f,0.15f,0.15f, 0.05f,0.05f,0.05f, 8);
    glPushMatrix();
      glTranslatef(-0.08f, 0.5f, 0);
      glRotatef(legSwing, 1,0,0);
      glTranslatef(0,-0.25f,0);
      drawBox(0.12f, 0.5f, 0.12f);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(0.08f, 0.5f, 0);
      glRotatef(-legSwing, 1,0,0);
      glTranslatef(0,-0.25f,0);
      drawBox(0.12f, 0.5f, 0.12f);
    glPopMatrix();

    // torso - this is the pedestrian's "colour"
    setMaterial(r*0.3f,g*0.3f,b*0.3f, r,g,b, 0.15f,0.15f,0.15f, 10);
    glPushMatrix(); glTranslatef(0, 0.95f, 0); drawBox(0.34f, 0.55f, 0.2f); glPopMatrix();

    // arms, swinging opposite to the legs (natural walk cycle)
    glPushMatrix();
      glTranslatef(-0.24f, 1.1f, 0);
      glRotatef(-legSwing, 1,0,0);
      glTranslatef(0,-0.22f,0);
      drawBox(0.1f, 0.42f, 0.1f);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(0.24f, 1.1f, 0);
      glRotatef(legSwing, 1,0,0);
      glTranslatef(0,-0.22f,0);
      drawBox(0.1f, 0.42f, 0.1f);
    glPopMatrix();

    // head
    setMaterial(0.25f,0.18f,0.12f, 0.55f,0.4f,0.28f, 0.1f,0.1f,0.1f, 10);
    glPushMatrix(); glTranslatef(0, 1.42f, 0); glutSolidSphere(0.16f, 10, 10); glPopMatrix();
}

///OID-19 : pedestrians - a handful of people patrolling the footpaths.
//           Each one's Z position oscillates smoothly (sin wave) between
//           two ends of its stretch of footpath, same idea already used
//           for the park swing's pendulum motion (swingAngle).
void drawPedestrians()
{
    const int NUM_PED = 4;
    static const float pedX[NUM_PED]      = { -8.0f, -8.0f,  8.0f,  8.0f };
    static const float pedCenterZ[NUM_PED]= { -10.0f, 15.0f,-20.0f,  5.0f };
    static const float pedRangeZ[NUM_PED] = {  12.0f, 10.0f, 14.0f,  9.0f };
    static const float pedSpeed[NUM_PED]  = {   0.5f,  0.4f,  0.6f, 0.45f };
    static const float pedPhase[NUM_PED]  = {   0.0f,  2.1f,  4.2f,  1.0f };
    static const float pedColor[NUM_PED][3] = {
        {0.70f,0.15f,0.15f}, {0.15f,0.30f,0.60f},
        {0.75f,0.65f,0.10f}, {0.20f,0.50f,0.25f}
    };

    for (int i=0;i<NUM_PED;i++){
        float arg = animClock*pedSpeed[i] + pedPhase[i];
        float z   = pedCenterZ[i] + sinf(arg)*pedRangeZ[i];
        float vel = cosf(arg);   // sign tells us which way they're facing
        glPushMatrix();
          glTranslatef(pedX[i], 0, z);
          glRotatef(vel>=0.0f ? 0.0f : 180.0f, 0,1,0);
          drawPerson(pedColor[i][0], pedColor[i][1], pedColor[i][2]);
        glPopMatrix();
    }
}

// =======================================================================
//  DISPLAY
// =======================================================================
void drawHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Larger, taller HUD panel so every line has its own space.
    const float panelLeft   = 10.0f;
    const float panelRight  = 720.0f;
    const float panelBottom = winH - 142.0f;
    const float panelTop    = winH - 8.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f, 0.03f, 0.05f, 0.78f);
    glBegin(GL_QUADS);
        glVertex2f(panelLeft,  panelBottom);
        glVertex2f(panelRight, panelBottom);
        glVertex2f(panelRight, panelTop);
        glVertex2f(panelLeft,  panelTop);
    glEnd();
    glDisable(GL_BLEND);

    // Four control lines with comfortable vertical spacing.
    glColor3f(1.0f, 1.0f, 1.0f);
    const char* lines[] = {
        "WASD: Move   Q/E: Turn   Mouse Drag: Look Around",
        "1: Overview   2: Inside Apartment   3: Street   4: Metro   5: High View",
        "Arrow: Move Mall   R/F: Rotate Mall   Z/X: Scale Mall",
        "N: Day/Night   T: Auto Cycle   V: Inside/Outside   0: Reset Camera"
    };

    for (int i=0; i<4; i++){
        glRasterPos2f(20.0f, winH - 27.0f - i*22.0f);
        for (const char* c = lines[i]; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // Status is on its own separate line and uses a smaller font so it
    // never overlaps the controls.
    const char* viewName =
        insideView ? "INSIDE APARTMENT" :
        cameraPreset==1 ? "CITY OVERVIEW" :
        cameraPreset==3 ? "STREET LEVEL" :
        cameraPreset==4 ? "METRO VIEW" :
        cameraPreset==5 ? "HIGH PANORAMIC" : "FREE CAMERA";

    char buf[160];
    sprintf(buf, "Status: %s   |   Signal: %s   |   View: %s",
            dayFactor > 0.5f ? "DAY" : "NIGHT",
            trafficState==0 ? "RED" : (trafficState==1 ? "GREEN" : "YELLOW"),
            viewName);

    glColor3f(1.0f, 0.9f, 0.55f);
    glRasterPos2f(20.0f, winH - 119.0f);
    for (const char* c = buf; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display()
{
    float bgTop = 0.15f+0.5f*dayFactor;
    glClearColor(bgTop*0.5f, bgTop*0.6f, bgTop*0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    // ---- VIEW (viewing-coordinate) transformation via gluLookAt -------
    // Two independent viewing setups share the same eye/target math:
    // the free-fly outdoor camera, and the fixed "inside the apartment"
    // scenario that looks out through its window. Only the eye position
    // differs; look direction (yaw/pitch, mouse-controlled) is shared.
    float yawR = camYaw*M_PI/180.0f, pitR = camPitch*M_PI/180.0f;
    float lx = cosf(pitR)*cosf(yawR);
    float ly = sinf(pitR);
    float lz = cosf(pitR)*sinf(yawR);
    float eyeX = insideView ? (APT_X)      : camX;
    float eyeY = insideView ? (APT_EYE_Y)  : camY;
    float eyeZ = insideView ? (APT_EYE_Z)  : camZ;
    gluLookAt(eyeX, eyeY, eyeZ, eyeX+lx, eyeY+ly, eyeZ+lz, 0,1,0);

    setupLights();
    drawSky();

    // one continuous base ground plane under the whole scene - avoids any
    // gaps (previously only two far lawn strips were drawn, leaving bare
    // patches beside the road that showed the background clear-colour,
    // looking like a dark/blue gap in the ground)
    drawGroundPlane(0, 0, 260, 260, texGrass, 26, 26, -0.04f);

    drawRoadSystem();
    drawPark();
    drawCitySkyline();   // extra procedural buildings (OID-17)

    // ---- Coffee shop (fixed) ----
    glPushMatrix();
      glTranslatef(11, 0, 6);
      drawBuilding(4.5f, 3.2f, 4.0f, texBrick, "Coffee Shop");
    glPopMatrix();

    // ---- City Mall : OBJECT transform (translate/rotate/scale) --------
    glPushMatrix();
      glTranslatef(objX, objY, objZ);
      glRotatef(objRotY, 0,1,0);
      glScalef(objScale, objScale, objScale);
      drawBuilding(9.0f, 6.5f, 6.0f, texBrick, "City Mall");
      // framed window revealing the park & sky behind the mall
      glPushMatrix();
        glTranslatef(0, 4.0f, -3.05f);
        glRotatef(180,0,1,0);
        drawFramedWindow(2.4f, 2.0f);
      glPopMatrix();
    glPopMatrix();

    // ---- Apartment building (fixed) ----
    glPushMatrix();
      glTranslatef(11, 0, -14);
      // real opening cut through the front wall (2.4 x 2.0, centred at
      // world-height APT_EYE_Y) so the "inside" viewing scenario can
      // actually see the outside city through it, not just a wall.
      drawBuildingWithOpenWindow(5.0f, 8.5f, 5.0f, texBrick, "Apartments",
                                  2.4f, 2.0f, APT_EYE_Y);
      glPushMatrix();
        glTranslatef(0, APT_EYE_Y, 2.5f);
        drawFramedWindow(2.4f, 2.0f);
      glPopMatrix();
    glPopMatrix();

    // ---- Decorative windmill (continuously rotating - REQ 5) ----------
    glPushMatrix();
      glTranslatef(-34, 0, 4);
      drawWindmill();
    glPopMatrix();

    drawStreetLamp(9.5f, -1.5f);
    drawTrafficLight(6.0f, -1.5f);
    drawTraffic();        // whole fleet of cars (OID-18)
    drawPedestrians();    // people walking the footpaths (OID-19)

    // ---- Elevated metro rail (viaduct + station + moving train) -------
    drawMetroViaduct();
    drawMetroStation(-30.0f);
    drawMetroTrain();

    drawHUD();
    glutSwapBuffers();
}

// =======================================================================
//  RESHAPE
// =======================================================================
void reshape(int w, int h)
{
    if (h==0) h=1;
    winW=w; winH=h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(72.0, (double)w/(double)h, 0.08, 320.0);
    glMatrixMode(GL_MODELVIEW);
}

// =======================================================================
//  ANIMATION / IDLE UPDATE
// =======================================================================
void idle()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTimeMs) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;
    lastTimeMs = now;

    // continuously rotating windmill
    windmillAngle += dt * 110.0f;
    if (windmillAngle > 360.0f) windmillAngle -= 360.0f;

    // general animation clock (keeps running even if day/night is frozen)
    animClock += dt;
    swingAngle = sinf(animClock * 1.6f) * 28.0f;   // gentle swing pendulum

    // day/night auto cycle
    if (autoCycle){
        cycleClock += dt;
        float t = fmodf(cycleClock, CYCLE_LENGTH) / CYCLE_LENGTH;
        dayFactor = 0.5f + 0.5f*cosf(t*2.0f*M_PI);
        // +90 deg phase shift so the sun is exactly at its highest point
        // (zenith) when dayFactor peaks, and below the horizon at night.
        sunAngle = t*360.0f + 90.0f;
    }

    // traffic signal state machine
    trafficClock += dt;
    if (trafficState==1 && trafficClock>GREEN_TIME){ trafficState=2; trafficClock=0; }
    else if (trafficState==2 && trafficClock>YELLOW_TIME){ trafficState=0; trafficClock=0; }
    else if (trafficState==0 && trafficClock>RED_TIME){ trafficState=1; trafficClock=0; }

    // every car obeys the same shared traffic signal, but which side of
    // the stop line counts as "approaching" mirrors depending on which
    // way that particular car is driving (near lane vs far lane)
    for (int i=0;i<NUM_CARS;i++){
        bool approaching = (carDir[i] > 0)
            ? (carZ[i] < STOP_LINE_Z && carZ[i] > STOP_LINE_Z-6.0f)
            : (carZ[i] > STOP_LINE_Z && carZ[i] < STOP_LINE_Z+6.0f);
        bool mustStop = (trafficState!=1) && approaching;
        if (!mustStop){
            carZ[i] += dt*CAR_SPEED*carSpeedMul[i]*carDir[i];
            carStopped[i] = false;
        } else carStopped[i] = true;
        // loop each car back around once it drives off the far end
        if (carDir[i] > 0 && carZ[i] >  60.0f) carZ[i] = -60.0f;
        if (carDir[i] < 0 && carZ[i] < -60.0f) carZ[i] =  60.0f;
    }

    // metro train glides along the elevated viaduct and loops around
    metroZ += dt * METRO_SPEED;
    if (metroZ > 95.0f) metroZ = -95.0f;

    glutPostRedisplay();
}

// =======================================================================
//  INPUT : keyboard (translation/rotation/scaling of object & view),
//          mouse (camera orbit -> view transform)
// =======================================================================
void keyboard(unsigned char key, int, int)
{
    float camRad = camYaw*M_PI/180.0f;
    float fx = cosf(camRad), fz = sinf(camRad);

    switch (key){
        // Free-fly camera. Inside view remains fixed at the apartment window
        // so the user gets a true "standing inside and looking outside" shot.
        case 'w': case 'W':
            if(!insideView){ camX += fx*1.0f; camZ += fz*1.0f; cameraPreset=0; }
            break;
        case 's': case 'S':
            if(!insideView){ camX -= fx*1.0f; camZ -= fz*1.0f; cameraPreset=0; }
            break;
        case 'a': case 'A':
            if(!insideView){ camX += fz*1.0f; camZ -= fx*1.0f; cameraPreset=0; }
            break;
        case 'd': case 'D':
            if(!insideView){ camX -= fz*1.0f; camZ += fx*1.0f; cameraPreset=0; }
            break;

        case 'q': case 'Q': camYaw -= 3.0f; cameraPreset=0; break;
        case 'e': case 'E': camYaw += 3.0f; cameraPreset=0; break;

        case 'r': case 'R': objRotY += 5.0f; break;
        case 'f': case 'F': objRotY -= 5.0f; break;
        case 'z': case 'Z': objScale = fmaxf(0.3f, objScale-0.05f); break;
        case 'x': case 'X': objScale = fminf(3.0f, objScale+0.05f); break;

        case 'n': case 'N':
            dayFactor = 1.0f-dayFactor;
            autoCycle=false;
            break;

        case 't': case 'T':
            autoCycle = !autoCycle;
            break;

        // Quick camera presets for seeing the complete 3D city.
        case '1': setCameraPreset(1); break;
        case '2': setCameraPreset(2); break;
        case '3': setCameraPreset(3); break;
        case '4': setCameraPreset(4); break;
        case '5': setCameraPreset(5); break;

        case 'v': case 'V':
            if (insideView) setCameraPreset(0);
            else setCameraPreset(2);
            break;

        case '0':
            setCameraPreset(0);
            break;

        case 27:
            exit(0);
            break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    switch (key){
        case GLUT_KEY_UP:    objZ -= 0.5f; break;
        case GLUT_KEY_DOWN:  objZ += 0.5f; break;
        case GLUT_KEY_LEFT:  objX -= 0.5f; break;
        case GLUT_KEY_RIGHT: objX += 0.5f; break;
        case GLUT_KEY_PAGE_UP:   objY += 0.3f; break;
        case GLUT_KEY_PAGE_DOWN: objY = fmaxf(0.0f, objY-0.3f); break;
    }
    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y)
{
    if (button==GLUT_LEFT_BUTTON){
        mouseDown = (state==GLUT_DOWN);
        lastMouseX=x; lastMouseY=y;
    }
    if (button==3) camY += 0.5f;      // scroll up (some GLUT impls)
    if (button==4) camY -= 0.5f;      // scroll down
    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    if (mouseDown){
        int dx = x-lastMouseX, dy = y-lastMouseY;
        camYaw   += dx*0.28f;
        camPitch -= dy*0.28f;
        if (camPitch>82) camPitch=82;
        if (camPitch<-82) camPitch=-82;
        if (!insideView) cameraPreset=0;
        lastMouseX=x; lastMouseY=y;
        glutPostRedisplay();
    }
}

// =======================================================================
//  INIT
// =======================================================================
void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    // NOTE: GL_COLOR_MATERIAL intentionally NOT enabled - it would make
    // every glColor() call (used only for the unlit sky gradient) leak
    // into and override every object's glMaterialfv() ambient/diffuse
    // colors afterwards, which is what caused everything to render in
    // one flat tan/yellow color.

    srand((unsigned)time(NULL));
    texBrick    = genBrickTexture();
    texWood     = genWoodTexture();
    texRoad     = genRoadTexture();
    texGrass    = genGrassTexture();
    texGlass    = genGlassTexture();
    texWater    = genWaterTexture();
    texRoof     = genRoofTexture();
    texConcrete = genConcreteTexture();

    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);
}

// =======================================================================
//  MAIN
// =======================================================================
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Dhaka City 3D - Computer Graphics Project");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    printf("=========================================================\n");
    printf(" DHAKA CITY 3D\n");
    printf(" Camera : W/A/S/D move, Q/E turn, mouse-drag look around\n");
    printf(" Views  : 1=Overview  2=Inside Apartments  3=Street  4=Metro  5=High View\n");
    printf(" 0 = reset/free camera, V = toggle inside/outside view\n");
    printf(" Mall   : Arrow keys move, R/F rotate, Z/X scale, PgUp/PgDn height\n");
    printf(" N = day/night manually   T = auto day-night cycle\n");
    printf("=========================================================\n");

    glutMainLoop();
    return 0;
}
