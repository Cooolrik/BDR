#pragma once
class MeshShaderRender;

class UI
    {
    public:
        MeshShaderRender* mv;

        bool ShowCameraControls = true;

        bool SelectSubmesh = false;
        int SelectedSubmeshIndex = 0;

        bool RenderAxies = true;
        bool RenderBoundingSphere = false;
        bool RenderAABB = false;
        bool RenderRejectionCone = false;

        void Update();
    };

