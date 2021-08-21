
#include "MeshViewer.h"

int main( int argc, char argv[] )
	{
	Application<MeshViewer> app;

#ifdef _DEBUG
	app.EnableValidation = true;
#endif

	return app.Run();
	}
