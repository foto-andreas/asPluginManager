#include "asPluginManager.h"

#include <QObject>
#include <QAbstractButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDebug>
#include <QString>

#include <QFile>
#include <QDir>

#include <PluginHub.h>
#include <PluginImageSettings.h>
#include <PluginOptionList.h>
#include <B5Plugin.h>

#include "TargetVersion.h"

extern "C" BIBBLE_API BaseB5Plugin *b5plugin() { return new asPluginManager; }

bool asPluginManager::init(PluginHub *hub, int id, int groupId, const QString&) {
    m_hub = hub;
    m_pluginId = id;
    m_groupId = groupId;
    return true;
}

void asPluginManager::toolWidgetCreated(QWidget *uiWidget) {
    QWidget *contents = uiWidget->findChild<QWidget*>("contents");
    QVBoxLayout *layout = (QVBoxLayout*)contents->layout();
    QString dir = m_hub->property("pluginStorageHome").toString().append("/../Plugins");
    m_dir = new QDir(dir);
    m_dir->setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    QStringList entries = m_dir->entryList();
    for (int i=0; i<entries.length(); i++) {
        QString name = entries[i];
        name.remove(QRegExp("\\.afplugin(\\.off)*$"));
        QCheckBox *c = new QCheckBox(name, contents);
        if (entries[i].endsWith("afplugin"))
            c->setChecked(true);
        layout->addWidget(c, 0, Qt::AlignLeft);
        connect(c, SIGNAL( toggled(bool) ), SLOT( handleToggle(bool) ) );
    }
}

void asPluginManager::handleToggle(bool enable) {
    QString who = ((QCheckBox*)QObject::sender())->text();
    if (enable)
        QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin.off",
                      m_dir->absoluteFilePath(who) + ".afplugin");
    else
        QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin",
                      m_dir->absoluteFilePath(who) + ".afplugin.off");

    qDebug() << "Toggled Plugin " << who << ":" << (enable ? "on" : "off");
}

QList<QString> asPluginManager::toolFiles()
{
    return QList<QString>();
}

QList<QWidget*> asPluginManager::toolWidgets()
{
    QList<QWidget*> lstWidgets;
    return lstWidgets;
}

