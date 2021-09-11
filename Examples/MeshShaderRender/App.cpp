
#include "MeshShaderRender.h"

int main( int argc, char argv[] )
	{
	Application<MeshShaderRender> app;

	app.RequireMeshShader = true;

#ifdef _DEBUG
	app.ShowConsole = true;
	app.EnableValidation = true;
#else
	app.ShowConsole = false;
#endif

	app.InitialWindowPosition[0] = 0;
	app.InitialWindowPosition[1] = 30;
	app.InitialWindowDimensions[0] = 1920;
	app.InitialWindowDimensions[1] = 1000;

	return app.Run();
	}
