#include "plotvarreplaceutil.h"
#include <fstream>
#include "wtextfilereader.h"
using std::ofstream;

PlotVarReplaceUtil::PlotVarReplaceUtil()
{

}

bool PlotVarReplaceUtil::replace( string inpltfilename , string outfilename , PlotOrder& order)
{
    //read inpupt
    string content0 ;
    {
        wTextFileReader tr ;
        content0 = tr.readAll(inpltfilename) ;
    }
    content0 = wStringUtils::replaceString(content0,"{{{outscatterpngfile}}}", order.outscatterpngfile.toStdString() ) ;
    content0 = wStringUtils::replaceString(content0,"{{{xrange0}}}", QString::number(order.scatterXmin).toStdString() ) ;
    content0 = wStringUtils::replaceString(content0,"{{{xrange1}}}", QString::number(order.scatterXmax).toStdString()) ;
    content0 = wStringUtils::replaceString(content0,"{{{yrange0}}}", QString::number(order.scatterYmin).toStdString()) ;
    content0 = wStringUtils::replaceString(content0,"{{{yrange1}}}", QString::number(order.scatterYmax).toStdString()) ;
    content0 = wStringUtils::replaceString(content0,"{{{xlabel}}}", order.xlabel.toStdString() ) ;
    content0 = wStringUtils::replaceString(content0,"{{{ylabel}}}", order.ylabel.toStdString() ) ;
    char eq[256] ;
    sprintf(eq , "y = %8.4fx + %8.4f" , order.linearK , order.linearB ) ;
    content0 = wStringUtils::replaceString(content0,"{{{fitequation}}}", string(eq) ) ;
    content0 = wStringUtils::replaceString(content0,"{{{corr}}}", QString::number(order.correlation).toStdString()  ) ;
    content0 = wStringUtils::replaceString(content0,"{{{r2}}}", QString::number(order.rsquared).toStdString()  ) ;
    content0 = wStringUtils::replaceString(content0,"{{{samplecount}}}", QString::number(order.matchingCount).toStdString()) ;
    content0 = wStringUtils::replaceString(content0,"{{{heatdatafile}}}", order.heatmapdatafile.toStdString() ) ;

    //temp linear data file
    string templineardatafile = order.outscatterpngfile.toStdString() + ".line.txt" ;
    {
        float tempy0 = order.linearK * order.scatterXmin + order.linearB ;
        float tempy1 = order.linearK * order.scatterXmax + order.linearB ;
        ofstream tempofs( templineardatafile.c_str() ) ;
        tempofs<<order.scatterXmin+0.00000001 <<" "<<tempy0 <<" 0.0\n" ;
        tempofs<<order.scatterXmax+0.00000001 <<" "<<tempy1 <<" 0.0\n" ;
    }

    content0 = wStringUtils::replaceString(content0,"{{{fitlinedatafile}}}", templineardatafile ) ;

    content0 = wStringUtils::replaceString(content0,"{{{outhistpngfile}}}",  order.outhistpngfile.toStdString() ) ;
    content0 = wStringUtils::replaceString(content0,"{{{histxlabel}}}", order.histXLabel.toStdString() ) ;
    content0 = wStringUtils::replaceString(content0,"{{{histylabel}}}", order.histYLabel.toStdString() ) ;
    content0 = wStringUtils::replaceString(content0,"{{{histXmin}}}", QString::number(order.histXmin).toStdString()  ) ;
    content0 = wStringUtils::replaceString(content0,"{{{histXmax}}}", QString::number(order.histXmax).toStdString()  ) ;
    content0 = wStringUtils::replaceString(content0,"{{{histYmax}}}", QString::number(order.histYmax).toStdString()  ) ;
    content0 = wStringUtils::replaceString(content0,"{{{histdatafile}}}", order.diffdatafile.toStdString() ) ;


    ofstream ofs( outfilename.c_str() ) ;
    if( ofs.is_open()==false ) return false ;
    ofs<<content0 ;
    ofs.close() ;
    return true ;
}
