#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "gl_includes.h"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Structure to hold component data
struct ComponentInfo {
  std::string name;
  std::string description;
  float x, y, z;
  float radius;
};

class TooltipSystem {
private:
  std::vector<ComponentInfo> components;
  int focusedIndex = -1;

  // Helper to render text at a specific 3D location
  void renderTextOriginal(std::string text, float x, float y,
                          void *font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y); // Use 2D raster position relative to the local frame
    for (char c : text) {
      glutBitmapCharacter(font, c);
    }
  }

  // Draw a fancy bracket around the object
  void drawBracket(float radius) {
    float r = radius;
    float corner = r * 0.3f; // Size of the corner lines

    glLineWidth(2.5f);
    glColor3f(0.0f, 0.8f, 1.0f); // Cyan/Ice blue

    static float pulse = 0.0f;
    pulse += 0.08f;
    float scale = 1.0f + 0.05f * sin(pulse);

    glPushMatrix();
    glScalef(scale, scale, scale);

    // Draw a partial wireframe box (corners only)
    glBegin(GL_LINES);
    // Bottom-Left-Front
    glVertex3f(-r, -r, r);
    glVertex3f(-r + corner, -r, r);
    glVertex3f(-r, -r, r);
    glVertex3f(-r, -r + corner, r);
    glVertex3f(-r, -r, r);
    glVertex3f(-r, -r, r - corner);
    // Top-Right-Front
    glVertex3f(r, r, r);
    glVertex3f(r - corner, r, r);
    glVertex3f(r, r, r);
    glVertex3f(r, r - corner, r);
    glVertex3f(r, r, r);
    glVertex3f(r, r, r - corner);
    // Top-Left-Back
    glVertex3f(-r, r, -r);
    glVertex3f(-r + corner, r, -r);
    glVertex3f(-r, r, -r);
    glVertex3f(-r, r - corner, -r);
    glVertex3f(-r, r, -r);
    glVertex3f(-r, r, -r + corner);
    // Bottom-Right-Back
    glVertex3f(r, -r, -r);
    glVertex3f(r - corner, -r, -r);
    glVertex3f(r, -r, -r);
    glVertex3f(r, -r + corner, -r);
    glVertex3f(r, -r, -r);
    glVertex3f(r, -r, -r + corner);
    glEnd();
    glPopMatrix();
  }

public:
  void registerComponent(std::string name, std::string description, float x,
                         float y, float z, float radius = 0.5f) {
    components.push_back({name, description, x, y, z, radius});
  }

  // Dynamic update for moving parts
  void updateComponent(std::string name, float x, float y, float z) {
    for (auto &c : components) {
      if (c.name == name) {
        c.x = x;
        c.y = y;
        c.z = z;
        return;
      }
    }
  }

  // Raycast from mouse position
  void update(int mouseX, int mouseY) {
    focusedIndex = -1;

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble nearX, nearY, nearZ;
    GLdouble farX, farY, farZ;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Flip Y for viewport coordinates
    GLint winY = viewport[3] - mouseY;

    // Unproject near plane (z=0)
    gluUnProject(mouseX, winY, 0.0, modelview, projection, viewport, &nearX,
                 &nearY, &nearZ);
    // Unproject far plane (z=1)
    gluUnProject(mouseX, winY, 1.0, modelview, projection, viewport, &farX,
                 &farY, &farZ);

    // Ray Origin and Direction
    double ox = nearX;
    double oy = nearY;
    double oz = nearZ;

    double dx = farX - nearX;
    double dy = farY - nearY;
    double dz = farZ - nearZ;

    // Normalize direction
    double len = sqrt(dx * dx + dy * dy + dz * dz);
    dx /= len;
    dy /= len;
    dz /= len;

    // Ray-Sphere Intersection for each component
    for (int i = 0; i < components.size(); i++) {
      const auto &c = components[i];

      // Vector from Ray Origin to Sphere Center
      double fx = c.x - ox;
      double fy = c.y - oy;
      double fz = c.z - oz;

      // Project f onto d (t_ca)
      double t = fx * dx + fy * dy + fz * dz;

      if (t < 0)
        continue; // Sphere is behind the ray origin

      // Closest point on ray to center
      double px = ox + t * dx;
      double py = oy + t * dy;
      double pz = oz + t * dz;

      // Distance squared from center to ray
      double d2 = (c.x - px) * (c.x - px) + (c.y - py) * (c.y - py) +
                  (c.z - pz) * (c.z - pz);

      if (d2 < (c.radius * c.radius)) {
        // Intersection!
        focusedIndex = i;
        // Since this loop goes linearly, the last one found will be focused.
        // Ideally should check nearest 't', but good enough for
        // non-overlapping.
        return;
      }
    }
  }

  // Draw now requires camera position to calculate billboard rotation
  void draw(float camX, float camY, float camZ) {
    if (focusedIndex == -1)
      return;

    const auto &c = components[focusedIndex];

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // 1. Draw the "Target" Bracket at the object location
    glPushMatrix();
    glTranslatef(c.x, c.y, c.z);
    drawBracket(c.radius);
    glPopMatrix();

    // 2. Draw "Leader Line" floating upwards
    float textHeightOffset = c.radius + 0.8f;
    glLineWidth(1.0f);
    glColor3f(0.0f, 0.8f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(c.x, c.y, c.z);
    glVertex3f(c.x, c.y + textHeightOffset, c.z);
    glEnd();

    // 3. Billboarded Text Panel
    float dx = camX - c.x;
    float dz = camZ - c.z;
    float angleY = atan2(dx, dz) * 180.0f / M_PI;

    glPushMatrix();
    glTranslatef(c.x, c.y + textHeightOffset, c.z);
    glRotatef(angleY, 0.0f, 1.0f, 0.0f);

    // Draw Semi-transparent background panel
    float panelWidth = 1.8f;
    float panelHeight = 0.6f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.1f, 0.2f, 0.8f);
    glBegin(GL_QUADS);
    glVertex3f(-panelWidth / 2, 0.0f, 0.0f);
    glVertex3f(panelWidth / 2, 0.0f, 0.0f);
    glVertex3f(panelWidth / 2, panelHeight, 0.0f);
    glVertex3f(-panelWidth / 2, panelHeight, 0.0f);
    glEnd();

    // Border
    glColor3f(0.0f, 0.8f, 1.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-panelWidth / 2, 0.0f, 0.0f);
    glVertex3f(panelWidth / 2, 0.0f, 0.0f);
    glVertex3f(panelWidth / 2, panelHeight, 0.0f);
    glVertex3f(-panelWidth / 2, panelHeight, 0.0f);
    glEnd();

    // Text
    glColor3f(1.0f, 1.0f, 1.0f);
    renderTextOriginal(c.name, -panelWidth / 2 + 0.1f, panelHeight - 0.25f,
                       GLUT_BITMAP_HELVETICA_18);
    renderTextOriginal(c.description, -panelWidth / 2 + 0.1f,
                       panelHeight - 0.45f, GLUT_BITMAP_HELVETICA_12);

    glDisable(GL_BLEND);
    glPopMatrix();

    glEnable(GL_LIGHTING); // Restore
  }
};

#endif
