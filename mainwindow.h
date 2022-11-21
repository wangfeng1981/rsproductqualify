#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "gdal_priv.h"
#include <string>
#include "assertlineedit.h"
#include <stdexcept>

using std::exception;
using std::string;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonAddInput_clicked();

    void on_pushButtonClearInput_clicked();

    void on_pushButtonAddRef_clicked();

    void on_pushButtonClearRef_clicked();

    void on_pushButtonOpenOutfile_clicked();

    void on_pushButtonOk_clicked();

    void on_pushButtonCancel_clicked();

    void on_pushButtonLandCover_clicked();


    void progressChanged(int type, int doneTaskCnt,int totalTaskCnt, int progressOrStatus, QString currTaskDoneInfo );

    void on_pushButtonOpenPython_clicked();

    void on_pushButtonOpenScript_clicked();

    void on_pushButtonAddInMask_clicked();

    void on_pushButtonClearInMask_clicked();

    void on_pushButtonAddReMask_clicked();

    void on_pushButtonClearReMask_clicked();

private:
    Ui::MainWindow *ui;
    int getBandCount(QString filename) ;
    //if no matching return -1
    int getMatchFnameFromList(QString matcher, int pos, int len, QStringList& fnameList) ;
};
#endif // MAINWINDOW_H
