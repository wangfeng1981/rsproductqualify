#ifndef WRASTERCOMPARE_H
#define WRASTERCOMPARE_H

#include <string>
#include "../sharedcodes2/wogrcoorconverter.h"
#include "maskvaluevalidator.h"
#include <iostream>
#include <QDebug>

using std::cout;
using std::endl;

class wGdalRaster ;

using std::string;

struct WRasterComparePointPair
{
    int inImgX , inImgY ;  //像素坐标 8Bytes
    int refImgX , refImgY ;//像素坐标 8Bytes
    double inVal , refVal ;   // 16Bytes
    double boxstd1,boxstd2;//标准差 16Bytes
    double boxmean1,boxmean2 ;//均值 16Bytes
    int landcover ;           // 4Bytes
    double diff,diff2 ;
    WRasterComparePointPair() ;
};


class WRasterCompareCSVOutput
{
public:
    inline WRasterCompareCSVOutput():m_pf(0){}
    inline bool isOk(){ return (m_pf==0)?false:true; }
    bool Open(string filename) ;
    ~WRasterCompareCSVOutput();
    bool AppendData(WRasterComparePointPair& cpp) ;

    WRasterCompareCSVOutput(WRasterCompareCSVOutput const&) = delete;
    WRasterCompareCSVOutput& operator=(WRasterCompareCSVOutput const&) = delete;

private:
    FILE* m_pf ;
};

//虚类 用于将像素图像坐标从图像1转换到图像2的图像坐标
class WPixelCoorTransform{
public:
    virtual bool transform(int x1, int y1, int & retx2, int & rety2) = 0;
    virtual ~WPixelCoorTransform();
};

//直接转换 直接把图像1坐标传给图像2
class WPixelCoorTransformDirectly : public WPixelCoorTransform{
public:
    virtual bool transform(int x1, int y1, int & retx2, int & rety2) ;
    virtual ~WPixelCoorTransformDirectly();
};

//通过地理坐标变换将图像1坐标转换到图像2的图像坐标
class WPixelCoorTransformByProjection : public WPixelCoorTransform {
public:
    WPixelCoorTransformByProjection();
    bool init(string filename1,string filename2);
    virtual bool transform(int x1, int y1, int & retx2, int & rety2) ;
    virtual ~WPixelCoorTransformByProjection();
    bool isValid(){return valid;}
private:
    bool valid;
    double trans1[6] ;
    double trans2[6] ;
    wOGRCoorConverter converter ;
    bool isFirstWgs84, isSecondWgs84 ;
    string crswkt1,crswkt2 ;
};


typedef void (*WRasterCompareProgressFunctionPointer)(void* objPtr,int progressVal);

class WRasterCompare
{
public:
    WRasterCompare();

    static bool IsFilenamesMatched(string inFilename, string refFilename, int inPos,int refPos, int len ) ;
    //filename1 - filename2 deprecated
    static bool Compare(string filename1,string filename2,
                        int inBandIndex,int reBandIndex,
                                  bool useProjectionCoordinate , //true for projection coordinate, false for image coordinate
                           double valid0,double valid1,
                           bool useStddevMask, int boxRadius,
                           double maxStddev,bool useBoxAverage,
                           bool useLandCover,
                           string landcoverfilename, string outcsvfilename,string& error,
                        WRasterCompareProgressFunctionPointer progressFuncPtr,
                        void* objPtr) ;

    //2022-11-15 in - ref
    //2023-1-9 add outfillvalue
    static bool Compare2(
        string filenameIn,string filenameRe,
        int inBandIndex,int reBandIndex,
        bool useProjectionCoordinate , //true for projection coordinate, false for image coordinate
        double valid0in ,double valid1in , double slopein, double offsetin,
        double valid0re ,double valid1re , double slopere, double offsetre,
        int histCount,
        int usePerFileMask, //0-not use, 1-use
        string inmaskfilename, string remaskfilename ,
        MaskValueValidator& inMvv,MaskValueValidator& reMvv,
        int useGlobalMask,//0-not use, 1-use
        string globalmaskfilename,
        MaskValueValidator& globalMvv,
        string indatarawfilename , string redatarawfilename ,//for scatter
        string diffrawfilename, string relerrrawfilename,//for hist
        string diffrasterfilename, //tiff output
        int& matchingCount ,
        double outFillvalue,
        string& error
        ) ;



private:
    //if all box pixels value are valid return true, otherwise return false.
    static bool ComputeBoxStddev(wGdalRaster* raster,int bandindex,int centerx,int centery,
                          int boxRadius,double valid0,double valid1,
                          int xsize,int ysize,
                          double& stddev, double& aver) ;

    static bool PixelCoorInsideImage(int ix,int iy,int xsize,int ysize);
};

#endif // WRASTERCOMPARE_H
