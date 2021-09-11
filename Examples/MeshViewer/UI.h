#pragma once
class MeshViewer;

class UI
    {
    public:
        MeshViewer* mv;

        bool ShowCameraControls = true;

        bool SelectSubmesh = false;
        int SelectedSubmeshIndex = 0;

        bool RenderAxies = true;
        bool RenderBoundingSphere = false;
        bool RenderAABB = false;
        bool RenderRejectionCone = false;

        bool OverrideQuantization = true;
        int BorderQuantization = 0;
        int InnerQuantization = 0;

        uint RenderedTriangles = 0;


        void Update();
    };

