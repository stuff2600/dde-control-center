#ifndef RESOLUTIONDETAILPAGE_H
#define RESOLUTIONDETAILPAGE_H

#include "contentwidget.h"
#include "optionitem.h"

#include <QMap>

namespace dcc
{
class SettingsGroup;

namespace display {

class Monitor;
class DisplayModel;
class ResolutionDetailPage : public ContentWidget
{
    Q_OBJECT

public:
    explicit ResolutionDetailPage(QWidget *parent = 0);

    void setModel(DisplayModel *model);

signals:
    void requestSetResolution(Monitor *mon, const int mode) const;

private slots:
    void onItemClicked();

private:
    DisplayModel *m_model;
    dcc::SettingsGroup *m_resolutions;

    QMap<dcc::widgets::OptionItem *, int> m_options;
};

}   // namespace dcc

}   // namespace display

#endif // RESOLUTIONDETAILPAGE_H
