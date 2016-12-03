#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <QModelIndex>

struct CrashItem {
    QString path;
    QString bundle;
    QString date;
};

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    // should be private :)
    void updateIndicatorLabel(QString status);
    idevice_t device;
    lockdownd_client_t client;
    std::vector<CrashItem> crashItems;
    void insertRow(QString title, QString date);

public slots:
    void onClickExportAllButton();
    void onClickExportSelectButton();
    void onCellClicked(int row, int column);

private:
    Ui::Widget *ui;


    void InitScoresTable();
};

#endif // WIDGET_H
