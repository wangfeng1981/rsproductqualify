#ifndef WCOMPARETASK_H
#define WCOMPARETASK_H

#include "wprocessqueue.h"
#include "wrastercompare.h"
#include <string>

using std::string;

#include <QObject>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"



class wCompareTask: public wProcessTask
{
    Q_OBJECT
public:
    wCompareTask();

    virtual void run() ;

    void init(string infile1,string infile2,
              bool useproj,double tvalid0,double tvalid1,
              bool usestddev,int boxrad, double maxstddev,
              bool useboxave, bool uselandcover, string landcoverfile,
              string toutcsvfilename, int inbindex,int rebindex ) ;

private:
    bool inited ;
    string infilename1,infilename2 ;
    bool useProj ;
    double valid0,valid1 ;
    bool useStddev ;
    int boxRadius ;
    double maxStddev ;
    bool useBoxAverage ;
    bool useLandCover ;
    string landCoverFilename ;
    string outcsvfilename ;
    int inBandIndex , reBandIndex ;

};

#endif // WCOMPARETASK_H


