#ifndef BITMAP
#define BITMAP

#include "gl_includes.h"
#include <cmath>

void *times10 = GLUT_BITMAP_TIMES_ROMAN_10;
void *helv18 = GLUT_BITMAP_HELVETICA_18;
void *helv12 = GLUT_BITMAP_HELVETICA_12;

float prog;

void renderBitmapString(float x, float y, float z, void *font, char *string) {

  char *c;
  glRasterPos3f(x, y, z);
  for (c = string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}

void front_page() {
  // Save current matrices
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  // Set up 2D orthographic projection
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  gluOrtho2D(0, w, 0, h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  // Draw dark background
  glColor3f(0.05f, 0.05f, 0.1f);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(w, 0);
  glVertex2f(w, h);
  glVertex2f(0, h);
  glEnd();

  // Center text rendering
  float centerX = w / 2.0f;
  float centerY = h / 2.0f;

  glColor3f(1.0f, 1.0f, 1.0f);

  // Title - RV College
  renderBitmapString(centerX - 120, centerY + 180, 0, (void *)helv18,
                     (char *)"RV College Of Engineering");
  renderBitmapString(centerX - 130, centerY + 150, 0, (void *)helv18,
                     (char *)"Computer Science Department");

  // Subtitle
  glColor3f(0.8f, 0.8f, 0.8f);
  renderBitmapString(centerX - 70, centerY + 100, 0, (void *)helv12,
                     (char *)"A MINI PROJECT ON");

  glColor3f(0.0f, 1.0f, 0.5f); // Green for main title
  renderBitmapString(
      centerX - 250, centerY + 60, 0, (void *)helv18,
      (char *)"GRAPHICAL SIMULATION OF DESKTOP AND ITS COMPONENTS");

  // Team members
  glColor3f(1.0f, 1.0f, 1.0f);
  renderBitmapString(centerX - 20, centerY - 20, 0, (void *)helv12,
                     (char *)"BY:");

  glColor3f(0.9f, 0.9f, 0.9f);
  renderBitmapString(centerX - 55, centerY - 50, 0, (void *)helv12,
                     (char *)"VIBHAV SIMHA");
  renderBitmapString(centerX - 40, centerY - 75, 0, (void *)helv12,
                     (char *)"AARYAN P");
  renderBitmapString(centerX - 90, centerY - 100, 0, (void *)helv12,
                     (char *)"SAMVIT SANAT GERSAPPA");

  glEnable(GL_DEPTH_TEST);

  // Restore matrices
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void progress_wheel(void) {
  double i;

  // Save current matrices
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  gluOrtho2D(0, w, 0, h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_DEPTH_TEST);

  float centerX = w / 2.0f;
  float centerY = h / 2.0f;

  // Progress wheel animation
  if (prog > 6.284f)
    prog = 0.0f;
  prog += 0.05f;

  glPointSize(6.0);
  glBegin(GL_POINTS);
  for (i = 0; i < prog; i = i + 0.15f) {
    float colorVal = (float)(i / 6.284f);
    glColor3f(colorVal, 0.3f * colorVal, 0.0f);
    glVertex2f(centerX + (float)(sin(i) * 30.0f),
               centerY - 180 + (float)(cos(i) * 30.0f));
  }
  glEnd();

  glColor3f(0.7f, 0.7f, 0.7f);
  renderBitmapString(centerX - 30, centerY - 230, 0, (void *)helv12,
                     (char *)"Loading...");

  glColor3f(1.0f, 1.0f, 1.0f);
  renderBitmapString(centerX - 95, centerY - 260, 0, (void *)helv12,
                     (char *)"Press ENTER to continue...");

  glEnable(GL_DEPTH_TEST);

  // Restore matrices
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

#endif BITMAP