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
  float hoverTime; // Track how long component has been hovered
  bool isVisible;  // Whether the component is visible (not disassembled out of
                   // view)
  std::string blockedBy; // Name of component that blocks this one's label
                         // (empty = not blocked)
  bool blockerRemoved;   // Whether the blocking component has been removed
};

class TooltipSystem {
private:
  std::vector<ComponentInfo> components;
  int focusedIndex = -1;
  int prevFocusedIndex = -1;
  float globalPulse = 0.0f;

  // Helper to render text at a specific 3D location with shadow for better
  // visibility
  void renderTextWithShadow(std::string text, float x, float y,
                            void *font = GLUT_BITMAP_HELVETICA_18) {
    // Draw shadow first (offset slightly)
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glRasterPos2f(x + 0.02f, y - 0.02f);
    for (char c : text) {
      glutBitmapCharacter(font, c);
    }
    // Draw main text
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (char c : text) {
      glutBitmapCharacter(font, c);
    }
  }

  // Draw glow effect around an object
  void drawGlowEffect(float radius, float intensity) {
    float r = radius * 1.3f;
    int segments = 32;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Multiple layers of glow for better effect
    for (int layer = 0; layer < 3; layer++) {
      float layerRadius = r * (1.0f + layer * 0.15f);
      float alpha = intensity * (0.3f - layer * 0.08f);

      glColor4f(0.0f, 0.8f, 1.0f, alpha);
      glBegin(GL_LINE_LOOP);
      for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(layerRadius * cos(angle), 0.0f, layerRadius * sin(angle));
      }
      glEnd();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  // Draw an enhanced fancy bracket around the object with glow
  void drawBracket(float radius, float hoverIntensity) {
    float r = radius;
    float corner = r * 0.4f; // Slightly larger corner lines

    // Pulsing effect
    float scale = 1.0f + 0.08f * sin(globalPulse) * hoverIntensity;

    // Draw glow layers first
    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (int glow = 2; glow >= 0; glow--) {
      float glowWidth = 2.5f + glow * 2.0f;
      float alpha = (0.4f - glow * 0.12f) * hoverIntensity;
      glLineWidth(glowWidth);
      glColor4f(0.0f, 0.8f, 1.0f, alpha);

      glPushMatrix();
      glScalef(scale, scale, scale);

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

    // Main sharp bracket
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(3.0f);
    glColor4f(0.0f, 1.0f, 1.0f, 1.0f); // Brighter cyan

    glPushMatrix();
    glScalef(scale, scale, scale);

    glBegin(GL_LINES);
    // All 8 corners
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
    // Additional corners for full visibility
    // Top-Left-Front
    glVertex3f(-r, r, r);
    glVertex3f(-r + corner, r, r);
    glVertex3f(-r, r, r);
    glVertex3f(-r, r - corner, r);
    glVertex3f(-r, r, r);
    glVertex3f(-r, r, r - corner);
    // Bottom-Right-Front
    glVertex3f(r, -r, r);
    glVertex3f(r - corner, -r, r);
    glVertex3f(r, -r, r);
    glVertex3f(r, -r + corner, r);
    glVertex3f(r, -r, r);
    glVertex3f(r, -r, r - corner);
    // Top-Right-Back
    glVertex3f(r, r, -r);
    glVertex3f(r - corner, r, -r);
    glVertex3f(r, r, -r);
    glVertex3f(r, r - corner, -r);
    glVertex3f(r, r, -r);
    glVertex3f(r, r, -r + corner);
    // Bottom-Left-Back
    glVertex3f(-r, -r, -r);
    glVertex3f(-r + corner, -r, -r);
    glVertex3f(-r, -r, -r);
    glVertex3f(-r, -r + corner, -r);
    glVertex3f(-r, -r, -r);
    glVertex3f(-r, -r, -r + corner);
    glEnd();

    glPopMatrix();
    glDisable(GL_BLEND);
    glPopMatrix();
  }

public:
  void registerComponent(std::string name, std::string description, float x,
                         float y, float z, float radius = 0.5f,
                         std::string blockedBy = "") {
    components.push_back(
        {name, description, x, y, z, radius, 0.0f, true, blockedBy, false});
  }

  // Dynamic update for moving parts - this keeps tooltip synced with component
  // position. Also tracks visibility based on whether component has moved
  // outside the case
  void updateComponent(std::string name, float x, float y, float z,
                       float offsetX = 0.0f) {
    bool componentRemoved = (offsetX < -3.5f); // Component fully disassembled

    for (auto &c : components) {
      if (c.name == name) {
        c.x = x;
        c.y = y;
        c.z = z;
        // Hide tooltip if component has moved significantly out of the case
        // (disassembled) Components move in negative X direction when
        // disassembling
        c.isVisible = (offsetX > -1.5f);
      }
      // If this component was blocking others, mark them as unblocked
      if (componentRemoved && c.blockedBy == name) {
        c.blockerRemoved = true;
      }
    }
  }

  // Raycast from mouse position with improved detection
  void update(int mouseX, int mouseY) {
    prevFocusedIndex = focusedIndex;
    focusedIndex = -1;

    // Update global pulse for animations
    globalPulse += 0.1f;
    if (globalPulse > 2 * M_PI)
      globalPulse -= 2 * M_PI;

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

    float closestT = 1e9f;

    // Ray-Sphere Intersection for each component with improved detection radius
    for (int i = 0; i < components.size(); i++) {
      auto &c = components[i];

      // Skip components that have been disassembled out of view
      if (!c.isVisible)
        continue;

      // Skip components that are blocked by another component that hasn't been
      // removed
      if (!c.blockedBy.empty() && !c.blockerRemoved)
        continue;

      // Use slightly larger detection radius for easier hovering
      float detectionRadius = c.radius * 1.2f;

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

      if (d2 < (detectionRadius * detectionRadius) && t < closestT) {
        // Intersection! Take the closest one
        closestT = t;
        focusedIndex = i;
      }
    }

    // Update hover times for smooth animations
    for (int i = 0; i < components.size(); i++) {
      if (i == focusedIndex) {
        components[i].hoverTime =
            std::min(components[i].hoverTime + 0.1f, 1.0f);
      } else {
        components[i].hoverTime =
            std::max(components[i].hoverTime - 0.15f, 0.0f);
      }
    }
  }

  // Draw now requires camera position to calculate billboard rotation
  void draw(float camX, float camY, float camZ) {
    if (focusedIndex == -1)
      return;

    auto &c = components[focusedIndex];
    float hoverIntensity = c.hoverTime; // Smooth fade-in

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST); // Draw on top of everything

    // 1. Draw the enhanced "Target" Bracket at the object location
    glPushMatrix();
    glTranslatef(c.x, c.y, c.z);
    drawBracket(c.radius, hoverIntensity);
    glPopMatrix();

    // 2. Draw glowing "Leader Line" floating upwards
    float textHeightOffset = c.radius + 1.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Glow effect for leader line
    for (int glow = 2; glow >= 0; glow--) {
      float glowWidth = 1.0f + glow * 1.5f;
      float alpha = (0.5f - glow * 0.15f) * hoverIntensity;
      glLineWidth(glowWidth);
      glColor4f(0.0f, 0.8f, 1.0f, alpha);
      glBegin(GL_LINES);
      glVertex3f(c.x, c.y + c.radius * 0.3f, c.z);
      glVertex3f(c.x, c.y + textHeightOffset, c.z);
      glEnd();
    }

    // Main leader line
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    glColor4f(0.0f, 1.0f, 1.0f, hoverIntensity);
    glBegin(GL_LINES);
    glVertex3f(c.x, c.y + c.radius * 0.3f, c.z);
    glVertex3f(c.x, c.y + textHeightOffset, c.z);
    glEnd();

    // Small connecting dot
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    glVertex3f(c.x, c.y + textHeightOffset, c.z);
    glEnd();

    // 3. Billboarded Text Panel with enhanced visibility
    float dx = camX - c.x;
    float dz = camZ - c.z;
    float angleY = atan2(dx, dz) * 180.0f / M_PI;

    glPushMatrix();
    glTranslatef(c.x, c.y + textHeightOffset + 0.05f, c.z);
    glRotatef(angleY, 0.0f, 1.0f, 0.0f);

    // Larger panel for better visibility
    float panelWidth = 2.2f;
    float panelHeight = 0.8f;
    float panelPadding = 0.08f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Outer glow effect for panel
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.0f, 0.5f, 0.8f, 0.3f * hoverIntensity);
    glBegin(GL_QUADS);
    glVertex3f(-panelWidth / 2 - 0.1f, -0.05f, 0.01f);
    glVertex3f(panelWidth / 2 + 0.1f, -0.05f, 0.01f);
    glVertex3f(panelWidth / 2 + 0.1f, panelHeight + 0.1f, 0.01f);
    glVertex3f(-panelWidth / 2 - 0.1f, panelHeight + 0.1f, 0.01f);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Main dark panel background with gradient effect
    glColor4f(0.02f, 0.08f, 0.15f, 0.92f * hoverIntensity);
    glBegin(GL_QUADS);
    glVertex3f(-panelWidth / 2, 0.0f, 0.0f);
    glVertex3f(panelWidth / 2, 0.0f, 0.0f);
    glVertex3f(panelWidth / 2, panelHeight, 0.0f);
    glVertex3f(-panelWidth / 2, panelHeight, 0.0f);
    glEnd();

    // Glowing border with multiple layers
    for (int b = 0; b < 2; b++) {
      float borderOffset = b * 0.02f;
      float borderAlpha = (1.0f - b * 0.4f) * hoverIntensity;
      glColor4f(0.0f, 0.9f, 1.0f, borderAlpha);
      glLineWidth(2.5f - b * 0.8f);
      glBegin(GL_LINE_LOOP);
      glVertex3f(-panelWidth / 2 - borderOffset, -borderOffset, 0.001f);
      glVertex3f(panelWidth / 2 + borderOffset, -borderOffset, 0.001f);
      glVertex3f(panelWidth / 2 + borderOffset, panelHeight + borderOffset,
                 0.001f);
      glVertex3f(-panelWidth / 2 - borderOffset, panelHeight + borderOffset,
                 0.001f);
      glEnd();
    }

    // Header bar accent
    glColor4f(0.0f, 0.7f, 0.9f, 0.4f * hoverIntensity);
    glBegin(GL_QUADS);
    glVertex3f(-panelWidth / 2 + panelPadding, panelHeight - 0.02f, 0.001f);
    glVertex3f(panelWidth / 2 - panelPadding, panelHeight - 0.02f, 0.001f);
    glVertex3f(panelWidth / 2 - panelPadding, panelHeight - 0.04f, 0.001f);
    glVertex3f(-panelWidth / 2 + panelPadding, panelHeight - 0.04f, 0.001f);
    glEnd();

    // Text with shadow effect for better readability
    if (hoverIntensity > 0.3f) {
      renderTextWithShadow(c.name, -panelWidth / 2 + 0.12f, panelHeight - 0.30f,
                           GLUT_BITMAP_HELVETICA_18);

      glColor3f(0.7f, 0.9f, 1.0f); // Slight blue tint for description
      glRasterPos2f(-panelWidth / 2 + 0.12f, panelHeight - 0.55f);
      for (char ch : c.description) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ch);
      }
    }

    glDisable(GL_BLEND);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST); // Restore depth test
    glEnable(GL_LIGHTING);   // Restore lighting
  }
};

#endif
