#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

public slots:
    void onClickExportAllButton();
    void onClickExportSelectButton();

private:
    Ui::Widget *ui;

    void InitScoresTable();
    void SetTableRow(int row, QString title, QString date);
};

#endif // WIDGET_H
