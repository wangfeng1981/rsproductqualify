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

using std::vector;

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
    //1. remove python staff use gnuplot-portable version5.4.6
    //2. 数字结果汇总到一个csv中
    //3. 增加配置参数保存
    setWindowTitle("遥感数据质量检验（图像对图像）V2.0.1") ;

    QObject::connect( wProcessQueue::getInstance() , &wProcessQueue::progressChanged,
                      this,&MainWindow::progressChanged ) ;
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
        // end output dir -------------------------


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
                //order1.indatarawfilename = outdir + outbasename + "-indata.raw" ;
                //order1.refdatarawfilename = outdir + outbasename + "-refdata.raw" ;
                //order1.diffrawfilename = outdir + outbasename + "-diff.raw" ;
                //order1.reldiffrawfilename = outdir + outbasename + "-reldiff.raw" ;
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


            }
        }
    } catch (std::logic_error& ex) {
        QMessageBox::warning(this,"WARN",ex.what()) ;
        return ;
    }
    QMessageBox::information(this,"info","Done") ;
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
        if( fnameList[i].length() > pos+len ){
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
