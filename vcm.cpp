/*---------------------------------------------------------------------------------------
vcm is a vapoursynth plugin and has several functions that modify, move pixel values. Also
It has frequency domain functions as well as some utility and test functions
 included functions are:
 a).functions that modify pixel values to reduce noise or introduce blur:-
 amp, variance, hist, gBlur, mBlur, median, saltPepper, veed, fan, neural
 b). functions that move pixels:-
 deBarrel, rotate, reform, deJitter, correctLD
 c) functions of miscellaneous nature;-
 jitter, pattern, grid, bokeh, colorbox
 d) Functions that use Freq Domain :- f1quiver, f1qtest, f2quiver, f2qtest, 
	f2qblur, f2qcorrelation, f2qsharp, f2qbokeh
 

Author V.C.Mohan.
Date 24 Dec 2020, 31 Dec 2020, 31 May 2021, 5 Sep 2021
copyright 2015- 2021

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    A copy of the GNU General Public License is at
    see <http://www.gnu.org/licenses/>.
	
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <string>
#endif

#include <stdlib.h>
#include <algorithm>
#include <queue>
#include <fstream>
#define _USE_MATH_DEFINES
#include "math.h"

#include <vector>
#include <stack>		
#include <functional>

#include <mutex>

#include "WSSegment.cpp"
#include <VapourSynth4.h>
#include <VSHelper4.h>
using namespace vsh;

// fftwf planning, allocation and plan destruction are not thread safe,
// so the frequency domain filters serialize them with this mutex.
static std::mutex g_mutex;
#include "modHistogramHelper.cpp"
#include <ctime>

#include "UnitSq2Quad_matrix.cpp"
#include "interpolationMethods.h"
#include "colorconverter.h"
#include "statsAndOffsets.h"
# include "ConvertBGRforInput.h"
#include "Squircles.h"
#include "FisheyeMethods.h"
#include "FourFoldSymmetricMarking.h"

#include "fftwlite.h"
//#include "fftw3.h"
#include "fillPlaneWithVal.h"
#define NYQUIST 512
#define ADDSAFE 64
#include "F2QFilters.h"
#include "F2QuiverSpectralDisplay.h"
#include "FQDomainHelper.h"
#include "statsAndOffsets.h"
#include "spotlightDim.h"

#include "colorBox.cpp"
#include "Grid.cpp"
#include "Pattern.cpp"
#include "Jitter.cpp"
#include "Dejitter.cpp"
#include "Circles.cpp"

#include "F1Quiver.cpp"
#include "F2Quiver.cpp"
#include "FQBlur.cpp"
#include "FQSharp.cpp"
#include "FQCorrelation.cpp"
#include "F2QBokeh.cpp"
#include "F2QLimit.cpp"
#include "F1QClean.cpp"

#include "moveRotate.cpp"
#include "moveDeBarrel.cpp"
#include "moveReform.cpp"
#include "Fisheye.cpp"

#include "Mean.cpp"
#include "modFan.cpp"
#include "modAmplitude.cpp"
#include "modHistogram.cpp"
#include "modMedian.cpp"
//#include "Median2.cpp"

#include "modGBlur.cpp"
#include "modMBlur.cpp"
#include "modVariance.cpp"
#include "modSaltPepper.cpp"
#include "modVeed.cpp"
#include "modNeural.cpp"
#include "modBokeh.cpp"
#include "StepFilter.cpp"


VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi)
{
	vspapi->configPlugin("in.vcmohan.cm", "vcm", "VapourSynth Plugin by vcmohan", VS_MAKE_VERSION(VCM_VERSION_MAJOR, VCM_VERSION_MINOR), VAPOURSYNTH_API_VERSION, 0, plugin);

	vspapi->registerFunction("Amp", "clip:vnode;useclip:int:opt;sclip:vnode:opt;connect4:int:opt;sh:int[]:opt;sm:int[]:opt;", "clip:vnode;", amplitudeCreate, NULL, plugin);
	vspapi->registerFunction("Fan", "clip:vnode;span:int:opt;edge:int:opt;plus:int:opt;minus:int:opt;uv:int:opt;", "clip:vnode;", fanCreate, NULL, plugin);
	vspapi->registerFunction("Hist", "clip:vnode;clipm:vnode:opt;type:int:opt;table:int[]:opt;mf:int:opt;window:int:opt;limit:int:opt;", "clip:vnode;", histogramadjustCreate, NULL, plugin);
	vspapi->registerFunction("Median", "clip:vnode;maxgrid:int:opt;plane:int[]:opt;", "clip:vnode;", adaptivemedianCreate, NULL, plugin);
	vspapi->registerFunction("GBlur", "clip:vnode;ksize:int:opt;sd:float:opt;", "clip:vnode;", gblurCreate, NULL, plugin);
	vspapi->registerFunction("MBlur", "clip:vnode;type:int:opt;x:int:opt;y:int:opt;", "clip:vnode;", mblurCreate, NULL, plugin);
	vspapi->registerFunction("Neural", "clip:vnode;txt:data:opt;fname:data:opt;tclip:vnode:opt;xpts:int:opt;ypts:int:opt;tlx:int:opt;tty:int:opt;trx:int:opt;tby:int:opt;iter:int:opt;bestof:int:opt;wset:int:opt;rgb:int:opt;", "clip:vnode;", neuralCreate, NULL, plugin);
	vspapi->registerFunction("Variance", "clip:vnode;lx:int;wd:int;ty:int;ht:int;fn:int:opt;uv:int:opt;xgrid:int:opt;ygrid:int:opt;", "clip:vnode;", varianceCreate, NULL, plugin);
	vspapi->registerFunction("SaltPepper", "clip:vnode;planes:int[]:opt;tol:int:opt;avg:int:opt;", "clip:vnode;", saltpepperCreate, NULL, plugin);
	vspapi->registerFunction("Veed", "clip:vnode;str:int:opt;rad:int:opt;planes:int[]:opt;plimit:int[]:opt;mlimit:int[]:opt;", "clip:vnode;", veedCreate, NULL, plugin);
	vspapi->registerFunction("Mean", "clip:vnode;grid:int:opt;tol:float:opt;", "clip:vnode;", meanCreate, NULL, plugin);
	vspapi->registerFunction("F1Quiver", "clip:vnode;filter:int[];morph:int:opt;custom:int:opt;test:int:opt;strow:int:opt;nrows:int:opt;gamma:float:opt;", "clip:vnode;", f1quiverCreate, NULL, plugin);
	vspapi->registerFunction("F1QClean", "clip:vnode;span:int:opt;fromf:int:opt;upto:int:opt;", "clip:vnode;", f1qcleanCreate, NULL, plugin);
	vspapi->registerFunction("F1QLimit", "clip:vnode;span:int:opt;limit:int:opt;freqs:int[]:opt;", "clip:vnode;", f1qlimitCreate, NULL, plugin);
	vspapi->registerFunction("F2Quiver", "clip:vnode;frad:int:opt;ham:int:opt;test:int:opt;morph:int:opt;gamma:float:opt;fspec:int[]:opt;", "clip:vnode;", f2quiverCreate, NULL, plugin);
	vspapi->registerFunction("F2QLimit", "clip:vnode;grid:int:opt;inner:int:opt;warn:int:opt;fspec:int[]:opt;", "clip:vnode;", f2qlimitCreate, NULL, plugin);
	vspapi->registerFunction("F2QBlur", "clip:vnode;line:int:opt;x:int:opt;y:int:opt;", "clip:vnode;", f2qblurCreate, NULL, plugin);
	vspapi->registerFunction("F2QSharp", "clip:vnode;line:int:opt;wn:float:opt;x:int:opt;y:int:opt;frad:int:opt;ham:int:opt;scale:float:opt;rgb:int[]:opt;yuv:int[]:opt;", "clip:vnode;", f2qsharpCreate, NULL, plugin);
	vspapi->registerFunction("F2QCorr", "clip:vnode;bclip:vnode;cx:int:opt;cy:int:opt;txt:int:opt;filename:data:opt;sf:int:opt;ef:int:opt;every:int:opt;", "clip:vnode;", f2qcorrCreate, NULL, plugin);
	vspapi->registerFunction("F2QBokeh", "clip:vnode;clipb:vnode;grid:int:opt;thresh:float:opt;rgb:int[]:opt;yuv:int[]:opt;", "clip:vnode;", f2qbokehCreate, NULL, plugin);
	vspapi->registerFunction("Rotate", "clip:vnode;bkg:vnode;angle:float;dinc:float:opt;lx:int:opt;wd:int:opt;ty:int:opt;ht:int:opt;axx:int:opt;axy:int:opt;intq:int:opt;", "clip:vnode;", rotateCreate, NULL, plugin);
	vspapi->registerFunction("DeBarrel", "clip:vnode;abc:float[];method:int:opt;pin:int:opt;q:int:opt;test:int:opt;dots:data:opt;dim:float:opt;", "clip:vnode;", debarrelCreate, NULL, plugin);
	vspapi->registerFunction("Reform", "clip:vnode;bkg:vnode;intq:int:opt;norm:int:opt;rect:float[]:opt;quad:float[]:opt;q2r:int:opt;", "clip:vnode;", reformCreate, NULL, plugin);
	vspapi->registerFunction("Fisheye", "clip:vnode;method:int:opt;xo:int:opt;yo:int:opt;frad:int:opt;sqr:int:opt;rix:float:opt;fov:float:opt;test:int:opt;dim:float:opt;q:int:opt;dots:int:opt;", "clip:vnode;", fisheyeCreate, NULL, plugin);
	vspapi->registerFunction("ColorBox", "format:int:opt;luma:int:opt;nbw:int:opt;nbh:int:opt;", "clip:vnode;", colorBoxCreate, NULL, plugin);
	vspapi->registerFunction("Grid", "clip:vnode;lineint:int:opt;bold:int:opt;vbold:int:opt;color:int[]:opt;bcolor:int[]:opt;vbcolor:int[]:opt;style:int:opt;", "clip:vnode;", gridCreate, NULL, plugin);
	vspapi->registerFunction("Pattern", "clip:vnode;type:int:opt;orient:int:opt;spk:int:opt;spike:float:opt;wl:int:opt;x:int:opt;y:int:opt;rad:int:opt;stat:int:opt;overlay:float:opt;bgr:int[]:opt;", "clip:vnode;", patternCreate, NULL, plugin);
	vspapi->registerFunction("Jitter", "clip:vnode;type:int:opt;jmax:int:opt;dense:data:opt;wl:int:opt;stat:int:opt;speed:data:opt;", "clip:vnode;", jitterCreate, NULL, plugin);
	vspapi->registerFunction("DeJitter", "clip:vnode;jmax:int:opt;wsyn:int:opt;thresh:float:opt;", "clip:vnode;", dejitterCreate, NULL, plugin);
	vspapi->registerFunction("Bokeh", "clip:vnode;clipb:vnode;grid:int:opt;thresh:float:opt;rgb:int[]:opt;yuv:int[]:opt;", "clip:vnode;", bokehCreate, NULL, plugin);
	vspapi->registerFunction("StepFilter", "clip:vnode;add:int:opt;boost:float:opt;segmenthor:int:opt;segmentvert:int:opt;limit:int:opt;", "clip:vnode;", stepfilterCreate, NULL, plugin);
	vspapi->registerFunction("Circles", "clip:vnode;xo:int:opt;yo:int:opt;frad:int:opt;cint:int:opt;dots:int:opt;rgb:int[]:opt;dim:float:opt;", "clip:vnode;", circlesCreate, NULL, plugin);
}
