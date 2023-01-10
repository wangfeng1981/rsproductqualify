#include "wrastercompare.h"
#include "../sharedcodes2/filenameutil.h"
#include "../sharedcodes2/wogrcoorconverter.h"
#include "../sharedcodes2/wGdalRaster.h"
#define WGDALRASTER_H_IMPLEMENTATION 1
#include "../sharedcodes2/wGdalRaster.h"
#include "spdlog/spdlog.h"

WPixelCoorTransform::~WPixelCoorTransform(){}




bool WPixelCoorTransformDirectly::transform(int x1, int y1, int & retx2, int & rety2)
{
    retx2 = x1; rety2 = y1 ; return true ;
}
WPixelCoorTransformDirectly::~WPixelCoorTransformDirectly(){
}




WPixelCoorTransformByProjection::WPixelCoorTransformByProjection(){
    isFirstWgs84 = false;
    isSecondWgs84 = false;
    valid = false;
}
bool WPixelCoorTransformByProjection::init(string filename1,string filename2)
{
    valid = false ;
    bool initok = this->converter.init(filename1,filename2) ;
    if( initok ==false ){
        valid = false;
    }else{
        wGdalRaster* pRaster1 = wGdalRasterFactory::OpenFile(filename1) ;
        wGdalRaster* pRaster2 = wGdalRasterFactory::OpenFile(filename2) ;
        if( pRaster1==0 || pRaster2 ==0 ){

            valid = false ;
        }else{
            memcpy( this->trans1 , pRaster1->getTrans() , 6*sizeof(double)) ;
            memcpy( this->trans2 , pRaster2->getTrans() , 6*sizeof(double)) ;
            crswkt1 = pRaster1->getProj() ;//GEOGCS
            crswkt2 = pRaster2->getProj() ;
            if( crswkt1.length()>6 && crswkt1.substr(0,6).compare("GEOGCS")==0 )
            {
                isFirstWgs84=true;
            }
            if( crswkt2.length()>6 && crswkt2.substr(0,6).compare("GEOGCS")==0 )
            {
                isSecondWgs84=true;
            }
            valid = true ;
        }
        if( pRaster1 ) delete pRaster1 ; pRaster1 = 0 ;
        if( pRaster2 ) delete pRaster2 ; pRaster2 = 0 ;
    }
    return valid ;
}


bool WPixelCoorTransformByProjection::transform(int x1, int y1, int & retx2, int & rety2)
{
    if( this->valid==false ) return false;
    double coorx1 = trans1[0] + trans1[1]*(x1+0.5) ;
    double coory1 = trans1[3] + trans1[5]*(y1+0.5) ;

    //if CRS is WGS84, use Latitude first, Longitude second.
    double coorx2 = 0 ;
    double coory2 = 0 ;
    if(isFirstWgs84){
        double temp1 = coory1 ;
        coory1 = coorx1 ;
        coorx1 = temp1 ;
    }
    bool convertOk = this->converter.convert(coorx1,coory1,  coorx2,coory2) ;
    if(isSecondWgs84){
        //if second CRS is WGS84 , reverse x2 and y2.
        double temp1 = coory2 ;
        coory2 = coorx2 ;
        coorx2 = temp1 ;
    }
    if( convertOk==false ) return false;
    retx2 = (coorx2 - trans2[0])/trans2[1];
    rety2 = (coory2 - trans2[3])/trans2[5];
    return true;
}

WPixelCoorTransformByProjection::~WPixelCoorTransformByProjection()
{
}









WRasterComparePointPair::WRasterComparePointPair()
{
    inImgX = inImgY =-1 ;  //像素坐标
    refImgX = refImgY=-1 ;//像素坐标
    inVal = refVal=-1 ;
    landcover = -1 ;
    boxstd1 = -1 ;
    boxstd2 = -1 ;
    boxmean1 = -1 ;
    boxmean2 = -1 ;
    diff = -1 ;
    diff2= -1;
}

bool WRasterCompareCSVOutput::Open(string filename)
{
    if( m_pf!=0 ) return false ;
    m_pf = fopen( filename.c_str() , "w") ;
    if(m_pf==0) return false ;
    fprintf(m_pf,"iix,iiy,rix,riy,iv,rv,istd,rstd,iave,rave,lc,diff,diff2\n") ;
    return true ;
}

WRasterCompareCSVOutput::~WRasterCompareCSVOutput()
{
    if( m_pf !=0 ){
        fclose(m_pf) ;
        m_pf = 0 ;
    }
}


bool WRasterCompareCSVOutput::AppendData(WRasterComparePointPair& cpp)
{
    if( m_pf==0 ){
        return false ;
    }else{
        fprintf(m_pf,"%d,%d,%d,%d,%f,%f,%f,%f,%f,%f,%d,%f,%f\n",
                cpp.inImgX,cpp.inImgY,
                cpp.refImgX,cpp.refImgY,
                cpp.inVal,cpp.refVal,
                cpp.boxstd1,cpp.boxstd2,
                cpp.boxmean1,cpp.boxmean2,
                cpp.landcover,
                cpp.diff,cpp.diff2) ;
        return true ;
    }
}

//------------------------------------------------------



WRasterCompare::WRasterCompare()
{

}




bool WRasterCompare::IsFilenamesMatched(
        string inFilename,
        string refFilename,
        int inPos,int refPos,
        int len )
{
    string name1 = wFilenameUtil::baseName(inFilename) ;
    string name2 = wFilenameUtil::baseName(refFilename) ;

    if( inPos + len > name1.length() ) return false ;
    if( refPos + len > name2.length() ) return false ;

    string sub1 = name1.substr(inPos , len) ;
    string sub2 = name2.substr(refPos , len) ;

    if( sub1.compare(sub2) == 0 )
    {
        return true ;
    }else{
        return false ;
    }
}


bool WRasterCompare::PixelCoorInsideImage(int ix,int iy,int xsize,int ysize)
{
    if(ix>=0 && iy>=0 && ix < xsize && iy < ysize ){
        return true;
    }else{
        return false;
    }

}


bool WRasterCompare::Compare(
    string filename1,string filename2,
    int inBandIndex,int reBandIndex,
    bool useProjectionCoordinate , //true for projection coordinate, false for image coordinate
    double valid0,double valid1,
    bool useStddevMask, int boxRadius,
    double maxStddev,bool useBoxAverage,
    bool useLandCover,
    string landcoverfilename,
        string outcsvfilename,
        string& error,
        WRasterCompareProgressFunctionPointer progressFuncPtr,
        void* objPtr
        )
{
    WRasterCompareCSVOutput csvOutput ;
    bool csvok = csvOutput.Open(outcsvfilename) ;
    if( csvok==false )
    {
        error = "failed to open csv output file." ;
        return false ;
    }


    //check box radius ok.
    if( boxRadius<0 )
    {
        error = "boxRadius lower equal 0, it is not valid." ;
        return false ;
    }

    spdlog::info("begin openning {}, {}" , filename1,filename2);
    wGdalRaster* pRaster1 = wGdalRasterFactory::OpenFile(filename1) ;
    wGdalRaster* pRaster2 = wGdalRasterFactory::OpenFile(filename2) ;

    if( pRaster1==0 || pRaster2 ==0 ){
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        error = "pRaster1 or pRaster2 is null." ;
        return false ;
    }

    if( inBandIndex >= pRaster1->getNBand() ){
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        error = "inBandIndex is invalid." ;
        return false ;
    }

    if( reBandIndex >= pRaster2->getNBand()  ){
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        error = "reBandIndex is invalid." ;
        return false ;
    }


    spdlog::info("begin making coor transform");
    //coor transform
    WPixelCoorTransform* transformer12WeakPtr = 0;//weak_ptr
    WPixelCoorTransformDirectly directTransform;
    WPixelCoorTransformByProjection projection12Transform ;
    if( useProjectionCoordinate==true ){
        bool ptok = projection12Transform.init(filename1,filename2);
        if( ptok==false ){
            error = "projection transformer is bad." ;
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            return false;
        }
        transformer12WeakPtr = &projection12Transform ;
    }else{
        transformer12WeakPtr = & directTransform ;
    }

    //land cover
    wGdalRaster* pRasterLandCover = 0;
    if(useLandCover)
    {
        spdlog::info("begin open landcover {}" , landcoverfilename);
        pRasterLandCover =  wGdalRasterFactory::OpenFile(landcoverfilename) ;
        if( pRasterLandCover==0 ){
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            error = "landcover is bad." ;
            return false ;
        }
    }

    //land cover transform
    WPixelCoorTransform* transformerLCWeakPtr = 0;//weak_ptr
    WPixelCoorTransformDirectly directTransformLC;
    WPixelCoorTransformByProjection projectionLCTransform ;
    if( useLandCover )
    {
        if( useProjectionCoordinate ){
            bool ptok = projectionLCTransform.init(filename1 , landcoverfilename);
            if( ptok==false ){
                error = "landcover projection transformer is bad." ;
                if( pRaster1 ) delete pRaster1 ;
                if( pRaster2 ) delete pRaster2 ;
                return false ;
            }
            transformerLCWeakPtr = &projectionLCTransform;
        }else{
            transformerLCWeakPtr = & directTransformLC ;
        }
    }


    //trans
    double trans1[6] ;
    double trans2[6] ;
    double transLC[6] ;
    memcpy( trans1 , pRaster1->getTrans() , 6*sizeof(double)) ;
    memcpy( trans2 , pRaster2->getTrans() , 6*sizeof(double)) ;


    //xsize and ysize
    int xsize1 = pRaster1->getXSize();
    int ysize1=  pRaster1->getYSize() ;
    int xsize2 = pRaster2->getXSize();
    int ysize2 = pRaster2->getYSize() ;
    int xsizeLC = 0; //pRasterLandCover->getXSize() ;
    int ysizeLC = 0; //pRasterLandCover->getYSize() ;

    if(pRasterLandCover!=0){
        xsizeLC = pRasterLandCover->getXSize() ;
        ysizeLC = pRasterLandCover->getYSize() ;
        memcpy( transLC , pRasterLandCover->getTrans() , 6*sizeof(double)) ;
    }

    //number of match pixel
    int numberOfMatchPixel = 0 ;

    int progress0 = -1 ;
    //for each pixel from 1
    for(int iy1 = 0 ; iy1 < ysize1 ; ++iy1 )
    {
        for(int ix1 = 0 ; ix1 < xsize1 ; ++ix1 )
        {
            //value1 ok?
            float value1 = pRaster1->getValuef(ix1,iy1,inBandIndex) ;
            if( value1 >= valid0 && value1 <= valid1 )
            {
                int ix2 = 0 ;
                int iy2 = 0 ;
                bool trans2ok = transformer12WeakPtr->transform(ix1,iy1,ix2,iy2);
                if( trans2ok )
                {
                    bool inside2Ok = PixelCoorInsideImage(ix2,iy2,xsize2,ysize2);
                    //1 2两个影像中心像素都在图像内
                    if( inside2Ok )
                    {
                        float value2 = pRaster2->getValuef(ix2,iy2,reBandIndex) ;
                        //value2 ok ?
                        if( value2 >= valid0 && value2 <= valid1 )
                        {
                            int landcoverValue = -1 ;
                            bool landcoverOk = false ;
                            //landcover
                            if( useLandCover )
                            {
                                int lcx = 0 ;
                                int lcy = 0 ;
                                bool transLcOk = transformerLCWeakPtr->transform(ix1,iy1,lcx,lcy) ;
                                if( transLcOk==true )
                                {
                                    bool insideLcOk = PixelCoorInsideImage(lcx,lcy,xsizeLC,ysizeLC);
                                    if( insideLcOk ){
                                        landcoverValue = pRasterLandCover->getValuei(lcx,lcy,0) ;
                                        landcoverOk = true ;
                                    }else{
                                        landcoverOk = false ;
                                    }
                                }else{
                                    landcoverOk = false ;
                                }
                            }

                            //landcover is ok or not use landcover
                            if( useLandCover==false || (useLandCover==true && landcoverOk==true ) )
                            {
                                //use stddev mask
                                bool stddevok = false ;
                                double boxAver1 = 0 ;
                                double boxAver2 = 0 ;
                                double stddev1 = 0 ;
                                double stddev2 = 0 ;

                                if( useStddevMask )
                                {
                                    //box1 ok?
                                    bool box1ok = ComputeBoxStddev( pRaster1 ,
                                                                    inBandIndex,
                                                                    ix1,iy1,boxRadius,
                                                                    valid0,valid1,
                                                                    xsize1,ysize1,
                                                                    stddev1,boxAver1) ;
                                    bool box2ok = ComputeBoxStddev( pRaster2 ,
                                                                    reBandIndex,
                                                                    ix2,iy2,boxRadius,
                                                                    valid0,valid1,
                                                                    xsize2,ysize2,
                                                                    stddev2,boxAver2) ;
                                    if( box1ok && box2ok )
                                    {
                                        if( stddev1 < maxStddev && stddev2<maxStddev )
                                        {
                                            //good pixel pair
                                            stddevok = true ;
                                        }else{
                                            stddevok = false ;
                                        }
                                    }else{
                                        stddevok = false ;
                                    }
                                }

                                if( useStddevMask==false || (useStddevMask==true && stddevok==true ))
                                {
                                    WRasterComparePointPair cpp ;
                                    cpp.inVal = value1 ;
                                    cpp.refVal = value2 ;
                                    cpp.inImgX = ix1 ;
                                    cpp.inImgY = iy1 ;
                                    cpp.refImgX = ix2 ;
                                    cpp.refImgY = iy2 ;
                                    if( useStddevMask )
                                    {
                                        cpp.boxstd1 = stddev1 ;
                                        cpp.boxstd2 = stddev2 ;
                                        cpp.boxmean1 = boxAver1 ;
                                        cpp.boxmean2 = boxAver2 ;
                                        cpp.diff = cpp.boxmean1 - cpp.boxmean2;
                                        cpp.diff2 = cpp.diff*cpp.diff ;
                                    }else{
                                        cpp.diff = cpp.inVal - cpp.refVal;
                                        cpp.diff2 = cpp.diff*cpp.diff ;
                                    }

                                    if( useLandCover ){
                                        cpp.landcover = landcoverValue ;
                                    }

                                    ++numberOfMatchPixel;

                                    csvOutput.AppendData(cpp) ;
                                }
                            }
                        }

                    }
                }

            }
        }

        //progress
        int progress1 = iy1 * 100.f / ysize1 ;
        if( progress1 != progress0 ){
            progress0 = progress1 ;
            if( progressFuncPtr != nullptr ){
                progressFuncPtr( objPtr , progress0 ) ;
            }
        }

    }

    if( pRaster1 ) delete pRaster1 ;
    if( pRaster2 ) delete pRaster2 ;
    if(pRasterLandCover) delete pRasterLandCover ;

    if( progressFuncPtr != nullptr ){
        progressFuncPtr( objPtr , 100 ) ;
    }

    return true;
}




//2022-11-15 in - ref
bool WRasterCompare::Compare2(
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
            )
{
    spdlog::info("begin openning {} - {}" , filenameIn,filenameRe);
    wGdalRaster* pRaster1 = wGdalRasterFactory::OpenFile(filenameIn) ;
    wGdalRaster* pRaster2 = wGdalRasterFactory::OpenFile(filenameRe) ;

    if( pRaster1==0 || pRaster2 ==0 ){
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        error = "pRaster1 or pRaster2 is null." ;
        return false ;
    }

    wGdalRaster* pRasterInMask = nullptr ;
    wGdalRaster* pRasterReMask = nullptr ;
    wGdalRaster* pGlobalMask = nullptr ;
    if( usePerFileMask )
    {
        pRasterInMask = wGdalRasterFactory::OpenFile(inmaskfilename) ;
        pRasterReMask = wGdalRasterFactory::OpenFile(remaskfilename) ;
        if( pRasterInMask==0 || pRasterReMask==0 ){
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            if( pRasterInMask ) delete pRasterInMask ;
            if( pRasterReMask ) delete pRasterReMask ;
            error = "pRasterInMask or pRasterReMask is null." ;
            return false ;
        }
    }
    if( useGlobalMask )
    {
        pGlobalMask = wGdalRasterFactory::OpenFile(globalmaskfilename) ;
        if( pGlobalMask==0 ){
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            if( pRasterInMask ) delete pRasterInMask ;
            if( pRasterReMask ) delete pRasterReMask ;
            error = "pGlobalMask is null." ;
            return false ;
        }
    }


    if( inBandIndex >= pRaster1->getNBand() ){
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        if( pRasterInMask ) delete pRasterInMask ;
        if( pRasterReMask ) delete pRasterReMask ;
        if( pGlobalMask ) delete pGlobalMask ;
        error = "inBandIndex is invalid." ;
        return false ;
    }

    if( reBandIndex >= pRaster2->getNBand()  ){
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        if( pRasterInMask ) delete pRasterInMask ;
        if( pRasterReMask ) delete pRasterReMask ;
        if( pGlobalMask ) delete pGlobalMask ;
        error = "reBandIndex is invalid." ;
        return false ;
    }
    spdlog::info("begin making coor transform");
    //coor transform
    WPixelCoorTransform* transformer12WeakPtr = 0;//weak_ptr
    WPixelCoorTransformDirectly directTransform;
    WPixelCoorTransformByProjection projection12Transform ;
    if( useProjectionCoordinate==true ){
        bool ptok = projection12Transform.init(filenameIn,filenameRe);
        if( ptok==false ){
            error = "projection transformer is bad." ;
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            if( pRasterInMask ) delete pRasterInMask ;
            if( pRasterReMask ) delete pRasterReMask ;
            if( pGlobalMask ) delete pGlobalMask ;
            return false;
        }
        transformer12WeakPtr = &projection12Transform ;
    }else{
        transformer12WeakPtr = & directTransform ;
    }

    //trans
    double trans1[6] ;
    double trans2[6] ;
    memcpy( trans1 , pRaster1->getTrans() , 6*sizeof(double)) ;
    memcpy( trans2 , pRaster2->getTrans() , 6*sizeof(double)) ;

    //xsize and ysize
    int xsize1 = pRaster1->getXSize() ;
    int ysize1=  pRaster1->getYSize() ;
    int xsize2 = pRaster2->getXSize() ;
    int ysize2 = pRaster2->getYSize() ;

    wGdalRasterFloat diffRaster ;
    bool diffRasterOk = diffRaster.create(xsize1,ysize1,1) ;
    if(diffRasterOk==false)
    {
        error = "diffRaster.create() bad." ;
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        if( pRasterInMask ) delete pRasterInMask ;
        if( pRasterReMask ) delete pRasterReMask ;
        if( pGlobalMask ) delete pGlobalMask ;
        return false;
    }
    //2023-1-9 use input proj and trans without useProjectionCoordinate
    if(true) { // if( useProjectionCoordinate ){
        diffRaster.copyProj( pRaster1->getProj() ) ;
        diffRaster.copyTrans(pRaster1->getTrans());
    }


    // per file mask
    if( usePerFileMask )
    {
        //see if indata and inmask same size;
        if( pRasterInMask->getXSize()!=xsize1 || pRasterInMask->getYSize()!=ysize1 )
        {
            error = "Indata and inmask are different size." ;
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            if( pRasterInMask ) delete pRasterInMask ;
            if( pRasterReMask ) delete pRasterReMask ;
            if( pGlobalMask ) delete pGlobalMask ;
            return false ;
        }
        //see if refdata and refmask same size;
        if( pRasterReMask->getXSize()!=xsize2 || pRasterReMask->getYSize()!=ysize2 )
        {
            error = "Refdata and Refmask are different size." ;
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            if( pRasterInMask ) delete pRasterInMask ;
            if( pRasterReMask ) delete pRasterReMask ;
            if( pGlobalMask ) delete pGlobalMask ;
            return false ;
        }
    }


    // global mask
    WPixelCoorTransformByProjection globalmaskTransform ;
    int globalMaskXSize = 0 ;
    int globalMaskYSize = 0 ;
    if(useGlobalMask)
    {
        globalMaskXSize = pGlobalMask->getXSize() ;
        globalMaskYSize = pGlobalMask->getYSize() ;
        bool ptok = globalmaskTransform.init(filenameIn,globalmaskfilename);
        if( ptok==false ){
            error = "global mask projection transformer is bad." ;
            if( pRaster1 ) delete pRaster1 ;
            if( pRaster2 ) delete pRaster2 ;
            if( pRasterInMask ) delete pRasterInMask ;
            if( pRasterReMask ) delete pRasterReMask ;
            if( pGlobalMask ) delete pGlobalMask ;
            return false;
        }
    }

    FILE* pf_rawin = fopen( indatarawfilename.c_str() , "wb") ;
    FILE* pf_rawref = fopen( redatarawfilename.c_str(),"wb") ;
    FILE* pf_diff = fopen( diffrawfilename.c_str(),"wb" ) ;
    FILE* pf_rdiff = fopen( relerrrawfilename.c_str(),"wb") ;
    if( pf_rawin==0 || pf_rawref==0 || pf_diff==0 || pf_rdiff==0 )
    {
        error = "open rawin or rawref failed." ;
        if(pf_rawin) fclose(pf_rawin) ;
        if( pf_rawref) fclose(pf_rawref) ;
        if(pf_diff) fclose(pf_diff) ;
        if(pf_rdiff) fclose(pf_rdiff) ;
        if( pRaster1 ) delete pRaster1 ;
        if( pRaster2 ) delete pRaster2 ;
        if( pRasterInMask ) delete pRasterInMask ;
        if( pRasterReMask ) delete pRasterReMask ;
        if( pGlobalMask ) delete pGlobalMask ;
        return false ;
    }

    //number of match pixel
    int numberOfMatchPixel = 0 ;
    int progress0 = -1 ;

    vector<float> tempIndataVec ,tempRefdataVec ;
    tempIndataVec.reserve(xsize1) ;
    tempRefdataVec.reserve(xsize1) ;

    vector<float> tempDiffVec ;
    vector<float> tempReVec ;

    tempDiffVec.reserve(xsize1) ;
    tempReVec.reserve(xsize1) ;


    //for each pixel from 1
    for(int iy1 = 0 ; iy1 < ysize1 ; ++iy1 )
    {
        tempIndataVec.clear() ;
        tempRefdataVec.clear() ;
        tempDiffVec.clear() ;
        tempReVec.clear() ;
        for(int ix1 = 0 ; ix1 < xsize1 ; ++ix1 )
        {
            //value1 ok?
            float value1 = pRaster1->getValuef(ix1,iy1,inBandIndex) ;

            //mask 1 ok?
            bool pass1ok = true ;
            if( usePerFileMask )
            {
                int inmaskvalue = pRasterInMask->getValuei(ix1,iy1,0) ;
                pass1ok = inMvv.pass(inmaskvalue) ;
            }

            //global mask ok?
            bool passgok = true ;
            if(useGlobalMask)
            {
                passgok = false ;
                int gmx,gmy ;
                bool gmTransOk = globalmaskTransform.transform(ix1,iy1,gmx,gmy) ;
                if( gmTransOk){
                    if(PixelCoorInsideImage(gmx,gmy,globalMaskXSize,globalMaskYSize) )
                    {
                        int gmval = pGlobalMask->getValuei(gmx,gmy,0) ;
                        if( globalMvv.pass(gmval) ){
                            passgok=true ;
                        }
                    }
                }
            }


            if( passgok && pass1ok && value1 >= valid0in && value1 <= valid1in )
            {
                int ix2 = 0 ;
                int iy2 = 0 ;
                bool trans2ok = transformer12WeakPtr->transform(ix1,iy1,ix2,iy2);
                if( trans2ok )
                {
                    bool inside2Ok = PixelCoorInsideImage(ix2,iy2,xsize2,ysize2);
                    //1 2两个影像中心像素都在图像内
                    if( inside2Ok )
                    {
                        float value2 = pRaster2->getValuef(ix2,iy2,reBandIndex) ;

                        bool pass2ok = true ;
                        if(usePerFileMask){
                            int remaskval = pRasterReMask->getValuei(ix2,iy2,0) ;
                            pass2ok = reMvv.pass(remaskval) ;
                        }

                        //value2 ok ?
                        if(pass2ok && value2 >= valid0re && value2 <= valid1re )
                        {
                            ++numberOfMatchPixel ;
                            float realvalue1 = (value1+offsetin)*slopein ;
                            float realvalue2 = (value2+offsetre)*slopere ;
                            float diff1 = realvalue1 - realvalue2 ;
                            tempDiffVec.push_back(diff1) ;

                            diffRaster.setValuef(ix1,iy1,0,diff1) ;

                            tempIndataVec.push_back(realvalue1) ;
                            tempRefdataVec.push_back(realvalue2) ;

                            if( realvalue2 != 0 ){
                                float re1 = diff1 / realvalue2 ;
                                tempReVec.push_back(re1) ;
                            }

                        }//if( value2 >= valid0re && value2 <= valid1re )
                        else {
                            diffRaster.setValuef(ix1,iy1,0,outFillvalue) ;
                        }
                    }//if( inside2Ok )
                    else {
                        diffRaster.setValuef(ix1,iy1,0,outFillvalue) ;
                    }
                }//if( trans2ok )
                else {
                    diffRaster.setValuef(ix1,iy1,0,outFillvalue) ;
                }
            }//if( value1 >= valid0in && value1 <= valid1in )
            else {
                diffRaster.setValuef(ix1,iy1,0,outFillvalue) ;
            }
        }//for(int ix1 = 0 ; ix1 < xsize1 ; ++ix1 )

        //write into files
        if( tempIndataVec.size() > 0 ){
            fwrite( (void*)tempIndataVec.data()  , 4 , tempIndataVec.size(), pf_rawin) ;
            fwrite( (void*)tempRefdataVec.data() , 4 , tempRefdataVec.size(), pf_rawref) ;
            fwrite( (void*)tempDiffVec.data()    , 4 , tempDiffVec.size(), pf_diff) ;
        }
        if( tempReVec.size() >0 ){
            fwrite( (void*)tempReVec.data() , 4 , tempReVec.size(), pf_rdiff) ;
        }


        //progress
        int progress1 = iy1 * 100.f / ysize1 ;
        if( progress1 != progress0 ){
            progress0 = progress1 ;
        }

    }

    if(pf_rawin) fclose(pf_rawin) ;
    if( pf_rawref) fclose(pf_rawref) ;
    if( pf_diff) fclose(pf_diff) ;
    if( pf_rdiff) fclose(pf_rdiff) ;
    if( pRaster1 ) delete pRaster1 ;
    if( pRaster2 ) delete pRaster2 ;
    if( pRasterInMask ) delete pRasterInMask ;
    if( pRasterReMask ) delete pRasterReMask ;
    if( pGlobalMask ) delete pGlobalMask ;

    bool saveok = diffRaster.save( diffrasterfilename ) ;
    if( saveok==false )
    {
        error = string("failed to save ")+diffrasterfilename ;
        return false ;
    }

    return true;
}


//检查半径内像素是否都有效，都有效的话计算标准差并返回true， 反正返回false
bool WRasterCompare::ComputeBoxStddev(wGdalRaster* raster,int bandindex,int centerx,int centery,
                                      int boxRadius,double valid0,double valid1,
                                      int xsize,int ysize,
                                      double& stddev,double& aver)
{
    bool foundBadPixel = false ;
    double sum = 0 ;
    double sq_sum = 0 ;
    int cnt = 0 ;
    for(int iy = centery - boxRadius; iy <= centery + boxRadius; ++iy )
    {
        if( iy < 0 || iy > ysize-1 )
        {
            foundBadPixel = true ;
            break ;
        }
        for(int ix = centerx - boxRadius; ix <= centerx + boxRadius; ++ ix )
        {
            if( ix <0 || ix > xsize -1 )
            {
                foundBadPixel = true ;
                break ;
            }
            float val1 = raster->getValuef(ix,iy,bandindex) ;
            if( val1 >= valid0 && val1 <= valid1 )
            {
                sum += val1 ;
                sq_sum += val1*val1 ;
                ++ cnt ;
            }else{
                foundBadPixel = true ;
                break ;
            }
        }
        if( foundBadPixel == true ) {
            break ;
        }
    }
    if( foundBadPixel==true ) return false ;
    double mean = sum/cnt ;
    double variance = sq_sum / cnt - mean*mean ;
    stddev = sqrt(variance) ;
    aver = mean ;
    return true ;
}
