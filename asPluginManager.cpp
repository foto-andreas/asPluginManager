#include "asPluginManager.h"

#include <QObject>
#include <QAbstractButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDebug>
#include <QString>
#include <QMessageBox>
#include <QPushButton>

#include <QFile>
#include <QDir>

#include <QList>
#include <QHash>

#include <QTimer>

#include <QDomDocument>

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

    return true;
}

void asPluginManager::checkForUpdates(QString id, int sdkVersion, QString installedVersion) {
    // check for updates
    if (m_config->checkForUpdates()) {
        WebInfos *webInfos = new WebInfos(id, QString("%1").arg(sdkVersion), installedVersion);
        connect(webInfos,
                SIGNAL(ready()),
                SLOT(webInfosReady()));
        qDebug() << "asPluginManager: fetching update infos for" << id;
        webInfos->fetch();
    }
}

#define min(a,b) (a>b ? b : a)

void asPluginManager::toolWidgetCreated(QWidget *uiWidget) {

    qDebug() << "asPluginManager::toolWidgetCreated";

    QWidget *contents = uiWidget->findChild<QWidget*>("contents");
    QGridLayout *layout = (QGridLayout*)contents->layout();

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

        QString infoName = name;
        QString infoId = name;
        QString infoVersion = "";
        int infoMajor = 0;
        int infoMinor = 0;
        int infoFix = 0;
        int infoSDK = 0;
        QString infoFile = dir + "/" + name + "/" + "Info.afpxml";
        try {
            QDomDocument infoDoc;
            infoDoc.setContent(new QFile(infoFile));
            QDomElement propertyScribe = infoDoc.documentElement();
            QDomElement pluginFileData = propertyScribe.elementsByTagName("PluginFileData").at(0).toElement();
            infoName = pluginFileData.attribute("name");
            infoId = pluginFileData.attribute("identifier");
            infoMajor = pluginFileData.attribute("majorVersion").toInt();
            infoMinor = pluginFileData.attribute("minorVersion").toInt();
            infoFix = pluginFileData.attribute("bugfixVersion").toInt();
            infoSDK = pluginFileData.attribute("sdkVersion").toInt();
            infoVersion = QString("%1.%2.%3").arg(infoMajor).arg(infoMinor).arg(infoFix);
            qDebug() << "asPluginManager: found plugin dir" << name
                     << "with name" << infoName << "with id" << infoId
                     << "in version" << infoVersion << "for sdk version" << infoSDK;
        } catch (exception e) {
            qDebug() << "asPluginManager:" << infoFile << "has unknown structure.";
        }

        name.remove(QRegExp("\\.afplugin(\\.off)*$"));
        QString internalName = m_config->getString(name,NULL);
        if (internalName == NULL) {
            internalName = infoName;
            m_config->setString(name,infoName);
        }
        QCheckBox *c = new QCheckBox(name, contents);
        m_cblist.insert(internalName, c);
        c->setFocusPolicy(Qt::NoFocus);
        QLabel *cc = new QLabel(tr("not loaded"));
        m_enlist.insert(internalName, cc);
        QAbstractButton *cv = new QPushButton(infoVersion);
        cv->setFixedHeight(16);
        cv->setFixedWidth(60);
        cv->setEnabled(false);
        m_vlist.insert(infoId, cv);
        if (entries[i].endsWith("afplugin")) {
            if (m_config->checkForUpdates()) {
                this->checkForUpdates(infoId, infoSDK, infoVersion);
            }
            c->setChecked(true);
            c->setStyleSheet("QCheckBox { font-weight: bold; };");
            cc->setText(tr("no ToolData"));
        }
        layout->addWidget(c, i, 0, Qt::AlignLeft);
        layout->addWidget(cv, i, 1, Qt::AlignLeft);
        layout->setHorizontalSpacing(5);
        layout->addWidget(cc, i, 2, Qt::AlignLeft);
        layout->setRowStretch(i,0);
        connect(c, SIGNAL( clicked() ), SLOT( handleClick() ) );
        height += c->height();
        c->setProperty("internalName", QVariant(internalName));
        layout->setRowStretch(i+1,1);
    }
    layout->setColumnStretch(0,1);
    layout->setColumnStretch(1,0);
    uiWidget->setMinimumSize(100, min(height, m_config->toolBoxHeight()));

    connect( m_hub,
                  SIGNAL( hotnessChanged( const PluginImageSettings & ) ),
                  SLOT( handleHotnessChanged( const PluginImageSettings & ) ) );

    connect( m_hub,
                  SIGNAL( settingsChanged( const PluginImageSettings &, const PluginImageSettings &, int ) ),
                  SLOT( handleSettingsChanged( const PluginImageSettings &, const PluginImageSettings &, int ) ) );

}

void asPluginManager::handleClick() {

    QCheckBox *c = (QCheckBox*)QObject::sender();
    bool enable = c->isChecked();
    QString who = c->text();
    if (enable) {
        if (!QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin.off",
                           m_dir->absoluteFilePath(who) + ".afplugin")) c->setChecked(false);
        qDebug() << "asPluginManager: enabled plugin" << who << "for next restart.";
    } else {
        if (!QFile::rename(m_dir->absoluteFilePath(who) + ".afplugin",
                      m_dir->absoluteFilePath(who) + ".afplugin.off")) c->setChecked(true);
        if (who.startsWith("asPluginManager")) {
            QMessageBox::information(NULL, "asPluginManager", tr("You disabled asPluginManager itself."));
        }
        qDebug() << "asPluginManager: disabled plugin" << who << "for next restart.";
    }

}

QList<QString> asPluginManager::toolFiles() {

    return QList<QString>();
}

QList<QWidget*> asPluginManager::toolWidgets() {

    QList<QWidget*> lstWidgets;
    return lstWidgets;
}

bool asPluginManager::registerFilters() {

    return true;
}

bool asPluginManager::registerOptions() {

    return true;
}

bool asPluginManager::finish() {

    return true;
}

void asPluginManager::handleHotnessChanged( const PluginImageSettings &options ) {

    qDebug() << "asPluginManager::handleHotnessChanged #toolList =" << m_toolDataList.size();

    // ask the plugins for their ToolData if not done already
    if (m_toolDataList.isEmpty()) {
        QHashIterator<QString, QCheckBox*> i(m_cblist);
        while (i.hasNext()) {
            QCheckBox *c = i.next().value();
            // get their internal names from the config file, use directory name as default
            QString dataName = QString("%1:ToolData").arg(c->property("internalName").toString());
            qDebug() << "asPluginManager: start PluginData" << dataName;
            m_hub->startPluginData(dataName);
        }
    } else {
        for (int layer = 0; layer<options.count(); layer++) {
            checkOptions(options, layer);
        }
    }
}

void asPluginManager::checkOptions(const PluginImageSettings &options, int layer) {
    if (options.options(layer)) {
        qDebug() << "asPluginManager: checking in layer" << layer;
        for (int i=0; i<m_toolDataList.size(); i++) {
            int ownerId = m_toolDataList[i]->ownerId;
            QString owner = m_toolDataList[i]->owner;
            qDebug() << "asPluginManager: checking on enabled ownerId =" << ownerId;
            QLabel *c = m_enlist.find(owner).value();
            c->setText(tr("no optionId"));
            for (int j=0; j<m_toolDataList[i]->enabledIds.size(); j++) {
                c->setText(tr("disabled"));
                c->setStyleSheet("QLabel { font-weight: bold; }");
                qDebug() << "asPluginManager: checking on enabled ownerId =" << ownerId << "option =" << m_toolDataList[i]->enabledIds.at(j);
                bool ok;
                bool enabled = options.options(0)->getBool(m_toolDataList[i]->enabledIds.at(j), ownerId, ok);
                if (ok) {
                    // qDebug() << "asPluginManager: ok";
                    if (c) {
                        qDebug() << "asPluginManager:" << owner << "enabled =" << enabled;
                        if (enabled) {
                            c->setText(tr("enabled"));
                            c->setStyleSheet("QLabel { font-weight: bold; }");
                            break;
                        }
                    }
                } else {
                    c->setText(tr("wrong optionId?"));
                    c->setStyleSheet("QLabel { font-weight: normal; }");

                }
            }
            c->update();
        }
    }
}

void asPluginManager::handleSettingsChanged( const PluginImageSettings &options,  const PluginImageSettings &changed, int layer ) {

    Q_UNUSED(changed);
    checkOptions(options, layer);

}

void asPluginManager::handleDataComplete(const QString &dataName, const PluginData *data) {

    qDebug() << "asPluginManager::handleDataComplete" << data;

    if (dataName.endsWith(":ToolData")) {
        const ToolData *toolData = (const ToolData*)(data);
        if (toolData) {
            if (toolData->version >= 1) {
                ToolData *ourToolData = new ToolData(m_hub);
                *ourToolData = *toolData;
                m_toolDataList.append(ourToolData);
                QLabel *c = m_enlist.find(ourToolData->owner).value();
                c->setText(tr("waiting for HC"));
                qDebug() << "asPluginManager: data complete" << dataName << "OwnerId =" << ourToolData->ownerId;
            } else {
                qDebug() << "asPluginManager: old ToolData version:" << toolData->version << "for" << dataName;
            }
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
            qDebug() << "asPluginManager: createDependency ToolData" << toolData;
            return toolData;
        }
    }

    qDebug() << "asPluginManager: createDependency NULL for" << depName;

    return NULL;

}

void asPluginManager::webInfosReady() {

    WebInfos *webInfos = (WebInfos*)(sender());

    qDebug() << "asPluginManager::webInfosReady" << webInfos->identifier()
             << "web:" << webInfos->webVersion() << "installed:" << webInfos->installedVersion();
    QAbstractButton *c = m_vlist.find(webInfos->identifier()).value();
    if (c!=NULL) {
        c->setStyleSheet("QAbstractButton { color : green; }");
    }
    if (webInfos->isWebNewer()) {
        QString text = QString(tr("There is a newer version of %1 available. "
                               "It is version %2. You are running %3. "
                               "You can download it under the following url: <a href='%4'>%4</a>"))
                       .arg(webInfos->name(), webInfos->webVersion(), webInfos->installedVersion(), webInfos->link());
        if (c!=NULL) {
            c->setStyleSheet("QAbstractButton { color : red; }");
            c->setText(tr("update"));
            c->setToolTip(text);
            c->setEnabled(true);
            connect(c, SIGNAL(clicked()), SLOT(handleClickForUpdate()));
        } else {
            QMessageBox::information(NULL, webInfos->name(), text);
        }
    }
    delete webInfos;
}

void asPluginManager::handleClickForUpdate() {
    QAbstractButton *button = (QAbstractButton*) sender();
    QMessageBox::information(NULL, tr("Update-Info from asPluginManager"), button->toolTip());
}
