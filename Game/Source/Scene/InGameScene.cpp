/**
 * @file InGameScene.cpp
 * @brief インゲーム（ステージを遊ぶシーン）
 */
#include "stdafx.h"
#include "InGameScene.h"

#include "imgui.h"
#include "ResultScene.h"
#include "Source/Actor/Character/CharacterStateMachine.h"
#include "Source/Actor/Character/CharacterStatus.h"
#include "Source/Core/DebugUI.h"
#include "Source/Sound/SoundManager.h"
#include "Source/Stage/Stages/Stage01.h"
#include <cmath>


