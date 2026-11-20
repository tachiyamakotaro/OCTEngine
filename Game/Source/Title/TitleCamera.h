#pragma once
#include "Source/Title/TitleParameter.h"


/**
 * @file TitleCamera.h
 * @brief タイトル画面のカメラ（平行投影。動かない）
 */
namespace app
{
	namespace title
	{
		/**
		 * @brief タイトル画面のカメラ
		 * @details ・GameCamera と同じく、注視点の手前（-Z）から少し上から見る平行投影
		 *          ・置き方は RuinEntranceLayout の camera～ の値
		 *          ・カメラ（g_camera3D）はどのシーンでも同じ物なので、毎フレーム Apply で置き直す
		 */
		class TitleCamera
		{
		public:
			TitleCamera() = default;
			~TitleCamera() = default;

			void Init(const RuinEntranceLayout& layout);
			/** カメラを置く（毎フレーム） */
			void Apply() const;

		private:
			/** 置き方 */
			RuinEntranceLayout m_layout;
		};
	}
}
