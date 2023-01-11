#pragma once

#include <DrawingWindow.h>
#include <CanvasPoint.h>
#include <Colour.h>
#include <vector>
#include <TextureMap.h>

void drawLine(DrawingWindow& window, CanvasPoint from, CanvasPoint to, Colour colour, std::vector<std::vector<float>>& depthArray);

void drawLineUsingTexture(DrawingWindow& window, CanvasPoint from, CanvasPoint to, TextureMap texture, std::vector<std::vector<float>>& depthArray);

void drawTriangle(DrawingWindow& window, CanvasPoint a, CanvasPoint b, CanvasPoint c, Colour colour, std::vector<std::vector<float>>& depthArray);

void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour, std::vector<std::vector<float>>& depthArray);

void drawTexturedTriangle(DrawingWindow& window, CanvasTriangle triangle, TextureMap texture, std::vector<std::vector<float>>& depthArray);