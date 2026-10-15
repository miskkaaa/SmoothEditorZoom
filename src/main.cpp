#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(a, EditorUI) { /* too lazy to name the fields shit anything else than "a" */
    struct Fields {
        float m_targetzoom = 1.f;
        bool m_targetzoomthing2 = false;
        bool m_applying = false;
    };

    void updateZoom(float zoom) {
        if (m_fields->m_applying || !m_editorLayer || !m_editorLayer->m_objectLayer) {
            EditorUI::updateZoom(zoom);
            return;
        }

        auto& fields = m_fields;
        const float current = m_editorLayer->m_objectLayer->getScale();

        if (!fields->m_targetzoomthing2) {
            fields->m_targetzoom = current;
            fields->m_targetzoomthing2 = true;
        }

        fields->m_targetzoom = std::clamp(fields->m_targetzoom + zoom - current, 0.2f, 2.0f);
    }
};

class $modify(LevelEditorLayer) {
    void updateEditor(float dt) {
        LevelEditorLayer::updateEditor(dt);
        if (!m_editorUI || !m_objectLayer || dt <= 0.f) return;

        auto editorUI = static_cast<a*>(m_editorUI);
        auto& fields = editorUI->m_fields;

        if (!fields->m_targetzoomthing2) {
            fields->m_targetzoom = m_objectLayer->getScale();
            fields->m_targetzoomthing2 = true;
            return;
        }

        const float current = m_objectLayer->getScale();
        fields->m_targetzoom = std::clamp(fields->m_targetzoom, 0.2f, 2.0f);
        const float difference = fields->m_targetzoom - current;

        if (std::abs(difference) < 0.001f) return;
        dt = std::min(dt, 0.05f);
    
        const float progress = 1.f - std::exp(-6.f * dt);
        const float next = std::abs(difference) < 0.01f ? fields->m_targetzoom : current + difference * progress;
        fields->m_applying = true;

        auto mouse = geode::cocos::getMousePos(); // everybody say thank you slideglide
        auto before = m_objectLayer->convertToNodeSpace(mouse);

        fields->m_applying = true;

        editorUI->updateZoom(next);

        if (m_groundLayer) {
            m_groundLayer->setScale(next);
        }

        auto after = m_objectLayer->convertToNodeSpace(mouse);

        m_objectLayer->setPosition(m_objectLayer->getPosition() + (after - before));

        fields->m_applying = false;
    }
};