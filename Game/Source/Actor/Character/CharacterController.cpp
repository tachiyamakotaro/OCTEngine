/**
 * @file CharacterController.cpp
 * @brief 横スクロール用キャラクターコントローラー（体の移動と地形との当たり判定）
 */
#include "stdafx.h"
#include "CharacterController.h"
#include <cfloat>
#include <cmath>
#include "Source/Physics/ICollisionWorld.h"


namespace app
{
	namespace actor
	{
		namespace
		{
			/** 押し出したときに地形との間に空けるすき間（浮動小数点の誤差でめり込まないように） */
			constexpr float SKIN = 0.01f;
			/** 接地・壁の判定で調べる距離（SKIN より大きくすること） */
			constexpr float CONTACT_PROBE = 0.1f;
			/** 1回の分割移動で動ける最大量（体の大きさに対する割合）。すり抜け防止 */
			constexpr float MAX_STEP_RATIO = 0.45f;
			/** 1フレームの最大分割数 */
			constexpr int MAX_SUB_STEPS = 16;
		}


		void CharacterController::Init(
			const math::Vec2& footPosition, const float width, const float height,
			const physics::ICollisionWorld* world)
		{
			m_position = footPosition;
			m_velocity = math::Vec2::Zero();
			m_width = width;
			m_height = height;
			m_collisionWorld = world;
			UpdateContacts();
		}


		void CharacterController::Move(const float deltaTime)
		{
			const bool wasOnGround = m_isOnGround;

			// 重力
			if (m_isGravityEnabled)
			{
				m_velocity.y += m_gravity * deltaTime;
				if (m_velocity.y < -m_maxFallSpeed)
				{
					m_velocity.y = -m_maxFallSpeed;
				}
			}

			m_isHitCeiling = false;
			const math::Vec2 delta = m_velocity * deltaTime;

			if (m_collisionWorld == nullptr)
			{
				m_position += delta;
				m_isOnGround = false;
				m_isJustLanded = false;
				m_isTouchingWallLeft = false;
				m_isTouchingWallRight = false;
				return;
			}

			// 前のフレームで地面の上にいたら、乗っている動く床が動いた分だけ一緒に動く
			// （ステージの更新で床はもう動いているので、めり込みを押し出すより先に運んでおく）
			if (wasOnGround)
			{
				math::Vec2 carry = math::Vec2::Zero();
				if (m_collisionWorld->GetCarryDelta(GetGroundProbe(), carry))
				{
					// 縦を先に動かす（床が上がったとき、足が床にめり込んだまま横に動かすと、床の横へはじき出されてしまう）
					MoveAxisY(carry.y);
					MoveAxisX(carry.x);
				}
			}

			// 地形の方が動いてめり込んでいたら先に押し出す
			ResolvePenetration();

			// 速いときに薄い床をすり抜けないよう、移動を細かく分ける
			const float maxStep = math::Min(m_width, m_height) * MAX_STEP_RATIO;
			const float largest = math::Max(math::Abs(delta.x), math::Abs(delta.y));
			int subSteps = static_cast<int>(std::ceil(largest / maxStep));
			if (subSteps < 1) { subSteps = 1; }
			if (subSteps > MAX_SUB_STEPS) { subSteps = MAX_SUB_STEPS; }
			const math::Vec2 step = delta * (1.0f / static_cast<float>(subSteps));

			// X と Y を別々に動かす（角に引っかかりにくく、どちら向きの衝突かがはっきりする）
			for (int i = 0; i < subSteps; ++i)
			{
				MoveAxisX(step.x);
				MoveAxisY(step.y);
			}

			UpdateContacts();
			m_isJustLanded = !wasOnGround && m_isOnGround;
		}


		void CharacterController::MoveAxisX(const float dx)
		{
			if (dx == 0.0f)
			{
				return;
			}

			m_position.x += dx;

			m_hitBuffer.clear();
			m_collisionWorld->QueryOverlap(GetBounds(), m_hitBuffer);
			for (const math::Rect& solid : m_hitBuffer)
			{
				// 前の押し出しで既に解消されていることがある
				if (!GetBounds().Overlaps(solid))
				{
					continue;
				}

				if (dx > 0.0f)
				{
					m_position.x = solid.min.x - m_width * 0.5f - SKIN;
					if (m_velocity.x > 0.0f) { m_velocity.x = 0.0f; }
				}
				else
				{
					m_position.x = solid.max.x + m_width * 0.5f + SKIN;
					if (m_velocity.x < 0.0f) { m_velocity.x = 0.0f; }
				}
			}
		}


		void CharacterController::MoveAxisY(const float dy)
		{
			if (dy == 0.0f)
			{
				return;
			}

			m_position.y += dy;

			m_hitBuffer.clear();
			m_collisionWorld->QueryOverlap(GetBounds(), m_hitBuffer);
			for (const math::Rect& solid : m_hitBuffer)
			{
				if (!GetBounds().Overlaps(solid))
				{
					continue;
				}

				if (dy < 0.0f)
				{
					// 床に着地
					m_position.y = solid.max.y + SKIN;
					if (m_velocity.y < 0.0f) { m_velocity.y = 0.0f; }
				}
				else
				{
					// 天井に頭をぶつけた
					m_position.y = solid.min.y - m_height - SKIN;
					if (m_velocity.y > 0.0f) { m_velocity.y = 0.0f; }
					m_isHitCeiling = true;
				}
			}
		}


		void CharacterController::ResolvePenetration()
		{
			// 複数の地形にめり込んでいることもあるので、数回くり返す
			constexpr int MAX_ITERATIONS = 4;
			for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration)
			{
				m_hitBuffer.clear();
				m_collisionWorld->QueryOverlap(GetBounds(), m_hitBuffer);
				if (m_hitBuffer.empty())
				{
					return;
				}

				const math::Rect body = GetBounds();
				const math::Rect& solid = m_hitBuffer.front();

				// 4方向それぞれ、押し出すのに必要な距離
				const float pushUp = solid.max.y - body.min.y;
				const float pushDown = body.max.y - solid.min.y;
				const float pushLeft = body.max.x - solid.min.x;
				const float pushRight = solid.max.x - body.min.x;

				// 一番短い方向へ。同じなら上を優先（足場に乗る方が自然なため）
				const float minHorizontal = math::Min(pushLeft, pushRight);
				if (pushUp <= minHorizontal && pushUp <= pushDown)
				{
					m_position.y += pushUp + SKIN;
					if (m_velocity.y < 0.0f) { m_velocity.y = 0.0f; }
				}
				else if (minHorizontal <= pushDown)
				{
					if (pushLeft < pushRight)
					{
						m_position.x -= pushLeft + SKIN;
						if (m_velocity.x > 0.0f) { m_velocity.x = 0.0f; }
					}
					else
					{
						m_position.x += pushRight + SKIN;
						if (m_velocity.x < 0.0f) { m_velocity.x = 0.0f; }
					}
				}
				else
				{
					m_position.y -= pushDown + SKIN;
					if (m_velocity.y > 0.0f) { m_velocity.y = 0.0f; }
				}
			}
		}


		void CharacterController::UpdateContacts()
		{
			if (m_collisionWorld == nullptr)
			{
				m_isOnGround = false;
				m_isTouchingWallLeft = false;
				m_isTouchingWallRight = false;
				return;
			}

			const math::Rect body = GetBounds();

			// 足元のすぐ下を調べる
			// 上昇中は接地していない扱い（ジャンプした瞬間に地面判定が残らないように）
			m_isOnGround = (m_velocity.y <= 0.0f) && m_collisionWorld->AnyOverlap(GetGroundProbe());

			// 左右の壁（上下を少し削って、床や天井を壁と誤判定しないようにする）
			const math::Rect rightProbe{
				{ body.max.x, body.min.y + CONTACT_PROBE },
				{ body.max.x + CONTACT_PROBE, body.max.y - CONTACT_PROBE }
			};
			const math::Rect leftProbe{
				{ body.min.x - CONTACT_PROBE, body.min.y + CONTACT_PROBE },
				{ body.min.x, body.max.y - CONTACT_PROBE }
			};
			m_isTouchingWallRight = m_collisionWorld->AnyOverlap(rightProbe);
			m_isTouchingWallLeft = m_collisionWorld->AnyOverlap(leftProbe);
		}


		math::Rect CharacterController::GetBounds() const
		{
			return GetBoundsAt(m_position);
		}


		math::Rect CharacterController::GetGroundProbe() const
		{
			// 足元のすぐ下の薄い箱（横幅を少し削って、壁に触れているだけで接地扱いにならないようにする）
			const math::Rect body = GetBounds();
			return {
				{ body.min.x + SKIN, body.min.y - CONTACT_PROBE },
				{ body.max.x - SKIN, body.min.y }
			};
		}


		math::Rect CharacterController::GetBoundsAt(const math::Vec2& footPosition) const
		{
			return math::Rect::FromFoot(footPosition, m_width, m_height);
		}


		bool CharacterController::CanResizeHeight(const float newHeight) const
		{
			if (newHeight <= m_height)
			{
				return true;
			}
			return IsBoxFree(math::Rect::FromFoot(m_position, m_width, newHeight));
		}


		bool CharacterController::TryResizeHeight(const float newHeight)
		{
			if (!CanResizeHeight(newHeight))
			{
				return false;
			}
			m_height = newHeight;
			return true;
		}


		bool CharacterController::IsBoxFree(const math::Rect& box) const
		{
			if (m_collisionWorld == nullptr)
			{
				return true;
			}
			return m_collisionWorld->IsBoxFree(box);
		}


		bool CharacterController::HasGroundAhead(const int direction, const float distance, const float depth) const
		{
			if (m_collisionWorld == nullptr)
			{
				return false;
			}
			const math::Rect body = GetBounds();
			const float frontX = (direction > 0 ? body.max.x : body.min.x) + static_cast<float>(direction) * distance;
			const math::Rect probe{
				{ frontX - 0.5f, m_position.y - depth },
				{ frontX + 0.5f, m_position.y }
			};
			return m_collisionWorld->AnyOverlap(probe);
		}


		bool CharacterController::FindLedge(const int facing, const LedgeQuery& query, LedgeInfo& out) const
		{
			if (m_collisionWorld == nullptr)
			{
				return false;
			}

			const float dir = facing > 0 ? 1.0f : -1.0f;
			const math::Rect body = GetBounds();
			const float bodyTop = body.max.y;
			const float frontX = facing > 0 ? body.max.x : body.min.x;

			// 手が届く範囲：体の中心〜前面＋reach、頭の高さ±α
			math::Rect probe;
			probe.min.y = bodyTop - query.below;
			probe.max.y = bodyTop + query.above;
			if (facing > 0)
			{
				probe.min.x = m_position.x;
				probe.max.x = frontX + query.reach;
			}
			else
			{
				probe.min.x = frontX - query.reach;
				probe.max.x = m_position.x;
			}

			std::vector<math::Rect> candidates;
			m_collisionWorld->QueryOverlap(probe, candidates);

			bool isFound = false;
			float bestScore = FLT_MAX;
			for (const math::Rect& solid : candidates)
			{
				// 崖の上面が手の届く高さにあるか
				const float ledgeY = solid.max.y;
				if (ledgeY < probe.min.y || ledgeY > probe.max.y)
				{
					continue;
				}

				// 手前側の角（右向きならブロックの左端）
				const float edgeX = facing > 0 ? solid.min.x : solid.max.x;
				// 角が体の中心より後ろにある＝頭上のブロック。つかむ対象ではない
				if ((edgeX - m_position.x) * dir < 0.0f)
				{
					continue;
				}

				// ぶら下がる位置：壁の手前で、頭のてっぺんが崖の上面にそろう位置
				const math::Vec2 hangPos{ edgeX - dir * (m_width * 0.5f + SKIN), ledgeY - m_height };
				// 登りきった位置：崖の上、角から体半分だけ奥
				const math::Vec2 standPos{ edgeX + dir * (m_width * 0.5f + SKIN), ledgeY + SKIN };

				// ぶら下がる場所がふさがっていない（壁の途中のブロックではない）
				if (!m_collisionWorld->IsBoxFree(GetBoundsAt(hangPos)))
				{
					continue;
				}
				// 崖の上に立てる（上にブロックが積まれていない）
				if (!m_collisionWorld->IsBoxFree(GetBoundsAt(standPos)))
				{
					continue;
				}

				// 頭の高さに一番近い崖を選ぶ
				const float score = math::Abs(ledgeY - bodyTop);
				if (score < bestScore)
				{
					bestScore = score;
					out.hangPosition = hangPos;
					out.standPosition = standPos;
					out.ledgeTopY = ledgeY;
					out.facing = facing > 0 ? 1 : -1;
					isFound = true;
				}
			}
			return isFound;
		}


		CharacterController::CharacterController()
			: m_collisionWorld(nullptr)
			, m_position(math::Vec2::Zero())
			, m_velocity(math::Vec2::Zero())
			, m_width(50.0f)
			, m_height(120.0f)
			, m_gravity(-3000.0f)
			, m_maxFallSpeed(1600.0f)
			, m_isGravityEnabled(true)
			, m_isOnGround(false)
			, m_isJustLanded(false)
			, m_isHitCeiling(false)
			, m_isTouchingWallLeft(false)
			, m_isTouchingWallRight(false)
		{}
	}
}
