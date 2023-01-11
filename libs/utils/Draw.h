#pragma once

void drawLine(DrawingWindow& window, CanvasPoint from, CanvasPoint to, Colour colour, std::vector<std::vector<float>>& depthArray);

void drawLineUsingTexture(DrawingWindow& window, CanvasPoint from, CanvasPoint to, TextureMap texture, std::vector<std::vector<float>>& depthArray);

void drawTriangle(DrawingWindow& window, CanvasPoint a, CanvasPoint b, CanvasPoint c, Colour colour, std::vector<std::vector<float>>& depthArray);