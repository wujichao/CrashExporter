#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

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

public slots:
    void onClickExportAllButton();
    void onClickExportSelectButton();

private:
    Ui::Widget *ui;

    void InitScoresTable();
    void SetTableRow(int row, QString title, QString date);
};

#endif // WIDGET_H
