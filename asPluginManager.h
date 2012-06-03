#pragma once

#include <QObject>
#include <QAbstractButton>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QCheckBox>

#include <QFile>
#include <QDir>

#include <QList>
#include <QHash>

#include <B5Plugin.h>
#include <PluginHub.h>
#include <PluginDependency.h>
#include <PluginImageSettings.h>

#include "WebInfos.h"

#include "ConfigurationMapper.h"

class asPluginManager : public QObject, public B5Plugin
{
    Q_OBJECT

public:
    asPluginManager() : m_hub( NULL ), m_pluginId( -1 ), m_groupId( -1 ), m_cblist(), m_ownerList(), m_enabledList() { ; }

    int priority() { return 100; }

    QString name() { return QString("asPluginManager"); }
    QString group() { return QString("asPluginManager"); }

    bool registerFilters();
    bool registerOptions();
    bool finish();

    int groupId() const { return m_groupId; }
    int pluginId() const { return m_pluginId; }

    PluginDependency * createDependency(const QString &);

    bool init(PluginHub *hub, int id, int groupId, const QString &bundlePath);
    void toolWidgetCreated(QWidget *uiWidget);

    QList<QString> toolFiles();
    QList<QWidget*> toolWidgets();

public slots:
    void handleClick() ;
    void handleHotnessChanged( const PluginImageSettings &options );
    void handleSettingsChanged(const PluginImageSettings &options, const PluginImageSettings &changed, int layer);

    void handleDataComplete(const QString &dataName, const PluginData *data);
    void handleDataInvalid(const QString &dataName);
    void webInfosReady();

private:

    QDir                    *m_dir;

    ConfigurationMapper     *m_config;

    PluginHub               *m_hub;
    int                     m_pluginId;
    int                     m_groupId;
    QList<QCheckBox*>       m_cblist;
    QHash<QString,int>              m_ownerList;
    QHash<QString,QList<int>*>      m_enabledList;

    WebInfos                *m_webInfos;

};
