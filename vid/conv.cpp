#include <fstream>
#include <sstream>
using namespace std;

string intToStr(int64_t i,int len)
{
	stringstream ss;
	ss<<i;
	string s = ss.str();
	while (s.size() < len) s = "0" + s;
	return s;
}

int64_t strToInt(string s)
{
	stringstream ss;
	ss<<s;
	int64_t i;
	ss>>i;
	return i;
}

// vid4294965275.bmp
/*
GenAnn
Cikk javítani
tdk start
erkölcsi biz (ügyfélkapu)
jogviszony igazolás
jobbkezek (ferenciek)

ffmpeg -y -framerate 60 -i "vid\vid%4d.bmp" -s:v 640x480 -c:v libx264 -profile:v high -crf 20 -pix_fmt yuv420p video.mp4
*/

#include <cstdlib>

int main()
{
	for (int64_t val = 4294966830;val <= 4294967295;++val) {
		string from = "vid" + intToStr(val,4) + ".bmp";
		string to   = "tmp/vid" + intToStr(val-4294966830,4) + ".bmp";
		
		rename(from.c_str(),to.c_str());
	}
	
}