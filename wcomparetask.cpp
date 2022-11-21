#include "wcomparetask.h"
#include <iostream>

using std::cout;
using std::endl;

wCompareTask::wCompareTask()
{
    inited = false;
    useProj =false;
    valid0 = 0 ; valid1 =0 ;
    useStddev = false ;
    boxRadius =0  ;
    maxStddev = 0 ;
    useBoxAverage=false ;
    useLandCover = true ;
    inBandIndex = 0 ;
    reBandIndex = 0 ;
}


void tempProgressFunction( void* obj , int progressVal)
{
    if( obj != nullptr ){
        wCompareTask* ptr = (wCompareTask*)obj;
        ptr->setProgress(progressVal);
        cout<<progressVal<<endl;
    }
}


void wCompareTask::run()
{
    if( spdlog::default_logger() == nullptr ){
        auto dailylogger = spdlog::daily_logger_mt("l", "daily.txt" , 2, 0) ;//2:00 am
        spdlog::flush_every(std::chrono::seconds(2) ) ;
        spdlog::set_default_logger(dailylogger );
    }

    if( inited==false ){
        spdlog::error("wCompareTask is not inited") ;
        return ;
    }
    string error1 ;
    spdlog::info("begin raster compare for {} vs {}", infilename1, infilename2) ;
    bool compareOk = WRasterCompare::Compare(infilename1,
                            infilename2,
                                             inBandIndex,reBandIndex,
                            useProj,
                            valid0,
                            valid1,
                            useStddev,
                            boxRadius,
                            maxStddev,
                            useBoxAverage,
                            useLandCover,
                            landCoverFilename,
                            outcsvfilename,
                            error1,
                            tempProgressFunction, this) ;

    if( compareOk==true ){
        spdlog::info("raster compare for {} vs {} success.",infilename1,infilename2) ;
        this->setDone(0,"");
    }else{
        spdlog::error("raster compare for {} vs {} failed: {}",infilename1,infilename2,error1) ;
        this->setDone(1,"");
    }
}


void wCompareTask::init(string infile1,string infile2,
                        bool useproj,double tvalid0,double tvalid1,
                        bool usestddev,int boxrad, double maxstddev,
                        bool useboxave, bool uselandcover, string landcoverfile,
                        string toutcsvfilename, int inbindex,int rebindex )
{
    infilename1 = infile1 ;
    infilename2 = infile2 ;
    useProj = useproj ;
    valid0 = tvalid0 ;
    valid1 = tvalid1 ;
    useStddev = usestddev;
    boxRadius = boxrad ;
    maxStddev = maxstddev;
    useBoxAverage = useboxave;
    useLandCover = uselandcover;
    landCoverFilename = landcoverfile;
    outcsvfilename = toutcsvfilename;
    inBandIndex = inbindex ;
    reBandIndex = rebindex;
    inited = true ;
}
