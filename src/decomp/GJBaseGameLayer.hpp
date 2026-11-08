#pragma once
#include <Geode/Geode.hpp>

class decomp_GJBaseGameLayer : public GJBaseGameLayer {
public:
    void moveObjects(cocos2d::CCArray* objects, double moveX, double moveY, bool unused);

    void processMoveActions();

    void processRotationActions();

    void optimized_processRotationActions();

    void processFollowActions();

    void optimized_processFollowActions();

    void processPlayerFollowActions(float dt);

    void processMoveActionsStep(float dt, bool visibleFrame);
};