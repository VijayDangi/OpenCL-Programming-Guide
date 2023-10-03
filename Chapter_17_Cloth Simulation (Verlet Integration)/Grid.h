#ifndef __VJD_GRID_H__
#define __VJD_GRID_H__

bool InitializeGrid( void);
void ResizeGrid( int width, int height);
void RenderGrid( vmath::mat4 projectionMatrix, vmath::mat4 viewMatrix);
void UpdateGrid(double deltaTime);
void UninitializeGrid( void);

#endif
