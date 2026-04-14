#pragma once

#include "IWatchFace.h"

class Face_ClassicClock : public IWatchFace {
 public:
  void render(const FaceRenderState& state) override;
};
