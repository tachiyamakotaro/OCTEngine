/**
 * @file GimmickManager.cpp
 * @brief ステージに置いた全ギミックをまとめて持つ
 */
#include "stdafx.h"
#include "GimmickManager.h"


namespace app
{
	namespace stage
	{
		GimmickBase& GimmickManager::Add(std::unique_ptr<GimmickBase> gimmick)
		{
			m_gimmicks.push_back(std::move(gimmick));
			return *m_gimmicks.back();
		}


		void GimmickManager::Remove(const size_t index)
		{
			if (index < m_gimmicks.size())
			{
				m_gimmicks.erase(m_gimmicks.begin() + index);
			}
		}


		GimmickBase* GimmickManager::Get(const size_t index)
		{
			return index < m_gimmicks.size() ? m_gimmicks[index].get() : nullptr;
		}


		const GimmickBase* GimmickManager::Get(const size_t index) const
		{
			return index < m_gimmicks.size() ? m_gimmicks[index].get() : nullptr;
		}


		int GimmickManager::FindIndex(const GimmickBase* gimmick) const
		{
			for (size_t i = 0; i < m_gimmicks.size(); ++i)
			{
				if (m_gimmicks[i].get() == gimmick)
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}


		const GimmickBase* GimmickManager::FindByName(const std::string& name) const
		{
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->name == name)
				{
					return gimmick.get();
				}
			}
			return nullptr;
		}


		void GimmickManager::Update(const UpdatePhase phase, const GimmickUpdateContext& context)
		{
			for (std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->GetUpdatePhase() == phase)
				{
					gimmick->Update(context);
				}
			}
		}


		void GimmickManager::ResetRuntimeState()
		{
			for (std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				gimmick->ResetRuntimeState();
			}
		}


		bool GimmickManager::IsSignalOn(const std::string& signalName) const
		{
			if (signalName.empty())
			{
				return false;
			}
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->IsSignalSource() && gimmick->name == signalName && gimmick->IsSignalOn())
				{
					return true;
				}
			}
			return false;
		}


		void GimmickManager::CollectSignalNames(std::vector<std::string>& out) const
		{
			out.clear();
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->IsSignalSource())
				{
					out.push_back(gimmick->name);
				}
			}
		}


		void GimmickManager::RenameSignal(const std::string& oldName, const std::string& newName)
		{
			for (std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				gimmick->RenameSignal(oldName, newName);
			}
		}


		void GimmickManager::RemoveSignal(const std::string& signalName)
		{
			// 同じ名前の信号がまだ残っていれば（受光器とスイッチで同じ名前など）つながりは残す
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->IsSignalSource() && gimmick->name == signalName)
				{
					return;
				}
			}
			for (std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				gimmick->RemoveSignal(signalName);
			}
		}


		void GimmickManager::QueryOverlap(const math::Rect& box, std::vector<math::Rect>& out) const
		{
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				gimmick->QueryCollision(box, &out);
			}
		}


		bool GimmickManager::AnyOverlap(const math::Rect& box) const
		{
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->QueryCollision(box, nullptr) != CollisionKind::None)
				{
					return true;
				}
			}
			return false;
		}


		bool GimmickManager::OverlapsTerrain(const math::Rect& box) const
		{
			for (const std::unique_ptr<GimmickBase>& gimmick : m_gimmicks)
			{
				if (gimmick->QueryCollision(box, nullptr) == CollisionKind::Terrain)
				{
					return true;
				}
			}
			return false;
		}


		std::string GimmickManager::MakeUniqueName(const char* prefix) const
		{
			for (int n = 1; ; ++n)
			{
				const std::string name = std::string(prefix) + std::to_string(n);
				if (FindByName(name) == nullptr)
				{
					return name;
				}
			}
		}


		GimmickManager::GimmickManager(const GimmickManager& other)
		{
			*this = other;
		}


		GimmickManager& GimmickManager::operator=(const GimmickManager& other)
		{
			if (this == &other)
			{
				return *this;
			}
			m_gimmicks.clear();
			m_gimmicks.reserve(other.m_gimmicks.size());
			for (const std::unique_ptr<GimmickBase>& gimmick : other.m_gimmicks)
			{
				m_gimmicks.push_back(gimmick->Clone());
			}
			return *this;
		}
	}
}
