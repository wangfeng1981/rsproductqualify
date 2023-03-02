#ifndef PLOTORDER_H
#define PLOTORDER_H

#include <QString>
#include "../../sharedcodes2/ajson5.h"
#include <string>
#include <fstream>

using std::string;
using std::ofstream;

class PlotOrder
{
public:
    PlotOrder();

    QString infilename, reffilename ;

    double valid0in,valid1in,valid0ref,valid1ref ;
    double slopein,interin,sloperef,interref ;
    int bandin,bandref ;
    int useProj ; //0-use image coordinate; 1-use projection coordinate
    QString inTag, reTag , gnuplotexe ;
    QString xlabel,ylabel ;//scatter fig
    QString plotscriptfile ;
    double scatterXmin,scatterXmax,scatterYmin,scatterYmax ;
    double histXmin,histXmax,histCount ;//error hist x range and hist count.

    double histXminRE,histXmaxRE ;//relative error hist x range
    QString histXLabel, histYLabel ;
    QString rehistXLabel,rehistYLabel ;

    //per file mask
    int usePerfileMask ;//0-not use, 1-use
    QString inMaskFilename, reMaskFilename ;
    QString inMaskValues, reMaskValues ;

    //global mask
    int useGlobalMask; //0-not use, 1-use
    QString globalMaskTag , globalMaskValues , globalMaskFilename ;

    QString outbasename ;

    //output results
    // QString indatarawfilename, refdatarawfilename ;//for scatter
    // QString diffrawfilename, reldiffrawfilename ;//for hist and rediff
    QString in_vs_ref_datafile ;//it is text file, this is for linear fit, will not use in gnuplot.

    QString diffdatafile ;// it is text file , x for bin, y for count, this is histgram data file.
    QString heatmapdatafile ;// for scatter; x y cnt
    QString diffrasterfilename;//tiff for bias

    QString outscatterpngfile ;
    QString outhistpngfile ;

    double correlation ;//相关系数
    double rsquared ;//R2
    double linearK ;//y=Kx+B
    double linearB ;
    double rmse ;//均方根误差

    int matchingCount ;//sampleCount 样本点

    QString matcher;

    bool writeToJsonFile(QString filename) ;
};

#endif // PLOTORDER_H
