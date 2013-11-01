#ifndef LOG_PASS_H
#define LOG_PASS_H

#include <QDialog>

namespace Ui {
class log_pass;
}

class log_pass : public QDialog
{
    Q_OBJECT

public:
    explicit log_pass(QWidget *parent = 0);
    ~log_pass();

public:
    Ui::log_pass *ui;
};

#endif // LOG_PASS_H
