/**
 * @file SunLight.h
 * @brief 日光（天窓などから、平行な光を何本も並べて差し込ませる）
 */
#pragma once
#include "GimmickBase.h"


namespace app
{
	namespace stage
	{
		/**
		 * @brief 日光
		 * @details ・窓（線分）に沿って raySpacing の間隔で、同じ向きの光を何本も出す。
		 *            光は1本ずつ、ふつうの光と同じように計算する（ブロックや日よけ板で止まり、反射鏡で反射する）。
		 *            何かにさえぎられて光が届かない所が「日陰」になる。
		 *          ・signal を入れると、その信号が ON の間だけ光が差し込む（天窓のシャッターが開く）。
		 *          ・窓は天井の下の面より少し下に置くこと（光の出る位置がブロックの中だと、すぐに止まってしまう）。
		 */
		class SunLight : public GimmickBase
		{
		public:
			/** 窓の中心 */
			math::Vec2 center;
			/** 窓の長さ（この長さに光が並ぶ） */
			float width = 400.0f;
			/** 窓の傾き（度。0=水平な天窓、90=縦の窓） */
			float windowAngleDeg = 0.0f;
			/** 光の向き（度。0=右、90=上、-90=真下、-60=右下） */
			float angleDeg = -60.0f;
			/** 光を並べる間隔（窓に沿った長さ。小さいほど日陰の形が細かくなるが、光の本数が増える） */
			float raySpacing = 25.0f;
			/** 光1本の太さ（描くときだけに使う。raySpacing と同じにすると隙間のない光の帯になる） */
			float rayWidth = 20.0f;
			/** 光を出すか */
			bool isOn = true;
			/** 光が差し込むきっかけの信号（受光器・スイッチの名前。空ならずっと差し込む） */
			std::string signal;

			/** 【実行時】信号が ON か（Update で調べておく。光の計算は信号を見られないため） */
			bool isSignalOn = false;


		public:
			const char* GetTypeName() const override { return "SunLight"; }
			std::unique_ptr<GimmickBase> Clone() const override { return std::make_unique<SunLight>(*this); }

			/** 光の計算より前に信号を調べる（スイッチと同じ順番） */
			UpdatePhase GetUpdatePhase() const override { return UpdatePhase::Input; }
			void Update(const GimmickUpdateContext& context) override;
			void ResetRuntimeState() override { isSignalOn = false; }

			void CollectEmittedRays(std::vector<EmittedRay>& out) const override;

			void RenameSignal(const std::string& oldName, const std::string& newName) override;
			void RemoveSignal(const std::string& signalName) override;

			math::Vec2 GetAnchor() const override { return center; }
			void SetAnchor(const math::Vec2& anchor) override { center = anchor; }
			math::Rect GetBounds() const override;
			bool HitTest(const math::Vec2& point, float pickDistance) const override;
			EditorLayer GetEditorLayer() const override { return EditorLayer::Front; }
			void DrawEditor(const EditorCanvas& canvas, bool isSelected) const override;
			void DrawInspector(GimmickInspector& inspector) override;
			void Serialize(GimmickArchive& archive) override;


		public:
			/** 今、光が差し込んでいるか */
			bool IsShining() const { return isOn && (signal.empty() || isSignalOn); }
			/** 光の向き（長さ1） */
			math::Vec2 GetDirection() const { return math::DirectionFromDeg(angleDeg); }
			/** 窓に沿った向き（長さ1） */
			math::Vec2 GetWindowDirection() const { return math::DirectionFromDeg(windowAngleDeg); }
			/** 窓の端（始点） */
			math::Vec2 GetStart() const { return center - GetWindowDirection() * (width * 0.5f); }
			/** 窓の端（終点） */
			math::Vec2 GetEnd() const { return center + GetWindowDirection() * (width * 0.5f); }
			/** 光の本数 */
			int GetRayCount() const;


		public:
			/** x, y は窓の中心 */
			SunLight(const char* name, float x, float y, float initialWidth, float initialAngleDeg);
		};
	}
}
