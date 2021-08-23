#pragma once
class MeshViewer;

class UI
    {
    public:
        MeshViewer* mv;

        bool show_camera_controls = true;


        void Update();
    };

