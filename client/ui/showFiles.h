#ifndef SHOWFILES_H
#define SHOWFILES_H

#include <QListWidget>
#include <QMainWindow>

namespace Ui {
class ShowFiles;
}

class ShowFiles : public QMainWindow
{
    Q_OBJECT

public:
    explicit ShowFiles(QWidget *parent = nullptr);
    ~ShowFiles();
    void addFiles(QStringList l);

private slots:
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_newFile_clicked();
    void on_actionNew_File_triggered();
    void on_actionLogout_triggered();

signals:
    void newFile(QString filename);
    void logout();

private:
    Ui::ShowFiles *ui;
    bool newFileShown = false;
};

#endif // SHOWFILES_H