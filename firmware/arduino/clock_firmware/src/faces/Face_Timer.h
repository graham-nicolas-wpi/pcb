#pragma once

#include "IWatchFace.h"

class Face_Timer : public IWatchFace {
 public:
  void render(const FaceRenderState& state) override;
};
