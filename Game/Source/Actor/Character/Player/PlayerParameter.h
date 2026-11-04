/**
 * @file PlayerParameter.h
 * @brief プレイヤーのパラメーター（調整値）
 * @details キャラクター共通の値に、プレイヤーだけが使う値（ジャンプ・崖つかまり・落下ダメージ）を足す。
 */
#pragma once
#include "Source/Actor/Character/CharacterParameter.h"


namespace app
{
	namespace actor
	{
		/**
		 * @brief プレイヤーパラメーター
		 */
		struct PlayerParameter : public CharacterParameter
		{
			PlayerParameter()
			{
				// 探検家の女の子は 3ds Max で身長 約69.75（インチ）。
				// ただし FBX を読み込んだときの単位の変換（センチ → インチ、0.394倍）が tka に2回かかっていて、
				// ゲームでは 0.394倍の大きさで表示される。その分も戻すと 1.7 × 2.54 ≒ 4.32倍で約119（体の高さ bodyHeight = 120 と同じ）。
				// 画面の中で小さく見えるので、見た目だけ 1.1倍の 4.75（身長 約131）にしている。当たり判定（bodyHeight）は変えない
				// ※ モデルを変えたら、ゲームで見ながら Tuning の「Model Scale」で合わせ、決まった値をここに書く
				modelScale = 4.75f;

				// ユニティちゃん … 使わなくなったのでコメントアウト（身長 約119なので拡大しない）
				//modelScale = 1.0f;

				// Remy（Mixamo）… 使わなくなったのでコメントアウト（身長 約149 → 0.8倍で約119）
				//modelScale = 0.8f;
			}

			// ---- ジャンプ ----
			/** ジャンプの初速 */
			float jumpVelocity = 1100.0f;
			/** 上昇中にジャンプを離したときの上昇速度の倍率（小ジャンプ用。1.0で無効） */
			float jumpCutMultiplier = 0.5f;
			/** 足場から落ちた直後でもジャンプできる猶予（コヨーテタイム） */
			float coyoteTime = 0.1f;
			/** 着地前に押したジャンプを覚えておく時間（先行入力） */
			float jumpBufferTime = 0.12f;

			// ---- 走りのアニメーション ----
			/**
			 * @brief 走っているときのアニメーションの再生速度（1.0 で元の速さ）
			 * @details Player_Run.tka は元の速さだと、進む速さ 約470 で足の動きと合う（modelScale 4.75 のとき。モデルを大きくすると歩幅も伸びる）。
			 *          「470 × この値」が runSpeed に近いほど、足がすべって見えない（runSpeed 400 なら 約0.85）
			 */
			float runAnimationSpeed = 0.85f;

			// ---- しゃがみ ----
			/** しゃがんだときの体の高さ */
			float crouchHeight = 70.0f;

			// ---- 崖つかまり（プリンス オブ ペルシャ）----
			/** true なら崖に自動でつかまる。false ならつかむボタンを押している間だけつかまる */
			bool isAutoLedgeGrab = true;
			/** 体の前面からどれだけ先の崖に手が届くか */
			float ledgeGrabReach = 25.0f;
			/** 頭より上、どこまでの崖をつかめるか */
			float ledgeGrabAbove = 40.0f;
			/** 頭より下、どこまでの崖をつかめるか */
			float ledgeGrabBelow = 40.0f;
			/** 崖から手を離したあと、再びつかめるようになるまでの時間 */
			float ledgeRegrabCooldown = 0.3f;
			/** 崖登りのアニメーションの長さ（元の速さのとき。Player_Climb.tka：72フレーム ÷ 30） */
			float climbUpDuration = 2.4f;
			/**
			 * @brief 崖登りのアニメーションの再生速度（1.0 で元の速さ）
			 * @details 登りきるまでの時間は climbUpDuration ÷ この値になる（1.5 なら 2.4 ÷ 1.5 = 1.6秒）
			 */
			float climbAnimationSpeed = 2.0f;

			/** 崖を登りきるまでの時間（アニメーションの長さ ÷ 再生速度） */
			float GetClimbUpTime() const
			{
				return climbAnimationSpeed > 0.0f ? climbUpDuration / climbAnimationSpeed : climbUpDuration;
			}

			// ---- 崖つかまり・崖登りの見た目（当たり判定は変わらない）----
			// 探検家の女の子：ゲームで見ながら Tuning の「Hang Model 〜」「Climb Model 〜」で合わせ、決まった値をここに書く
			/** ぶら下がっているときのモデルのずらし：前（崖の方）が正 */
			float ledgeHangModelForward = 0.0f;
			/** ぶら下がっているときのモデルのずらし：上が正 */
			float ledgeHangModelY = 0.0f;
			/** 崖登りのときに、ぶら下がりのずらしにさらに足す量：前が正 */
			float climbModelForward = 0.0f;
			/** 崖登りのときに、ぶら下がりのずらしにさらに足す量：上が正（登りのアニメーションの始まりの姿勢を、ぶら下がりの姿勢にそろえる） */
			float climbModelY = 0.0f;
			/**
			 * @brief 崖登りの間、モデルをぶら下がった位置に置いたままにするか
			 * @details true  … アニメーションの中で体が上がるモデル。体の動きはアニメーションに任せる
			 *          false … 登りのアニメーションがないモデル（ユニティちゃん）。モデルは当たり判定と一緒に上がる
			 */
			bool isClimbModelAnchored = true;

			// ユニティちゃん … 使わなくなったのでコメントアウト（専用アニメーションがなく idle / walk で代用していた）
			//float climbUpDuration = 0.5f;
			//float ledgeHangModelForward = 0.0f;
			//float ledgeHangModelY = 0.0f;
			//float climbModelForward = 0.0f;
			//float climbModelY = 0.0f;
			//bool isClimbModelAnchored = false;

			// Remy（Mixamo）… 使わなくなったのでコメントアウト（新しいモデルで崖登りのアニメーションを入れるときの参考）
			//float climbUpDuration = 1.13f;
			//float ledgeHangModelForward = 15.0f;
			//float ledgeHangModelY = -20.0f;
			//float climbModelForward = 0.0f;
			//float climbModelY = 30.0f;
			//bool isClimbModelAnchored = true;

			// ---- 物の操作（鏡・日よけ板）----
			/** 物をつかんで引きずっているときの速さ */
			float grabMoveSpeed = 200.0f;
			/** 鏡を回したときに「押す」アニメーションを見せる時間 */
			float rotateActionTime = 0.4f;

			// ---- 落下ダメージ（プリンス オブ ペルシャ：1段は平気、2段で痛い、3段で死ぬ）----
			/** この高さ以上から落ちると着地でよろけてダメージ */
			float hardLandingHeight = 500.0f;
			/** この高さ以上から落ちると死ぬ */
			float deadlyFallHeight = 800.0f;
			/** 落下ダメージ量 */
			int fallDamage = 1;
			/** 着地でよろけている時間 */
			float hardLandingDuration = 0.6f;

			// ---- 被ダメージ ----
			/** ダメージを受けたあとの無敵時間 */
			float invincibleTime = 1.0f;
			/** 無敵中に点滅する間隔（秒。この時間ごとに 見える ⇄ 見えない を切り替える。0 なら点滅しない） */
			float blinkInterval = 0.08f;
		};
	}
}
