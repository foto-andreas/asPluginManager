#pragma once

#include <QObject>
#include <QAbstractButton>
#include <QList>
#include <QString>
#include <QVBoxLayout>

#include <QFile>
#include <QDir>

#include <B5Plugin.h>
#include <PluginHub.h>
#include <PluginDependency.h>
#include <PluginImageSettings.h>

class asPluginManager : public QObject, public B5Plugin
{
    Q_OBJECT

public:
    asPluginManager() : m_hub( NULL ), m_pluginId( -1 ), m_groupId( -1 ) { ; }
    int priority() { return 100; }
    QString name() { return QString("asPluginManager"); }
    QString group() { return QString("asPluginManager"); }
    bool registerFilters() {return true; }
    bool registerOptions() {return true; }
    bool finish() {return true; }
    int groupId() const { return m_groupId; }
    int pluginId() const { return m_pluginId; }

    PluginDependency * createDependency(const QString &) { return new PluginDependency; }

    bool init(PluginHub *hub, int id, int groupId, const QString &bundlePath);
    void toolWidgetCreated(QWidget *uiWidget);

    QList<QString> toolFiles();
    QList<QWidget*> toolWidgets();

public slots:
    void handleToggle(bool);

private:

    QDir             *m_dir;

    PluginHub *m_hub;
    int m_pluginId;
    int m_groupId;

};
