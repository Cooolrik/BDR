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



        void Update();
    };

