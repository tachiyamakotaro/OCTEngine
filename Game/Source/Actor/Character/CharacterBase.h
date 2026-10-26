/**
 * @file CharacterBase.h
 * @brief キャラクターの基底クラス
 * @details プレイヤー・鏡の中の自分・エネミーはすべてこのクラスを継承する。
 *
 *          持ち物（以前のプロジェクトと同じ分け方）
 *          ・CharacterParameter    … 調整値（派生クラスが持ち、GetParameter() で渡す）
 *          ・CharacterStatus       … HPなど変化する値（CreateStatus() で作る）
 *          ・CharacterStateMachine … ステート管理（CreateStateMachine() で作る）
 *          ・CharacterController   … 体の移動と当たり判定
 *          ・CharacterCommand      … 「こう動きたい」という命令（UpdateCommand() で作る）
 *
 *          1フレームの流れ（UpdateLogic）
 *          ステータス更新 → コマンド作成 → ステート更新 → 移動 → ステート遷移 → モデル更新
 */
#pragma once
#include <memory>
#include "Source/Actor/Actor.h"
#include "CharacterCommand.h"
#include "CharacterController.h"


namespace app
{
	namespace physics
	{
		class ICollisionWorld;
	}


	namespace actor
	{
		/** アニメーションデータ */
		struct AnimationData
		{
			/** ファイルネーム(.tka) */
			const char* fileName;
			/** ループフラグ */
			bool isLoop;
		};


		/** モデルデータ */
		struct ModelData
		{
			/** ファイルネーム(.tkm) */
			const char* fileName;
			/** アニメーションデータの配列 */
			const AnimationData* animationData;
			/** アニメーションクリップの数 */
			int clipNum;
			/** モデルの上方向 */
			EnModelUpAxis upAxis;
		};




		/************************************/


		struct CharacterParameter;
		class CharacterStatus;
		class CharacterStateMachine;


		/**
		 * @brief キャラクターの基底クラス
		 */
		class CharacterBase : public Actor
		{
		public:
			/**
			 * @brief 地形の当たり判定を設定する（Start より前に呼ぶ）
			 * @details 鏡の中の自分には、鏡の中の地形を渡すといった使い方ができる
			 */
			void SetCollisionWorld(const physics::ICollisionWorld* world);
			/**
			 * @brief キャラクターコントローラーを取得
			 */
			inline CharacterController& GetController() { return m_controller; }
			inline const CharacterController& GetController() const { return m_controller; }
			/**
			 * @brief 今フレームのコマンドを取得
			 */
			inline const CharacterCommand& GetCommand() const { return m_command; }
			/**
			 * @brief ステートマシンを取得
			 */
			inline CharacterStateMachine* GetStateMachine() const { return m_stateMachine.get(); }
			/**
			 * @brief ステータスを取得
			 * @tparam T 派生クラスのステータス型
			 */
			template<class T = CharacterStatus>
			T* GetStatus() const { return static_cast<T*>(m_status.get()); }
			/**
			 * @brief 向きを取得（右なら 1、左なら -1）
			 */
			inline int GetFacing() const { return m_facing; }
			/**
			 * @brief 向きを設定（0以上なら右、負なら左）
			 */
			inline void SetFacing(const int facing) { m_facing = facing >= 0 ? 1 : -1; }
			/**
			 * @brief モデルレンダーを取得
			 */
			inline ModelRender& GetModelRender() { return m_modelRender; }
			/**
			 * @brief 奥の鏡に映る像を描けるようにする（Start より前に呼ぶ）
			 * @details 同じモデルをもう1つ読み込み、鏡に映した位置・向きで描く
			 */
			inline void EnableReflection() { m_isReflectionEnabled = true; }
			/**
			 * @brief 鏡に映る像を表示するか・鏡の面の位置を設定する（毎フレーム呼んでよい）
			 * @param isVisible 表示するか
			 * @param planeZ 鏡の面のZ座標
			 */
			inline void SetReflectionPlane(const bool isVisible, const float planeZ)
			{
				m_isReflectionVisible = isVisible;
				m_reflectionPlaneZ = planeZ;
			}
			/**
			 * @brief モデルの見た目だけをずらす（当たり判定は動かさない）
			 * @param offset ずらす量（x は向いている方向が正、y は上が正）
			 * @details 崖につかまっているときに、手が崖の角にかかるように合わせるときなどに使う
			 */
			inline void SetModelOffset(const math::Vec2& offset)
			{
				m_modelOffset = offset;
				m_modelOffsetFadeTime = 0.0f;
				m_modelOffsetFadeDuration = 0.0f;
			}
			/**
			 * @brief 見た目のずらしを、offset から duration 秒かけて 0 に戻していく
			 * @details アニメーションの補間時間とそろえると、アニメーションの切り替えで体が飛んで見えない
			 *          （崖登りの最後の姿勢 → 立ちの姿勢に補間している間、体の位置も少しずつ戻す）
			 */
			inline void FadeOutModelOffset(const math::Vec2& offset, const float duration)
			{
				m_modelOffset = duration > 0.0f ? offset : math::Vec2::Zero();
				m_modelOffsetFadeDuration = duration > 0.0f ? duration : 0.0f;
				m_modelOffsetFadeTime = m_modelOffsetFadeDuration;
			}
			/**
			 * @brief モデルを決まった位置に置いて描く（当たり判定は動かさない）
			 * @details 崖登りのように、アニメーションの中で体が移動するときに使う。ClearModelAnchor で元に戻す
			 */
			inline void SetModelAnchor(const math::Vec2& position)
			{
				m_modelAnchor = position;
				m_isModelAnchored = true;
			}
			/**
			 * @brief SetModelAnchor をやめて、当たり判定の位置に描くように戻す
			 */
			inline void ClearModelAnchor() { m_isModelAnchored = false; }


		public:
			/** アニメーションを切り替えるときの補間時間（秒） */
			static constexpr float DEFAULT_ANIMATION_INTERPOLATE_TIME = 0.15f;
			/**
			 * @brief アニメーションを再生する
			 * @param clipNo アニメーションクリップ番号（InitModel に渡した配列の添字）
			 * @param interpolateTime 補間時間（秒）
			 */
			void PlayAnimation(int clipNo, float interpolateTime = DEFAULT_ANIMATION_INTERPOLATE_TIME);
			/**
			 * @brief 再生中のアニメーションクリップ番号
			 */
			inline int GetCurrentAnimation() const { return m_currentAnimation; }


		public:
			/**
			 * @brief ゲームロジックのセットアップ（Start から呼ばれる）
			 * @details コントローラー・ステータス・ステートマシンを作る。
			 *          モデルを使わないテストなどからも直接呼べるよう、Start とは分けている。
			 */
			void SetupCharacter();
			/**
			 * @brief ゲームロジックの1フレーム更新（Update から呼ばれる）
			 * @param deltaTime 経過時間（秒）
			 */
			void UpdateLogic(float deltaTime);


		public:
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		public:
			CharacterBase();
			virtual ~CharacterBase() override;


		protected:
			/**
			 * @brief モデルとアニメーションを読み込む（派生クラスの Start で、基底の Start より前に呼ぶ）
			 */
			void InitModel(const ModelData& data);
			/**
			 * @brief モデルの位置・向きを更新する
			 * @details 向きの付け方を変えたいときはオーバーライドする
			 */
			virtual void UpdateModel();
			/**
			 * @brief モデルのY軸回転（度）
			 * @details 右向きで 90、左向きで -90 に CharacterParameter::modelYawOffsetDeg を足す
			 */
			virtual float GetModelYawDegree() const;


		protected:
			/**
			 * @brief パラメーターを返す（派生クラスが持っているものを渡す）
			 */
			virtual const CharacterParameter& GetParameter() const = 0;
			/**
			 * @brief ステータスを作る（派生クラス固有のステータスを使うならオーバーライド）
			 */
			virtual std::unique_ptr<CharacterStatus> CreateStatus();
			/**
			 * @brief ステートマシンを作る
			 */
			virtual std::unique_ptr<CharacterStateMachine> CreateStateMachine() = 0;
			/**
			 * @brief 今フレームのコマンドを作る（プレイヤーなら入力、エネミーならAI）
			 */
			virtual void UpdateCommand(float deltaTime) = 0;


		protected:
			/** キャラクターコントローラー */
			CharacterController m_controller;
			/** ステータス */
			std::unique_ptr<CharacterStatus> m_status;
			/** ステートマシン */
			std::unique_ptr<CharacterStateMachine> m_stateMachine;
			/** 今フレームのコマンド */
			CharacterCommand m_command;
			/** 地形の当たり判定 */
			const physics::ICollisionWorld* m_collisionWorld;
			/** 向き（右なら 1、左なら -1） */
			int m_facing;

			/** モデルレンダー */
			ModelRender m_modelRender;
			/** アニメーションクリップ */
			std::unique_ptr<AnimationClip[]> m_animationClips;
			/** アニメーションクリップの数 */
			int m_clipNum;
			/** 再生中のアニメーション */
			int m_currentAnimation;
			/** モデルを読み込んだか */
			bool m_isModelInited;
			/** 鏡に映る像のモデルレンダー（EnableReflection したときだけ使う） */
			ModelRender m_reflectionRender;
			/** 鏡に映る像を作るか */
			bool m_isReflectionEnabled;
			/** 鏡に映る像を表示するか */
			bool m_isReflectionVisible;
			/** 鏡の面のZ座標 */
			float m_reflectionPlaneZ;
			/** モデルの見た目のずらし（x は向いている方向が正、y は上が正） */
			math::Vec2 m_modelOffset;
			/** モデルを置く位置（m_isModelAnchored のときだけ使う） */
			math::Vec2 m_modelAnchor;
			/** モデルを決まった位置に置いて描くか */
			bool m_isModelAnchored;
			/** 見た目のずらしを 0 に戻していく残り時間（秒） */
			float m_modelOffsetFadeTime;
			/** 見た目のずらしを 0 に戻していく時間（秒。0 なら戻さない） */
			float m_modelOffsetFadeDuration;
		};
	}
}
