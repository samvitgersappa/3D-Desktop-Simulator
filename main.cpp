/*
This project was made by :
Adarsh Revankar, Akshaya M.
As Mini-Project for the course 'Computer Graphics Laboratory' under VTU.

This project was made with high effort from team, please do approciate the work.
I expect you guys to take few code snippets from this code & not the whole
project. Any improvements made will be accepted, mail the code to :
adarshrevankar0123@gmail.com
*/
#include "gl_includes.h"
#include <cstdlib>
#include <iostream>

#include "audio.h"
#include "bitmap.h"
#include "light.h"
#include "tooltip.h"

TooltipSystem tooltipSystem;
#include "motion.h"
#include "objects.h"
#include "parameter.h"

/* TEXTURE HANDLING */
void loadTexture(GLuint texture, const char *filename) {
  BmpLoader image(filename);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, image.iWidth, image.iHeight, GL_RGB,
                    GL_UNSIGNED_BYTE, image.data);
}

void textureInit() {
  // Create Texture.
  textures = new GLuint[NUM_TEXTURE];
  glGenTextures(NUM_TEXTURE, textures);

  // Load the Texture.
  for (int i = 0; i < NUM_TEXTURE; i++)
    loadTexture(textures[i], texPath[i]);
}

/* REDNDERING HANDLING */
void change_size(int w, int h) {
  // Update global parameters
  width = w;
  hight = h;
  // Do reshape
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  float ratio = 0.0f;
  h = h == 0 ? 1 : h;
  ratio = (float)w / (float)h;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(80.0f, ratio, 0.7f, 100.0f);
  glMatrixMode(GL_MODELVIEW);
}

void renderScene() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  gluLookAt(x, 5.0f, z, x + lx, y, z + lz, 0.0f, 1.0f, 0.0f);

  // 3D audio listener follows the camera.
  audio::update_listener({(float)x, 5.0f, (float)z},
                         {(float)lx, (float)(y - 5.0), (float)lz},
                         {0.0f, 1.0f, 0.0f});

  // Update and Draw Tooltips (AR Overlay)
  // Update and Draw Tooltips (AR Overlay)
  tooltipSystem.update(mouseGlobalX, mouseGlobalY);

  if (page == 1) {
    drawGround();
    drawCube();
    cpuView();
    drawCPU();

    // Update dynamic positions and visibility based on offset
    point3D gpuOff = gpu_.getOffset();
    tooltipSystem.updateComponent("NVIDIA GTX Graphics", 7.55f + gpuOff.x,
                                  4.2f + gpuOff.y, -4.65f + gpuOff.z, gpuOff.x);

    point3D fanOff = fan_.getOffset();
    // Fan position: glTranslatef(-0.974, .52, -0.745) +
    // glTranslatef(8.72, 4.321, -3.821) = (7.746, 4.841, -4.566)
    tooltipSystem.updateComponent("CPU Cooling Unit", 7.746f + fanOff.x,
                                  4.841f + fanOff.y, -4.566f + fanOff.z,
                                  fanOff.x);

    point3D ramOff = ram_.getOffset();
    // RAM renders in 3 spots, we track the main one or the one that moves first
    tooltipSystem.updateComponent("DDR4 RAM", 8.0f + ramOff.x, 4.8f + ramOff.y,
                                  -4.3f + ramOff.z, ramOff.x);

    point3D psuOff = psu_.getOffset();
    tooltipSystem.updateComponent("Power Supply", 8.0f + psuOff.x,
                                  3.4f + psuOff.y, -4.79f + psuOff.z, psuOff.x);

    point3D hddOff = harddisk_.getOffset();
    // HDD has scale factor 0.4, base pos was translated by 1/scale.
    // The render function: glScalef(0.4...); glTranslatef(8./0.4, 3.86/0.4,
    // -3.2/0.4); So world position is (8.0, 3.86, -3.2) + offset.
    tooltipSystem.updateComponent("Hard Disk", 8.0f + hddOff.x,
                                  3.86f + hddOff.y, -3.2f + hddOff.z, hddOff.x);

    // Processor (chipset) position update
    point3D chipOff = chipset_.getOffset();
    tooltipSystem.updateComponent("Processor", 8.0f + chipOff.x,
                                  4.77f + chipOff.y, -4.7f + chipOff.z,
                                  chipOff.x);

    // Draw tooltips on top of the CPU view
    tooltipSystem.draw((float)x, 5.0f, (float)z);
  } else if (page == 0) {
    front_page();
    progress_wheel();
  }
  glutSwapBuffers();
}

void opengl_init(void) {
  glEnable(GL_DEPTH_TEST);
  // Optional 3D audio (enabled when built with USE_OPENAL).
  if (audio::init()) {
    audio::preload_defaults();
    std::atexit(audio::shutdown);
  }

  // Register AR Tooltips
  // Positions derived from cpu_gpu.h, cpu_fan.h, etc.
  tooltipSystem.registerComponent("NVIDIA GTX Graphics", "High performance GPU",
                                  7.55f, 4.2f, -4.65f, 0.6f);
  // Fan position: (-0.974 + 8.72, 0.52 + 4.321, -0.745 + -3.821) =
  // (7.746, 4.841, -4.566)
  tooltipSystem.registerComponent("CPU Cooling Unit", "Spinning at 2000 RPM",
                                  7.746f, 4.841f, -4.566f, 0.5f);
  tooltipSystem.registerComponent("DDR4 RAM", "16GB 3200MHz", 8.0f, 4.8f, -4.3f,
                                  0.4f);
  // Processor is behind the fan - only show label after fan is removed
  // Position: glTranslatef(8., 4.77, -4.7) in cpu_chipset.h
  tooltipSystem.registerComponent("Processor", "Intel Core i7 CPU", 8.0f, 4.77f,
                                  -4.7f, 0.3f, "CPU Cooling Unit");
  // New Components
  tooltipSystem.registerComponent("Power Supply", "750W Gold Rated", 8.0f, 3.4f,
                                  -4.79f, 0.6f);
  tooltipSystem.registerComponent("Hard Disk", "2TB Mechanical Storage", 8.0f,
                                  3.86f, -3.2f, 0.5f);

  textureInit();
  glutDisplayFunc(renderScene);
  glutIdleFunc(renderScene);
  glutReshapeFunc(change_size);
  glutKeyboardFunc(processNormalKeys);
  glutSpecialFunc(processSpecialKeys);
  glutPassiveMotionFunc(mouse_follow); // Track mouse when button IS NOT pressed
  show_light_effect();
}

void setDeltaTime() {
  int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
  deltaTime = (timeSinceStart - oldTimeSinceStart) * 0.2f;
  oldTimeSinceStart = timeSinceStart;
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(width, hight);
  glutCreateWindow("Graphical Simulation of Desktop & it's Components");
  opengl_init();
  glutFullScreen();
  setDeltaTime();
  glutMainLoop();
  audio::shutdown();
  getchar();
  return 0;
}
