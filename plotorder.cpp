#include "plotorder.h"

PlotOrder::PlotOrder()
{

}

bool PlotOrder::writeToJsonFile(QString filename)
{
    DynamicJsonBuffer jsonbuffer ;
    JsonObject& root = jsonbuffer.createObject() ;

    root["infilename"]= infilename.toStdString();
    root["reffilename"]= reffilename.toStdString();

    root["valid0in"]=valid0in ;
    root["valid1in"]=valid1in ;
    root["valid0ref"]=valid0ref ;
    root["valid1ref"]=valid1ref ;
    root["slopein"]=slopein ;
    root["interin"]=interin ;
    root["sloperef"]= sloperef;
    root["interref"]=interref ;

    root["bandin"]= bandin;
    root["bandref"]= bandref;

    root["useProj"] = useProj ;
    root["inTag"] = inTag.toStdString() ;
    root["reTag"] = reTag.toStdString() ;

    root["gnuplot"]= gnuplotexe.toStdString();
    root["xlabel"] = xlabel.toStdString() ;
    root["ylabel"] = ylabel.toStdString() ;

    root["scatterXmin"]=scatterXmin ;
    root["scatterXmax"]=scatterXmax ;
    root["scatterYmin"]=scatterYmin ;
    root["scatterYmax"]=scatterYmax ;

    root["histXmin"]=histXmin ;
    root["histXmax"]=histXmax ;
    root["histCount"]=histCount ;

    root["histXminRE"]=histXminRE ;
    root["histXmaxRE"]=histXmaxRE ;

    root["histXLabel"] = histXLabel.toStdString() ;
    root["histYLabel"] = histYLabel.toStdString() ;

    root["rehistXLabel"] = rehistXLabel.toStdString();
    root["rehistYLabel"] = rehistYLabel.toStdString();

    root["usePerfileMask"] = usePerfileMask ;
    root["inMaskFilename"] = inMaskFilename.toStdString() ;
    root["reMaskFilename"] = reMaskFilename.toStdString() ;
    root["inMaskValues"] = inMaskValues.toStdString() ;
    root["reMaskValues"] = reMaskValues.toStdString() ;

    root["useGlobalMask"]=useGlobalMask;
    root["globalMaskTag"] = globalMaskTag.toStdString() ;
    root["globalMaskValues"] = globalMaskValues.toStdString() ;
    root["globalMaskFilename"] = globalMaskFilename.toStdString() ;

    root["outbasename"]= outbasename.toStdString();

    //results
    root["in_vs_ref_datafile"]= in_vs_ref_datafile.toStdString();
    root["diffdatafile"]=diffdatafile.toStdString();
    root["heatmapdatafile"]=heatmapdatafile.toStdString();

    root["outhistpngfile"]   =outhistpngfile.toStdString();
    root["outscatterpngfile"]=outscatterpngfile.toStdString();
    root["plotscriptfile"] = plotscriptfile.toStdString() ;

    root["diffrasterfilename"]=diffrasterfilename.toStdString();//tiff data

    root["matchingCount"] = matchingCount ;
    root["correlation"] = correlation ;
    root["rsquared"] = rsquared ;
    root["linearK"] = linearK ;
    root["linearB"] = linearB ;
    root["rmse"] = rmse ;

    root["matcher"] = matcher.toStdString() ;

    string outtext ;
    root.prettyPrintTo(outtext) ;
    std::ofstream ofs(filename.toStdString().c_str());
    if( ofs.good() == true )
    {
        ofs<<outtext ;
        ofs.close() ;
        return true ;
    }else{
        return false ;
    }
}
