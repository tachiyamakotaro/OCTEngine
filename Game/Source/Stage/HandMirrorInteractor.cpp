/**
 * @file HandMirrorInteractor.cpp
 * @brief プレイヤーによる手鏡の操作（構える・傾ける）
 */
#include "stdafx.h"
#include "HandMirrorInteractor.h"
#include "Stage.h"
#include "Source/Actor/Character/CharacterBase.h"


namespace app
{
	namespace stage
	{
		void HandMirrorInteractor::Update(Stage& stage, const actor::CharacterBase& character, const float deltaTime)
		{
			const actor::CharacterCommand& command = character.GetCommand();
			const math::Rect body = character.GetController().GetBounds();
			const int facing = character.GetFacing();

			m_hasHandMirror = false;
			m_isRaised = false;
			for (HandMirror& mirror : stage.GetGimmicks().View<HandMirror>())
			{
				// 拾っていない手鏡と、2つ目からの手鏡は構えない
				if (!mirror.isCollected || m_hasHandMirror)
				{
					mirror.isRaised = false;
					continue;
				}
				m_hasHandMirror = true;

				// 構えている間だけ傾ける（下ろしたらまっすぐに戻す）
				mirror.isRaised = command.handMirrorHold;
				if (mirror.isRaised)
				{
					mirror.Tilt(command.handMirrorAim, deltaTime);
				}
				else
				{
					mirror.tiltDeg = 0.0f;
				}
				m_isRaised = mirror.isRaised;

				// 手元の位置（体の中心から前へ、胸のあたり）
				mirror.facing = facing;
				mirror.holdPosition = {
					(body.min.x + body.max.x) * 0.5f + static_cast<float>(facing) * handOffsetX,
					body.min.y + body.Height() * handHeightRate,
				};
			}
		}


		void HandMirrorInteractor::Reset()
		{
			m_isRaised = false;
		}
	}
}
