#include "log_pass.h"
#include "ui_log_pass.h"

log_pass::log_pass(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::log_pass)
{
    ui->setupUi(this);
}

log_pass::~log_pass()
{
    delete ui;
}
