#pragma once

#include "Common.h"
#include <Vlk_Image.h>

class Texture
    {
    private:
        Vlk::Image *Image = nullptr;

    public:
        void LoadDDS( Vlk::Renderer* renderer, const char *path );

        ~Texture();

        GetConstMacro( Vlk::Image* , Image );
    };

