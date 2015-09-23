#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

#include <libdui/dconstants.h>
#include <libdui/libdui_global.h>
#include <libdui/dthememanager.h>

#include "imagenamebutton.h"
#include "multiaddcheckbutton.h"

#include "firstletterclassify.h"

DUI_USE_NAMESPACE

KeyboardLayoutDelegate::KeyboardLayoutDelegate(const QString &title, QWidget *parent):
    QFrame(parent),
    m_layout(new QHBoxLayout),
    m_label(new QLabel),
    m_checkButton(new MultiAddCheckButton)
{
    D_THEME_INIT_WIDGET(KeyboardLayoutDelegate);

    setTitle(title);

    m_checkButton->setCheckable(true);
    m_checkButton->setChecked(false);
    m_checked = false;

    m_layout->setMargin(0);
    m_layout->addSpacing(HEADER_LEFT_MARGIN);
    m_layout->addWidget(m_label);
    m_layout->addWidget(m_checkButton, 0, Qt::AlignVCenter|Qt::AlignRight);
    m_layout->addSpacing(HEADER_RIGHT_MARGIN);

    setLayout(m_layout);

    connect(m_checkButton, &MultiAddCheckButton::stateChanged, [&]{
        if(m_checked != m_checkButton->isChecked()){
            m_checked = m_checkButton->isChecked();
            emit checkedChanged(m_checked);
        }
    });
}

QStringList KeyboardLayoutDelegate::keyWords()
{
    if(m_pinyinFirstLetterList.isEmpty()){
        QDBusInterface dbus_pinyin( "com.deepin.api.Pinyin", "/com/deepin/api/Pinyin",
                                          "com.deepin.api.Pinyin" );
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(dbus_pinyin.asyncCall("Query", QString(title()[0])), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [this, watcher]{
            QDBusPendingReply<QStringList> reply = *watcher;
            if(!reply.isError()){
                m_pinyinFirstLetterList = reply.value();
                emit this->getKeyWordsFinished();
            }
            watcher->deleteLater();
        });
    }

    return m_pinyinFirstLetterList;
}

QString KeyboardLayoutDelegate::title() const
{
    return m_label->text();
}

bool KeyboardLayoutDelegate::checked() const
{
    return m_checkButton->isChecked();
}

void KeyboardLayoutDelegate::setChecked(bool checked)
{
    m_checkButton->setChecked(checked);
}

void KeyboardLayoutDelegate::mouseReleaseEvent(QMouseEvent *e)
{
    QFrame::mouseReleaseEvent(e);

    m_checkButton->setChecked(!m_checkButton->isChecked());
}

void KeyboardLayoutDelegate::setTitle(const QString &title)
{
    m_label->setText(title);
}

FirstLetterClassify::FirstLetterClassify(QWidget *parent) :
    QFrame(parent),
    m_layout(new QVBoxLayout),
    m_letterList(new DSegmentedControl)
{
    D_THEME_INIT_WIDGET(FirstLetterClassify);

    m_letterList->setObjectName("LetterList");

    for (int i = 'A'; i <= 'Z'; ++i) {
        ListWidget *w = new ListWidget;
        w->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        w->setItemSize(310, EXPAND_HEADER_HEIGHT);
        m_listWidgetList << w;

        m_letterList->addSegmented(QString(i));
    }
    m_currentList = m_listWidgetList.first();

    m_layout->setMargin(0);
    m_layout->addWidget(m_letterList);
    m_layout->addStretch(1);
    setLayout(m_layout);

    connect(m_letterList, &DSegmentedControl::currentTitleChanged, [&](const QString& key){
        int index = key[0].toUpper().toLatin1() - 65;
        m_currentList->setParent(0);
        m_layout->removeItem(m_layout->itemAt(1));
        m_currentList = m_listWidgetList[index];
        m_layout->addWidget(m_currentList);

        emit currentLetterChanged(key);
    });

    foreach (QObject *obj, m_letterList->children()) {
        QWidget *w = qobject_cast<QWidget*>(obj);
        if(w){
            w->setObjectName("");
        }
    }
}

ListWidget *FirstLetterClassify::searchList() const
{
    return m_currentList;
}

DSegmentedControl *FirstLetterClassify::letterList() const
{
    return m_letterList;
}

void FirstLetterClassify::addItem(KeyboardLayoutDelegate *data)
{
    if(!data->keyWords().isEmpty()){
        foreach (QString key, data->keyWords()) {
            int index = key[0].toUpper().toLatin1() - 65;

            if(index>=0){
                m_listWidgetList[index]->addWidget(data);
            }
        }
    }else{
        connect(data, &KeyboardLayoutDelegate::getKeyWordsFinished, this, [this, data]{
            QStringList keywords = data->keyWords();

            int index = keywords[0][0].toUpper().toLatin1() - 65;
            if(index>=0){
                m_listWidgetList[index]->addWidget(data);
            }
            for (int i=1; i < keywords.count(); ++i) {
                QString key = keywords[i];
                int index = key[0].toUpper().toLatin1() - 65;
                if(index>=0){
                    KeyboardLayoutDelegate *item = new KeyboardLayoutDelegate(data->title());
                    connect(item, &KeyboardLayoutDelegate::checkedChanged, data, &KeyboardLayoutDelegate::setChecked);
                    m_listWidgetList[index]->addWidget(item);
                }
            }
        });
    }
}

void FirstLetterClassify::removeItems(QList<KeyboardLayoutDelegate *> datas)
{
    foreach (KeyboardLayoutDelegate* w, datas) {
        foreach (QString pinyin, w->keyWords()) {
            int index = pinyin[0].toUpper().toLatin1() - 65;

            ListWidget *list = m_listWidgetList[index];
            for (int i = 0; i < list->count(); ++i) {
                KeyboardLayoutDelegate* ww = qobject_cast<KeyboardLayoutDelegate*>(list->getWidget(i));
                if(ww->title() == w->title())
                    list->removeWidget(i);
            }
        }
    }
}

QString FirstLetterClassify::currentLetter() const
{
    return m_letterList->getText(m_letterList->currentIndex());
}

void FirstLetterClassify::setCurrentLetter(QString currentLetter)
{
    m_letterList->setCurrentIndexByTitle(currentLetter);
}

void FirstLetterClassify::show()
{
    if(m_layout->count() == 2){
        m_layout->addWidget(m_currentList);
    }else{
        m_letterList->currentTitleChanged(m_letterList->getText(m_listWidgetList.indexOf(m_currentList)));
    }
    QFrame::show();
}
