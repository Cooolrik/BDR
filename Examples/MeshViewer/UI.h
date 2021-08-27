#pragma once
class MeshViewer;

class UI
    {
    public:
        MeshViewer* mv;

        bool show_camera_controls = true;

        bool select_a_zeptomesh = false;
        int selected_zeptomesh_index = 0;
        int max_zeptomesh_index = 0;
        int selected_submesh_index = 0;
        int max_submesh_index = 0;


        float coneAngle = 1.f;
        //float coneYaw = 0.f;
        //float coneTilt = 0.f;
        glm::vec3 coneDir = glm::vec3(1,0,0);

        float viewDot;
        bool visible;

        void Update();
    };

