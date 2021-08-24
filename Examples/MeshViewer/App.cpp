
#include "MeshViewer.h"

int main( int argc, char argv[] )
	{
	Application<MeshViewer> app;

#ifdef _DEBUG
	app.ShowConsole = true;
	app.EnableValidation = true;
#else
	app.ShowConsole = false;
#endif

	app.InitialWindowPosition[0] = 30;
	app.InitialWindowPosition[1] = 60;
	app.InitialWindowDimensions[0] = 1800;
	app.InitialWindowDimensions[1] = 900;

	return app.Run();
	}
