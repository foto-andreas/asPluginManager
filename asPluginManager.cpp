#include "asPluginManager.h"

#include <QObject>
#include <QAbstractButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDebug>
#include <QString>
#include <QMessageBox>

#include <QFile>
#include <QDir>

#include <QList>
#include <QHash>

#include <PluginHub.h>
#include <PluginImageSettings.h>
#include <PluginOptionList.h>
#include <B5Plugin.h>

#include "TargetVersion.h"

#include "ToolData.h"

extern "C" BIBBLE_API BaseB5Plugin *b5plugin() { return new asPluginManager; }

bool asPluginManager::init(PluginHub *hub, int id, int groupId, const QString&) {

    qDebug() << "asPluginManager::init";

    m_hub = hub;
    m_pluginId = id;
    m_groupId = groupId;

    // configuration file abstraction with a configuration mapper
    QString configDir = m_hub->property("pluginStorageHome").toString() + "/asPluginManager";
    QDir qdir;
    qdir.mkdir(configDir);
    QString configPath = configDir + "/asPluginManager.conf";
    m_config = new ConfigurationMapper(configPath);
    if (m_config == NULL) {
        QMessageBox::information(NULL,"asPluginManager",tr("Configuration file problem with file:") + "<br/>" + configPath);
        return false;
    }

    // check for updates
    if (m_config->checkForUpdates()) {
        m_webInfos = new WebInfos("de.schrell.asPluginManager", "8");
        connect(m_webInfos,
                SIGNAL(ready()),
                SLOT(webInfosReady()));

        m_webInfos->fetch();
    }

    return true;
}

#define min(a,b) (a>b ? b : a)

void asPluginManager::toolWidgetCreated(QWidget *uiWidget) {

    qDebug() << "asPluginManager::toolWidgetCreated";

    QWidget *contents = uiWidget->findChild<QWidget*>("contents");
    QVBoxLayout *layout = (QVBoxLayout*)contents->layout();

    int height = 0;

    connect(m_hub, SIGNAL(pluginDataComplete(const QString &, const PluginData *)), SLOT(handleDataComplete(const QString &, const PluginData *)));
    connect(m_hub, SIGNAL(pluginDataInvalid(const QString &)), SLOT(handleDataInvalid(const QString &)));

    // search the plugins and build the user interface
    QString dir = m_hub->property("pluginStorageHome").toString().append("/../Plugins");
    m_dir = new QDir(dir);
    m_dir->setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    QStringList entries = m_dir->entryList();
    for (int i=0; i<entries.length(); i++) {
        QString name = entries[i];
        name.remove(QRegExp("\\.afplugin(\\.off)*$"));
        if ((m_config->getString(name,NULL)) == NULL) {
            m_config->setString(name,"");
        }
        QCheckBox *c = new QCheckBox(name, contents);
        if (entries[i].endsWith("afplugin")) {
            c->setChecked(true);
            c->setStyleSheet("QCheckBox { font-weight: bold; }");
        }
        layout->addWidget(c, 0, Qt::AlignLeft);
        connect(c, SIGNAL( clicked() ), SLOT( handleClick() ) );
        height += c->height();
        m_cblist.append(c);
    }
    layout->addStretch(1);
    uiWidget->setMinimumSize(100, min(height, 400));

    connect( m_hub,
                  SIGNAL( hotnessChanged( const PluginImageSettings & ) ),
                  SLOT( handleHotnessChanged( const PluginImageSettings & ) ) );

    connect( m_hub,
                  SIGNAL( settingsChanged( const PluginImageSettings &, const PluginImageSettings &, int ) ),
                  SLOT( handleSettingsChanged( const PluginImageSettings &, const PluginImageSettings &, int ) ) );

}

void asPluginManager::handleClick() {

    qDebug() << "asPluginManager::handleClick";

    QCheckBox *c = (QCheckBox*)QObject::sender();
    bool enable = c->isChecked();
    QString who = c->text();
    if (enable) {
        if (!QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin.off",
                           m_dir->absoluteFilePath(who) + ".afplugin")) c->setChecked(false);
    } else {
        if (!QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin",
                      m_dir->absoluteFilePath(who) + ".afplugin.off")) c->setChecked(true);
        if (who.startsWith("asPluginManager")) {
            QMessageBox::information(NULL, "asPluginManager", tr("You disabled asPluginManager itself."));
        }
    }

    qDebug() << "Toggled Plugin " << who << ":" << (c->isChecked() ? "on" : "off");
}

QList<QString> asPluginManager::toolFiles() {

    qDebug() << "asPluginManager::toolFiles";

    return QList<QString>();
}

QList<QWidget*> asPluginManager::toolWidgets() {

    qDebug() << "asPluginManager::toolWidgets";

    QList<QWidget*> lstWidgets;
    return lstWidgets;
}

bool asPluginManager::registerFilters() {

    qDebug() << "asPluginManager::registerFilters";

    return true;
}

bool asPluginManager::registerOptions() {

    qDebug() << "asPluginManager::registerOptions";

    return true;
}

bool asPluginManager::finish() {

    qDebug() << "asPluginManager::finish";

    return true;
}

void asPluginManager::handleHotnessChanged( const PluginImageSettings &options ) {

    // qDebug() << "asPluginManager::handleHotnessChanged";

    // ask the plugins for their ToolData if not done already
    if (m_ownerList.isEmpty()) {
        for (int i=0; i<m_cblist.length(); i++) {
            QString name = (m_cblist[i])->text();
            // get their internal names from the config file, use directory name as default
            QString dataName = QString("%1:ToolData").arg(m_config->getString(name,name));
            qDebug() << "asPluginManager: start PluginData" << dataName;
            m_hub->startPluginData(dataName);
        }
    }

    Q_UNUSED(options);
}

void asPluginManager::handleSettingsChanged( const PluginImageSettings &options,  const PluginImageSettings &changed, int layer ) {

    // qDebug() << "asPluginManager::handleSettingsChanged";

    Q_UNUSED(changed);
    Q_UNUSED(layer);

    if (options.options(layer) != NULL) {
    }

}

void asPluginManager::handleDataComplete(const QString &dataName, const PluginData *data) {

    qDebug() << "asPluginManager::handleDataComplete";

    if (dataName.endsWith(":ToolData")) {
        const ToolData *toolData = dynamic_cast<const ToolData*>(data);
        if (toolData) {
            QStringList naming = dataName.split(":");
            m_ownerList.insert(naming[0], toolData->ownerId);
            qDebug() << "asPluginManager: data complete" << dataName << "OwnerId =" << toolData->ownerId;
        } else {
            qDebug() << "asPluginManager: data complete" << dataName << "got NULL as data.";
        }
    }

}

void asPluginManager::handleDataInvalid(const QString &dataName) {

    qDebug() << "asPluginManager::handleDataInvalid";

    if (dataName.endsWith(":ToolData")) {
        qDebug() << "asPluginManager: data invalid" << dataName;
    }
}

PluginDependency *asPluginManager::createDependency(const QString &depName) {

    qDebug() << "asPluginManager::createDependency";

    if (depName == "ToolData") {
        ToolData *toolData = new ToolData(m_hub);
        if (toolData) {
            toolData->owner = name();
            toolData->group = group();
            toolData->ownerId = pluginId();
            toolData->groupId = groupId();
            toolData->addEnabledId(0);
            qDebug() << "asPluginManager: createDependency ToolData" << toolData;
            return toolData;
        }
    }

    qDebug() << "asPluginManager: createDependency NULL for" << depName;

    return NULL;

}

void asPluginManager::webInfosReady() {

    qDebug() << "asPluginManager::webInfosReady" << m_webInfos->identifier() << m_webInfos->version();

    if (m_webInfos->isWebNewer()) {
        QString text = QString(tr("There is a newer version of %1 available. "
                               "It is version %2. You are running %3. "
                               "You can download it under the following url: <a href='%4'>%4</a>"))
                        .arg(m_webInfos->name(), m_webInfos->version(), TARGET_VERSION_STRING, m_webInfos->link());
        QMessageBox::information(NULL, m_webInfos->name(), text);
    }
    delete m_webInfos;
    m_webInfos = NULL;
}

