#pragma once
#include <imgui.h>
#include <cmath>
#include <imgui_internal.h>

inline void Spinner(const char* label, float radius, int thickness) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size(radius * 2.0f, radius * 2.0f);

    ImRect bb(
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y)
    );

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id)) return;

    float t = (float)g.Time;
    int num_segments = 30;
    float start = std::abs(std::sin(t * 1.8f) * (num_segments - 5));

    float a_min = IM_PI * 2.0f * (start / num_segments);
    float a_max = IM_PI * 2.0f * ((num_segments - 3) / num_segments);

    window->DrawList->PathClear();
    for (int i = 0; i < num_segments; i++) {
        float a = a_min + ((float)i / num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(
            pos.x + radius + cosf(a + t * 8) * radius,
            pos.y + radius + sinf(a + t * 8) * radius
        ));
    }

    window->DrawList->PathStroke(
        ImGui::GetColorU32(ImGuiCol_ButtonHovered),
        false,
        thickness
    );
}
