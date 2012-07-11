#pragma once

/** \mainpage asPluginManager - Plugin Management for AfterShot Pro
 *
 * @author    Andeas Schrell
 * @version   1.0.9
 * @date      2012-07-10
 * @warning   Like removing the plugins manually, disabling a plugin here has the following effect:
 *            If you change settings for an image with a plugin disabled all settings of that plugin are lost for the image.
 *
 * \section intro_sec Introduction
 *
 * asPluginManager is a tool to manage your installed plugins. It can be used
 * to disable or enable a plugin by renaming its plugin folder. It displays the current
 * version and informs you about updates of your plugins on the Corel web page.
 *
 * If you want to register your plugin in asPluginManger please see here:
 * http://forum.corel.com/EN/viewtopic.php?f=96&t=47074
 *
 * \section install_sec Installation
 *
 * \subsection binary The binary install process
 *
 * The normal user can download asPluginManager as a binary ASP plugin here:
 * http://aftershotpro.com/plugins/?plug=aspluginmanager
 * From the AfterShot Pro file menu, he/she can select "install plugin" to install
 * the plugin.
 *
 * A deinstallation can be done by deleting the asPluginManager.afplugin folder from the
 * plugins folder in the ASP user home.
 *
 * \subsection source The source code
 *
 * The source code can be obtained with the git commands
 *
\verbatim
git clone http://schrell.de/asPluginManager.git
git clone http://schrell.de/PluginTools.git
git clone http://schrell.de/PluginDefaults.git
\endverbatim
 * or by browsing to the following web page
 * http://schrell.de/cgi-bin/gitweb.cgi?p=asPluginManager.git
 *
 * \section license License
 *
 * You may use asPluginManager free of charge. Small donations for coffee or beer can
 * provided using the Paypal Donation option on the Corels plugin page, see above.
 *
 * asPluginManager is licensed under the BSD 2-clause license.
 *
 * \section other Other Information
 *
 * @bug    additional hotness change required to get ToolData (I see this as an ASP bug)
 *
 * @todo   shorter names in config file to get a smaller ui
 * @todo   dark layout
 * @todo   perhaps hide plugins from the ui by manipulating their Info file
 * @todo   select/unselect all
 * @todo   save and reload configurations
 *
 */

#include <QObject>
#include <QAbstractButton>
#include <QList>
#include <QString>
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>

#include <QFile>
#include <QDir>

#include <QList>
#include <QHash>

#include <B5Plugin.h>
#include <PluginHub.h>
#include <PluginDependency.h>
#include <PluginImageSettings.h>

#include "WebInfos.h"
#include "ToolData.h"

#include "ConfigurationMapper.h"

class LayoutData {
public:
    QCheckBox *loadedCB;
    QAbstractButton * versionBTN;
    QHash<int, QCheckBox*> enabledCBHash;
    QGridLayout *boxLayout;
    QWidget *boxWidget;
    ToolData *toolData;
    QString name;
    QString internalName;
    QString infoId;
    QLabel *infoLabel;
    bool loaded;
    LayoutData() : loadedCB(0), versionBTN(0), enabledCBHash(),
        boxLayout(), boxWidget(), toolData(0), name(""), internalName(""), infoId(""), infoLabel(),
        loaded(false) {}
};

class asPluginManager : public QObject, public B5Plugin
{
    Q_OBJECT

public:
    asPluginManager() :
        m_hub( NULL ), m_pluginId( -1 ), m_groupId( -1 ),
        m_layoutData() { ; }

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
    void handleClickForUpdate() ;
    void handleHotnessChanged( const PluginImageSettings &options );
    void handleSettingsChanged(const PluginImageSettings &options, const PluginImageSettings &changed, int layer);

    void handleDataComplete(const QString &dataName, const PluginData *data);
    void handleDataInvalid(const QString &dataName);
    void webInfosReady(QList<WebInfos*>&);

    void enablerClicked();

private:

    void checkForUpdates(int sdkVersion);
    void checkOptions(const PluginImageSettings &options, int layer);

    QDir                    *m_dir;

    ConfigurationMapper     *m_config;

    PluginHub               *m_hub;

    int                     m_pluginId;
    int                     m_groupId;

    QHash<QString, LayoutData*> m_layoutData;
    QGridLayout             *m_layout;
    WebInfos                *m_webInfos;

};
