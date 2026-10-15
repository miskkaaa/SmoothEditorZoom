#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/utils/Keyboard.hpp>

using namespace geode::prelude;

// thank you alpha :D
class $modify(GJBaseGameLayer) {
    void updateCameraBGArt(CCPoint position, float zoom) {
        GJBaseGameLayer::updateCameraBGArt(position, zoom);
        if (m_playbackMode == PlaybackMode::Playing) return;

        auto editorLayer = typeinfo_cast<LevelEditorLayer*>(this);
        if (!editorLayer || !m_groundLayer || !m_objectLayer) return;

        float camX = m_objectLayer->getPositionX();

        if (m_groundLayer->m_ground1Sprite ||
            m_groundLayer->m_ground2Sprite) {

            float scaleX = m_groundLayer->getScaleX();
            if (scaleX == 0.f || m_groundLayer->m_textureWidth <= 0.f) return;

            float originX = camX / scaleX;
            int tilesToMove = static_cast<int>(std::ceil(originX / m_groundLayer->m_textureWidth));
            float finalX = originX - (tilesToMove * m_groundLayer->m_textureWidth);

            if (m_groundLayer->m_ground1Sprite) m_groundLayer->m_ground1Sprite->setPositionX(finalX);
            if (m_groundLayer->m_ground2Sprite) m_groundLayer->m_ground2Sprite->setPositionX(finalX);
        }
    }
};

class $modify(a, EditorUI) { /* too lazy to name the fields shit anything else than "a" */
    struct Fields {
        float m_targetzoom = 1.f;
        bool m_targetzoomthing2 = false;
        bool m_applying = false;

        bool m_middleheld = false;
        bool m_hasmousepos = false;
        CCPoint m_lastmousepos;

        geode::comm::ListenerHandle m_mouseListener;
    };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        m_fields->m_mouseListener = MouseInputEvent().listen([this](MouseInputData& data) {
            if (data.button != MouseInputData::Button::Middle)
                return ListenerResult::Propagate;

            if (data.action == MouseInputData::Action::Press) {
                m_fields->m_middleheld = true;
                m_fields->m_hasmousepos = false;
            }
            else if (data.action == MouseInputData::Action::Release) {
                m_fields->m_middleheld = false;
                m_fields->m_hasmousepos = false;
            }

            return ListenerResult::Propagate;
        });

        return true;
    }

    void updateZoom(float zoom) {
        if (m_fields->m_applying || !m_editorLayer || !m_editorLayer->m_objectLayer || m_editorLayer->m_playbackMode != PlaybackMode::Not) {
            /* h.,,,,,,,,,,,,,,,..,.,.,.,sd,.a,.,.,d.,. */

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

        if (m_playbackMode != PlaybackMode::Not) {
            fields->m_targetzoomthing2 = false;
            fields->m_hasmousepos = false;
            return;
        }

        auto mouse = geode::cocos::getMousePos(); // everybody say thank you slideglide

        if (fields->m_middleheld) {
            auto parent = m_objectLayer->getParent();

            if (parent) {
                auto pos = parent->convertToNodeSpace(mouse);

                if (fields->m_hasmousepos) {
                    auto delta = pos - fields->m_lastmousepos;

                    m_objectLayer->setPosition(m_objectLayer->getPosition() + delta);

                    if (m_groundLayer) {
                        m_groundLayer->setPosition(m_groundLayer->getPosition() + delta);
                    }
                }

                fields->m_lastmousepos = pos;
                fields->m_hasmousepos = true;
            }
        }
        else {
            fields->m_hasmousepos = false;
        }

        if (!fields->m_targetzoomthing2) {
            fields->m_targetzoom = m_objectLayer->getScale();
            fields->m_targetzoomthing2 = true;
            return;
        }

        const float current = m_objectLayer->getScale();
        fields->m_targetzoom = std::clamp(fields->m_targetzoom, 0.2f, 2.0f);
        const float difference = fields->m_targetzoom - current;

        if (std::abs(difference) < fields->m_targetzoom * 0.0001f) return;
        dt = std::min(dt, 0.05f);

        const float progress = 1.f - std::exp(-12.f * dt);
        const float next = std::abs(fields->m_targetzoom - (current + difference * progress)) < fields->m_targetzoom * 0.0005f ? fields->m_targetzoom : current + difference * progress;

        auto parent = m_objectLayer->getParent();
        auto before = m_objectLayer->convertToNodeSpace(mouse);

        fields->m_applying = true;

        editorUI->updateZoom(next);
        fields->m_applying = false;

        auto after = m_objectLayer->convertToWorldSpace(before);
        auto d = parent ? parent->convertToNodeSpace(mouse) - parent->convertToNodeSpace(after) : mouse - after;
        m_objectLayer->setPosition(m_objectLayer->getPosition() + d);
    }
};