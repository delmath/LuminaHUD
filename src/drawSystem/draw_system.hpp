#pragma once

#include "config_manager.hpp"

void drawTextBlock(const TextBlock& block);
bool drawButtonBlock(const ButtonBlock& btn);
void drawBarBlock(const BarBlock& bar, float fraction);
void drawImageBlock(const ImageBlock& img);
void drawErrors(const std::vector<std::string>& errors);
void clearImageCache();
