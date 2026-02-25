#pragma once
#include <vector>
#include <cstdint>
#include "Core/Types.h"

namespace eng::gameplay
{
    enum class TriggerEventType : uint8_t
    {
        Enter,
        Exit,
        Stay
    };

    struct TriggerEvent
    {
        int triggerId = 0;
        TriggerEventType type{};
    };

    struct TriggerVolume
    {
        int id = 0;
        eng::RectF bounds{};
        uint32_t kind = 0;     // optional: user-defined type/category bitfield
        bool enabled = true;
    };

    class TriggerWorld
    {
    public:
        int AddTrigger(const eng::RectF& bounds, uint32_t kind = 0)
        {
            TriggerVolume t;
            t.id = m_nextId++;
            t.bounds = bounds;
            t.kind = kind;
            t.enabled = true;

            m_triggers.push_back(t);
            m_prevOverlap.push_back(0);
            return t.id;
        }

        void SetEnabled(int triggerId, bool enabled)
        {
            for (size_t i = 0; i < m_triggers.size(); ++i)
            {
                if (m_triggers[i].id == triggerId)
                {
                    m_triggers[i].enabled = enabled;
                    if (!enabled) m_prevOverlap[i] = 0;
                    return;
                }
            }
        }

        const TriggerVolume* GetTrigger(int triggerId) const
        {
            for (const auto& t : m_triggers)
                if (t.id == triggerId) return &t;
            return nullptr;
        }

        void Clear()
        {
            m_triggers.clear();
            m_prevOverlap.clear();
            m_nextId = 1;
        }

        // Call once per FixedUpdate.
        void Update(const eng::RectF& actorBounds, std::vector<TriggerEvent>& outEvents)
        {
            outEvents.clear();

            for (size_t i = 0; i < m_triggers.size(); ++i)
            {
                const auto& t = m_triggers[i];

                if (!t.enabled)
                {
                    m_prevOverlap[i] = 0;
                    continue;
                }

                const bool now = Intersects(actorBounds, t.bounds);
                const bool prev = (m_prevOverlap[i] != 0);

                if (now && !prev)
                    outEvents.push_back({ t.id, TriggerEventType::Enter });
                else if (now && prev)
                    outEvents.push_back({ t.id, TriggerEventType::Stay });
                else if (!now && prev)
                    outEvents.push_back({ t.id, TriggerEventType::Exit });

                m_prevOverlap[i] = now ? 1 : 0;
            }
        }

    private:
        static bool Intersects(const eng::RectF& a, const eng::RectF& b)
        {
            return (a.x < b.x + b.w) && (a.x + a.w > b.x) &&
                (a.y < b.y + b.h) && (a.y + a.h > b.y);
        }

        std::vector<TriggerVolume> m_triggers;
        std::vector<uint8_t> m_prevOverlap;
        int m_nextId = 1;
    };
}