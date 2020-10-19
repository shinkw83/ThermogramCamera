#pragma once 
#define TO_GRID 0
#define TO_GPS 1

struct LATXLNGY
{
	double lat;
	double lng;
	double x;
	double y;
};

class CartesianToGPS 
{
	private :
	public :
		CartesianToGPS();
		~CartesianToGPS();
		LATXLNGY CartesianToGPSConvert(int mode, double lat_X, double lng_Y );
};

