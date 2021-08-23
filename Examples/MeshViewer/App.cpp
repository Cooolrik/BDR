
#include "MeshViewer.h"

int main( int argc, char argv[] )
	{
	Application<MeshViewer> app;

#ifdef _DEBUG
	app.EnableValidation = true;
#endif

	app.InitialWindowPosition[0] = 30;
	app.InitialWindowPosition[1] = 60;
	app.InitialWindowDimensions[0] = 1800;
	app.InitialWindowDimensions[1] = 900;

	app.ShowConsole = false;

	return app.Run();
	}
