#pragma execution_character_set("utf-8")

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "gdal_priv.h"
#include <vector>
#include "wrastercompare.h"
#include "wfilepair.h"
#include "wprocessqueue.h"
#include "wcomparetask.h"
#include <QDebug>
#include "plotorder.h"
#include "plotvarreplaceutil.h"
#include "ajson5.h"
#include "computeapu.h"

using std::vector;
using namespace ArduinoJson;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    GDALAllRegister() ;


    // setWindowTitle("遥感数据质量检验（图像对图像）V1.0.2") ;

    // 2023-1-9
    // 1.图像坐标对比，直接把输入数据坐标写入结果影像
    // 2.增加无效值输出
    //setWindowTitle("遥感数据质量检验（图像对图像）V1.1.0") ;

    //2023-3-1
    //1. remove python staff use gnuplot-portable version5.4.6 ok v2.0.1
    //2. 数字结果汇总到一个csv中 ok v2.0.1
    //3. 增加配置参数保存 v2.0.2 ok
    //4. 修复进度条 没有分离线程，搞不定，这个版本算了。
    //5. 直方图Y轴数据改为百分比 0-100 单位% ok
    //6. 增加APU空间图计算参考王圆圆文章 ok
    //7. 空间偏差绘图 ok
    setWindowTitle("遥感数据质量检验（图像对图像）V2.0.2") ;

    QObject::connect( wProcessQueue::getInstance() , &wProcessQueue::progressChanged,
                      this,&MainWindow::progressChanged ) ;

    //载入质检配置
    QObject::connect( ui->actionLoad , &QAction::triggered , this , &MainWindow::on_actionLoadProject_clicked ) ;
    //保存质检配置
    QObject::connect( ui->actionSave , &QAction::triggered , this , &MainWindow::on_actionSaveProject_clicked ) ;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButtonAddInput_clicked()
{

    QFileDialog dlg(this) ;
    QStringList fnames = dlg.getOpenFileNames(this,"Select Input files","","Tiff files(*.tif)") ;

    if( fnames.length()> 0 )
    {
        if( ui->listWidgetInfiles->count()==0 ){
            int bandcount = getBandCount(fnames[0]) ;
            ui->comboBoxBandIn->clear() ;
            for(int iband = 0 ; iband < bandcount;++iband ){
                ui->comboBoxBandIn->addItem( QString::number(iband) ) ;
            }
        }
        for(int i = 0 ; i<fnames.length(); ++ i )
        {
            ui->listWidgetInfiles->addItem(
                        fnames[i]
                        ) ;
        }
    }

}

void MainWindow::on_pushButtonClearInput_clicked()
{
    ui->listWidgetInfiles->clear() ;
}

void MainWindow::on_pushButtonAddRef_clicked()
{
    QFileDialog dlg(this) ;
    QStringList fnames = dlg.getOpenFileNames(this,"Select Ref files","","Tiff files(*.tif)") ;

    if( fnames.length()> 0 )
    {
        if( ui->comboBoxBandRef->count()==0 ){
            int bandcount = getBandCount(fnames[0]) ;
            ui->comboBoxBandRef->clear() ;
            for(int iband = 0 ; iband < bandcount;++iband ){
                ui->comboBoxBandRef->addItem( QString::number(iband) ) ;
            }
        }
        for(int i = 0 ; i<fnames.length(); ++ i )
        {
            ui->listWidgetReffiles->addItem(
                        fnames[i]
                        ) ;
        }
    }
}

void MainWindow::on_pushButtonClearRef_clicked()
{
    ui->listWidgetReffiles->clear() ;
}

void MainWindow::on_pushButtonOpenOutfile_clicked()
{
    QString name = QFileDialog::getExistingDirectory(this) ;
    if( name.isEmpty()==false ){
        ui->lineEditOutfile->setText(name);
    }
}

double getDoubleFromLineEdit(QLineEdit* lineEdit)
{
    if( lineEdit->text().isEmpty() ){
        return 0 ;
    }else{
        return lineEdit->text().toDouble() ;
    }
}

void MainWindow::on_pushButtonOk_clicked()
{
    int progressMaxValue = 0 ;
    if( ui->listWidgetInfiles->count()==0 ){
        QMessageBox::information(this,"info","infiles empty") ;
        return ;
    }

    if( ui->listWidgetReffiles->count()==0 ){
        QMessageBox::information(this,"info","reffiles empty") ;
        return ;
    }

    AssertLineEdit as ;
    MaskValueValidator inMvv, reMvv, globalMvv ;// Mask Data Validator

    try {
        // ///// in ///////////////////////////
        double valid0In = as.getDouble(ui->lineEditValid0,"Input Valid Min") ;
        double valid1In = as.getDouble(ui->lineEditValid1,"Input Valid Max") ;
        double slopeIn = as.getDouble(ui->lineEditSlopeIn,"Input Slope") ;
        double interIn = as.getDouble(ui->lineEditInterIn,"Input Interception") ;

        if(ui->comboBoxBandIn->currentIndex()<0 ){
            QMessageBox::information(this,"info","band of indata empty") ;
            return ;
        }
        int bandindexIn = ui->comboBoxBandIn->currentIndex();

        // //// in end -----------------------

        // ////// out fill value /////////////
        double outFillvalue = as.getDouble(ui->lineEditOutFillvalue,"Output Fill Value") ;

        // ////// out fill value end ---------


        //  ref ///////////////
        double valid0ref = as.getDouble(ui->lineEditValid0Ref,"Ref Valid Min") ;
        double valid1ref = as.getDouble(ui->lineEditValid1Ref,"Ref Valid Max") ;
        double slopeRef = as.getDouble(ui->lineEditSlopeRef,"Ref Slope") ;
        double interRef = as.getDouble(ui->lineEditInterRef,"Ref Interception") ;
        if(ui->comboBoxBandRef->currentIndex()<0 ){
            QMessageBox::information(this,"info","band of refdata empty") ;
            return ;
        }
        int bandindexRef = ui->comboBoxBandRef->currentIndex() ;
        //  ref end -----------


        // About Gnuplot and script.plt //////////////////////
        QString xlabel = ui->lineEditXLabel->text();
        QString ylabel = ui->lineEditYLabel->text() ;
        QString gnuplotexe = as.getStr(ui->lineEditGnuplot,"Gnuplot Exec") ;
        QString scriptPlot = as.getStr(ui->lineEditScriptPlot,"Script.plt") ;

        // Scatter staff
        double scatterXmin = as.getDouble(ui->lineEditScatterXMin,"Scatter X Min") ;
        double scatterXmax = as.getDouble(ui->lineEditScatterXMax,"Scatter X Max") ;
        double scatterYmin = as.getDouble(ui->lineEditScatterYMin,"Scatter Y Min") ;
        double scatterYmax = as.getDouble(ui->lineEditScatterYMax,"Scatter Y Max") ;

        // Histgram Staff
        double histXmin = as.getDouble(ui->lineEditHistXMin,"Diff Hist X Min") ;
        double histXmax = as.getDouble(ui->lineEditHistXMax,"Diff Hist X Max") ;
        int histCount = as.getDouble(ui->lineEditHistCount,"Hist Count") ;
        double histYmax = as.getDouble(ui->lineEditBiasPlotMaxY,"Bias Hist Y Max") ;
        double histXminRe = as.getDouble(ui->lineEditHistXminRE,"RelDiff Hist X Min") ;
        double histXmaxRe = as.getDouble(ui->lineEditHistXmaxRE,"RelDiff Hist X Max") ;

        //hist x axis and y axis
        QString histXLabel = ui->lineEditDiffXLabel->text() ;
        QString histYLabel = ui->lineEditDiffYLabel->text() ;

        //rel error hist x axis and yaxis
        QString rehistXLabel = ui->lineEditREXLabel->text() ;
        QString rehistYLabel = ui->lineEditREYLabel->text() ;

        // End About plot  -------------------

        // select coordinate for matching /////////////////////
        bool useProj = false ;//use image coordinate
        if( ui->radioButtonMatchProj->isChecked() )
        {
            useProj = true ;
        }
        // end coordinate --------------------------------------


        //input tag or ref tag ///////////////////////////
        QString intag = as.getStr(ui->lineEditInTag,"Input File Tag") ;
        QString retag = as.getStr(ui->lineEditReTag,"Ref File Tag") ;

        // end input or ref tag --------------------------



        // match filename /////////////////
        int infilenamePos = as.getDouble(ui->lineEditInDatePos ,"In Fname Pos") ;
        int reffilenamePos = as.getDouble(ui->lineEditRefDatePos,"Ref Fname Pos") ;
        int filenameLen = as.getDouble(ui->lineEditDateLen,"Match Fname Len") ;

        // end match filename -------------



        // perfile MASK files ////////////////////////////////////
        bool usePerFileMask = ui->checkBoxPerFileMask->isChecked() ;
        int perfileInMaskFnamePos = 0 ;
        int perfileReMaskFnamePos = 0 ;
        QString perfileInMaskValues = "";
        QString perfileReMaskValues = "";
        QStringList inMaskFilepathList , reMaskFilepathList , inMaskFnameList, reMaskFnameList ;
        if(usePerFileMask==true ){
            perfileInMaskFnamePos = as.getDouble(ui->lineEditInMaskFnamePos,"InMask Fname Pos") ;
            perfileReMaskFnamePos = as.getDouble(ui->lineEditReMaskFnamePos,"RefMask Fname Pos") ;
            perfileInMaskValues = as.getStr(ui->lineEditInMaskValues,"InMask Values") ;
            perfileReMaskValues = as.getStr(ui->lineEditReMaskValues,"RefMask Values") ;
            inMvv.init(perfileInMaskValues.toStdString());
            reMvv.init(perfileReMaskValues.toStdString());
            for(int i1 = 0 ;i1 < ui->listWidgetInMaskList->count();++i1 )
            {
                QString fpath = ui->listWidgetInMaskList->item(i1)->text() ;
                QFileInfo finfo(fpath) ;
                inMaskFilepathList.push_back(fpath ) ;
                inMaskFnameList.push_back(finfo.baseName()) ;
            }
            for(int i1 = 0 ;i1 < ui->listWidgetReMaskList->count();++i1 )
            {
                QString fpath = ui->listWidgetReMaskList->item(i1)->text() ;
                QFileInfo finfo(fpath) ;
                reMaskFilepathList.push_back(fpath ) ;
                reMaskFnameList.push_back(finfo.baseName()) ;
            }
        }
        // END perfile Mask files --------------------------------



        // global mask ( must be georeferenced /////////////
        bool useGlobalMask = ui->checkBoxUseGlobalMask->isChecked();
        QString globalMaskTag = "" ;
        QString globalMaskValues = "" ;
        QString globalMaskFilename = "" ;
        if(useGlobalMask==true )
        {
            globalMaskTag = as.getStr(ui->lineEditGlobalMaskTag,"Global Mask Tag") ;
            globalMaskValues = as.getStr(ui->lineEditGlobalMaskValues,"Global Mask Values") ;
            globalMaskFilename = as.getStr(ui->lineEditGlobalMaskFilename,"Global Mask Filename") ;
            globalMvv.init(globalMaskValues.toStdString());
        }

        // end global mask ---------------------------------



        // output dir ////////////////////////////
        QString outdir = as.getStr(ui->lineEditOutfile,"Out Dir") ;
        if( outdir.back()=='/' || outdir.back()=='\\' ){
            //ok
        }else{
            outdir += '/' ;
        }

        QString alloutcsvfile = as.getStr(ui->lineEditOutCSV,"Out CSV") ;
        ofstream alloutOfs(alloutcsvfile.toStdString().c_str() , std::ios::app ) ;
        alloutOfs<<"basename,sampleCnt,RMSE,correlation,Rsquared,lineK,lineB\n" ;

        //APU from WangYuanYuan
        string outAfilename = alloutcsvfile.toStdString()+"_Acc.tif" ;
        string outPfilename = alloutcsvfile.toStdString()+"_Pre.tif" ;
        string outUfilename = alloutcsvfile.toStdString()+"_Unc.tif" ;
        string outCfilename = alloutcsvfile.toStdString()+"_Cnt.tif" ;
        vector<string> biasFilenameVec ;

        // end output dir -------------------------



        // QGS绘图 --------------------------
        QString mapPrintExe = ui->lineEditMapPrintExe->text() ;
        QString qgsfilename = ui->lineEditQgsTemplate->text() ;
        bool qgsZoomToData = ui->checkBoxZoom->isChecked() ;
        bool qgsClipContent = ui->checkBoxClipContent->isChecked() ;
        int qgsDPI = (int)as.getDouble(ui->lineEditDpi,"DPI") ;
        if(qgsDPI<72) qgsDPI=72;
        if(qgsDPI>600) qgsDPI=600 ;

        //----------------------------------




        QStringList infilenameArr, refilenameArr ;
        for(int ifile1 = 0 ; ifile1 < ui->listWidgetInfiles->count(); ++ifile1 )
        {
            QString filename1 = ui->listWidgetInfiles->item(ifile1)->text() ;
            QFileInfo finfo(filename1) ;
            infilenameArr.push_back( finfo.baseName() ) ;
        }

        for(int ifile2 = 0 ; ifile2 < ui->listWidgetReffiles->count(); ++ifile2)
        {
            QString filename2 = ui->listWidgetReffiles->item(ifile2)->text() ;
            QFileInfo finfo(filename2) ;
            refilenameArr.push_back( finfo.baseName() ) ;
        }
        ui->progressBar->setRange(0, infilenameArr.size() ) ;
        ui->progressBar->setValue(0) ;
        progressMaxValue=infilenameArr.size();
        for(int ifile1 = 0 ; ifile1 < infilenameArr.size() ; ++ifile1 )
        {
            QString matcher = infilenameArr[ifile1].mid( infilenamePos, filenameLen) ;
            int iref = getMatchFnameFromList(matcher,reffilenamePos,filenameLen,refilenameArr) ;
            int inmaskIndex = getMatchFnameFromList(matcher,perfileInMaskFnamePos,filenameLen,inMaskFnameList) ;
            int remaskIndex = getMatchFnameFromList(matcher,perfileReMaskFnamePos,filenameLen,reMaskFnameList) ;

            if( (usePerFileMask==false && iref>=0)
                    || (usePerFileMask==true && iref>=0 && inmaskIndex>=0 && remaskIndex>=0) )
            {
                PlotOrder order1 ;
                order1.infilename = ui->listWidgetInfiles->item(ifile1)->text();//infilenameArr[ifile1] ;
                order1.reffilename = ui->listWidgetReffiles->item(iref)->text();//refilenameArr[iref] ;

                order1.valid0in = valid0In ;
                order1.valid1in = valid1In ;
                order1.slopein = slopeIn ;
                order1.interin = interIn ;
                order1.bandin = bandindexIn ;

                order1.valid0ref = valid0ref ;
                order1.valid1ref = valid1ref ;
                order1.sloperef = slopeRef ;
                order1.interref = interRef ;
                order1.bandref = bandindexRef ;

                order1.useProj = (int)useProj ;
                order1.inTag = intag ;
                order1.reTag = retag ;
                order1.gnuplotexe = gnuplotexe ;

                order1.xlabel = xlabel ;
                order1.ylabel = ylabel ;

                order1.scatterXmin = scatterXmin ;
                order1.scatterXmax = scatterXmax ;
                order1.scatterYmin = scatterYmin ;
                order1.scatterYmax = scatterYmax ;
                order1.histXmin = histXmin ;
                order1.histXmax = histXmax ;
                order1.histCount = histCount ;
                order1.histYmax =histYmax ;

                order1.histXminRE = histXminRe ;
                order1.histXmaxRE = histXmaxRe ;

                order1.histXLabel = histXLabel ;
                order1.histYLabel = histYLabel ;
                order1.rehistXLabel = rehistXLabel ;
                order1.rehistYLabel = rehistYLabel ;

                order1.usePerfileMask = (int)usePerFileMask ;
                if(order1.usePerfileMask==1){
                    order1.inMaskValues = perfileInMaskValues ;
                    order1.reMaskValues = perfileReMaskValues ;
                    order1.inMaskFilename = ui->listWidgetInMaskList->item(inmaskIndex)->text() ;
                    order1.reMaskFilename = ui->listWidgetReMaskList->item(remaskIndex)->text() ;
                }

                order1.useGlobalMask = (int)useGlobalMask ;
                if(order1.useGlobalMask==1){
                    order1.globalMaskTag = globalMaskTag;
                    order1.globalMaskValues = globalMaskValues ;
                    order1.globalMaskFilename = globalMaskFilename ;
                }

                QString outbasename = QString("order-") + order1.inTag+"_VS_"+order1.reTag
                        +"-m_"+matcher
                        +"-g_"+order1.globalMaskTag
                        +"-pfm_"+QString::number(order1.usePerfileMask) ;
                QString order1filename = outdir + outbasename+".json" ;

                order1.outbasename = outdir + outbasename ;
                order1.in_vs_ref_datafile = outdir + outbasename + "-inNre.txt" ;
                order1.diffdatafile = outdir + outbasename + "-diff.txt" ;
                order1.heatmapdatafile = outdir + outbasename + "-heat.txt" ;
                order1.diffrasterfilename = outdir + outbasename + "-diff.tif" ;
                order1.outhistpngfile = outdir + outbasename + "-hist.png" ;
                order1.outscatterpngfile = outdir + outbasename + "-scat.png" ;

                //copy scriptPlot
                order1.plotscriptfile = outdir + outbasename + "-plot.plt" ;
                bool copyScriptOk = QFile::copy( scriptPlot , order1.plotscriptfile) ;
                if(copyScriptOk==false )
                {
                    string msg = string("Failed to copy scriptPlot from ")
                            + scriptPlot.toStdString() + " to " + order1.plotscriptfile.toStdString() ;
                    std::logic_error ex1( msg ) ;
                    throw ex1 ;
                }

                // 坐标匹配
                order1.matcher = matcher ;


                string comapreError;
                bool cok = WRasterCompare::Compare2(
                            order1.infilename.toStdString(),
                            order1.reffilename.toStdString(),
                            bandindexIn,bandindexRef,
                            useProj,
                            valid0In,valid1In,slopeIn,interIn,
                            valid0ref,valid1ref,slopeRef,interRef,
                            histCount, histXmin, histXmax ,
                            usePerFileMask,
                            order1.inMaskFilename.toStdString(), order1.reMaskFilename.toStdString(),
                            inMvv, reMvv,
                            useGlobalMask,
                            order1.globalMaskFilename.toStdString(), globalMvv,
                            order1.in_vs_ref_datafile.toStdString(), //scatter
                            order1.diffdatafile.toStdString(),       //error
                            order1.heatmapdatafile.toStdString() ,   //scatter heatmap
                            order1.scatterXmin,
                            order1.scatterXmax,
                            order1.scatterYmin,
                            order1.scatterYmax,
                            order1.diffrasterfilename.toStdString(),
                            order1.matchingCount,
                            order1.correlation,
                            order1.rsquared,
                            order1.linearK,
                            order1.linearB,
                            order1.rmse ,
                            outFillvalue,
                            comapreError
                            ) ;
                if( cok==false ){
                    QMessageBox::information(this,"info", QString("compare failed for ") + comapreError.c_str()) ;
                    return ;
                }

                order1.writeToJsonFile(order1filename) ;

                // 使用对比生成中间数据替换plt模板中的预定义变量{{{...}}}
                PlotVarReplaceUtil replacer ;
                replacer.replace(
                            order1.plotscriptfile.toStdString(),
                            order1.plotscriptfile.toStdString() ,
                            order1) ;


                //call gnuplot
                // plot result filename is order1filename , write into scriptPlot.
                QString cmd1 = order1.gnuplotexe + " " + order1.plotscriptfile ;
                spdlog::info("begin call command:{}" , cmd1.toStdString());
                int ret = system(cmd1.toStdString().c_str()) ;
                spdlog::info("command return code:{}" , ret);

                //汇总结果
                alloutOfs<<order1.outbasename.toStdString()<<","
                        <<order1.matchingCount<<","
                        <<order1.rmse<<","
                       <<order1.correlation<<","
                      <<order1.rsquared<<","
                     <<order1.linearK<<","
                    <<order1.linearB<<","<<"\n" ;

                //偏差输出文件数组
                biasFilenameVec.push_back(order1.diffrasterfilename.toStdString()) ;

                //QGS绘图
                if( mapPrintExe.isEmpty()==false && qgsfilename.isEmpty()==false )
                {
                    QString tempZoomStr = "0" ;
                    if( qgsZoomToData ) tempZoomStr="1";
                    QString tempClipContent="0" ;
                    if( qgsClipContent ) tempClipContent="1" ;
                    QString tempQgsExportFilename=order1.diffrasterfilename+".qgs.png" ;
                    QString qgscmd1 = mapPrintExe + " "
                            + qgsfilename + " "
                            + order1.diffrasterfilename + " "
                            + " \"\" "
                            + tempZoomStr + " " + tempClipContent + " " + QString::number(qgsDPI) + " "
                            + tempQgsExportFilename ;
                    int qgs_ret = system( qgscmd1.toStdString().c_str() ) ;

                }
                ui->progressBar->setValue(ifile1) ;
                this->repaint() ;
            }
        }
        //A accuracy P precision U uncertainty C count
        // 一个文件计算没有意义
        if( biasFilenameVec.size() > 1 )
        {
            string apuError;
            ComputeAPU computer ;
            bool apuok1 = computer.computeAnU(biasFilenameVec,(int)outFillvalue,
                                outAfilename,outUfilename,outCfilename,apuError) ;
            if(apuok1==false)
            {
                QMessageBox::information(this,"error",  apuError.c_str() ) ;
                return ;
            }
            apuok1 = computer.computeP(biasFilenameVec,(int)outFillvalue,
                                       outAfilename,outCfilename,outPfilename,apuError) ;
            if(apuok1==false)
            {
                QMessageBox::information(this,"error",  apuError.c_str() ) ;
                return ;
            }
        }
    } catch (std::logic_error& ex) {
        QMessageBox::warning(this,"WARN",ex.what()) ;
        return ;
    }
    QMessageBox::information(this,"info","Done") ;
    ui->progressBar->setValue(progressMaxValue) ;
}

void MainWindow::progressChanged(int type, int doneTaskCnt,int totalTaskCnt, int progressOrStatus, QString currTaskDoneInfo )
{
    if(type==0 ){
        this->ui->progressBar->setValue(progressOrStatus) ;
    }
}

void MainWindow::on_pushButtonCancel_clicked()
{
    this->close() ;
}

//open file dialog for global mask file
void MainWindow::on_pushButtonLandCover_clicked()
{
    QString infile = QFileDialog::getOpenFileName(this," 选择输入文件",".","GeoTiff(*.tif)");
    if( infile.isEmpty() ==false ){
        ui->lineEditGlobalMaskFilename->setText(infile) ;
    }
}

//get raster band count
int MainWindow::getBandCount(QString filename)
{
    GDALDataset* ds = (GDALDataset*)GDALOpen( filename.toStdString().c_str() , GA_ReadOnly) ;
    if( ds==0 ){
        QMessageBox::information(this,"info",QString("bad input file ") + filename) ;
        return 0 ;
    }else{
        int bcnt = ds->GetRasterCount() ;
        GDALClose(ds) ;
        return  bcnt;
    }
}



// Open File Dialog for script
void MainWindow::on_pushButtonOpenScript_clicked()
{
    QString file1 = QFileDialog::getOpenFileName(this,"Please select plot.plt",".","Script(*.plt)");
    if( file1.isEmpty() ==false ){
        ui->lineEditScriptPlot->setText(file1) ;
    }
}

// Open File Dialog for perfile mask
void MainWindow::on_pushButtonAddInMask_clicked()
{
    QFileDialog dlg(this) ;
    QStringList fnames = dlg.getOpenFileNames(this,"Select IN MASK files","","Tiff files(*.tif)") ;
    if( fnames.length()> 0 )
    {
        for(int i = 0 ; i<fnames.length(); ++ i )
        {
            ui->listWidgetInMaskList->addItem(
                        fnames[i]
                        ) ;
        }
    }
}
// Clear perfile mask files
void MainWindow::on_pushButtonClearInMask_clicked()
{
    ui->listWidgetInMaskList->clear() ;
}

// Open File Dialog for perfile reference mask
void MainWindow::on_pushButtonAddReMask_clicked()
{
    QFileDialog dlg(this) ;
    QStringList fnames = dlg.getOpenFileNames(this,"Select REF MASK files","","Tiff files(*.tif)") ;
    if( fnames.length()> 0 )
    {
        for(int i = 0 ; i<fnames.length(); ++ i )
        {
            ui->listWidgetReMaskList->addItem(
                        fnames[i]
                        ) ;
        }
    }
}

// Clear reference perfile mask files
void MainWindow::on_pushButtonClearReMask_clicked()
{
    ui->listWidgetReMaskList->clear() ;
}

//if no matching return -1
int MainWindow::getMatchFnameFromList(QString matcher, int pos, int len, QStringList& fnameList)
{
    for(int i = 0 ; i<fnameList.size(); ++ i )
    {
        if( fnameList[i].length() >= pos+len ){ //bugfixed 2023-3-3
            QString part = fnameList[i].mid(pos,len) ;
            if( matcher.compare(part) == 0 ){
                return i ;
            }
        }
    }
    return -1 ;
}

//Open Dialog for Gnuplot.exe
void MainWindow::on_pushButtonOpenGnuplot_clicked()
{
    QString file1 = QFileDialog::getOpenFileName(this,"Please select gnuplot.exe",".","Exec(*.exe)");
    if( file1.isEmpty() ==false ){
        ui->lineEditGnuplot->setText(file1) ;
    }
}
//统计信息以追加的形式添加到CSV文件中
void MainWindow::on_pushButtonOpenOutCSV_clicked()
{
    QString file1 = QFileDialog::getSaveFileName(this,"Please select CSV",".","CSV(*.csv)");
    if( file1.isEmpty() ==false ){
        ui->lineEditOutCSV->setText(file1) ;
    }
}

//质检方案
void MainWindow::on_actionLoadProject_clicked()
{
    QString file1 = QFileDialog::getOpenFileName(this,"Open QA Project",".","JSON(*.json)");
    if( file1.isEmpty() ==false ){
        //Read Staff
        std::ifstream ifs(file1.toStdString().c_str());
        if( ifs.good()==false )
        {
            QMessageBox::warning(this,"Warning","质检方案文件读取失败.") ;
            return ;
        }
        DynamicJsonBuffer jsonBuffer;
        JsonObject& jsonRoot = jsonBuffer.parseObject(ifs);

        ui->lineEditValid0->setText( jsonRoot["in_valid0"].as<char*>() ) ;
        ui->lineEditValid1->setText( jsonRoot["in_valid1"].as<char*>() ) ;
        ui->lineEditSlopeIn->setText( jsonRoot["in_slope"].as<char*>() ) ;
        ui->lineEditInterIn->setText( jsonRoot["in_inter"].as<char*>() ) ;
        ui->lineEditOutFillvalue->setText( jsonRoot["out_fillvalue"].as<char*>() ) ;
        ui->lineEditValid0Ref->setText( jsonRoot["re_valid0"].as<char*>() ) ;
        ui->lineEditValid1Ref->setText( jsonRoot["re_valid1"].as<char*>() ) ;
        ui->lineEditSlopeRef->setText( jsonRoot["re_slope"].as<char*>() ) ;
        ui->lineEditInterRef->setText( jsonRoot["re_inter"].as<char*>() ) ;
        ui->lineEditXLabel->setText( jsonRoot["scat_xlabel"].as<char*>() ) ;
        ui->lineEditYLabel->setText( jsonRoot["scat_ylabel"].as<char*>() ) ;
        ui->lineEditGnuplot->setText( jsonRoot["gnuplotexe"].as<char*>() ) ;
        ui->lineEditScriptPlot->setText( jsonRoot["plotscript"].as<char*>() ) ;
        ui->lineEditScatterXMin->setText( jsonRoot["scat_xmin"].as<char*>() ) ;
        ui->lineEditScatterXMax->setText( jsonRoot["scat_xmax"].as<char*>() ) ;
        ui->lineEditScatterYMin->setText( jsonRoot["scat_ymin"].as<char*>() ) ;
        ui->lineEditScatterYMax->setText( jsonRoot["scat_ymax"].as<char*>() ) ;
        ui->lineEditHistXMin->setText( jsonRoot["hist_xmin"].as<char*>() ) ;
        ui->lineEditHistXMax->setText( jsonRoot["hist_xmax"].as<char*>() ) ;
        ui->lineEditBiasPlotMaxY->setText( jsonRoot["hist_ymax"].as<char*>() ) ;
        ui->lineEditHistCount->setText( jsonRoot["hist_barcount"].as<char*>() ) ;
        ui->lineEditHistXminRE->setText( jsonRoot["hist_rexmin"].as<char*>() ) ;
        ui->lineEditHistXmaxRE->setText( jsonRoot["hist_rexmax"].as<char*>() ) ;
        ui->lineEditDiffXLabel->setText( jsonRoot["hist_xlabel"].as<char*>() ) ;
        ui->lineEditDiffYLabel->setText( jsonRoot["hist_ylabel"].as<char*>() ) ;
        ui->lineEditREXLabel->setText( jsonRoot["hist_rexlabel"].as<char*>() ) ;
        ui->lineEditREYLabel->setText( jsonRoot["hist_reylabel"].as<char*>() ) ;
        ui->lineEditInTag->setText( jsonRoot["in_tag"].as<char*>() ) ;
        ui->lineEditReTag->setText( jsonRoot["re_tag"].as<char*>() ) ;
        ui->lineEditInDatePos->setText( jsonRoot["in_datepos"].as<char*>() ) ;
        ui->lineEditRefDatePos->setText( jsonRoot["re_datepos"].as<char*>() ) ;
        ui->lineEditDateLen->setText( jsonRoot["in_datelen"].as<char*>() ) ;
        ui->lineEditInMaskFnamePos->setText( jsonRoot["mask_indatepos"].as<char*>() ) ;
        ui->lineEditReMaskFnamePos->setText( jsonRoot["mask_redatepos"].as<char*>() ) ;
        ui->lineEditInMaskValues->setText( jsonRoot["mask_inpassval"].as<char*>() ) ;
        ui->lineEditReMaskValues->setText( jsonRoot["mask_repassval"].as<char*>() ) ;
        ui->lineEditGlobalMaskTag->setText( jsonRoot["mask_global_tag"].as<char*>() ) ;
        ui->lineEditGlobalMaskFilename->setText( jsonRoot["mask_global_filename"].as<char*>() ) ;
        ui->lineEditGlobalMaskValues->setText( jsonRoot["mask_global_passval"].as<char*>() ) ;
        ui->lineEditOutfile->setText( jsonRoot["out_dir"].as<char*>() ) ;
        ui->lineEditOutCSV->setText( jsonRoot["out_allcsvfilename"].as<char*>() ) ;

        int inbandcount = jsonRoot["in_bandcount"].as<int>() ;
        int rebandcount = jsonRoot["re_bandcount"].as<int>() ;
        int inbandindex = jsonRoot["in_bandindex"].as<int>() ;
        int rebandindex = jsonRoot["re_bandindex"].as<int>() ;
        ui->comboBoxBandIn->clear() ;
        ui->comboBoxBandRef->clear() ;
        for(int iband = 0 ; iband < inbandcount;++iband ){
            ui->comboBoxBandIn->addItem( QString::number(iband) ) ;
        }
        for(int iband = 0 ; iband < rebandcount;++iband ){
            ui->comboBoxBandRef->addItem( QString::number(iband) ) ;
        }
        ui->comboBoxBandIn->setCurrentIndex(inbandindex) ;
        ui->comboBoxBandRef->setCurrentIndex(rebandindex) ;

        if(jsonRoot["match_proj"].as<bool>() == true )
        {
            ui->radioButtonMatchProj->setChecked(true) ;
            ui->radioButtonMatchPixel->setChecked(false) ;
        }else{
            ui->radioButtonMatchProj->setChecked(false) ;
            ui->radioButtonMatchPixel->setChecked(true) ;
        }

        ui->checkBoxPerFileMask->setChecked( jsonRoot["mask_perfile"].as<bool>() ) ;
        ui->checkBoxUseGlobalMask->setChecked( jsonRoot["mask_useglobal"].as<bool>() ) ;

        JsonArray& mask_inArr = jsonRoot["mask_infiles"].as<JsonArray>() ;
        JsonArray& mask_reArr = jsonRoot["mask_refiles"].as<JsonArray>() ;
        JsonArray& inArr = jsonRoot["in_files"].as<JsonArray>() ;
        JsonArray& reArr = jsonRoot["re_files"].as<JsonArray>() ;

        ui->listWidgetInMaskList->clear() ;
        ui->listWidgetReMaskList->clear() ;
        ui->listWidgetInfiles->clear() ;
        ui->listWidgetReffiles->clear() ;


        for(int i1 = 0 ;i1 < mask_inArr.size() ;++i1 )
        {
            string fpath = mask_inArr[i1].as<char*>() ;//
            ui->listWidgetInMaskList->addItem( fpath.c_str() ) ;
        }
        for(int i1 = 0 ;i1 < mask_reArr.size() ;++i1 )
        {
            string fpath = mask_reArr[i1].as<char*>() ;//
            ui->listWidgetReMaskList->addItem( fpath.c_str() ) ;
        }
        for(int ifile1 = 0 ; ifile1 < inArr.size() ; ++ifile1 )
        {
            string filename1 = inArr[ifile1].as<char*>() ;
            ui->listWidgetInfiles->addItem( filename1.c_str() ) ;
        }
        for(int ifile2 = 0 ; ifile2 < reArr.size() ; ++ifile2)
        {
            string filename2 = reArr[ifile2].as<char*>() ;
            ui->listWidgetReffiles->addItem( filename2.c_str() ) ;
        }

        //Qgis
        ui->lineEditMapPrintExe->setText(jsonRoot["qgs_mapprint"].as<char*>())  ;
        ui->lineEditQgsTemplate->setText(jsonRoot["qgs_template"].as<char*>())  ;
        ui->lineEditDpi->setText(jsonRoot["qgs_dpi"].as<char*>())  ;
        ui->checkBoxZoom->setChecked( jsonRoot["qgs_zoom"].as<bool>() ) ;
        ui->checkBoxClipContent->setChecked( jsonRoot["qgs_clipextent"].as<bool>() ) ;

        QMessageBox::information(this,"Done","质检方案已加载") ;
    }
}
void MainWindow::on_actionSaveProject_clicked()
{
    QString file1 = QFileDialog::getSaveFileName(this,"Save QA Project",".","JSON(*.json)");
    if( file1.isEmpty() ==false ){
        //Saveing staff
        ArduinoJson::DynamicJsonBuffer jsonBuffer;
        JsonObject& jsonRoot = jsonBuffer.createObject() ;
        jsonRoot["in_valid0"] = ui->lineEditValid0->text().toStdString() ;
        jsonRoot["in_valid1"] = ui->lineEditValid1->text().toStdString() ;
        jsonRoot["in_slope"] = ui->lineEditSlopeIn->text().toStdString() ;
        jsonRoot["in_inter"] = ui->lineEditInterIn->text().toStdString() ;

        jsonRoot["in_bandindex"] = ui->comboBoxBandIn->currentIndex() ;
        jsonRoot["in_bandcount"] = ui->comboBoxBandIn->count() ;

        jsonRoot["out_fillvalue"] = ui->lineEditOutFillvalue->text().toStdString() ;



        jsonRoot["re_valid0"] = ui->lineEditValid0Ref->text().toStdString() ;
        jsonRoot["re_valid1"] = ui->lineEditValid1Ref->text().toStdString() ;

        jsonRoot["re_slope"] = ui->lineEditSlopeRef->text().toStdString() ;
        jsonRoot["re_inter"] = ui->lineEditInterRef->text().toStdString() ;

        jsonRoot["re_bandindex"] = ui->comboBoxBandRef->currentIndex() ;
        jsonRoot["re_bandcount"] = ui->comboBoxBandRef->count() ;


        jsonRoot["scat_xlabel"] = ui->lineEditXLabel->text().toStdString() ;
        jsonRoot["scat_ylabel"] = ui->lineEditYLabel->text().toStdString() ;
        jsonRoot["gnuplotexe"] = ui->lineEditGnuplot->text().toStdString() ;
        jsonRoot["plotscript"] = ui->lineEditScriptPlot->text().toStdString() ;

        jsonRoot["scat_xmin"] = ui->lineEditScatterXMin->text().toStdString() ;
        jsonRoot["scat_xmax"] = ui->lineEditScatterXMax->text().toStdString() ;
        jsonRoot["scat_ymin"] = ui->lineEditScatterYMin->text().toStdString() ;
        jsonRoot["scat_ymax"] = ui->lineEditScatterYMax->text().toStdString() ;

        jsonRoot["hist_xmin"] = ui->lineEditHistXMin->text().toStdString() ;
        jsonRoot["hist_xmax"] = ui->lineEditHistXMax->text().toStdString() ;
        jsonRoot["hist_ymax"] = ui->lineEditBiasPlotMaxY->text().toStdString() ;//new
        jsonRoot["hist_barcount"] = ui->lineEditHistCount->text().toStdString() ;
        jsonRoot["hist_rexmin"] = ui->lineEditHistXminRE->text().toStdString() ;
        jsonRoot["hist_rexmax"] = ui->lineEditHistXmaxRE->text().toStdString() ;

        jsonRoot["hist_xlabel"] = ui->lineEditDiffXLabel->text().toStdString() ;
        jsonRoot["hist_ylabel"] = ui->lineEditDiffYLabel->text().toStdString() ;
        jsonRoot["hist_rexlabel"] = ui->lineEditREXLabel->text().toStdString() ;
        jsonRoot["hist_reylabel"] = ui->lineEditREYLabel->text().toStdString() ;

        jsonRoot["match_proj"] = ui->radioButtonMatchProj->isChecked() ;
        jsonRoot["match_pixel"] = ui->radioButtonMatchPixel->isChecked() ;
        jsonRoot["in_tag"] = ui->lineEditInTag->text().toStdString() ;
        jsonRoot["re_tag"] = ui->lineEditReTag->text().toStdString() ;


        jsonRoot["in_datepos"] = ui->lineEditInDatePos->text().toStdString() ;
        jsonRoot["re_datepos"] = ui->lineEditRefDatePos->text().toStdString() ;
        jsonRoot["in_datelen"] = ui->lineEditDateLen->text().toStdString() ;

        jsonRoot["mask_perfile"] = ui->checkBoxPerFileMask->isChecked()  ;
        jsonRoot["mask_indatepos"] = ui->lineEditInMaskFnamePos->text().toStdString() ;
        jsonRoot["mask_redatepos"] = ui->lineEditReMaskFnamePos->text().toStdString() ;
        jsonRoot["mask_inpassval"] = ui->lineEditInMaskValues->text().toStdString() ;
        jsonRoot["mask_repassval"] = ui->lineEditReMaskValues->text().toStdString() ;

        JsonArray& mask_inArr = jsonRoot.createNestedArray("mask_infiles") ;
        JsonArray& mask_reArr = jsonRoot.createNestedArray("mask_refiles") ;
        for(int i1 = 0 ;i1 < ui->listWidgetInMaskList->count();++i1 )
        {
            QString fpath = ui->listWidgetInMaskList->item(i1)->text() ;
            mask_inArr.add( fpath.toStdString() ) ;
        }
        for(int i1 = 0 ;i1 < ui->listWidgetReMaskList->count();++i1 )
        {
            QString fpath = ui->listWidgetReMaskList->item(i1)->text() ;
            mask_reArr.add( fpath.toStdString() ) ;
        }

        jsonRoot["mask_useglobal"] =ui->checkBoxUseGlobalMask->isChecked() ;
        jsonRoot["mask_global_tag"] = ui->lineEditGlobalMaskTag->text().toStdString() ;
        jsonRoot["mask_global_filename"] = ui->lineEditGlobalMaskFilename->text().toStdString() ;
        jsonRoot["mask_global_passval"] = ui->lineEditGlobalMaskValues->text().toStdString() ;


        jsonRoot["out_dir"] = ui->lineEditOutfile->text().toStdString() ;
        jsonRoot["out_allcsvfilename"] = ui->lineEditOutCSV->text().toStdString() ;

        JsonArray& inArr = jsonRoot.createNestedArray("in_files") ;
        JsonArray& reArr = jsonRoot.createNestedArray("re_files") ;
        for(int ifile1 = 0 ; ifile1 < ui->listWidgetInfiles->count(); ++ifile1 )
        {
            QString filename1 = ui->listWidgetInfiles->item(ifile1)->text() ;
            inArr.add( filename1.toStdString() ) ;
        }
        for(int ifile2 = 0 ; ifile2 < ui->listWidgetReffiles->count(); ++ifile2)
        {
            QString filename2 = ui->listWidgetReffiles->item(ifile2)->text() ;
            reArr.add( filename2.toStdString() ) ;
        }

        //Qgis
        jsonRoot["qgs_mapprint"] =ui->lineEditMapPrintExe->text().toStdString() ;
        jsonRoot["qgs_template"] = ui->lineEditQgsTemplate->text().toStdString() ;
        jsonRoot["qgs_dpi"] = ui->lineEditDpi->text().toStdString() ;
        jsonRoot["qgs_zoom"] = ui->checkBoxZoom->isChecked();
        jsonRoot["qgs_clipextent"] = ui->checkBoxClipContent->isChecked() ;

        string outtext ;
        jsonRoot.prettyPrintTo(outtext);// prettyPrintTo()
        std::ofstream ofs(file1.toStdString().c_str());
        if( ofs.good() == true )
        {
            ofs<<outtext ;
            ofs.close() ;
            QMessageBox::information(this,"Done","质检方案已保存") ;
        }else{
            QMessageBox::warning(this,"Warning","质检方案写入文件失败.") ;
            return ;
        }

    }
}

void MainWindow::on_pushButtonBrowsePrint_clicked()
{
    QString file1 = QFileDialog::getOpenFileName(this,"Map Print",".","EXE(*.exe)");
    if( file1.isEmpty() ==false ){
        ui->lineEditMapPrintExe->setText( file1 ) ;
    }
}

void MainWindow::on_pushButtonBrowseQgs_clicked()
{
    QString file1 = QFileDialog::getOpenFileName(this,"Qgs Bias Template",".","QGS(*.qgs)");
    if( file1.isEmpty() ==false ){
        ui->lineEditQgsTemplate->setText( file1 ) ;
    }
}
